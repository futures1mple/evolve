#ifndef VOTER_H
#define VOTER_H

#include "commitment.h"

typedef struct {
    int vote;
    int shares[NUM_AUTHORITIES];
    PolyVec randomness[NUM_AUTHORITIES];
    Commitment commitments[NUM_AUTHORITIES];
} Ballot;

Ballot create_ballot(int vote, CommitmentKey *ck);

#endif