#ifndef POLY_H
#define POLY_H

#include "config.h"

typedef struct {
    int coeffs[N];
} Poly;

typedef struct {
    Poly items[D];
} PolyVec;

Poly poly_zero(void);
Poly poly_constant(int x);

Poly poly_add(Poly a, Poly b);
Poly poly_sub(Poly a, Poly b);

/* General dense O(N^2) multiplication mod (X^N+1, Q) */
Poly poly_mul_dense(Poly a, Poly b);

/* Optimised: use only when second operand has small coefficients */
Poly poly_mul_by_small(Poly a, Poly b);

Poly poly_mul(Poly a, Poly b);

Poly poly_scalar_mul(Poly a, int scalar);

/* Sample a polynomial whose coefficients are uniform in {-bound..+bound} */
Poly sample_small_poly(int bound);

/* Sample a polynomial whose coefficients are uniform in [0, Q)
 * Used for the commitment key (Fix 1: uniform key) */
Poly sample_uniform_poly(void);

PolyVec sample_small_polyvec(int bound);

/* Returns 1 iff every coefficient of v (in centred representation) is ≤ bound
 * Used in the Fiat-Shamir abort check (Fix 2) */
int polyvec_is_small(PolyVec v, int bound);

PolyVec polyvec_add(PolyVec a, PolyVec b);
PolyVec polyvec_scalar_mul(PolyVec v, int scalar);

void poly_print(Poly p);

#endif /* POLY_H */