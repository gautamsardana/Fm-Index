#include "fm_index.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <stdexcept>

#include "suffix_array.h"

void build_suffix_array(FmIndex &idx, const std::vector<uint8_t> &s) {
    idx.n = s.size();
    idx.suffix_array.clear();
    if (idx.n == 0) {
        return;
    }

    std::vector<char> buffer(idx.n);
    for (uint64_t i = 0; i < idx.n; ++i) {
        buffer[i] = static_cast<char>(s[i]);
    }

    int *sa = build_suffix_array(buffer.data(), static_cast<int>(idx.n));
    if (!sa) {
        throw std::runtime_error("suffix array construction failed");
    }

    idx.suffix_array.resize(idx.n);
    for (uint64_t i = 0; i < idx.n; ++i) {
        idx.suffix_array[i] = static_cast<uint64_t>(sa[i]);
    }
    free(sa);
}

void build_bwt(FmIndex &idx, const std::vector<uint8_t> &s) {
    if (s.empty()) {
        idx.bwt.clear();
        idx.n = 0;
        return;
    }
    if (idx.suffix_array.empty() || idx.n != s.size()) {
        build_suffix_array(idx, s);
    }

    idx.bwt.resize(idx.n);
    for (uint64_t i = 0; i < idx.n; ++i) {
        uint64_t pos = idx.suffix_array[i];
        uint64_t prev = (pos + idx.n - 1) % idx.n;
        idx.bwt[i] = s[prev];
    }
}

void build_rank(FmIndex &idx) {
    // TODO
}

uint64_t get_rank(const FmIndex &idx, uint8_t c, uint64_t i) {
    // TODO
    return 0;
}

void store_index(const FmIndex &idx, const std::string &filepath) {
    // TODO
}

FmIndex load_index(const std::string &filepath) {
    // TODO
    return {};
}

uint64_t query_count(const FmIndex &idx, const std::vector<uint8_t> &pattern) {
    // TODO
    return 0;
}

std::vector<uint64_t> query_locate(const FmIndex &idx, const std::vector<uint8_t> &pattern) {
    // TODO
    return {};
}
