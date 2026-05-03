#include "zkproof.h"
#include "utils.h"

static double proof_generation_time_ms = 0.0;
static double proof_verification_time_ms = 0.0;

static int mod_q_int(long long x) {
    int r = (int)(x % Q);
    if (r < 0) r += Q;
    return r;
}

static int fs_challenge(Commitment c, Commitment t) {
    unsigned long long h = 1469598103934665603ULL;

    for (int i = 0; i < N; i++) {
        h ^= (unsigned long long)c.a.coeffs[i];
        h *= 1099511628211ULL;

        h ^= (unsigned long long)c.b.coeffs[i];
        h *= 1099511628211ULL;

        h ^= (unsigned long long)t.a.coeffs[i];
        h *= 1099511628211ULL;

        h ^= (unsigned long long)t.b.coeffs[i];
        h *= 1099511628211ULL;
    }

    int e = (int)(h % FS_CHALLENGE_MOD);
    if (e == 0) e = 1;

    return e;
}

static int commitments_equal(Commitment x, Commitment y) {
    for (int i = 0; i < N; i++) {
        if (x.a.coeffs[i] != y.a.coeffs[i]) return 0;
        if (x.b.coeffs[i] != y.b.coeffs[i]) return 0;
    }

    return 1;
}

static Commitment commitment_scalar_mul(Commitment c, int scalar) {
    Commitment out;

    out.a = poly_scalar_mul(c.a, scalar);
    out.b = poly_scalar_mul(c.b, scalar);

    return out;
}

ZKProof generate_zk_proof(
    CommitmentKey *ck,
    Commitment c,
    int x,
    PolyVec r
) {
    double t0 = now_ms();

    ZKProof proof;

    PolyVec r_prime = sample_small_polyvec(BOUND_R);

    proof.t = commit_value(ck, 0, r_prime);

    int e = fs_challenge(c, proof.t);
    proof.challenge = e;

    PolyVec er = polyvec_scalar_mul(r, e);
    proof.z = polyvec_add(r_prime, er);

    double t1 = now_ms();
    proof_generation_time_ms += (t1 - t0);

    return proof;
}

int verify_zk_proof(
    CommitmentKey *ck,
    Commitment c,
    int x,
    ZKProof proof
) {
    double t0 = now_ms();

    int expected_e = fs_challenge(c, proof.t);

    if (proof.challenge != expected_e) {
        double t1 = now_ms();
        proof_verification_time_ms += (t1 - t0);
        return 0;
    }

    int ex = mod_q_int((long long)proof.challenge * x);

    Commitment left = commit_value(ck, ex, proof.z);

    Commitment right_part = commitment_scalar_mul(c, proof.challenge);
    Commitment right = add_commitments(proof.t, right_part);

    int ok = commitments_equal(left, right);

    double t1 = now_ms();
    proof_verification_time_ms += (t1 - t0);

    return ok;
}

void reset_zk_timers(void) {
    proof_generation_time_ms = 0.0;
    proof_verification_time_ms = 0.0;
}

double get_zk_proof_generation_time_ms(void) {
    return proof_generation_time_ms;
}

double get_zk_proof_verification_time_ms(void) {
    return proof_verification_time_ms;
}