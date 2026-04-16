#ifndef TALLY_H
#define TALLY_H

#include "recommitment.h"

int compute_authority_sum(Ballot *ballots, int authority_index, int count);
int compute_final_tally(int authority_sums[NUM_AUTHORITIES]);

#endif