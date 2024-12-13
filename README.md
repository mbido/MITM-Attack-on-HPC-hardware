# Direct Meet-in-the-Middle Attack
This repo relate the project in parallel computing course. It is all about parallelizing a given sequential algorithm *mitm_base.c* using OpenMP and MPI. This README file will guide you through the project, how it is implemented and why it is implemented in this way.

## Instances of the problem :
We can use the web service https://ppar.tme-crypto.fr/<username>/<n> with a *username* and a reasonable number $n$ with $2^n$ being the size of the words taken by $f(x)$, $g(x)$ and $\pi(x, y)$.

ex : https://ppar.tme-crypto.fr/matthieu.bidault/25

It provides us how to invoke the sequential program. Also, it tells us how much RAM is needed to run the program with a given $n$.

| n | RAM needed |
|---|------------|
|$\le21$| 0.0GB|
|$25$|0.4GB|
|$30$|13.5GB|
|$35$| 432.0GB|
|$40$| 13824.0GB|

## My goal :
My goal during this project is to approche the size of the problem with $n=42$ for the beauty of the challenge and which "<u>*begins to be seriously challenging*</u>".

## Before starting the project :
**Learn about <u>sharding</u> apply to dictionaries**
The goal of sharding is to split the dictionary into several parts and to distribute them to the different nodes. This way, each node will have a part of the dictionary and will be able to compute the corresponding values of $f(x)$ and $g(x)$.

To do this, we will calculate the key in the dictionary modulo the number of nodes. This way, we will be able to distribute the keys in the dictionary to the different nodes. Making searching an element in the dictionary faster.

The Probe operation can be easily multithreaded the hard part is to parallelize the insertion part. For that I will use a master process that will use its threads to calculate the keys, put them (and the value) in a buffer (one buffer per MPI process) when a buffer is full, we send it to the correct MPI process that will insert all the keys and their values in its shard. *(Maybe we'll use every MPI process as master process...)*

**Combining <u>OpenMP</u> and <u>MPI</u>**
To use efficiently the ressources of th cluster, we want **MPI** to use only one process on one core of each **CPU** which will be in charge of the communication between the **CPUs** and the **nodes**. Then, we want **OpenMP** to use all the cores/threads of the **CPU**.

Since we start using **MPI** we have been using it with it's default configuration that uses one process per core on every nodes it can. To make sure that **MPI** uses only one process per **CPU** we have to use the following command :

```bash
mpirun --hostfile $OAR_NODEFILE \
       --map-by ppr:1:socket \
       --bind-to socket \
       ./mitm ...
```

Explanation of the command :
- `--hostfile $OAR_NODEFILE` : This command tells **MPI** to use the nodes given in the file `$OAR_NODEFILE`.
- `--map-by ppr:1:socket` : This command tells **MPI** to use only one process per socket.
- `--bind-to socket` : This command tells **MPI** to bind the process to the socket.

On some clusters we will need to use the following command :

```bash
mpirun --hostfile $OAR_NODEFILE \
       --map-by ppr:1:socket \
       --bind-to socket \
       --mca pml ^ucx \
       ./mitm ...
```

`--mca pml ^ucx` : This command tells **MPI** to use the `ob1` protocol instead of the `ucx` protocol. Otherwise, the program try to use the `ucx` protocol which is not supported by the cluster and send an error.




**Before Compiling on Grid5000 :**
```sh
module load gcc/12
```
