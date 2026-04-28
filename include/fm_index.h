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
    std::vector<uint8_t> bwt;
    std::vector<uint64_t> suffix_array;
    uint64_t n = 0;
    uint64_t c_table[3];                 // C[c] = # of chars in bwt less than c (binary alphabet: 0, 1, $)
    std::vector<uint64_t> rank_table;    // rank_table[i] = # of 1s in bwt[0..i)
    uint64_t sentinel_row = 0;           // row in BWT where $ appears
};

void store_index(const FmIndex &idx, const std::string &filepath);
FmIndex load_index(const std::string &filepath);

void build_suffix_array(FmIndex &idx, const std::vector<uint8_t> &s);
void build_bwt(FmIndex &idx, const std::vector<uint8_t> &s);
void build_rank(FmIndex &idx);

uint64_t get_rank(const FmIndex &idx, uint8_t c, uint64_t i); // returns # of occurrences of character c in bwt[0..i)

uint64_t query_count(const FmIndex &idx, const std::vector<uint8_t> &pattern, uint64_t pattern_len);
std::vector<uint64_t> query_locate(const FmIndex &idx, const std::vector<uint8_t> &pattern, uint64_t pattern_len);

#endif
