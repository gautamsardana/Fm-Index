#include "fm_index.h"
#include "debug.h"

#include <algorithm>
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

std::string extract_substring(const FmIndex &idx, uint64_t start, uint64_t end) {
    if (start > end) {
        throw std::runtime_error("extract_substring: start must be <= end");
    }
    if (end > idx.n - 1) {
        throw std::runtime_error("extract_substring: end out of bounds");
    }
    if (end == start) {
        return {};
    }

    auto lf_map = [&](uint64_t row) -> uint64_t {
        if (row == idx.sentinel_row) return 0; // '$' maps to first row
        uint8_t c = get_bit(idx.bwt, row);
        return idx.c_table[c] + get_rank(idx, c, row);
    };

    uint64_t row = 0;
    if (!idx.use_sparse_sa || idx.sa_sampling_rate <= 1) {
        std::vector<uint64_t> inv_sa(idx.n, 0);
        for (uint64_t i = 0; i < idx.n; ++i) {
            inv_sa[idx.suffix_array[i]] = i;
        }
        row = inv_sa[end];
    } else {
        uint64_t k = idx.sa_sampling_rate;
        uint64_t best_row = 0;
        uint64_t best_steps = idx.n + 1;

        for (uint64_t i = 0; i < idx.suffix_array.size(); ++i) {
            uint64_t sa_val = idx.suffix_array[i];
            uint64_t steps = (sa_val >= end) ? (sa_val - end) : (sa_val + idx.n - end);
            if (steps < best_steps) {
                best_steps = steps;
                best_row = i * k;
            }
        }

        if (best_steps > idx.n) {
            throw std::runtime_error("extract_substring: failed to locate row for end position");
        }

        row = best_row;
        for (uint64_t i = 0; i < best_steps; ++i) {
            row = lf_map(row);
        }
    }
    std::string out;
    out.reserve(end - start);

    for (uint64_t i = 0; i < end - start; ++i) {
        if (row == idx.sentinel_row) {
            throw std::runtime_error("extract_substring: encountered sentinel during extraction");
        }
        uint8_t bit = get_bit(idx.bwt, row);
        out.push_back(bit ? '1' : '0');
        row = lf_map(row);
    }

    std::reverse(out.begin(), out.end());
    return out;
}
