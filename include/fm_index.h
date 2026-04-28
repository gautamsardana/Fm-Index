#ifndef FM_INDEX_H
#define FM_INDEX_H

#include <cstdint>
#include <string>
#include <vector>

inline uint8_t get_bit(const std::vector<uint8_t> &v, uint64_t i) {
    return (v[i / 8] >> (7 - (i % 8))) & 1;
}

inline void set_bit(std::vector<uint8_t> &v, uint64_t i, uint8_t bit) {
    if (bit) v[i / 8] |=  (1 << (7 - (i % 8)));
    else     v[i / 8] &= ~(1 << (7 - (i % 8)));
}

struct FmIndex {
    std::vector<uint8_t> bwt;            // BWT of input, bit-packed, size (n+7)/8
    std::vector<uint64_t> suffix_array;  // SA of input+sentinel, size n (includes sentinel)
    uint64_t n = 0;                      // input length in bits + 1 (sentinel)
    uint64_t c_table[3];                 // c_table[c] = # of suffixes before first suffix starting with c
    std::vector<uint64_t> rank_table;    // rank_table[i] = # of 1s in bwt[0..i), size n+1
    uint64_t sentinel_row = 0;           // row in BWT where $ (sentinel) appears
};

// time: O(n log^2 n)  space: O(64n bits) — prefix doubling sort over n+1 suffixes
void build_suffix_array(FmIndex &idx, const std::vector<uint8_t> &s);

// time: O(n)  space: O(n bits) — single pass over SA, BWT stored bit-packed
void build_bwt(FmIndex &idx, const std::vector<uint8_t> &s);

// time: O(n)  space: O(64n bits) — prefix sum of 1s over BWT
void build_rank(FmIndex &idx);

// time: O(1)  space: O(1)
uint64_t get_rank(const FmIndex &idx, uint8_t c, uint64_t i);

// time: O(p)  space: O(1) — backward search over pattern bits
uint64_t query_count(const FmIndex &idx, const std::vector<uint8_t> &pattern, uint64_t pattern_len);

// time: O(p + occ)  space: O(64*occ bits) — backward search + SA lookup per occurrence
std::vector<uint64_t> query_locate(const FmIndex &idx, const std::vector<uint8_t> &pattern, uint64_t pattern_len);

void store_index(const FmIndex &idx, const std::string &filepath);

FmIndex load_index(const std::string &filepath);

#endif
