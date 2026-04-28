#include "jacobson_rank.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

std::pair<std::vector<uint8_t>, uint64_t>
create_bitvector(const std::string &bits) {
  std::vector<uint8_t> packed((bits.size() + 7) / 8, 0);
  for (size_t i = 0; i < bits.size(); i++) {
    if (bits[i] == '1') {
      set_bit(packed, i, 1);
    }
  }
  return {packed, bits.size()};
}

uint64_t naive_rank(const std::vector<uint8_t> &bwt, uint64_t offset) {
  uint64_t count = 0;
  for (uint64_t i = 0; i < offset; i++) {
    count += get_bit(bwt, i);
  }
  return count;
}

void test_small() {
  std::cout << "Testing small bitvector: 110101100...\n";
  auto [bwt, n] = create_bitvector("110101100");

  JacobsonRank rank;
  build_rank(rank, bwt, n);

  std::cout << "  n = " << n << "\n";
  std::cout << "  chunk_size = " << rank.chunk_size << "\n";
  std::cout << "  sub_chunk_size = " << rank.sub_chunk_size << "\n";
  std::cout << "  bits_per_relative_rank = " << (int)rank.bits_per_relative_rank
            << "\n";

  for (uint64_t i = 0; i <= n; i++) {
    uint64_t jacobson_result = get_rank(rank, i);
    uint64_t naive_result = naive_rank(bwt, i);
    std::cout << "  rank(" << i << ") = " << jacobson_result;
    if (jacobson_result != naive_result) {
      std::cout << " FAIL (expected " << naive_result << ")\n";
      assert(false);
    } else {
      std::cout << " OK\n";
    }
  }
  std::cout << "Small test PASSED!\n\n";
}

void test_medium() {
  std::cout << "Testing medium bitvector (64 bits)...\n";
  std::string bits =
      "1010101010101010111100001111000011110000111100001111000011110000";
  auto [bwt, n] = create_bitvector(bits);

  JacobsonRank rank;
  build_rank(rank, bwt, n);

  std::cout << "  n = " << n << "\n";
  std::cout << "  chunk_size = " << rank.chunk_size << "\n";
  std::cout << "  sub_chunk_size = " << rank.sub_chunk_size << "\n";

  // Test some positions
  std::vector<uint64_t> test_positions = {0, 1, 10, 20, 32, 50, 63, 64};
  for (uint64_t i : test_positions) {
    uint64_t jacobson_result = get_rank(rank, i);
    uint64_t naive_result = naive_rank(bwt, i);
    std::cout << "  rank(" << i << ") = " << jacobson_result;
    if (jacobson_result != naive_result) {
      std::cout << " FAIL (expected " << naive_result << ")\n";
      assert(false);
    } else {
      std::cout << " OK\n";
    }
  }
  std::cout << "Medium test PASSED!\n\n";
}

void test_large() {
  std::cout << "Testing large bitvector (1000 bits)...\n";
  std::string bits;
  for (int i = 0; i < 1000; i++) {
    bits += (i % 3 == 0) ? '1' : '0';
  }
  auto [bwt, n] = create_bitvector(bits);

  JacobsonRank rank;
  build_rank(rank, bwt, n);

  std::cout << "  n = " << n << "\n";
  std::cout << "  chunk_size = " << rank.chunk_size << "\n";
  std::cout << "  sub_chunk_size = " << rank.sub_chunk_size << "\n";
  std::cout << "  num_chunks = " << rank.chunk_rank.size() << "\n";

  // Test various positions
  std::vector<uint64_t> test_positions = {0, 1, 50, 100, 200, 500, 999, 1000};
  for (uint64_t i : test_positions) {
    uint64_t jacobson_result = get_rank(rank, i);
    uint64_t naive_result = naive_rank(bwt, i);
    std::cout << "  rank(" << i << ") = " << jacobson_result;
    if (jacobson_result != naive_result) {
      std::cout << " FAIL (expected " << naive_result << ")\n";
      assert(false);
    } else {
      std::cout << " OK\n";
    }
  }
  std::cout << "Large test PASSED!\n\n";
}

void test_all_zeros() {
  std::cout << "Testing all zeros (100 bits)...\n";
  std::string bits(100, '0');
  auto [bwt, n] = create_bitvector(bits);

  JacobsonRank rank;
  build_rank(rank, bwt, n);

  for (uint64_t i = 0; i <= n; i += 10) {
    uint64_t result = get_rank(rank, i);
    assert(result == 0);
  }
  std::cout << "All zeros test PASSED!\n\n";
}

void test_all_ones() {
  std::cout << "Testing all ones (100 bits)...\n";
  std::string bits(100, '1');
  auto [bwt, n] = create_bitvector(bits);

  JacobsonRank rank;
  build_rank(rank, bwt, n);

  for (uint64_t i = 0; i <= n; i += 10) {
    uint64_t result = get_rank(rank, i);
    assert(result == i);
  }
  std::cout << "All ones test PASSED!\n\n";
}

int main() {
  std::cout << "=== Jacobson Rank Tests ===\n\n";

  test_small();
  test_medium();
  test_large();
  test_all_zeros();
  test_all_ones();

  std::cout << "=== All tests PASSED! ===\n";
  return 0;
}
