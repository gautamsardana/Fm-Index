#ifndef JACOBSON_RANK_H
#define JACOBSON_RANK_H

#include <cstdint>
#include <vector>

struct JacobsonRank {
  const std::vector<uint8_t> *bwt = nullptr; // pointer to BWT — no copy
  std::vector<uint64_t> chunk_rank;          // cumulative rank at each chunk
  std::vector<uint8_t> relative_ranks;       // relative rank for each sub-chunk
  uint64_t n = 0;
  uint64_t chunk_size = 0;           // (log2(n))^2
  uint64_t sub_chunk_size = 0;       // (log2(n))/2
  uint64_t sub_chunks_per_chunk = 0; // cached to avoid recomputing per query
  uint8_t bits_per_relative_rank = 0;
};

uint64_t get_bits(const std::vector<uint8_t> &v, uint64_t pos, uint64_t width);

void set_bits(std::vector<uint8_t> &v, uint64_t pos, uint64_t value,
              uint8_t width);

uint64_t popcount(uint64_t value);

// time: O(n)  space: O(n/log(n) + n/log^2(n))
void build_rank(JacobsonRank &rank, const std::vector<uint8_t> &bwt,
                uint64_t n);

// time: O(1)  space: O(1) — constant-time rank query
uint64_t get_rank(const JacobsonRank &rank, uint64_t offset);

#endif
