#ifndef AUTHORITY_H
#define AUTHORITY_H

#include "voter.h"

int is_small_poly(Poly p, int bound);
int is_small_polyvec(PolyVec r, int bound);

int verify_ballot_for_authority(Ballot *b, int authority_index, CommitmentKey *ck);

#endif