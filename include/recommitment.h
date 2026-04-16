#ifndef RECOMMITMENT_H
#define RECOMMITMENT_H

#include "authority.h"

typedef struct {
    Commitment summed_commitment;
    int summed_share;
    PolyVec fresh_randomness;
    Commitment recommitted;
} BucketNode;

typedef struct {
    BucketNode *nodes;
    int count;
} TreeLevel;

typedef struct {
    TreeLevel levels[32];
    int num_levels;
    int total_bucket_nodes;
} RecommitmentTree;

RecommitmentTree build_recommitment_tree(
    Commitment *commitments,
    int *shares,
    int count,
    CommitmentKey *ck,
    int bucket_size
);

void free_recommitment_tree(RecommitmentTree *tree);

#endif