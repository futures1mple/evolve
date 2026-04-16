#include "tally.h"

int compute_authority_sum(Ballot *ballots, int authority_index, int count) {
    int sum = 0;
    for (int i = 0; i < count; i++) {
        sum += ballots[i].shares[authority_index];
    }
    return sum;
}

int compute_final_tally(int authority_sums[NUM_AUTHORITIES]) {
    int total = 0;
    for (int i = 0; i < NUM_AUTHORITIES; i++) {
        total += authority_sums[i];
    }
    return total;
}