#ifndef COMMITMENT_H
#define COMMITMENT_H

#include "poly.h"

typedef struct {
    Poly A[D];   /* uniform random — public commitment matrix */
    Poly B[D];   /* uniform random — public commitment matrix */
} CommitmentKey;

typedef struct {
    Poly a;
    Poly b;
} Commitment;

CommitmentKey commitment_keygen(void);

/* Com(x, r) = (A·r,  B·r + x) */
Commitment commit_value(CommitmentKey *ck, int x, PolyVec r);

int open_commitment(CommitmentKey *ck, Commitment c, int x, PolyVec r);

/* Homomorphic operations */
Commitment add_commitments(Commitment c1, Commitment c2);
Commitment commitment_sub(Commitment c1, Commitment c2);       /* c1 − c2 */
Commitment commitment_scalar_mul(Commitment c, int scalar);    /* scalar · c */

void commitment_print(Commitment c);

#endif /* COMMITMENT_H */