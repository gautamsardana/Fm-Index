#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include "fm_index.h"
#include "debug.h"

std::pair<std::vector<uint8_t>, uint64_t> read_input(const std::string &filepath) {
    bool is_bin = filepath.size() >= 4 &&
                  filepath.substr(filepath.size() - 4) == ".bin";

    if (is_bin) {
        std::ifstream f(filepath, std::ios::binary);
        if (!f) throw std::runtime_error("Cannot open file: " + filepath);
        std::vector<uint8_t> packed(std::istreambuf_iterator<char>(f), {});
        uint64_t n_bits = packed.size() * 8;
        return {packed, n_bits};
    } else {
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
        if (bit_pos > 0)
            packed.push_back(byte << (8 - bit_pos));
        return {packed, n_bits};
    }
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
    auto [s, n_bits] = read_input(input_file);
    idx.n = n_bits + 1; // +1 for sentinel
    idx.use_jacobson = use_jacobson;

    build_suffix_array(idx, s);
    build_bwt(idx, s);
    build_rank(idx);
    debug_print(idx);
}

void print_usage(const char *prog) {
    std::cerr << "Usage:\n"
              << "  " << prog << " [--jacobson] --input <file>                        (build only)\n"
              << "  " << prog << " [--jacobson] --input <file> --count <p1> <p2> ...  (build + count queries)\n"
              << "  " << prog << " [--jacobson] --input <file> --locate <p1> <p2> ... (build + locate queries)\n";
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
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
        if (std::string(argv[1]) != "--input") {
            print_usage(argv[0]);
            return 1;
        }

        std::string input_file = argv[2];
        FmIndex idx;
        build_index(idx, input_file);

        if (argc == 3) {
            // build only
            std::cout << "build: n=" << (idx.n - 1) << " bits\n";
            return 0;
        }

        std::string query_mode = argv[3];
        if (query_mode != "--count" && query_mode != "--locate") {
            print_usage(argv[0]);
            return 1;
        }

        for (int i = 4; i < argc; i++) {
            auto [pattern, pattern_len] = parse_pattern(argv[i]);
            if (query_mode == "--count") {
                uint64_t cnt = query_count(idx, pattern, pattern_len);
                std::cout << "count=" << cnt << " pattern=" << argv[i] << "\n";
            } else {
                std::vector<uint64_t> positions = query_locate(idx, pattern, pattern_len);
                std::cout << "count=" << positions.size() << " positions=";
                for (uint64_t p : positions) std::cout << p << " ";
                std::cout << "pattern=" << argv[i] << "\n";
            }
        }

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
