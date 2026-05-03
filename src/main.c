#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "voter.h"
#include "authority.h"
#include "recommitment.h"
#include "tally.h"
#include "utils.h"
#include "zkproof.h"

int main(void) {
    srand((unsigned)time(NULL));

    printf("Start program\n");
    fflush(stdout);

    double t0, t1;
    reset_zk_timers();

    // ===== Open results file =====
    FILE *f = fopen("results.csv", "a");
    if (!f) {
        printf("Failed to open results.csv\n");
        return 1;
    }

    // Add header if file is empty
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size == 0) {
        fprintf(f, "N,D,Voters,Authorities,Bucket,Keygen,Gen,ProofGen,Verify,ProofVerify,Recommit,Levels,Nodes,Tally,Plain,Result,Correct\n");
    }

    // ===== Key generation =====
    printf("Starting key generation...\n");
    fflush(stdout);

    t0 = now_ms();
    CommitmentKey ck = commitment_keygen();
    t1 = now_ms();
    double keygen_time_ms = t1 - t0;

    printf("Key generation done (%.3f ms)\n", keygen_time_ms);
    fflush(stdout);

    // ===== Allocate ballots =====
    printf("Allocating ballots...\n");
    fflush(stdout);

    Ballot *ballots = malloc(sizeof(Ballot) * NUM_VOTERS);
    if (!ballots) {
        printf("malloc failed for ballots\n");
        fclose(f);
        return 1;
    }

    printf("Ballot allocation done\n");
    fflush(stdout);

    // ===== Ballot generation =====
    printf("Starting ballot generation...\n");
    fflush(stdout);

    t0 = now_ms();
    int plain_vote_sum = 0;

    for (int i = 0; i < NUM_VOTERS; i++) {
        int vote = rand() % 2;
        plain_vote_sum += vote;
        ballots[i] = create_ballot(vote, &ck);
    }

    t1 = now_ms();
    double ballot_generation_time_ms = t1 - t0;

    printf("Ballot generation done (%.3f ms)\n", ballot_generation_time_ms);
    fflush(stdout);

    // ===== Authority verification =====
    printf("Starting authority verification...\n");
    fflush(stdout);

    double authority_verification_time_ms = 0.0;
    int all_valid = 1;

    for (int j = 0; j < NUM_AUTHORITIES; j++) {
        double ta0 = now_ms();

        for (int i = 0; i < NUM_VOTERS; i++) {
            if (!verify_ballot_for_authority(&ballots[i], j, &ck)) {
                all_valid = 0;
                printf("Verification failed: authority=%d ballot=%d\n", j, i);
                fflush(stdout);
                break;
            }
        }

        double ta1 = now_ms();
        authority_verification_time_ms += (ta1 - ta0);

        printf("Authority %d verification done (%.3f ms)\n", j, ta1 - ta0);
        fflush(stdout);

        if (!all_valid) {
            free(ballots);
            fclose(f);
            printf("Stopped due to verification failure\n");
            return 1;
        }
    }

    printf("Authority verification complete (%.3f ms total)\n",
           authority_verification_time_ms);
    fflush(stdout);

    // ===== Recommitment =====
    printf("Starting recommitment trees...\n");
    fflush(stdout);

    double total_recommitment_time_ms = 0.0;
    int total_levels_all = 0;
    int total_bucket_nodes_all = 0;

    for (int j = 0; j < NUM_AUTHORITIES; j++) {
        Commitment *column_commitments = malloc(sizeof(Commitment) * NUM_VOTERS);
        int *column_shares = malloc(sizeof(int) * NUM_VOTERS);

        if (!column_commitments || !column_shares) {
            printf("malloc failed for authority data\n");
            free(ballots);
            fclose(f);
            return 1;
        }

        for (int i = 0; i < NUM_VOTERS; i++) {
            column_commitments[i] = ballots[i].commitments[j];
            column_shares[i] = ballots[i].shares[j];
        }

        double tr0 = now_ms();

        RecommitmentTree tree = build_recommitment_tree(
            column_commitments,
            column_shares,
            NUM_VOTERS,
            &ck,
            BUCKET_SIZE
        );

        double tr1 = now_ms();

        double time = tr1 - tr0;
        total_recommitment_time_ms += time;
        total_levels_all += tree.num_levels;
        total_bucket_nodes_all += tree.total_bucket_nodes;

        printf("Authority %d recommitment: levels=%d nodes=%d time=%.3f ms\n",
               j, tree.num_levels, tree.total_bucket_nodes, time);
        fflush(stdout);

        free_recommitment_tree(&tree);
        free(column_commitments);
        free(column_shares);
    }

    printf("Recommitment complete (%.3f ms total)\n",
           total_recommitment_time_ms);
    fflush(stdout);

    // ===== Final tally =====
    printf("Starting tally...\n");
    fflush(stdout);

    t0 = now_ms();

    int partials[NUM_AUTHORITIES];
    for (int j = 0; j < NUM_AUTHORITIES; j++) {
        partials[j] = compute_authority_sum(ballots, j, NUM_VOTERS);
    }

    int result = compute_final_tally(partials);

    t1 = now_ms();
    double tally_time_ms = t1 - t0;

    printf("Tally done (%.3f ms)\n", tally_time_ms);
    fflush(stdout);

    double proof_generation_time_ms = get_zk_proof_generation_time_ms();
    double proof_verification_time_ms = get_zk_proof_verification_time_ms();

    // ===== Write to file =====
    fprintf(f,
        "%d,%d,%d,%d,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d\n",
        N,
        D,
        NUM_VOTERS,
        NUM_AUTHORITIES,
        BUCKET_SIZE,
        keygen_time_ms,
        ballot_generation_time_ms,
        proof_generation_time_ms,
        authority_verification_time_ms,
        proof_verification_time_ms,
        total_recommitment_time_ms,
        (double)total_levels_all / NUM_AUTHORITIES,
        (double)total_bucket_nodes_all / NUM_AUTHORITIES,
        tally_time_ms,
        plain_vote_sum,
        result,
        (plain_vote_sum == result)
    );

    fclose(f);

    // ===== Summary =====
    printf("\n===== SUMMARY =====\n");
    printf("N = %d, D = %d\n", N, D);
    printf("Voters = %d, Authorities = %d\n", NUM_VOTERS, NUM_AUTHORITIES);
    printf("Bucket size = %d\n", BUCKET_SIZE);
    printf("Plain sum = %d\n", plain_vote_sum);
    printf("Final tally = %d\n", result);
    printf("Correct = %s\n", (plain_vote_sum == result) ? "YES" : "NO");
    printf("proof_generation_time_ms    = %.3f\n", proof_generation_time_ms);
    printf("proof_verification_time_ms  = %.3f\n", proof_verification_time_ms);

    free(ballots);

    printf("Program finished\n");
    fflush(stdout);

    return 0;
}