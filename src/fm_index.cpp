#include "fm_index.h"
#include "debug.h"

#include <fstream>
#include <stdexcept>

void store_index(const FmIndex &idx, const std::string &filepath) {
    // TODO
}

FmIndex load_index(const std::string &filepath) {
    // TODO
    return {};
}

uint64_t query_count(const FmIndex &idx, const std::vector<uint8_t> &pattern, uint64_t pattern_len) {
    uint64_t lo = 0, hi = idx.n;

    for (int i = (int)pattern_len - 1; i >= 0; i--) {
        uint8_t c = get_bit(pattern, i);
        lo = idx.c_table[c] + get_rank(idx, c, lo);
        hi = idx.c_table[c] + get_rank(idx, c, hi);
        if (lo >= hi) return 0;
    }

    DEBUG_PRINT("query_count: lo=" << lo << " hi=" << hi << " count=" << (hi - lo));
    return hi - lo;
}

std::vector<uint64_t> query_locate(const FmIndex &idx, const std::vector<uint8_t> &pattern, uint64_t pattern_len) {
    uint64_t lo = 0, hi = idx.n;

    for (int i = (int)pattern_len - 1; i >= 0; i--) {
        uint8_t c = get_bit(pattern, i);
        lo = idx.c_table[c] + get_rank(idx, c, lo);
        hi = idx.c_table[c] + get_rank(idx, c, hi);
        if (lo >= hi) return {};
    }

    std::vector<uint64_t> positions;
    for (uint64_t r = lo; r < hi; r++) {
        positions.push_back(idx.suffix_array[r]);
    }
    return positions;
}
