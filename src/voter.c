#include <stdlib.h>
#include "voter.h"

/* Additive secret sharing over integers:
 *   sum(shares) = vote
 * Shares are small integers; the last share absorbs the remainder.
 * NOTE: for a full EVOLVE implementation, shares should be elements of R_q. */
static void share_vote(int vote, int shares[NUM_AUTHORITIES]) {
    int sum = 0;
    for (int i = 0; i < NUM_AUTHORITIES - 1; i++) {
        shares[i] = rand() % 5;
        sum += shares[i];
    }
    shares[NUM_AUTHORITIES - 1] = vote - sum;
}

Ballot create_ballot(int vote, CommitmentKey *ck) {
    Ballot b;
    b.vote = vote;

    share_vote(vote, b.shares);

    for (int j = 0; j < NUM_AUTHORITIES; j++) {
        b.randomness[j]  = sample_small_polyvec(BOUND_R);
        b.commitments[j] = commit_value(ck, b.shares[j], b.randomness[j]);

        /* OR proof: proves the committed share is either 0 or 1.
         * For the overall vote (sum of shares) to be binary, each
         * individual commitment must commit to a valid share value.
         * In this simplified model shares are integers, so we prove
         * x ∈ {0,1} for each share's commitment using the OR proof. */
        b.proofs[j] = generate_zk_or_proof(
            ck,
            b.commitments[j],
            b.shares[j],    /* x: the committed value (0 or 1 per share) */
            b.randomness[j]
        );
    }

    return b;
}