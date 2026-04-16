#ifndef COMMITMENT_H
#define COMMITMENT_H

#include "poly.h"

typedef struct {
    Poly A[D];
    Poly B[D];
} CommitmentKey;

typedef struct {
    Poly a;
    Poly b;
} Commitment;

CommitmentKey commitment_keygen(void);

Commitment commit_value(CommitmentKey *ck, int x, PolyVec r);
int open_commitment(CommitmentKey *ck, Commitment c, int x, PolyVec r);

Commitment add_commitments(Commitment c1, Commitment c2);

void commitment_print(Commitment c);

#endif