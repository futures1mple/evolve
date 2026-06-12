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

/*
 * Usage: evolve.exe [num_voters] [bucket_size]
 *
 * If not provided, defaults to NUM_VOTERS and BUCKET_SIZE from config.h
 *
 * Examples:
 *   evolve.exe           -> uses config.h defaults
 *   evolve.exe 500       -> 500 voters, default bucket
 *   evolve.exe 1000 15   -> 1000 voters, bucket size 15
 */
int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL));

    /* --- Parse command-line args --- */
    int num_voters  = NUM_VOTERS;
    int bucket_size = BUCKET_SIZE;

    if (argc >= 2) {
        num_voters = atoi(argv[1]);
        if (num_voters <= 0) {
            fprintf(stderr, "Error: num_voters must be > 0\n");
            return 1;
        }
    }
    if (argc >= 3) {
        bucket_size = atoi(argv[2]);
        if (bucket_size <= 0) {
            fprintf(stderr, "Error: bucket_size must be > 0\n");
            return 1;
        }
    }

    printf("=== EVOLVE Experiment ===\n");
    printf("N=%d  D=%d  Q=%d\n", N, D, Q);
    printf("Voters=%d  Authorities=%d  Bucket=%d\n",
           num_voters, NUM_AUTHORITIES, bucket_size);
    fflush(stdout);

    double t0, t1;
    reset_zk_timers();

    /* --- Open results file --- */
    FILE *f = fopen("results.csv", "a");
    if (!f) {
        printf("Failed to open results.csv\n");
        return 1;
    }

    /* Write header only if file is empty */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size == 0) {
        fprintf(f, "N,D,Voters,Authorities,Bucket,Keygen,"
                   "Gen,ProofGen,Verify,ProofVerify,"
                   "Recommit,Levels,Nodes,Tally,Plain,Result,Correct\n");
    }

    /* --- Key generation --- */
    printf("Starting key generation...\n");
    fflush(stdout);

    t0 = now_ms();
    CommitmentKey ck = commitment_keygen();
    t1 = now_ms();
    double keygen_time_ms = t1 - t0;
    printf("Key generation done (%.3f ms)\n", keygen_time_ms);
    fflush(stdout);

    /* --- Allocate ballots --- */
    Ballot *ballots = malloc(sizeof(Ballot) * num_voters);
    if (!ballots) {
        fprintf(stderr, "malloc failed for %d ballots "
                "(each ~%zu KB, total ~%.0f MB)\n",
                num_voters,
                sizeof(Ballot) / 1024,
                (double)sizeof(Ballot) * num_voters / (1024 * 1024));
        fclose(f);
        return 1;
    }

    /* --- Ballot generation --- */
    printf("Starting ballot generation (%d voters)...\n", num_voters);
    fflush(stdout);

    t0 = now_ms();
    int plain_vote_sum = 0;

    for (int i = 0; i < num_voters; i++) {
        int vote = rand() % 2;
        plain_vote_sum += vote;
        ballots[i] = create_ballot(vote, &ck);

        /* Progress log every 10% */
        if (num_voters >= 10 && (i + 1) % (num_voters / 10) == 0) {
            printf("  Ballot generation: %d/%d (%.0f%%)\n",
                   i + 1, num_voters,
                   100.0 * (i + 1) / num_voters);
            fflush(stdout);
        }
    }

    t1 = now_ms();
    double ballot_generation_time_ms = t1 - t0;
    printf("Ballot generation done (%.3f ms)\n", ballot_generation_time_ms);
    fflush(stdout);

    /* --- Authority verification --- */
    printf("Starting authority verification...\n");
    fflush(stdout);

    double authority_verification_time_ms = 0.0;
    int all_valid = 1;

    for (int j = 0; j < NUM_AUTHORITIES; j++) {
        double ta0 = now_ms();

        for (int i = 0; i < num_voters; i++) {
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

    /* --- Recommitment --- */
    printf("Starting recommitment trees (bucket_size=%d)...\n", bucket_size);
    fflush(stdout);

    double total_recommitment_time_ms = 0.0;
    int total_levels_all = 0;
    int total_bucket_nodes_all = 0;

    for (int j = 0; j < NUM_AUTHORITIES; j++) {
        Commitment *column_commitments = malloc(sizeof(Commitment) * num_voters);
        int        *column_shares      = malloc(sizeof(int)        * num_voters);

        if (!column_commitments || !column_shares) {
            fprintf(stderr, "malloc failed for authority %d data\n", j);
            free(ballots);
            fclose(f);
            return 1;
        }

        for (int i = 0; i < num_voters; i++) {
            column_commitments[i] = ballots[i].commitments[j];
            column_shares[i]      = ballots[i].shares[j];
        }

        double tr0 = now_ms();

        RecommitmentTree tree = build_recommitment_tree(
            column_commitments,
            column_shares,
            num_voters,
            &ck,
            bucket_size
        );

        double tr1 = now_ms();
        double rtime = tr1 - tr0;

        total_recommitment_time_ms   += rtime;
        total_levels_all             += tree.num_levels;
        total_bucket_nodes_all       += tree.total_bucket_nodes;

        printf("Authority %d recommitment: levels=%d nodes=%d time=%.3f ms\n",
               j, tree.num_levels, tree.total_bucket_nodes, rtime);
        fflush(stdout);

        free_recommitment_tree(&tree);
        free(column_commitments);
        free(column_shares);
    }

    printf("Recommitment complete (%.3f ms total)\n",
           total_recommitment_time_ms);
    fflush(stdout);

    /* --- Final tally --- */
    printf("Starting tally...\n");
    fflush(stdout);

    t0 = now_ms();

    int partials[NUM_AUTHORITIES];
    for (int j = 0; j < NUM_AUTHORITIES; j++) {
        partials[j] = compute_authority_sum(ballots, j, num_voters);
    }

    int result = compute_final_tally(partials);

    t1 = now_ms();
    double tally_time_ms = t1 - t0;
    printf("Tally done (%.3f ms)\n", tally_time_ms);
    fflush(stdout);

    double proof_gen_ms    = get_zk_proof_generation_time_ms();
    double proof_verify_ms = get_zk_proof_verification_time_ms();

    /* --- Write CSV row --- */
    fprintf(f,
        "%d,%d,%d,%d,%d,"
        "%.3f,%.3f,%.3f,%.3f,%.3f,"
        "%.3f,%.3f,%.3f,%.3f,"
        "%d,%d,%d\n",
        N, D, num_voters, NUM_AUTHORITIES, bucket_size,
        keygen_time_ms,
        ballot_generation_time_ms,
        proof_gen_ms,
        authority_verification_time_ms,
        proof_verify_ms,
        total_recommitment_time_ms,
        (double)total_levels_all      / NUM_AUTHORITIES,
        (double)total_bucket_nodes_all / NUM_AUTHORITIES,
        tally_time_ms,
        plain_vote_sum,
        result,
        (plain_vote_sum == result)
    );

    fclose(f);

    /* --- Summary --- */
    int correct = (plain_vote_sum == result);
    printf("\n===== SUMMARY =====\n");
    printf("N=%d  D=%d  Q=%d\n", N, D, Q);
    printf("Voters=%d  Authorities=%d  Bucket=%d\n",
           num_voters, NUM_AUTHORITIES, bucket_size);
    printf("Ballot size in memory: ~%.1f KB/voter, total ~%.1f MB\n",
           (double)sizeof(Ballot) / 1024.0,
           (double)sizeof(Ballot) * num_voters / (1024.0 * 1024.0));
    printf("Plain sum = %d\n", plain_vote_sum);
    printf("Final tally = %d\n", result);
    printf("Correct = %s\n", correct ? "YES" : "NO");
    printf("Keygen          = %.3f ms\n", keygen_time_ms);
    printf("BallotGen       = %.3f ms\n", ballot_generation_time_ms);
    printf("ProofGen        = %.3f ms\n", proof_gen_ms);
    printf("BallotVerify    = %.3f ms\n", authority_verification_time_ms);
    printf("ProofVerify     = %.3f ms\n", proof_verify_ms);
    printf("Recommit        = %.3f ms\n", total_recommitment_time_ms);
    printf("Tally           = %.3f ms\n", tally_time_ms);
    double total_ms = keygen_time_ms + ballot_generation_time_ms
                    + proof_gen_ms + authority_verification_time_ms
                    + proof_verify_ms + total_recommitment_time_ms
                    + tally_time_ms;
    printf("TOTAL           = %.3f ms  (%.2f s)\n", total_ms, total_ms / 1000.0);
    printf("===================\n");

    free(ballots);

    printf("Program finished\n");
    fflush(stdout);

    return 0;
}