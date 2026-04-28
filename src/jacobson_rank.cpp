#include "jacobson_rank.h"
#include <cmath>

uint64_t get_bits(const std::vector<uint8_t> &v, uint64_t pos, uint64_t width) {
  uint64_t result = 0;
  for (uint64_t i = 0; i < width; i++) {
    result = (result << 1) | get_bit(v, pos + i);
  }
  return result;
}

void set_bits(std::vector<uint8_t> &v, uint64_t pos, uint64_t value,
              uint8_t width) {
  for (uint8_t i = 0; i < width; i++) {
    set_bit(v, pos + width - 1 - i, (value >> i) & 1);
  }
}

uint64_t popcount(uint64_t value) { return __builtin_popcountll(value); }

void build_rank(JacobsonRank &rank, const std::vector<uint8_t> &bwt,
                uint64_t n) {
  rank.bwt = bwt;
  rank.n = n;

  uint64_t log_n = (uint64_t)std::ceil(std::log2(n));
  rank.chunk_size = log_n * log_n; // (log2(n))^2
  rank.sub_chunk_size = log_n / 2; // (log2(n))/2

  uint64_t num_chunks = (n + rank.chunk_size - 1) / rank.chunk_size;

  rank.bits_per_relative_rank = (uint8_t)std::ceil(std::log2(rank.chunk_size));

  rank.chunk_rank.resize(num_chunks, 0);

  uint64_t sub_chunks_per_chunk =
      (rank.chunk_size + rank.sub_chunk_size - 1) / rank.sub_chunk_size;
  uint64_t total_sub_chunks = num_chunks * sub_chunks_per_chunk;

  uint64_t total_bits_for_relative_ranks =
      total_sub_chunks * rank.bits_per_relative_rank;
  rank.relative_ranks.resize((total_bits_for_relative_ranks + 7) / 8, 0);

  uint64_t cumulative_rank = 0;

  for (uint64_t chunk_idx = 0; chunk_idx < num_chunks; chunk_idx++) {
    rank.chunk_rank[chunk_idx] = cumulative_rank;

    uint64_t chunk_start = chunk_idx * rank.chunk_size;
    uint64_t chunk_end = std::min(chunk_start + rank.chunk_size, n);

    uint64_t relative_rank = 0;
    for (uint64_t sub_idx = 0; sub_idx < sub_chunks_per_chunk; sub_idx++) {
      uint64_t sub_chunk_start = chunk_start + sub_idx * rank.sub_chunk_size;
      uint64_t sub_chunk_end =
          std::min(sub_chunk_start + rank.sub_chunk_size, chunk_end);

      if (sub_chunk_start >= chunk_end)
        break;

      uint64_t sub_chunk_global_idx =
          chunk_idx * sub_chunks_per_chunk + sub_idx;
      uint64_t bit_pos = sub_chunk_global_idx * rank.bits_per_relative_rank;
      set_bits(rank.relative_ranks, bit_pos, relative_rank,
               rank.bits_per_relative_rank);

      for (uint64_t i = sub_chunk_start; i < sub_chunk_end; i++) {
        relative_rank += get_bit(bwt, i);
      }
    }

    cumulative_rank += relative_rank;
  }
}

uint64_t get_rank(const JacobsonRank &rank, uint64_t offset) {
  if (offset == 0)
    return 0;
  if (offset > rank.n)
    offset = rank.n;

  uint64_t which_chunk = offset / rank.chunk_size;

  if (which_chunk >= rank.chunk_rank.size()) {
    which_chunk = rank.chunk_rank.size() - 1;
    uint64_t cumulative = rank.chunk_rank[which_chunk];
    uint64_t chunk_start = which_chunk * rank.chunk_size;

    uint64_t remaining = 0;
    for (uint64_t i = chunk_start; i < offset && i < rank.n; i++) {
      remaining += get_bit(rank.bwt, i);
    }
    return cumulative + remaining;
  }

  uint64_t chunk_start = which_chunk * rank.chunk_size;
  uint64_t offset_in_chunk = offset - chunk_start;
  uint64_t which_sub_chunk = offset_in_chunk / rank.sub_chunk_size;

  uint64_t cumulative = rank.chunk_rank[which_chunk];

  uint64_t sub_chunks_per_chunk =
      (rank.chunk_size + rank.sub_chunk_size - 1) / rank.sub_chunk_size;
  uint64_t sub_chunk_global_idx =
      which_chunk * sub_chunks_per_chunk + which_sub_chunk;
  uint64_t bit_pos = sub_chunk_global_idx * rank.bits_per_relative_rank;
  uint64_t relative =
      get_bits(rank.relative_ranks, bit_pos, rank.bits_per_relative_rank);

  uint64_t sub_chunk_start =
      chunk_start + which_sub_chunk * rank.sub_chunk_size;
  uint64_t num_remaining_bits = offset - sub_chunk_start;

  if (sub_chunk_start + num_remaining_bits > rank.n) {
    num_remaining_bits = rank.n - sub_chunk_start;
  }

  uint64_t bits = 0;
  if (num_remaining_bits > 0) {
    bits = get_bits(rank.bwt, sub_chunk_start, num_remaining_bits);
  }
  uint64_t remaining = popcount(bits);

  return cumulative + relative + remaining;
}
