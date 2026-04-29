#ifndef FM_INDEX_H
#define FM_INDEX_H

#include <cstdint>
#include <string>
#include <vector>
#include "jacobson_rank.h"

inline uint8_t get_bit(const std::vector<uint8_t> &v, uint64_t i) {
    return (v[i / 8] >> (7 - (i % 8))) & 1;
}

inline void set_bit(std::vector<uint8_t> &v, uint64_t i, uint8_t bit) {
    if (bit) v[i / 8] |=  (1 << (7 - (i % 8)));
    else     v[i / 8] &= ~(1 << (7 - (i % 8)));
}

struct FmIndex {
    uint64_t n = 0;                      // input length in bits + 1 (sentinel)
    uint64_t sentinel_row = 0;           // row in BWT where $ (sentinel) appears

    std::vector<uint8_t> bwt;            // BWT of input, bit-packed, size (n+7)/8
    std::vector<uint64_t> suffix_array;  // SA of input+sentinel, size n (includes sentinel)
    std::vector<uint64_t> rank_table;    // rank_table[i] = # of 1s in bwt[0..i), size n+1
    uint64_t c_table[3];                 // c_table[c] = # of suffixes before first suffix starting with c

    JacobsonRank jacobson_rank;          // Jacobson rank structure for O(1) rank with O(n/log n) space
    bool use_jacobson = false;           // Flag to control which rank method to use
    bool use_sparse_sa = false;          // Whether suffix_array is stored as a sparse sample
    uint64_t sa_sampling_rate = 1;       // Keep every k-th suffix array entry when sparse
};

// n = text length in bits, m = pattern length in bits, occ = number of occurrences

// time: O(n log^2 n)  space: O(32n bytes) peak during build (SA + rank arrays)
void build_suffix_array(FmIndex &idx, const std::vector<uint8_t> &s);

// time: O(n)  space: O(n/8 bytes) — BWT stored bit-packed
void build_bwt(FmIndex &idx, const std::vector<uint8_t> &s);

// time: O(n/k)  space: O(n/k) — keep every k-th suffix array entry
void sample_suffix_array(FmIndex &idx, uint64_t sampling_rate);

// time: O(n)  space: O(8n bytes) naive — one uint64_t per position; O(n/log n) bytes Jacobson
void build_rank(FmIndex &idx);

// time: O(1)  space: O(1)
uint64_t get_rank(const FmIndex &idx, uint8_t c, uint64_t i);

// time: O(m)  space: O(1) — m backward search steps, each O(1) rank lookup
uint64_t query_count(const FmIndex &idx, const std::vector<uint8_t> &pattern, uint64_t pattern_len);

// time: O(m + occ) full SA, O(m + occ*k) sparse SA  space: O(8*occ bytes)
std::vector<uint64_t> query_locate(const FmIndex &idx, const std::vector<uint8_t> &pattern, uint64_t pattern_len);

// time: O(n + len) full SA, O(n/k + k + len) sparse SA  space: O(n) full SA, O(n/k) sparse SA
std::string extract_substring(const FmIndex &idx, uint64_t start, uint64_t end);

#endif
