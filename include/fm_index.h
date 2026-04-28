#ifndef FM_INDEX_H
#define FM_INDEX_H

#include <cstdint>
#include <string>
#include <vector>

struct FmIndex {
    std::vector<uint8_t> bwt;
    uint64_t c_table[3];                 // C[c] = # of chars in bwt less than c (binary alphabet: 0, 1, $)
    // TODO: rank
    // TODO: sparse suffix array
};

void store_index(const FmIndex &idx, const std::string &filepath);
FmIndex load_index(const std::string &filepath);

void build_suffix_array(FmIndex &idx, const std::vector<uint8_t> &s);
void build_bwt(FmIndex &idx, const std::vector<uint8_t> &s);
void build_rank(FmIndex &idx);

uint64_t get_rank(const FmIndex &idx, uint8_t c, uint64_t i); // returns # of occurrences of character c in bwt[0..i)

uint64_t query_count(const FmIndex &idx, const std::vector<uint8_t> &pattern);
std::vector<uint64_t> query_locate(const FmIndex &idx, const std::vector<uint8_t> &pattern);

#endif
