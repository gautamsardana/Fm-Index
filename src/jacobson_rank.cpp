#include "jacobson_rank.h"
#include "fm_index.h"
#include <cmath>

uint64_t get_bits(const std::vector<uint8_t> &v, uint64_t pos, uint64_t width) {
  if (width == 0) return 0;
  uint64_t result = 0;
  uint64_t byte_idx = pos / 8;
  uint64_t bit_off  = pos % 8;

  if (bit_off != 0) {
    uint64_t bits_in_first = 8 - bit_off;
    uint64_t take = (width < bits_in_first) ? width : bits_in_first;
    uint8_t mask = ((1u << take) - 1);
    result = (v[byte_idx] >> (bits_in_first - take)) & mask;
    width -= take;
    byte_idx++;
  }

  while (width >= 8) {
    result = (result << 8) | v[byte_idx++];
    width -= 8;
  }

  if (width > 0) {
    uint8_t mask = ((1u << width) - 1);
    result = (result << width) | ((v[byte_idx] >> (8 - width)) & mask);
  }

  return result;
}

// Fast path for get_bits when width <= 16 (always true for sub-chunk reads in get_rank).
// Reads at most 3 bytes, extracts `width` bits starting at bit `pos`.
static inline uint64_t get_bits_small(const std::vector<uint8_t> &v,
                                      uint64_t pos, uint64_t width) {
  if (width == 0) return 0;
  uint64_t byte_idx = pos / 8;
  uint64_t bit_off  = pos % 8;
  // load up to 3 bytes into a 32-bit window
  uint32_t window = (uint32_t)v[byte_idx] << 16;
  if (byte_idx + 1 < v.size()) window |= (uint32_t)v[byte_idx + 1] << 8;
  if (byte_idx + 2 < v.size()) window |= (uint32_t)v[byte_idx + 2];
  // shift so the desired bits are at the top of a `width`-bit field
  uint32_t shifted = window >> (24 - bit_off - width);
  return shifted & ((1u << width) - 1);
}

void set_bits(std::vector<uint8_t> &v, uint64_t pos, uint64_t value,
              uint8_t width) {
  if (width == 0) return;
  uint64_t byte_idx = pos / 8;
  uint64_t bit_off  = pos % 8;
  uint8_t  remaining = width;

  if (bit_off != 0) {
    uint8_t space = 8 - bit_off;
    uint8_t take  = (remaining < space) ? remaining : space;
    uint8_t shift = space - take;
    uint8_t bits  = (value >> (remaining - take)) & ((1u << take) - 1);
    uint8_t mask  = ((1u << take) - 1) << shift;
    v[byte_idx] = (v[byte_idx] & ~mask) | (bits << shift);
    remaining -= take;
    byte_idx++;
  }

  while (remaining >= 8) {
    v[byte_idx++] = (value >> (remaining - 8)) & 0xFF;
    remaining -= 8;
  }

  if (remaining > 0) {
    uint8_t shift = 8 - remaining;
    uint8_t bits  = value & ((1u << remaining) - 1);
    uint8_t mask  = ((1u << remaining) - 1) << shift;
    v[byte_idx] = (v[byte_idx] & ~mask) | (bits << shift);
  }
}

uint64_t popcount(uint64_t value) { return __builtin_popcountll(value); }

void build_rank(JacobsonRank &rank, const std::vector<uint8_t> &bwt,
                uint64_t n) {
  rank.bwt = &bwt;
  rank.n = n;

  uint64_t log_n = (n <= 1) ? 1 : (uint64_t)std::ceil(std::log2(n));
  rank.chunk_size           = log_n * log_n;
  rank.sub_chunk_size       = (log_n / 2 == 0) ? 1 : log_n / 2;
  rank.sub_chunks_per_chunk = (rank.chunk_size + rank.sub_chunk_size - 1) / rank.sub_chunk_size;

  uint64_t num_chunks = (n + rank.chunk_size - 1) / rank.chunk_size;
  rank.bits_per_relative_rank = (uint8_t)std::ceil(std::log2((double)rank.chunk_size + 1));

  rank.chunk_rank.resize(num_chunks, 0);

  uint64_t total_bits = num_chunks * rank.sub_chunks_per_chunk * rank.bits_per_relative_rank;
  rank.relative_ranks.resize((total_bits + 7) / 8, 0);

  uint64_t cumulative_rank = 0;

  for (uint64_t chunk_idx = 0; chunk_idx < num_chunks; chunk_idx++) {
    rank.chunk_rank[chunk_idx] = (uint32_t)cumulative_rank;

    uint64_t chunk_start = chunk_idx * rank.chunk_size;
    uint64_t chunk_end   = std::min(chunk_start + rank.chunk_size, n);

    uint64_t relative_rank = 0;
    for (uint64_t sub_idx = 0; sub_idx < rank.sub_chunks_per_chunk; sub_idx++) {
      uint64_t sub_start = chunk_start + sub_idx * rank.sub_chunk_size;
      if (sub_start >= chunk_end) break;
      uint64_t sub_end = std::min(sub_start + rank.sub_chunk_size, chunk_end);

      uint64_t global_sub = chunk_idx * rank.sub_chunks_per_chunk + sub_idx;
      uint64_t bit_pos    = global_sub * rank.bits_per_relative_rank;
      set_bits(rank.relative_ranks, bit_pos, relative_rank, rank.bits_per_relative_rank);

      uint64_t len = sub_end - sub_start;
      relative_rank += popcount(get_bits(bwt, sub_start, len));
    }

    cumulative_rank += relative_rank;
  }
}

uint64_t get_rank(const JacobsonRank &rank, uint64_t offset) {
  if (offset == 0) return 0;
  if (offset > rank.n) offset = rank.n;

  const std::vector<uint8_t> &bwt = *rank.bwt;

  uint64_t which_chunk      = offset / rank.chunk_size;
  uint64_t chunk_start      = which_chunk * rank.chunk_size;
  uint64_t offset_in_chunk  = offset - chunk_start;
  uint64_t which_sub        = offset_in_chunk / rank.sub_chunk_size;
  uint64_t sub_start        = chunk_start + which_sub * rank.sub_chunk_size;

  // edge: offset lands past the last chunk boundary
  if (which_chunk >= rank.chunk_rank.size()) {
    which_chunk  = rank.chunk_rank.size() - 1;
    chunk_start  = which_chunk * rank.chunk_size;
    uint64_t len = std::min(offset, rank.n) - chunk_start;
    return rank.chunk_rank[which_chunk] + popcount(get_bits(bwt, chunk_start, len));
  }

  // edge: sub_start is out of range
  if (sub_start >= rank.n) {
    uint64_t len = std::min(offset, rank.n) - chunk_start;
    return rank.chunk_rank[which_chunk] + popcount(get_bits(bwt, chunk_start, len));
  }

  uint64_t global_sub = which_chunk * rank.sub_chunks_per_chunk + which_sub;
  uint64_t bit_pos    = global_sub * rank.bits_per_relative_rank;
  uint64_t relative   = get_bits_small(rank.relative_ranks, bit_pos, rank.bits_per_relative_rank);

  uint64_t num_remaining = std::min(offset, rank.n) - sub_start;
  // num_remaining <= sub_chunk_size = log_n/2 <= 32, so get_bits_small is safe
  uint64_t remaining = (num_remaining > 0)
                       ? popcount(get_bits_small(bwt, sub_start, num_remaining))
                       : 0;

  return rank.chunk_rank[which_chunk] + relative + remaining;
}
