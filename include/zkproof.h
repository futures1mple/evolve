#ifndef ZKPROOF_H
#define ZKPROOF_H

#include "commitment.h"

/* FS_CHALLENGE_MOD is now in config.h */

/* ─────────────────────────────────────────────────────────────────────────
 * ZKOrProof  —  OR zero-knowledge proof  (Fix 3)
 *
 * Proves: "commitment c opens to x = 0"  OR  "commitment c opens to x = 1"
 * without revealing which case holds and without revealing x or the
 * randomness r.
 *
 * Structure (Cramer–Damgård–Schoenmakers OR-proof adapted to lattices):
 *
 *   Let c_shifted = (c.a,  c.b − 1)
 *     • If x=0: c       = Com(0, r)   ← real proof
 *               c_shifted = Com(-1, r) ← simulated proof
 *     • If x=1: c_shifted = Com(0, r)  ← real proof
 *               c          = Com(1, r) ← simulated proof
 *
 *   Global Fiat-Shamir challenge: e = H(c, c_shifted, t0, t1)
 *   Constraint:  (e0 + e1) ≡ e  (mod FS_CHALLENGE_MOD)
 *
 *   Sub-proof 0 verifies:  Com(0, z0) == t0 + e0 · c
 *   Sub-proof 1 verifies:  Com(0, z1) == t1 + e1 · c_shifted
 * ──────────────────────────────────────────────────────────────────────── */
typedef struct {
    Commitment t0;   /* first  announcement  */
    int        e0;   /* first  challenge share  */
    PolyVec    z0;   /* first  response          */
    Commitment t1;   /* second announcement  */
    int        e1;   /* second challenge share  */
    PolyVec    z1;   /* second response          */
} ZKOrProof;

/* Generate the OR proof for ballot commitment c with vote x ∈ {0,1}
 * and randomness r (known by the voter).
 * Implements Fiat-Shamir-with-Aborts loop (Fix 2). */
ZKOrProof generate_zk_or_proof(
    CommitmentKey *ck,
    Commitment     c,
    int            x,
    PolyVec        r
);

/* Verify the OR proof.  Returns 1 if valid, 0 otherwise. */
int verify_zk_or_proof(
    CommitmentKey *ck,
    Commitment     c,
    ZKOrProof      proof
);

/* Timer management (called from main.c) */
void   reset_zk_timers(void);
double get_zk_proof_generation_time_ms(void);
double get_zk_proof_verification_time_ms(void);

#endif /* ZKPROOF_H */