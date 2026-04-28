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
    auto lf_map = [&](uint64_t row) -> uint64_t {
        if (row == idx.sentinel_row) return 0; // '$' maps to first row
        uint8_t c = get_bit(idx.bwt, row);
        return idx.c_table[c] + get_rank(idx, c, row);
    };

    uint64_t lo = 0, hi = idx.n;

    for (int i = (int)pattern_len - 1; i >= 0; i--) {
        uint8_t c = get_bit(pattern, i);
        lo = idx.c_table[c] + get_rank(idx, c, lo);
        hi = idx.c_table[c] + get_rank(idx, c, hi);
        if (lo >= hi) return {};
    }

    std::vector<uint64_t> positions;
    positions.reserve(hi - lo);

    if (!idx.use_sparse_sa || idx.sa_sampling_rate <= 1) {
        for (uint64_t r = lo; r < hi; r++) {
            positions.push_back(idx.suffix_array[r]);
        }
        return positions;
    }

    uint64_t k = idx.sa_sampling_rate;
    for (uint64_t r = lo; r < hi; r++) {
        uint64_t steps = 0;
        uint64_t cur = r;

        while (cur % k != 0) {
            cur = lf_map(cur);
            steps++;
            if (steps > idx.n) {
                throw std::runtime_error("LF-mapping loop exceeded text length");
            }
        }

        uint64_t sampled_sa = idx.suffix_array[cur / k];
        uint64_t pos = (sampled_sa + steps) % idx.n;
        positions.push_back(pos);
    }

    return positions;
}
