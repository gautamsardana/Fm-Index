#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <sys/resource.h>
#include <sdsl/suffix_arrays.hpp>

using namespace sdsl;
using namespace std;

// Read any file as binary (treat all bytes as data, extract bits)
pair<vector<uint8_t>, uint64_t> read_binary_input(const string &filepath) {
    ifstream f(filepath, ios::binary);
    if (!f) throw runtime_error("Cannot open file: " + filepath);

    // Read all bytes from file
    vector<uint8_t> packed(istreambuf_iterator<char>(f), {});
    uint64_t n_bits = packed.size() * 8;

    return {packed, n_bits};
}

// Convert binary bit vector to alphabet string (0→'A', 1→'B')
string encode_binary_to_alphabet(const vector<uint8_t> &packed, uint64_t n_bits) {
    string encoded;
    encoded.reserve(n_bits);

    for (uint64_t i = 0; i < n_bits; i++) {
        uint8_t bit = (packed[i / 8] >> (7 - (i % 8))) & 1;
        encoded.push_back(bit ? 'B' : 'A');
    }

    return encoded;
}

// Convert binary pattern to alphabet string
string encode_pattern(const string &binary_pattern) {
    string encoded;
    encoded.reserve(binary_pattern.size());

    for (char c : binary_pattern) {
        if (c == '0') encoded.push_back('A');
        else if (c == '1') encoded.push_back('B');
    }

    return encoded;
}

// Get current RSS memory in KB
long get_rss_kb() {
    struct rusage r;
    getrusage(RUSAGE_SELF, &r);
#ifdef __APPLE__
    return r.ru_maxrss / 1024; // macOS returns bytes
#else
    return r.ru_maxrss; // Linux returns KB
#endif
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file> [pattern1] [pattern2] ...\n";
        cerr << "Example: " << argv[0] << " data.bin 010101 11001\n";
        return 1;
    }

    string input_file = argv[1];

    try {
        // Read binary input
        cout << "Reading input file: " << input_file << "\n";
        auto [packed, n_bits] = read_binary_input(input_file);
        cout << "Input size: " << n_bits << " bits\n";

        // Encode to alphabet
        cout << "Encoding to alphabet...\n";
        string encoded_text = encode_binary_to_alphabet(packed, n_bits);
        cout << "Encoded size: " << encoded_text.size() << " characters\n";

        // Build SDSL FM-index
        cout << "\nBuilding SDSL FM-index...\n";
        auto build_start = chrono::high_resolution_clock::now();
        long mem_before = get_rss_kb();

        csa_wt<> fm_index;
        construct_im(fm_index, encoded_text, 1);

        auto build_end = chrono::high_resolution_clock::now();
        long mem_after = get_rss_kb();

        auto build_time_ms = chrono::duration_cast<chrono::milliseconds>(build_end - build_start).count();

        cout << "Build complete!\n";
        cout << "Build time: " << build_time_ms << " ms\n";
        cout << "Peak RSS: " << mem_after << " KB\n";
        cout << "Memory delta: " << (mem_after - mem_before) << " KB\n";

        // Run queries if provided
        if (argc > 2) {
            cout << "\nRunning queries...\n";
            for (int i = 2; i < argc; i++) {
                string pattern = argv[i];
                string encoded_pattern = encode_pattern(pattern);

                auto query_start = chrono::high_resolution_clock::now();
                size_t count_result = count(fm_index, encoded_pattern);
                auto query_end = chrono::high_resolution_clock::now();

                auto query_time_us = chrono::duration_cast<chrono::microseconds>(query_end - query_start).count();

                cout << "Pattern: " << pattern
                     << " | Count: " << count_result
                     << " | Time: " << query_time_us << " μs\n";
            }
        }

        // Index size info
        cout << "\nSDSL Index info:\n";
        cout << "Index size in bytes: " << size_in_bytes(fm_index) << "\n";
        cout << "Index size in MB: " << (size_in_bytes(fm_index) / 1024.0 / 1024.0) << "\n";

    } catch (const exception &e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
