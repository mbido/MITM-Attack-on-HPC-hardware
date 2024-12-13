#### Commands
```sh
./mitm --n 20 --C0 6184c52d0df0137a --C1 915d64a00528cfc0
./mitm --n 31 --C0 8f382b169e3ef269 --C1 a4e74e0ceb927493
./mitm --n 32 --C0 9dd43cc240131938 --C1 03a38ed28e9e6171
./mitm --n 35 --C0 baf19ec5610866cb --C1 7e074786450ee37f
./mitm --n 37 --C0 be9a6fc43423c485 --C1 789d8c2530c54c8d
./mitm --n 40 --C0 70e20a208dd9a4ee --C1 22b9c07cffb0fae1
./mitm --n 42 --C0 46da2c4e8d8d7705 --C1 5da937a1279f80f6
```

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
  - time : 2m2.320s
  - solution : (43c0a3b0, 409e555b)

### Only OpenMP
- n = 31
  - time : 0m29.179s
  - solution : (2d0e1ead, 6f3f80ad)
- n = 32
  - time : 
  - solution : (43c0a3b0, 409e555b)
- n = 35
  - time : 6m5.802s
  - solution : (2ff78ba9b, 2984bb346)
