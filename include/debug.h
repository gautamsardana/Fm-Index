#ifndef DEBUG_H
#define DEBUG_H

#include "fm_index.h"
#include <iostream>

#ifdef DEBUG
#define DEBUG_PRINT(x) std::cerr << x << "\n"
#else
#define DEBUG_PRINT(x)
#endif

#ifdef DEBUG
inline void debug_print(const FmIndex &idx) {
    std::cerr << "\n=== FM Index Debug ===\n";

    std::cerr << "n = " << idx.n << "\n";

    std::cerr << "SA: ";
    for (uint64_t v : idx.suffix_array) std::cerr << v << " ";
    std::cerr << "\n";

    std::cerr << "BWT: ";
    for (uint64_t i = 0; i < idx.n; i++) std::cerr << (int)get_bit(idx.bwt, i);
    std::cerr << "\n";

    std::cerr << "sentinel_row = " << idx.sentinel_row << "\n";
    std::cerr << "c_table: 0=" << idx.c_table[0]
              << " 1=" << idx.c_table[1] << "\n";

    std::cerr << "rank_table: ";
    for (uint64_t v : idx.rank_table) std::cerr << v << " ";
    std::cerr << "\n";

    std::cerr << "======================\n\n";
}
#else
inline void debug_print(const FmIndex &) {}
#endif

#endif
