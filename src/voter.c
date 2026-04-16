#include <stdlib.h>
#include "voter.h"

static void share_vote(int vote, int shares[NUM_AUTHORITIES]) {
    int sum = 0;
    for (int i = 0; i < NUM_AUTHORITIES - 1; i++) {
        shares[i] = rand() % 5;   // baseline toy sharing
        sum += shares[i];
    }
    shares[NUM_AUTHORITIES - 1] = vote - sum;
}

Ballot create_ballot(int vote, CommitmentKey *ck) {
    Ballot b;
    b.vote = vote;

    share_vote(vote, b.shares);

    for (int j = 0; j < NUM_AUTHORITIES; j++) {
        b.randomness[j] = sample_small_polyvec(BOUND_R);
        b.commitments[j] = commit_value(ck, b.shares[j], b.randomness[j]);
    }

    return b;
}