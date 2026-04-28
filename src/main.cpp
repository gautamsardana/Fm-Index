#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include "fm_index.h"
#include "debug.h"

std::pair<std::vector<uint8_t>, uint64_t> read_bin(const std::string &filepath) {
    std::ifstream f(filepath);
    if (!f) throw std::runtime_error("Cannot open file: " + filepath);
    std::vector<uint8_t> packed;
    char c;
    uint8_t byte = 0;
    int bit_pos = 0;
    uint64_t n_bits = 0;
    while (f.get(c)) {
        if (c != '0' && c != '1') continue;
        byte = (byte << 1) | (c == '1' ? 1 : 0);
        n_bits++;
        if (++bit_pos == 8) {
            packed.push_back(byte);
            byte = 0;
            bit_pos = 0;
        }
    }
    return {packed, n_bits};
}

std::pair<std::vector<uint8_t>, uint64_t> parse_pattern(const std::string &s) {
    std::vector<uint8_t> packed;
    uint8_t byte = 0;
    int bit_pos = 0;
    uint64_t n_bits = 0;
    for (char c : s) {
        if (c != '0' && c != '1')
            throw std::runtime_error("Invalid pattern character: " + std::string(1, c));
        byte = (byte << 1) | (c == '1' ? 1 : 0);
        n_bits++;
        if (++bit_pos == 8) {
            packed.push_back(byte);
            byte = 0;
            bit_pos = 0;
        }
    }
    if (bit_pos > 0)
        packed.push_back(byte << (8 - bit_pos));
    return {packed, n_bits};
}

void build_index(FmIndex &idx, const std::string &input_file, bool use_jacobson = false) {
    auto [s, n_bits] = read_bin(input_file);
    idx.n = n_bits + 1; // +1 for sentinel
    idx.use_jacobson = use_jacobson;

    build_suffix_array(idx, s);
    build_bwt(idx, s);
    build_rank(idx);
    debug_print(idx);
}

void print_usage(const char *prog) {
    std::cerr << "Usage:\n"
              << "  " << prog << " [--jacobson] --convert           <input_file> <index_file>\n"
              << "  " << prog << " [--jacobson] --query             <index_file> <pattern>\n"
              << "  " << prog << " [--jacobson] --convert-and-query <input_file> <pattern>\n"
              << "\nOptions:\n"
              << "  --jacobson  Use Jacobson rank (O(n/log n) space) instead of naive rank table (O(n) space)\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Check for --jacobson flag
    bool use_jacobson = false;
    int arg_offset = 1;
    if (std::string(argv[1]) == "--jacobson") {
        use_jacobson = true;
        arg_offset = 2;
        if (argc < 3) {
            print_usage(argv[0]);
            return 1;
        }
    }

    std::string mode = argv[arg_offset];

    try {
        if (mode == "--convert") {
            if (argc != arg_offset + 3) { print_usage(argv[0]); return 1; }
            FmIndex idx;
            build_index(idx, argv[arg_offset + 1], use_jacobson);
            store_index(idx, argv[arg_offset + 2]);
            std::cout << "Stored index: " << argv[arg_offset + 2] << "\n";

        } else if (mode == "--query") {
            if (argc != arg_offset + 3) { print_usage(argv[0]); return 1; }
            FmIndex idx = load_index(argv[arg_offset + 1]);
            auto [pattern, pattern_len] = parse_pattern(argv[arg_offset + 2]);
            uint64_t cnt = query_count(idx, pattern, pattern_len);
            std::vector<uint64_t> positions = query_locate(idx, pattern, pattern_len);
            std::cout << "count=" << cnt << " positions=";
            for (uint64_t p : positions) std::cout << p << " ";
            std::cout << "\n";

        } else if (mode == "--convert-and-query") {
            if (argc != arg_offset + 3) { print_usage(argv[0]); return 1; }
            FmIndex idx;
            build_index(idx, argv[arg_offset + 1], use_jacobson);
            auto [pattern, pattern_len] = parse_pattern(argv[arg_offset + 2]);
            uint64_t cnt = query_count(idx, pattern, pattern_len);
            std::vector<uint64_t> positions = query_locate(idx, pattern, pattern_len);
            std::cout << "count=" << cnt << " positions=";
            for (uint64_t p : positions) std::cout << p << " ";
            std::cout << "\n";

        } else {
            std::cerr << "Unknown mode: " << mode << "\n";
            print_usage(argv[0]);
            return 1;
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
