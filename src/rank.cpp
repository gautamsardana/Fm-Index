#include "fm_index.h"

void build_rank(FmIndex &idx) {
    idx.rank_table.resize(idx.n + 1, 0);
    for (uint64_t i = 0; i < idx.n; i++) {
        idx.rank_table[i + 1] = idx.rank_table[i] + get_bit(idx.bwt, i);
    }
}

uint64_t get_rank(const FmIndex &idx, uint8_t c, uint64_t i) {
    if (c == 1) return idx.rank_table[i];
    if (c == 0) return i - idx.rank_table[i] - (i > idx.sentinel_row ? 1 : 0);
    return 0;
}
