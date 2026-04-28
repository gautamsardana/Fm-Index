#ifndef JACOBSON_RANK_H
#define JACOBSON_RANK_H

#include <cstdint>
#include <vector>

struct JacobsonRank {
  std::vector<uint8_t> bwt;
  std::vector<uint64_t> chunk_rank;    // cumulative rank at each chunk
  std::vector<uint8_t> relative_ranks; // relative rank for each sub-chunk
  uint64_t n = 0;
  uint64_t chunk_size = 0;            // (log2(n))^2
  uint64_t sub_chunk_size = 0;        // (log2(n))/2
  uint8_t bits_per_relative_rank = 0; // log2((log2(n))^2) = 2*log2(log2(n))
};

inline uint8_t get_bit(const std::vector<uint8_t> &v, uint64_t i) {
  return (v[i / 8] >> (7 - (i % 8))) & 1;
}

inline void set_bit(std::vector<uint8_t> &v, uint64_t i, uint8_t bit) {
  if (bit)
    v[i / 8] |= (1 << (7 - (i % 8)));
  else
    v[i / 8] &= ~(1 << (7 - (i % 8)));
}

uint64_t get_bits(const std::vector<uint8_t> &v, uint64_t pos, uint64_t width);

void set_bits(std::vector<uint8_t> &v, uint64_t pos, uint64_t value,
              uint8_t width);

uint64_t popcount(uint64_t value); // Counts how many 1's are set in a
                                   // uint_64_t, should be a constant operation

// time: O(n)  space: O(n/log(n) + n/log^2(n)) — build chunk & sub-chunk
void build_rank(JacobsonRank &rank, const std::vector<uint8_t> &bwt,
                uint64_t n);

// time: O(1)  space: O(1) — constant-time rank query
uint64_t get_rank(const JacobsonRank &rank, uint64_t offset);

#endif
