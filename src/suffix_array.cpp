#include "fm_index.h"

#include <algorithm>
#include <vector>

struct SuffixRank {
    uint64_t index;
    int64_t  rank[2];
};

// returns -1 for sentinel position n, otherwise the bit at position i
// n is idx.n (includes sentinel), sentinel is at position n-1
static inline int64_t get_bit_or_sentinel(const std::vector<uint8_t> &s, uint64_t i, uint64_t n) {
    if (i >= n - 1) return -1;
    return get_bit(s, i);
}

void build_suffix_array(FmIndex &idx, const std::vector<uint8_t> &s) {
    uint64_t n = idx.n; // includes sentinel
    idx.suffix_array.resize(n);

    if (n == 0) return;

    std::vector<SuffixRank> sr(n);
    for (uint64_t i = 0; i < n; i++) {
        sr[i].index   = i;
        sr[i].rank[0] = get_bit_or_sentinel(s, i,     n);
        sr[i].rank[1] = get_bit_or_sentinel(s, i + 1, n);
    }

    auto cmp = [](const SuffixRank &a, const SuffixRank &b) {
        if (a.rank[0] != b.rank[0]) return a.rank[0] < b.rank[0];
        return a.rank[1] < b.rank[1];
    };

    std::sort(sr.begin(), sr.end(), cmp);

    std::vector<int64_t> rank(n);

    for (uint64_t k = 2; k < n; k *= 2) {
        rank[sr[0].index] = 0;
        for (uint64_t i = 1; i < n; i++) {
            rank[sr[i].index] = rank[sr[i-1].index];
            if (sr[i].rank[0] != sr[i-1].rank[0] ||
                sr[i].rank[1] != sr[i-1].rank[1]) {
                rank[sr[i].index]++;
            }
        }

        if (rank[sr[n-1].index] == (int64_t)(n - 1)) break;

        for (uint64_t i = 0; i < n; i++) {
            uint64_t next = sr[i].index + k;
            sr[i].rank[0] = rank[sr[i].index];
            sr[i].rank[1] = (next < n - 1) ? rank[next] : -1;
        }

        std::sort(sr.begin(), sr.end(), cmp);
    }

    for (uint64_t i = 0; i < n; i++) idx.suffix_array[i] = sr[i].index;

    // c_table[c] = # of suffixes in sorted order before first suffix starting with c
    // sorted order: $ < 0 < 1
    uint64_t zeros = 0;
    for (uint64_t i = 0; i < n; i++) if (get_bit_or_sentinel(s, i, n) == 0) zeros++;
    idx.c_table[0] = 1;          // 1 sentinel before all 0s
    idx.c_table[1] = 1 + zeros;  // sentinel + all 0s before all 1s
    idx.c_table[2] = 0;
}
