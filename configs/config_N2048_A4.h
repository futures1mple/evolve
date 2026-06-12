#ifndef CONFIG_H
#define CONFIG_H

/* Lattice parameters */
#define N         2048
#define Q         2147483647
#define D         7
#define BOUND_R   1

/* ZKP parameters (required by zkproof.c) */
#define FS_CHALLENGE_MOD  17
#define BOUND_R_PRIME     64
#define BOUND_Z           81       /* = BOUND_R_PRIME + FS_CHALLENGE_MOD * BOUND_R */
#define MAX_ZKP_ATTEMPTS  200

/* Protocol parameters */
#define NUM_AUTHORITIES  4
#define NUM_VOTERS       100
#define BUCKET_SIZE      30

#endif