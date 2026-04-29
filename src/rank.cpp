#include "fm_index.h"
#include "jacobson_rank.h"

void build_rank(FmIndex &idx) {
    if (idx.use_jacobson) {
        build_rank(idx.jacobson_rank, idx.bwt, idx.n);
    } else {
        idx.rank_table.resize(idx.n + 1, 0);
        for (uint64_t i = 0; i < idx.n; i++) {
            idx.rank_table[i + 1] = idx.rank_table[i] + get_bit(idx.bwt, i);
        }
    }
}

uint64_t get_rank(const FmIndex &idx, uint8_t c, uint64_t i) {
    uint64_t rank_1;

    if (idx.use_jacobson) {
        rank_1 = get_rank(idx.jacobson_rank, i);
    } else {
        rank_1 = idx.rank_table[i];
    }

    if (c == 1) return rank_1;
    if (c == 0) return i - rank_1 - (i > idx.sentinel_row ? 1 : 0);
    return 0;
}
