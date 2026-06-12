#ifndef VOTER_H
#define VOTER_H

#include "commitment.h"
#include "zkproof.h"

typedef struct {
    int     vote;
    int     shares     [NUM_AUTHORITIES];
    PolyVec randomness [NUM_AUTHORITIES];
    Commitment commitments [NUM_AUTHORITIES];
    ZKOrProof  proofs      [NUM_AUTHORITIES];   /* OR proof per authority */
} Ballot;

Ballot create_ballot(int vote, CommitmentKey *ck);

#endif /* VOTER_H */