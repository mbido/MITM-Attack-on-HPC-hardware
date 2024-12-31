#### Commands
```sh
./mitm --n 25 --C0 31ee5bc1768c926e --C1 5e3d5c404731f41b
./mitm --n 26 --C0 3b8a5737c6c27f68 --C1 87278fe44a61c77b
./mitm --n 27 --C0 cf01b262bdfd732c --C1 36874317b6eca0aa
./mitm --n 28 --C0 3f4c537cdeda25ec --C1 3aabcecd9eb40348
./mitm --n 29 --C0 dae7174727466f5b --C1 d433f34f27a046eb
./mitm --n 30 --C0 a5d4594d6e280f62 --C1 15119ab5cd8bda08
./mitm --n 31 --C0 ab1558e06baac72d --C1 52a5a49e48ea195e
./mitm --n 32 --C0 dfee6b909f763142 --C1 c6eea53f30623e17
./mitm --n 33 --C0 9cf56b655ed39da5 --C1 69a849812079c2f1
./mitm --n 34 --C0 4820438465644560 --C1 26aa4556a22550b7
./mitm --n 35 --C0 c6e809f51cb4a1bb --C1 475b32527284e267
./mitm --n 36 --C0 46ee609ee5affc2c --C1 9defa2257f0f48d0
./mitm --n 37 --C0 3b552c34a5c6c8f4 --C1 1e3f0c95a766f626
./mitm --n 38 --C0 3b7b031d14e12ed7 --C1 d99f2cfe9295ef99
./mitm --n 39 --C0 60d7eb9930550602 --C1 16251bc7f0080c45
./mitm --n 40 --C0 ef1cade1d23db055 --C1 bf66b3be166329ab
./mitm --n 41 --C0 625dbffce4336b90 --C1 b73f8d06214f22bd
./mitm --n 42 --C0 f874afa6931781f7 --C1 21d39aab009c46d4
```

### Real Tests





























### tests on 15 paradoxe nodes at Rennes

#### Only MPI
- n = 32
  - time : 2m26.868s
  - solution : (43c0a3b0, 409e555b)

#### MPI + OpenMP
- n = 32
  - time : 1m57.977s
  - solution : (43c0a3b0, 409e555b)
- n = 35
  - ??

#### Only OpenMP (no communications between the shards)
- n = 32
  - time : 0m22.810s
  - solution : (43c0a3b0, 409e555b)
- n = 35
  - time : 2m19.511s
  - solution : (2ff78ba9b, 2984bb346)
- n = 37
  - time : 8m58.987s
  - solution : (1bba107715, d24b49a9c)

### tests on 40 gros nodes at Nancy (990 cores)

#### MPI + OpenMP
- n = 20
  - time : 0m9.658s
  - solution : (ceab4, f7e66)
- n = 28
  - time : 0m12.196s
  - solution : (fd8c5d4, 272f42)
- n = 31
  - time : 0m37.829s
  - solution : (2d0e1ead, 6f3f80ad)
- n = 32
  - time : 0m44.773s
  - solution : (43c0a3b0, 409e555b)
- n = 35
  - time : 
  - solution : 

### tests on 55 gros nodes at Nancy (990 cores)

#### MPI + OpenMP
- n = 31
  - time : 0m47.601s
  - solution : (2d0e1ead, 6f3f80ad)
- n = 35
  - time : 11m18.889s
  - solution : (2ff78ba9b, 2984bb346)

#### Only OpenMP (no communications between the shards)
- n = 32
  - time : 0m32.305s
  - solution : (43c0a3b0, 409e555b)
- n = 35
  - time : 3m25.613s
  - solution : (2ff78ba9b, 2984bb346)

### tests on 30 nodes at Nantes (60 processes, 600 cores):

#### MPI + OpenMP
- n = 31
  - time : 1m7.707s
  - solution : (2d0e1ead, 6f3f80ad)
- n = 32
  - time : 2m2.697s
  - solution : (43c0a3b0, 409e555b)

### Only OpenMP
- n = 31
  - time : 0m29.179s
  - solution : (2d0e1ead, 6f3f80ad)
- n = 32
  - time : 0m39.373s
  - solution : (43c0a3b0, 409e555b)
- n = 35
  - time : 6m5.802s
  - solution : (2ff78ba9b, 2984bb346)
