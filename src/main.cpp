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

void build_index(FmIndex &idx, const std::string &input_file) {
    auto [s, n_bits] = read_bin(input_file);
    idx.n = n_bits + 1; // +1 for sentinel

    build_suffix_array(idx, s);
    build_bwt(idx, s);
    build_rank(idx);
    debug_print(idx);
}

void print_usage(const char *prog) {
    std::cerr << "Usage:\n"
              << "  " << prog << " --convert           <input_file> <index_file>\n"
              << "  " << prog << " --query             <index_file> <pattern>\n"
              << "  " << prog << " --convert-and-query <input_file> <pattern>\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string mode = argv[1];

    try {
        if (mode == "--convert") {
            if (argc != 4) { print_usage(argv[0]); return 1; }
            FmIndex idx;
            build_index(idx, argv[2]);
            store_index(idx, argv[3]);
            std::cout << "Stored index: " << argv[3] << "\n";

        } else if (mode == "--query") {
            if (argc != 4) { print_usage(argv[0]); return 1; }
            FmIndex idx = load_index(argv[2]);
            auto [pattern, pattern_len] = parse_pattern(argv[3]);
            uint64_t cnt = query_count(idx, pattern, pattern_len);
            std::vector<uint64_t> positions = query_locate(idx, pattern, pattern_len);
            std::cout << "count=" << cnt << " positions=";
            for (uint64_t p : positions) std::cout << p << " ";
            std::cout << "\n";

        } else if (mode == "--convert-and-query") {
            if (argc != 4) { print_usage(argv[0]); return 1; }
            FmIndex idx;
            build_index(idx, argv[2]);
            auto [pattern, pattern_len] = parse_pattern(argv[3]);
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
