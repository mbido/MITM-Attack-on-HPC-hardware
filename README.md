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
**Learn about <u>sharding</u>**
