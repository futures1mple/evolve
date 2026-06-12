#include <stdio.h>
#include <stdlib.h>
#include "poly.h"

static int mod_q(long long x) {
    int r = (int)(x % Q);
    if (r < 0) r += Q;
    return r;
}

/* Centred representative: maps [0,Q) -> (−Q/2, Q/2] */
static int centered_coeff(int x) {
    if (x > Q / 2) x -= Q;
    return x;
}

Poly poly_zero(void) {
    Poly p;
    for (int i = 0; i < N; i++) p.coeffs[i] = 0;
    return p;
}

Poly poly_constant(int x) {
    Poly p = poly_zero();
    p.coeffs[0] = mod_q(x);
    return p;
}

Poly poly_add(Poly a, Poly b) {
    Poly r;
    for (int i = 0; i < N; i++)
        r.coeffs[i] = mod_q((long long)a.coeffs[i] + b.coeffs[i]);
    return r;
}

Poly poly_sub(Poly a, Poly b) {
    Poly r;
    for (int i = 0; i < N; i++)
        r.coeffs[i] = mod_q((long long)a.coeffs[i] - b.coeffs[i]);
    return r;
}

/* Full dense multiplication in Z_q[x]/(x^N + 1) */
Poly poly_mul_dense(Poly a, Poly b) {
    long long tmp[2 * N];
    for (int i = 0; i < 2 * N; i++) tmp[i] = 0;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            tmp[i + j] += (long long)a.coeffs[i] * b.coeffs[j];
    Poly r = poly_zero();
    for (int i = 0; i < N; i++)        r.coeffs[i] = mod_q(tmp[i]);
    for (int i = N; i < 2 * N; i++)    r.coeffs[i - N] = mod_q((long long)r.coeffs[i - N] - tmp[i]);
    return r;
}

/* Optimised multiplication: b must have small (centred) coefficients.
 * Works whether a is small or uniform — only b needs to be small. */
Poly poly_mul_by_small(Poly a, Poly b) {
    Poly r = poly_zero();
    for (int j = 0; j < N; j++) {
        int coeff = centered_coeff(b.coeffs[j]);
        if (coeff == 0) continue;
        for (int i = 0; i < N; i++) {
            int idx = i + j;
            long long term = (long long)a.coeffs[i] * coeff;
            if (idx < N)
                r.coeffs[idx]     = mod_q((long long)r.coeffs[idx]     + term);
            else
                r.coeffs[idx - N] = mod_q((long long)r.coeffs[idx - N] - term);
        }
    }
    return r;
}

Poly poly_mul(Poly a, Poly b) { return poly_mul_dense(a, b); }

Poly poly_scalar_mul(Poly a, int scalar) {
    Poly r;
    for (int i = 0; i < N; i++) {
        long long val = (long long)a.coeffs[i] * scalar;
        int m = (int)(val % Q);
        if (m < 0) m += Q;
        r.coeffs[i] = m;
    }
    return r;
}

/* Small polynomial: coefficients uniform in {−bound, …, +bound} */
Poly sample_small_poly(int bound) {
    Poly p;
    for (int i = 0; i < N; i++) {
        int x = (rand() % (2 * bound + 1)) - bound;
        p.coeffs[i] = mod_q(x);
    }
    return p;
}

/* Uniform polynomial: coefficients uniform in [0, Q).
 *
 * On Windows (MinGW) RAND_MAX = 32767 (15 bits).
 * We combine three 15-bit calls to get ≥ 31 bits of randomness,
 * then reduce mod Q.  The bias from the modulo is negligible
 * (|{0..Q−1}| = Q ≈ 2^31, drawn range ≈ 2^45).
 *
 * FIX 1: commitment keys now use uniform polynomials so that the
 * commitment scheme has the hiding/binding properties described in [1].
 */
Poly sample_uniform_poly(void) {
    Poly p;
    for (int i = 0; i < N; i++) {
        unsigned long long r =
            ((unsigned long long)(rand() & 0x7FFF) << 30) |
            ((unsigned long long)(rand() & 0x7FFF) << 15) |
            ((unsigned long long)(rand() & 0x7FFF));
        p.coeffs[i] = (int)(r % (unsigned long long)Q);
    }
    return p;
}

PolyVec sample_small_polyvec(int bound) {
    PolyVec v;
    for (int i = 0; i < D; i++) v.items[i] = sample_small_poly(bound);
    return v;
}

/* Returns 1 iff ‖v‖∞ ≤ bound (centred representation).
 * Used in the Fiat-Shamir abort check. */
int polyvec_is_small(PolyVec v, int bound) {
    for (int i = 0; i < D; i++) {
        for (int j = 0; j < N; j++) {
            int c = v.items[i].coeffs[j];
            if (c > Q / 2) c -= Q;
            if (c < 0) c = -c;
            if (c > bound) return 0;
        }
    }
    return 1;
}

PolyVec polyvec_add(PolyVec a, PolyVec b) {
    PolyVec out;
    for (int i = 0; i < D; i++) out.items[i] = poly_add(a.items[i], b.items[i]);
    return out;
}

PolyVec polyvec_scalar_mul(PolyVec v, int scalar) {
    PolyVec out;
    for (int i = 0; i < D; i++) out.items[i] = poly_scalar_mul(v.items[i], scalar);
    return out;
}

void poly_print(Poly p) {
    printf("[");
    for (int i = 0; i < 8 && i < N; i++) {
        printf("%d", p.coeffs[i]);
        if (i != 7 && i != N - 1) printf(", ");
    }
    if (N > 8) printf(", ...");
    printf("]\n");
}