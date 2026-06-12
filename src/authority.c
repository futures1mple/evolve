#include <stdlib.h>
#include "authority.h"
#include "zkproof.h"

static int centered_abs(int x) {
    if (x > Q / 2) x -= Q;
    if (x < 0) x = -x;
    return x;
}

int is_small_poly(Poly p, int bound) {
    for (int i = 0; i < N; i++)
        if (centered_abs(p.coeffs[i]) > bound) return 0;
    return 1;
}

int is_small_polyvec(PolyVec r, int bound) {
    for (int i = 0; i < D; i++)
        if (!is_small_poly(r.items[i], bound)) return 0;
    return 1;
}

/* Verify a single ballot commitment for authority j using the OR proof.
 * Returns 1 if valid (vote is 0 or 1 and proof is consistent), 0 otherwise. */
int verify_ballot_for_authority(Ballot *b, int authority_index, CommitmentKey *ck) {
    return verify_zk_or_proof(
        ck,
        b->commitments[authority_index],
        b->proofs[authority_index]
    );
}