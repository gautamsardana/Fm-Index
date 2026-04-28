#include "fm_index.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>

void build_suffix_array(FmIndex &idx, const std::vector<uint8_t> &s) {
    // TODO
}

void build_bwt(FmIndex &idx, const std::vector<uint8_t> &s) {
    // TODO
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

int query_count(const FmIndex &idx, const std::vector<uint8_t> &pattern) {
    // TODO
    return 0;
}

std::vector<int> query_locate(const FmIndex &idx, const std::vector<uint8_t> &pattern) {
    // TODO
    return {};
}
