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

/* General dense multiplication */
Poly poly_mul_dense(Poly a, Poly b);

/* Optimized multiplication when second operand is a small polynomial */
Poly poly_mul_by_small(Poly a, Poly b);

/* Alias for default multiplication */
Poly poly_mul(Poly a, Poly b);

Poly sample_small_poly(int bound);
PolyVec sample_small_polyvec(int bound);

void poly_print(Poly p);

#endif