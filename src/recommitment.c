#include <stdlib.h>
#include "recommitment.h"

static BucketNode recommit_bucket(
    Commitment *bucket_commitments,
    int *bucket_shares,
    int bucket_count,
    CommitmentKey *ck
) {
    BucketNode node;

    node.summed_commitment = bucket_commitments[0];
    node.summed_share = bucket_shares[0];

    for (int i = 1; i < bucket_count; i++) {
        node.summed_commitment = add_commitments(node.summed_commitment, bucket_commitments[i]);
        node.summed_share += bucket_shares[i];
    }

    node.fresh_randomness = sample_small_polyvec(BOUND_R);
    node.recommitted = commit_value(ck, node.summed_share, node.fresh_randomness);

    return node;
}

RecommitmentTree build_recommitment_tree(
    Commitment *commitments,
    int *shares,
    int count,
    CommitmentKey *ck,
    int bucket_size
) {
    RecommitmentTree tree;
    tree.num_levels = 0;
    tree.total_bucket_nodes = 0;

    Commitment *current_commitments = malloc(sizeof(Commitment) * count);
    int *current_shares = malloc(sizeof(int) * count);

    for (int i = 0; i < count; i++) {
        current_commitments[i] = commitments[i];
        current_shares[i] = shares[i];
    }

    int current_count = count;

    while (current_count > 1) {
        int next_count = (current_count + bucket_size - 1) / bucket_size;

        tree.levels[tree.num_levels].nodes = malloc(sizeof(BucketNode) * next_count);
        tree.levels[tree.num_levels].count = next_count;
        tree.total_bucket_nodes += next_count;

        Commitment *next_commitments = malloc(sizeof(Commitment) * next_count);
        int *next_shares = malloc(sizeof(int) * next_count);

        for (int i = 0; i < next_count; i++) {
            int start = i * bucket_size;
            int end = start + bucket_size;
            if (end > current_count) end = current_count;

            BucketNode node = recommit_bucket(
                &current_commitments[start],
                &current_shares[start],
                end - start,
                ck
            );

            tree.levels[tree.num_levels].nodes[i] = node;
            next_commitments[i] = node.recommitted;
            next_shares[i] = node.summed_share;
        }

        free(current_commitments);
        free(current_shares);

        current_commitments = next_commitments;
        current_shares = next_shares;
        current_count = next_count;
        tree.num_levels++;
    }

    free(current_commitments);
    free(current_shares);

    return tree;
}

void free_recommitment_tree(RecommitmentTree *tree) {
    for (int i = 0; i < tree->num_levels; i++) {
        free(tree->levels[i].nodes);
    }
}