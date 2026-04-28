#include "fm_index.h"

#include <algorithm>
#include <vector>

// returns -1 for sentinel position, otherwise the bit at position i
// n is idx.n (includes sentinel), sentinel is at position n-1
static inline int64_t get_bit_or_sentinel(const std::vector<uint8_t> &s, uint64_t i, uint64_t n) {
    if (i >= n - 1) return -1;
    return get_bit(s, i);
}

void build_suffix_array(FmIndex &idx, const std::vector<uint8_t> &s) {
    uint64_t n = idx.n; // includes sentinel
    idx.suffix_array.resize(n);

    if (n == 0) return;

    // rank[i] = current rank of suffix starting at i
    // rank2[i] = rank of suffix starting at i+k (lookahead)
    std::vector<int64_t> rank(n), rank2(n);

    // initial ranking by first two characters
    for (uint64_t i = 0; i < n; i++) {
        rank[i]  = get_bit_or_sentinel(s, i,     n);
        rank2[i] = get_bit_or_sentinel(s, i + 1, n);
        idx.suffix_array[i] = i;
    }

    for (uint64_t k = 1; k < n; k *= 2) {
        auto cmp = [&](uint64_t a, uint64_t b) {
            if (rank[a] != rank[b]) return rank[a] < rank[b];
            return rank2[a] < rank2[b];
        };

        std::sort(idx.suffix_array.begin(), idx.suffix_array.end(), cmp);

        // recompute ranks from sorted order
        std::vector<int64_t> new_rank(n);
        new_rank[idx.suffix_array[0]] = 0;
        for (uint64_t i = 1; i < n; i++) {
            new_rank[idx.suffix_array[i]] = new_rank[idx.suffix_array[i-1]];
            if (rank[idx.suffix_array[i]]  != rank[idx.suffix_array[i-1]] ||
                rank2[idx.suffix_array[i]] != rank2[idx.suffix_array[i-1]]) {
                new_rank[idx.suffix_array[i]]++;
            }
        }

        rank = new_rank;
        if (rank[idx.suffix_array[n-1]] == (int64_t)(n - 1)) break;

        // update rank2 for next iteration (lookahead by 2k)
        for (uint64_t i = 0; i < n; i++) {
            uint64_t next = i + 2 * k;
            rank2[i] = (next < n - 1) ? rank[next] : -1;
        }
    }

    // c_table[c] = # of suffixes in sorted order before first suffix starting with c
    // sorted order: $ < 0 < 1
    uint64_t zeros = 0;
    for (uint64_t i = 0; i < n; i++) if (get_bit_or_sentinel(s, i, n) == 0) zeros++;
    idx.c_table[0] = 1;          // 1 sentinel before all 0s
    idx.c_table[1] = 1 + zeros;  // sentinel + all 0s before all 1s
    idx.c_table[2] = 0;
}
