#include <assert.h>
#include <err.h>
#include <getopt.h>
#include <inttypes.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

typedef uint64_t u64; /* portable 64-bit integer */
typedef uint32_t u32; /* portable 32-bit integer */
struct __attribute__((packed)) entry {
  u32 k;
  u64 v;
}; /* hash table entry */

/***************************** global variables ******************************/

u64 n = 0; /* block size (in bits) */
u64 mask;  /* this is 2**n - 1 */

u64 dict_size;   /* number of slots in the hash table */
struct entry *A; /* the hash table */

/* (P, C) : two plaintext-ciphertext pairs */
u32 P[2][2] = {{0, 0}, {0xffffffff, 0xffffffff}};
u32 C[2][2];

/* mpi variables */
int mpi_size;
int mpi_rank;
#define TO_INSERT_TAG 0
MPI_Request *requests;

/* buffers to exchange informations */
u64 **buffers; // un par processus
u64 buffer_size;
u64 null_buffer_value = (u64)(-1);
u64 *buffer_idx;

/************************ tools and utility functions *************************/

double wtime() {
  struct timeval ts;
  gettimeofday(&ts, NULL);
  return (double)ts.tv_sec + ts.tv_usec / 1E6;
}

// murmur64 hash functions, tailorized for 64-bit ints / Cf. Daniel Lemire
u64 murmur64(u64 x) {
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdull;
  x ^= x >> 33;
  x *= 0xc4ceb9fe1a85ec53ull;
  x ^= x >> 33;
  return x;
}

/* represent n in 4 bytes */
void human_format(u64 n, char *target) {
  if (n < 1000) {
    sprintf(target, "%" PRId64, n);
    return;
  }
  if (n < 1000000) {
    sprintf(target, "%.1fK", n / 1e3);
    return;
  }
  if (n < 1000000000) {
    sprintf(target, "%.1fM", n / 1e6);
    return;
  }
  if (n < 1000000000000ll) {
    sprintf(target, "%.1fG", n / 1e9);
    return;
  }
  if (n < 1000000000000000ll) {
    sprintf(target, "%.1fT", n / 1e12);
    return;
  }
}

/******************************** SPECK block cipher **************************/

#define ROTL32(x, r) (((x) << (r)) | (x >> (32 - (r))))
#define ROTR32(x, r) (((x) >> (r)) | ((x) << (32 - (r))))

#define ER32(x, y, k)                                                          \
  (x = ROTR32(x, 8), x += y, x ^= k, y = ROTL32(y, 3), y ^= x)
#define DR32(x, y, k)                                                          \
  (y ^= x, y = ROTR32(y, 3), x ^= k, x -= y, x = ROTL32(x, 8))

void Speck64128KeySchedule(const u32 K[], u32 rk[]) {
  u32 i, D = K[3], C = K[2], B = K[1], A = K[0];
  for (i = 0; i < 27;) {
    rk[i] = A;
    ER32(B, A, i++);
    rk[i] = A;
    ER32(C, A, i++);
    rk[i] = A;
    ER32(D, A, i++);
  }
}

void Speck64128Encrypt(const u32 Pt[], u32 Ct[], const u32 rk[]) {
  u32 i;
  Ct[0] = Pt[0];
  Ct[1] = Pt[1];
  for (i = 0; i < 27;)
    ER32(Ct[1], Ct[0], rk[i++]);
}

void Speck64128Decrypt(u32 Pt[], const u32 Ct[], u32 const rk[]) {
  int i;
  Pt[0] = Ct[0];
  Pt[1] = Ct[1];
  for (i = 26; i >= 0;)
    DR32(Pt[1], Pt[0], rk[i--]);
}

/******************************** buffers ********************************/

void reset_buffer(u64 *buffer) {
  // printf("null_buffer_value = %llu\n", null_buffer_value);
  for (u64 i = 0; i < buffer_size; i++) {
    buffer[i] = null_buffer_value;
  }
}

void buffer_setup(u64 size) {
  buffer_size = size;

  char hdsize[8];
  human_format(size * sizeof(*A), hdsize);
  if (mpi_rank == 0)
    printf("Buffer size: %sB\n", hdsize);

  buffers = malloc(sizeof(*buffers) * mpi_size);
  if (buffers == NULL) {
    err(1, "impossible de réserver la liste des buffers");
  }
  buffer_idx = malloc(sizeof(*buffer_idx) * mpi_size);
  if (buffer_idx == NULL) {
    err(1, "impossible de réserver la liste des index");
  }
  requests = malloc(sizeof(*requests) * mpi_size);
  if (requests == NULL) {
    err(1, "impossible de réserver la liste des requêtes MPI");
  }
  for (int i = 0; i < mpi_size; i++) {
    buffer_idx[i] = 0;
    buffers[i] = malloc(sizeof(*buffers[i]) * size * 2); // x et f(x)
    if (buffers[i] == NULL) {
      err(1, "impossible de réserver les buffers");
    }
    reset_buffer(buffers[i]);
  }
}

/******************************** dictionary ********************************/

/*
 * "classic" hash table for 64-bit key-value pairs, with linear probing.
 * It operates under the assumption that the keys are somewhat random 64-bit
 * integers. The keys are only stored modulo 2**32 - 5 (a prime number), and
 * this can lead to some false positives.
 */
static const u32 EMPTY = 0xffffffff;
static const u64 PRIME = 0xfffffffb;

/* allocate a hash table with `size` slots (12*size bytes) */
void dict_setup(u64 size) {
  dict_size = size;
  char hdsize[8];
  human_format(dict_size * sizeof(*A), hdsize);
  if (mpi_rank == 0)
    printf("Dictionary size: %sB\n", hdsize);

  A = malloc(sizeof(*A) * dict_size);
  if (A == NULL)
    err(1, "impossible to allocate the dictionnary");
  for (u64 i = 0; i < dict_size; i++)
    A[i].k = EMPTY;
}

/* Insert the binding key |----> value in the dictionnary */
// thread safe version
// ! Must be using gcc version >= 12 to compile
void dict_insert(u64 key, u64 value) {
  u64 h = murmur64(key) % dict_size;

  key = key % PRIME;

  bool ok = false;
  while (!ok) {
#pragma omp atomic compare capture
    {
      ok = A[h].k == EMPTY;
      if (ok) {
        A[h].k = key;
      }
    }
    h = (h + 1) % dict_size;
  }

  h = ((h - 1) + dict_size) % dict_size;

  assert(A[h].k == key);
  A[h].v = value;
}

/* Query the dictionnary with this `key`.  Write values (potentially)
 *  matching the key in `values` and return their number. The `values`
 *  array must be preallocated of size (at least) `maxval`.
 *  The function returns -1 if there are more than `maxval` results.
 */
// already thread safe
int dict_probe(u64 key, int maxval, u64 values[]) {
  u32 k = key % PRIME;
  u64 h = murmur64(key) % dict_size;
  int nval = 0;
  for (;;) {
    if (A[h].k == EMPTY)
      return nval;
    if (A[h].k == k) {
      if (nval == maxval)
        return -1;
      values[nval] = A[h].v;
      nval += 1;
    }
    h += 1;
    if (h == dict_size)
      h = 0;
  }
}

/***************************** MITM problem ***********************************/

/* f : {0, 1}^n --> {0, 1}^n.  Speck64-128 encryption of P[0], using k */
u64 f(u64 k) {
  assert((k & mask) == k);
  u32 K[4] = {k & 0xffffffff, k >> 32, 0, 0};
  u32 rk[27];
  Speck64128KeySchedule(K, rk);
  u32 Ct[2];
  Speck64128Encrypt(P[0], Ct, rk);
  return ((u64)Ct[0] ^ ((u64)Ct[1] << 32)) & mask;
}

/* g : {0, 1}^n --> {0, 1}^n.  speck64-128 decryption of C[0], using k */
u64 g(u64 k) {
  assert((k & mask) == k);
  u32 K[4] = {k & 0xffffffff, k >> 32, 0, 0};
  u32 rk[27];
  Speck64128KeySchedule(K, rk);
  u32 Pt[2];
  Speck64128Decrypt(Pt, C[0], rk);
  return ((u64)Pt[0] ^ ((u64)Pt[1] << 32)) & mask;
}

bool is_good_pair(u64 k1, u64 k2) {
  u32 Ka[4] = {k1 & 0xffffffff, k1 >> 32, 0, 0};
  u32 Kb[4] = {k2 & 0xffffffff, k2 >> 32, 0, 0};
  u32 rka[27];
  u32 rkb[27];
  Speck64128KeySchedule(Ka, rka);
  Speck64128KeySchedule(Kb, rkb);
  u32 mid[2];
  u32 Ct[2];
  Speck64128Encrypt(P[1], mid, rka);
  Speck64128Encrypt(mid, Ct, rkb);
  return (Ct[0] == C[1][0]) && (Ct[1] == C[1][1]);
}

/******************************************************************************/

/* search the "golden collision" */
int golden_claw_search(u64 k1[], u64 k2[]) {
  double start = wtime();
  u64 N = 1ull << n;

#pragma omp parallel for schedule(static)
  for (u64 x = mpi_rank; x < N; x += mpi_size) {
    u64 z = f(x);
    int r = z % mpi_size;
    if (r == mpi_rank) {
      dict_insert(z, x);
    } else {
      u64 index;
#pragma omp atomic capture
      index = buffer_idx[r]++;
      buffers[r][index * 2] = x;
      buffers[r][index * 2 + 1] = z;
      if (buffer_idx[r] >= buffer_size) {
        err(1, "cannot insert in buffer anymore !\n");
      }
    }
  }
  // printf("rank %d filled its buffers\n", mpi_rank);

  // for each rank
  for (int rank = 0; rank < mpi_size; rank++) {
    if (rank == mpi_rank) {
      // send data to every other rank
      for (int i = 0; i < mpi_size; i++) {
        if (i != mpi_rank) {
          // printf("rank %d sending to rank %d\n", mpi_rank, i);
          // ! soit non bloquant ou all-to-all
          MPI_Send(buffers[i], buffer_size * 2, MPI_UINT64_T, i, TO_INSERT_TAG,
                   MPI_COMM_WORLD);
          // printf("rank %d sent to rank %d\n", mpi_rank, i);
        }
      }
    } else {
      // receive data from process of rank 'rank'
      // printf("rank %d receiving from rank %d\n", mpi_rank, rank);
      MPI_Status status;
      MPI_Recv(buffers[mpi_rank], buffer_size * 2, MPI_UINT64_T, rank,
               TO_INSERT_TAG, MPI_COMM_WORLD, &status);

      // printf("rank %d received from rank %d\n", mpi_rank, rank);
      // insert data

#pragma omp parallel for schedule(static)
      for (u64 j = 0; j < buffer_size; j++) {

        u64 x = buffers[mpi_rank][j * 2];
        u64 z = buffers[mpi_rank][j * 2 + 1];
        // !!!! SHOULD BE null_buffer_value = 18446744073709551615
        // !!!! BUT IT IS REPLACED BY 0
        if (x == 0 || z == 0 || x == null_buffer_value ||
            z == null_buffer_value)
          continue;
        // printf("%llu, %llu\n", z, x);
        dict_insert(z, x);
      }
      reset_buffer(buffers[mpi_rank]);
      // printf("rank %d inserted from rank %d\n", mpi_rank, rank);
    }
  }

  double mid = wtime();
  if (mpi_rank == 0)
    printf("Fill: %.1fs\n", mid - start);

  int nres = 0;
  u64 ncandidates = 0;
  u64 x[256];

  // reset buffers
  for (int i = 0; i < mpi_size; i++) {
    buffer_idx[i] = 0;
    reset_buffer(buffers[i]);
  }
#pragma omp parallel for schedule(static)
  for (u64 z = mpi_rank; z < N; z += mpi_size) {
    u64 y = g(z);
    int r = y % mpi_size;
    if (r == mpi_rank) {
      int nx = dict_probe(y, 256, x);
      assert(nx >= 0);
      ncandidates += nx;
      for (int i = 0; i < nx; i++) {
        if (is_good_pair(x[i], z)) {
          // if (nres == maxres)
          //   return -1;
          int index;
#pragma omp atomic capture
          index = nres++;
          k1[index] = x[i];
          k2[index] = z;
          printf("SOLUTION FOUND!\n");
          break; // Optionnel : arrêter si une solution est trouvée
        }
      }
    } else {
      // add to buffer
      u64 index;
#pragma omp atomic capture
      index = buffer_idx[r]++;
      buffers[r][index * 2] = y;
      buffers[r][index * 2 + 1] = z;
      if (buffer_idx[r] >= buffer_size) {
        err(1, "cannot insert in buffer anymore !\n");
      }
    }
  }

  // for each rank
  for (int rank = 0; rank < mpi_size; rank++) {
    // if a solution is found no need to share the data anymore
    int global_nres;
    MPI_Allreduce(&nres, &global_nres, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    if (global_nres > 0) {
      break;
    }
    if (rank == mpi_rank) {
      // send data to every other processes
      for (int i = 0; i < mpi_size; i++) {
        if (i != mpi_rank) {
          MPI_Send(buffers[i], buffer_size * 2, MPI_UINT64_T, i, TO_INSERT_TAG,
                   MPI_COMM_WORLD);
        }
      }
    } else {
      // receive data from processes of rank 'rank'
      MPI_Status status;
      MPI_Recv(buffers[mpi_rank], buffer_size * 2, MPI_UINT64_T, rank,
               TO_INSERT_TAG, MPI_COMM_WORLD, &status);

      // #pragma omp parallel for schedule(static)
      for (u64 j = 0; j < buffer_size; j++) {
        u64 y = buffers[mpi_rank][j * 2];
        u64 z = buffers[mpi_rank][j * 2 + 1];
        if (y == null_buffer_value || z == null_buffer_value)
          continue;
        int nx = dict_probe(y, 256, x);
        assert(nx >= 0);
        ncandidates += nx;
        for (int i = 0; i < nx; i++) {
          if (is_good_pair(x[i], z)) {
            int index;
#pragma omp atomic capture
            index = nres++;
            k1[index] = x[i];
            k2[index] = z;
            printf("SOLUTION FOUND!\n");
            assert(f(k1[index]) == g(k2[index]));
            assert(is_good_pair(k1[index], k2[index]));
            printf("Solution found: (%" PRIx64 ", %" PRIx64 ") [checked OK]\n",
                   k1[index], k2[index]);
          }
        }
      }
      reset_buffer(buffers[mpi_rank]);
    }
  }
  if (mpi_rank == 0)
    printf("Probe: %.1fs. %" PRId64 " candidate pairs tested\n", wtime() - mid,
           ncandidates);
  return nres;
}

/************************** command-line options ****************************/

void usage(char **argv) {
  printf("%s [OPTIONS]\n\n", argv[0]);
  printf("Options:\n");
  printf("--n N                       block size [default 24]\n");
  printf("--C0 N                      1st ciphertext (in hex)\n");
  printf("--C1 N                      2nd ciphertext (in hex)\n");
  printf("\n");
  printf("All arguments are required\n");
  exit(0);
}

void process_command_line_options(int argc, char **argv) {
  struct option longopts[4] = {{"n", required_argument, NULL, 'n'},
                               {"C0", required_argument, NULL, '0'},
                               {"C1", required_argument, NULL, '1'},
                               {NULL, 0, NULL, 0}};
  char ch;
  int set = 0;
  while ((ch = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
    switch (ch) {
    case 'n':
      n = atoi(optarg);
      mask = (1ull << n) - 1;
      break;
    case '0':
      set |= 1;
      u64 c0 = strtoull(optarg, NULL, 16);
      C[0][0] = c0 & 0xffffffff;
      C[0][1] = c0 >> 32;
      break;
    case '1':
      set |= 2;
      u64 c1 = strtoull(optarg, NULL, 16);
      C[1][0] = c1 & 0xffffffff;
      C[1][1] = c1 >> 32;
      break;
    default:
      errx(1, "Unknown option\n");
    }
  }
  if (n == 0 || set != 3) {
    usage(argv);
    exit(1);
  }
}

/******************************************************************************/

int main(int argc, char **argv) {
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
  process_command_line_options(argc, argv);
  if (mpi_rank == 0)
    printf("Running with n=%d, C0=(%08x, %08x) and C1=(%08x, %08x)\n", (int)n,
           C[0][0], C[0][1], C[1][0], C[1][1]);

  dict_setup((1.125 * (1ull << n)) / mpi_size);
  buffer_setup(dict_size / mpi_size * 1.2);

  /* search */
  u64 k1[16], k2[16];
  golden_claw_search(k1, k2);
  // assert(nkey > 0);

  /* validation */
  // for (int i = 0; i < nkey; i++) {
  //   assert(f(k1[i]) == g(k2[i]));
  //   assert(is_good_pair(k1[i], k2[i]));
  //   printf("Solution found: (%" PRIx64 ", %" PRIx64 ") [checked OK]\n",
  //   k1[i],
  //          k2[i]);
  // }
  MPI_Finalize();
}