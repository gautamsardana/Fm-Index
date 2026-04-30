#include "fm_index.h"

void build_bwt(FmIndex &idx, const std::vector<uint8_t> &s) {
    if (idx.n == 0) {
        idx.bwt.clear();
        return;
    }
    if (idx.suffix_array.empty()) {
        build_suffix_array(idx, s);
    }

    uint64_t n = idx.n;
    idx.bwt.assign((n + 7) / 8, 0);

    for (uint64_t i = 0; i < n; i++) {
        uint64_t pos = idx.suffix_array[i];
        if (pos == 0) {
            // wrap - previous char is sentinel
            set_bit(idx.bwt, i, 0);
            idx.sentinel_row = i;
        } else {
            set_bit(idx.bwt, i, get_bit(s, pos - 1));
        }
    }
}
