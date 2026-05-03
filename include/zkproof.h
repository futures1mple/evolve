#ifndef ZKPROOF_H
#define ZKPROOF_H

#include "commitment.h"

#define FS_CHALLENGE_MOD 17

typedef struct {
    Commitment t;
    int challenge;
    PolyVec z;
} ZKProof;

ZKProof generate_zk_proof(
    CommitmentKey *ck,
    Commitment c,
    int x,
    PolyVec r
);

int verify_zk_proof(
    CommitmentKey *ck,
    Commitment c,
    int x,
    ZKProof proof
);

void reset_zk_timers(void);
double get_zk_proof_generation_time_ms(void);
double get_zk_proof_verification_time_ms(void);

#endif