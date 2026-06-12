#include <stdlib.h>
#include "zkproof.h"
#include "utils.h"

/* ── Accumulated timers ─────────────────────────────────────── */
static double or_proof_generation_time_ms   = 0.0;
static double or_proof_verification_time_ms = 0.0;

/* ── Static helpers ─────────────────────────────────────────── */

static int mod_q_int(long long x) {
    int r = (int)(x % Q);
    if (r < 0) r += Q;
    return r;
}

/* Check whether two commitments are identical (component-wise) */
static int commitments_equal(Commitment x, Commitment y) {
    for (int i = 0; i < N; i++) {
        if (x.a.coeffs[i] != y.a.coeffs[i]) return 0;
        if (x.b.coeffs[i] != y.b.coeffs[i]) return 0;
    }
    return 1;
}

/* ── Fiat-Shamir hash ───────────────────────────────────────── */

/* OR-proof hash: takes the full public input (c, c_shifted, t0, t1)
 * to bind both sub-proofs to a single global challenge e. */
static int fs_challenge_or(
    Commitment c,
    Commitment c_shifted,
    Commitment t0,
    Commitment t1
) {
    unsigned long long h = 1469598103934665603ULL;

    for (int i = 0; i < N; i++) {
        h ^= (unsigned long long)c.a.coeffs[i];         h *= 1099511628211ULL;
        h ^= (unsigned long long)c.b.coeffs[i];         h *= 1099511628211ULL;
        h ^= (unsigned long long)c_shifted.a.coeffs[i]; h *= 1099511628211ULL;
        h ^= (unsigned long long)c_shifted.b.coeffs[i]; h *= 1099511628211ULL;
        h ^= (unsigned long long)t0.a.coeffs[i];        h *= 1099511628211ULL;
        h ^= (unsigned long long)t0.b.coeffs[i];        h *= 1099511628211ULL;
        h ^= (unsigned long long)t1.a.coeffs[i];        h *= 1099511628211ULL;
        h ^= (unsigned long long)t1.b.coeffs[i];        h *= 1099511628211ULL;
    }

    int e = (int)(h % (unsigned long long)FS_CHALLENGE_MOD);
    if (e == 0) e = 1;
    return e;
}

/* ── c_shifted = (c.a,  c.b − 1 mod Q) ─────────────────────── */
static Commitment compute_c_shifted(Commitment c) {
    Commitment cs = c;                        /* copy a unchanged */
    cs.b.coeffs[0] = mod_q_int((long long)c.b.coeffs[0] - 1);
    return cs;
}

/* ── OR proof generation ────────────────────────────────────── */
/*
 * Proves x ∈ {0,1} via the Cramer–Damgård–Schoenmakers construction:
 *
 *   REAL side   → honest Sigma sub-proof for the disjunct that is true
 *   SIMULATED side → programmed transcript for the other disjunct
 *
 * Fiat-Shamir-with-Aborts (Fix 2):
 *   After computing z_real = r' + e_real · r we check ‖z_real‖∞ ≤ BOUND_Z.
 *   If the check fails we restart.  With bounded-uniform r' and small N,
 *   this ensures z is statistically independent of r.
 *   NOTE: for large N (1024, 2048) the per-polynomial acceptance probability
 *   approaches 1 with these parameters; a Gaussian r' gives ~37% acceptance
 *   regardless of N and is required for a production implementation.
 */
ZKOrProof generate_zk_or_proof(
    CommitmentKey *ck,
    Commitment     c,
    int            x,
    PolyVec        r
) {
    double t_start = now_ms();

    ZKOrProof proof;
    Commitment c_shifted = compute_c_shifted(c);
    int attempts = 0;

    do {
        attempts++;

        if (x == 0) {
            /* ── x = 0: REAL proof for c,  SIMULATED for c_shifted ── */

            /* 1. Simulated side: pick e1 and z1 freely, back-compute t1.
             *    t1 = Com(0, z1) − e1 · c_shifted   ensures
             *    Com(0, z1) = t1 + e1 · c_shifted  (verification eq. passes) */
            proof.e1 = (rand() % (FS_CHALLENGE_MOD - 1)) + 1;
            proof.z1 = sample_small_polyvec(BOUND_R_PRIME);
            Commitment cv1 = commit_value(ck, 0, proof.z1);
            Commitment ec1 = commitment_scalar_mul(c_shifted, proof.e1);
            proof.t1 = commitment_sub(cv1, ec1);

            /* 2. Real side: fresh masking r', announcement t0 */
            PolyVec r_prime = sample_small_polyvec(BOUND_R_PRIME);
            proof.t0 = commit_value(ck, 0, r_prime);

            /* 3. Global Fiat-Shamir challenge */
            int e = fs_challenge_or(c, c_shifted, proof.t0, proof.t1);

            /* 4. Real challenge share: e0 = e − e1  (mod FS_CHALLENGE_MOD) */
            proof.e0 = ((e - proof.e1) % FS_CHALLENGE_MOD + FS_CHALLENGE_MOD)
                       % FS_CHALLENGE_MOD;
            if (proof.e0 == 0) continue;   /* degenerate — restart */

            /* 5. Real response: z0 = r' + e0 · r */
            PolyVec er = polyvec_scalar_mul(r, proof.e0);
            proof.z0   = polyvec_add(r_prime, er);

        } else {
            /* ── x = 1: SIMULATED for c,  REAL proof for c_shifted ── */

            /* 1. Simulated side: pick e0 and z0 freely, back-compute t0.
             *    t0 = Com(0, z0) − e0 · c  ensures
             *    Com(0, z0) = t0 + e0 · c  (verification eq. passes) */
            proof.e0 = (rand() % (FS_CHALLENGE_MOD - 1)) + 1;
            proof.z0 = sample_small_polyvec(BOUND_R_PRIME);
            Commitment cv0 = commit_value(ck, 0, proof.z0);
            Commitment ec0 = commitment_scalar_mul(c, proof.e0);
            proof.t0 = commitment_sub(cv0, ec0);

            /* 2. Real side (c_shifted = Com(0, r) when x=1):
             *    fresh masking r', announcement t1 */
            PolyVec r_prime = sample_small_polyvec(BOUND_R_PRIME);
            proof.t1 = commit_value(ck, 0, r_prime);

            /* 3. Global Fiat-Shamir challenge */
            int e = fs_challenge_or(c, c_shifted, proof.t0, proof.t1);

            /* 4. Real challenge share: e1 = e − e0  (mod FS_CHALLENGE_MOD) */
            proof.e1 = ((e - proof.e0) % FS_CHALLENGE_MOD + FS_CHALLENGE_MOD)
                       % FS_CHALLENGE_MOD;
            if (proof.e1 == 0) continue;   /* degenerate — restart */

            /* 5. Real response: z1 = r' + e1 · r */
            PolyVec er = polyvec_scalar_mul(r, proof.e1);
            proof.z1   = polyvec_add(r_prime, er);
        }

        /* ── Fiat-Shamir ABORT check (Fix 2) ───────────────────
         * Reject if the real response is too large.
         * Both z0 and z1 are checked; the simulated z is always small
         * (sampled directly from BOUND_R_PRIME), so only the real one
         * can possibly exceed BOUND_Z. */
    } while ((!polyvec_is_small(proof.z0, BOUND_Z) ||
              !polyvec_is_small(proof.z1, BOUND_Z)) &&
             attempts < MAX_ZKP_ATTEMPTS);

    double t_end = now_ms();
    or_proof_generation_time_ms += (t_end - t_start);

    return proof;
}

/* ── OR proof verification ──────────────────────────────────── */
/*
 * Checks all three conditions of the OR proof:
 *   (a)  (e0 + e1) ≡ e   mod FS_CHALLENGE_MOD  [challenge consistency]
 *   (b)  Com(0, z0) == t0 + e0 · c              [sub-proof 0]
 *   (c)  Com(0, z1) == t1 + e1 · c_shifted      [sub-proof 1]
 *
 * Note: we do NOT reveal which disjunct is real — both checks are always run.
 */
int verify_zk_or_proof(
    CommitmentKey *ck,
    Commitment     c,
    ZKOrProof      proof
) {
    double t_start = now_ms();

    Commitment c_shifted = compute_c_shifted(c);

    /* (a) Recompute global challenge and check share consistency */
    int e = fs_challenge_or(c, c_shifted, proof.t0, proof.t1);
    if ((proof.e0 + proof.e1) % FS_CHALLENGE_MOD != e) {
        or_proof_verification_time_ms += now_ms() - t_start;
        return 0;
    }

    /* (b) Sub-proof 0:  Com(0, z0) == t0 + e0 · c */
    Commitment left0  = commit_value(ck, 0, proof.z0);
    Commitment right0 = add_commitments(
                            proof.t0,
                            commitment_scalar_mul(c, proof.e0));
    if (!commitments_equal(left0, right0)) {
        or_proof_verification_time_ms += now_ms() - t_start;
        return 0;
    }

    /* (c) Sub-proof 1:  Com(0, z1) == t1 + e1 · c_shifted */
    Commitment left1  = commit_value(ck, 0, proof.z1);
    Commitment right1 = add_commitments(
                            proof.t1,
                            commitment_scalar_mul(c_shifted, proof.e1));
    if (!commitments_equal(left1, right1)) {
        or_proof_verification_time_ms += now_ms() - t_start;
        return 0;
    }

    or_proof_verification_time_ms += now_ms() - t_start;
    return 1;
}

/* ── Timer API ──────────────────────────────────────────────── */
void reset_zk_timers(void) {
    or_proof_generation_time_ms   = 0.0;
    or_proof_verification_time_ms = 0.0;
}

double get_zk_proof_generation_time_ms(void) {
    return or_proof_generation_time_ms;
}

double get_zk_proof_verification_time_ms(void) {
    return or_proof_verification_time_ms;
}