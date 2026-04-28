#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>

std::vector<uint8_t> read_bin(const std::string &filepath) {
    std::ifstream f(filepath, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open file: " + filepath);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), {});
}

void print_usage(const char *prog) {
    std::cerr << "Usage:\n"
              << "  " << prog << " --convert         <input_file> <index_file>\n"
              << "  " << prog << " --query           <index_file> <pattern>\n"
              << "  " << prog << " --convert-and-query <input_file> <index_file> <pattern>\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "--convert") {
        if (argc != 4) { print_usage(argv[0]); return 1; }
        std::string input_file = argv[2];
        std::string index_file = argv[3];
        std::cout << "convert: " << input_file << " -> " << index_file << "\n";
        // TODO: call fm_index build

    } else if (mode == "--query") {
        if (argc != 4) { print_usage(argv[0]); return 1; }
        std::string index_file = argv[2];
        std::string pattern    = argv[3];
        std::cout << "query: pattern=" << pattern << " in " << index_file << "\n";
        // TODO: call fm_index count/locate

    } else if (mode == "--convert-and-query") {
        if (argc != 5) { print_usage(argv[0]); return 1; }
        std::string input_file = argv[2];
        std::string index_file = argv[3];
        std::string pattern    = argv[4];
        std::cout << "convert: " << input_file << " -> " << index_file << "\n";
        // TODO: call fm_index build
        std::cout << "query: pattern=" << pattern << " in " << index_file << "\n";
        // TODO: call fm_index count/locate

    } else {
        std::cerr << "Unknown mode: " << mode << "\n";
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
