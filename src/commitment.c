#include <stdio.h>
#include "commitment.h"

/* Inner product: Σ coeffs[i] · r[i]
 * coeffs[i] can be uniform (large); r[i] must be small — use poly_mul_by_small. */
static Poly lincomb(Poly coeffs[D], PolyVec r) {
    Poly sum = poly_zero();
    for (int i = 0; i < D; i++) {
        Poly t = poly_mul_by_small(coeffs[i], r.items[i]);
        sum = poly_add(sum, t);
    }
    return sum;
}

/* FIX 1: sample A and B uniformly from R_q instead of from a small set.
 * The original code used sample_small_poly(BOUND_R) for the key, which
 * weakened binding.  Uniform A, B gives the standard MLWE-based commitment. */
CommitmentKey commitment_keygen(void) {
    CommitmentKey ck;
    for (int i = 0; i < D; i++) {
        ck.A[i] = sample_uniform_poly();
        ck.B[i] = sample_uniform_poly();
    }
    return ck;
}

/* Com(x, r) = ( A·r,  B·r + x ) */
Commitment commit_value(CommitmentKey *ck, int x, PolyVec r) {
    Commitment c;
    c.a = lincomb(ck->A, r);
    c.b = poly_add(lincomb(ck->B, r), poly_constant(x));
    return c;
}

int open_commitment(CommitmentKey *ck, Commitment c, int x, PolyVec r) {
    Commitment expected = commit_value(ck, x, r);
    for (int i = 0; i < N; i++) {
        if (c.a.coeffs[i] != expected.a.coeffs[i]) return 0;
        if (c.b.coeffs[i] != expected.b.coeffs[i]) return 0;
    }
    return 1;
}

/* c1 + c2  (homomorphic addition) */
Commitment add_commitments(Commitment c1, Commitment c2) {
    Commitment out;
    out.a = poly_add(c1.a, c2.a);
    out.b = poly_add(c1.b, c2.b);
    return out;
}

/* c1 − c2  (needed for OR-proof simulation step) */
Commitment commitment_sub(Commitment c1, Commitment c2) {
    Commitment out;
    out.a = poly_sub(c1.a, c2.a);
    out.b = poly_sub(c1.b, c2.b);
    return out;
}

/* scalar · c  (needed for Sigma-protocol right-hand side) */
Commitment commitment_scalar_mul(Commitment c, int scalar) {
    Commitment out;
    out.a = poly_scalar_mul(c.a, scalar);
    out.b = poly_scalar_mul(c.b, scalar);
    return out;
}

void commitment_print(Commitment c) {
    printf("Commitment.a = "); poly_print(c.a);
    printf("Commitment.b = "); poly_print(c.b);
}