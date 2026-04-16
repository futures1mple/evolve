#include <stdio.h>
#include "commitment.h"

/*
 * Compute linear combination:
 *   sum_i coeffs[i] * r[i]
 *
 * Here r[i] is sampled from a small distribution, so we use the optimized
 * multiplication routine poly_mul_by_small().
 */
static Poly lincomb(Poly coeffs[D], PolyVec r) {
    Poly sum = poly_zero();

    for (int i = 0; i < D; i++) {
        Poly t = poly_mul_by_small(coeffs[i], r.items[i]);
        sum = poly_add(sum, t);
    }

    return sum;
}

CommitmentKey commitment_keygen(void) {
    CommitmentKey ck;

    for (int i = 0; i < D; i++) {
        ck.A[i] = sample_small_poly(BOUND_R);
        ck.B[i] = sample_small_poly(BOUND_R);
    }

    return ck;
}

Commitment commit_value(CommitmentKey *ck, int x, PolyVec r) {
    Commitment c;

    c.a = lincomb(ck->A, r);
    c.b = poly_add(lincomb(ck->B, r), poly_constant(x));

    return c;
}

int open_commitment(CommitmentKey *ck, Commitment c, int x, PolyVec r) {
    Commitment expected = commit_value(ck, x, r);

    for (int i = 0; i < N; i++) {
        if (c.a.coeffs[i] != expected.a.coeffs[i]) {
            return 0;
        }
        if (c.b.coeffs[i] != expected.b.coeffs[i]) {
            return 0;
        }
    }

    return 1;
}

Commitment add_commitments(Commitment c1, Commitment c2) {
    Commitment out;
    out.a = poly_add(c1.a, c2.a);
    out.b = poly_add(c1.b, c2.b);
    return out;
}

void commitment_print(Commitment c) {
    printf("Commitment.a = ");
    poly_print(c.a);
    printf("Commitment.b = ");
    poly_print(c.b);
}