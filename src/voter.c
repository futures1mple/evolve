#include <stdlib.h>
#include "voter.h"

/* Binary secret sharing: shares[0] = vote, shares[1..k-1] = 0.
 *
 * This ensures every share value is in {0, 1}, which is required for the
 * OR proof to be valid.  The additive tally (sum of all shares) therefore
 * equals the original vote: vote + 0 + ... + 0 = vote.
 *
 * Note: in a production EVOLVE implementation, sharing would be done over
 * the ring R_q with polynomial randomness so that no single authority
 * learns the vote.  This simplified model is used for performance
 * evaluation only. */
static void share_vote(int vote, int shares[NUM_AUTHORITIES]) {
    shares[0] = vote;                              /* authority 0 holds the vote */
    for (int i = 1; i < NUM_AUTHORITIES; i++)
        shares[i] = 0;                             /* all others hold zero       */
}

Ballot create_ballot(int vote, CommitmentKey *ck) {
    Ballot b;
    b.vote = vote;

    share_vote(vote, b.shares);

    for (int j = 0; j < NUM_AUTHORITIES; j++) {
        b.randomness[j]  = sample_small_polyvec(BOUND_R);
        b.commitments[j] = commit_value(ck, b.shares[j], b.randomness[j]);

        /* OR proof: proves the committed share is 0 or 1.
         * shares[0] = vote ∈ {0,1}, shares[1..k-1] = 0 ∈ {0,1}
         * so the proof is valid for every authority. */
        b.proofs[j] = generate_zk_or_proof(
            ck,
            b.commitments[j],
            b.shares[j],
            b.randomness[j]
        );
    }

    return b;
}