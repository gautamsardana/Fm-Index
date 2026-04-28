#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "suffix_array.h"

// comparison struct for sorting
typedef struct {
    int index;
    int rank[2];
} SuffixRank;

int cmp_suffix_rank(const void *a, const void *b) {
    SuffixRank *sa = (SuffixRank *)a;
    SuffixRank *sb = (SuffixRank *)b;
    if (sa->rank[0] != sb->rank[0]) return sa->rank[0] - sb->rank[0];
    return sa->rank[1] - sb->rank[1];
}

// prefix doubling - O(n log^2 n) algorithm to build suffix array
int *build_suffix_array(const char *s, int n) {
    SuffixRank *sr = malloc(n * sizeof(SuffixRank));
    int *sa = malloc(n * sizeof(int));
    int *rank = malloc(n * sizeof(int));
    int *tmp = malloc(n * sizeof(int));

    // initial ranking by first character
    for (int i = 0; i < n; i++) {
        sr[i].index = i;
        sr[i].rank[0] = s[i];
        sr[i].rank[1] = (i + 1 < n) ? s[i + 1] : -1;
    }

    qsort(sr, n, sizeof(SuffixRank), cmp_suffix_rank);

    for (int k = 2; k < n; k *= 2) {
        // assign ranks from sorted order
        rank[sr[0].index] = 0;
        for (int i = 1; i < n; i++) {
            rank[sr[i].index] = rank[sr[i-1].index];
            if (sr[i].rank[0] != sr[i-1].rank[0] ||
                sr[i].rank[1] != sr[i-1].rank[1]) {
                rank[sr[i].index]++;
            }
        }

        // early exit if all ranks unique
        if (rank[sr[n-1].index] == n - 1) break;

        // update ranks for next iteration
        for (int i = 0; i < n; i++) {
            sr[i].rank[0] = rank[sr[i].index];
            sr[i].rank[1] = (sr[i].index + k < n) ? rank[sr[i].index + k] : -1;
        }

        qsort(sr, n, sizeof(SuffixRank), cmp_suffix_rank);
    }

    for (int i = 0; i < n; i++) sa[i] = sr[i].index;

    free(sr);
    free(rank);
    free(tmp);
    return sa;
}