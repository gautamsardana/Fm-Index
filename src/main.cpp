#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <sys/resource.h>
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

#ifdef PERF
struct PerfSnapshot {
    double cpu_ms;
    long   peak_rss_kb;
};

static PerfSnapshot take_snapshot() {
    struct rusage r;
    getrusage(RUSAGE_SELF, &r);
    double cpu_ms = (r.ru_utime.tv_sec + r.ru_stime.tv_sec) * 1000.0
                  + (r.ru_utime.tv_usec + r.ru_stime.tv_usec) / 1000.0;
    long peak_kb = r.ru_maxrss / 1024; // macOS returns bytes
    return {cpu_ms, peak_kb};
}
#endif

void build_index(FmIndex &idx, const std::string &input_file, bool use_jacobson = false,
                 bool use_sparse_sa = false, uint64_t sa_sampling_rate = 1) {
    auto [s, n_bits] = read_input(input_file);
    idx.n = n_bits + 1; // +1 for sentinel
    idx.use_jacobson = use_jacobson;
    idx.use_sparse_sa = use_sparse_sa;
    idx.sa_sampling_rate = sa_sampling_rate;

    build_suffix_array(idx, s);
    build_bwt(idx, s);
    build_rank(idx);
    if (idx.use_sparse_sa) {
        sample_suffix_array(idx, idx.sa_sampling_rate);
    }
    debug_print(idx);
}

void print_usage(const char *prog) {
    std::cerr << "Usage:\n"
              << "  " << prog << " [--jacobson] [--ssa <k>] --input <file>                        (build only)\n"
              << "  " << prog << " [--jacobson] [--ssa <k>] --input <file> --count <p1> <p2> ...  (build + count queries)\n"
              << "  " << prog << " [--jacobson] [--ssa <k>] --input <file> --locate <p1> <p2> ... (build + locate queries)\n";
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    // Parse optional flags
    bool use_jacobson = false;
    bool use_sparse_sa = false;
    uint64_t sa_sampling_rate = 1;
    int arg_offset = 1;
    while (arg_offset < argc && std::string(argv[arg_offset]).rfind("--", 0) == 0) {
        std::string flag = argv[arg_offset];
        if (flag == "--jacobson") {
            use_jacobson = true;
            arg_offset++;
        } else if (flag == "--ssa") {
            if (arg_offset + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            use_sparse_sa = true;
            sa_sampling_rate = std::stoull(argv[arg_offset + 1]);
            if (sa_sampling_rate == 0) {
                throw std::runtime_error("--ssa sampling rate must be >= 1");
            }
            arg_offset += 2;
        } else {
            break;
        }
    }
    if (arg_offset >= argc) {
        print_usage(argv[0]);
        return 1;
    }
    try {
        if (std::string(argv[arg_offset]) != "--input") {
            print_usage(argv[0]);
            return 1;
        }

        std::string input_file = argv[arg_offset + 1];
        FmIndex idx;

#ifdef PERF
        auto s0 = take_snapshot();
    build_index(idx, input_file, use_jacobson, use_sparse_sa, sa_sampling_rate);
        auto s1 = take_snapshot();
        std::cout << "perf build: cpu_ms=" << (s1.cpu_ms - s0.cpu_ms)
                  << " peak_rss_kb=" << s1.peak_rss_kb << "\n";
#else
    build_index(idx, input_file, use_jacobson, use_sparse_sa, sa_sampling_rate);
#endif

        if (argc == arg_offset + 2) {
            std::cout << "build: n=" << (idx.n - 1) << " bits\n";
            return 0;
        }

        std::string query_mode = argv[arg_offset + 2];
        if (query_mode != "--count" && query_mode != "--locate") {
            print_usage(argv[0]);
            return 1;
        }

        for (int i = arg_offset + 3; i < argc; i++) {
            auto [pattern, pattern_len] = parse_pattern(argv[i]);
            if (query_mode == "--count") {
#ifdef PERF
                auto sq0 = take_snapshot();
                uint64_t cnt = query_count(idx, pattern, pattern_len);
                auto sq1 = take_snapshot();
                std::cout << "perf count: cpu_ms=" << (sq1.cpu_ms - sq0.cpu_ms)
                          << " mem_delta_kb=" << (sq1.peak_rss_kb - sq0.peak_rss_kb)
                          << " pattern=" << argv[i] << " count=" << cnt << "\n";
#else
                uint64_t cnt = query_count(idx, pattern, pattern_len);
                std::cout << "count=" << cnt << " pattern=" << argv[i] << "\n";
#endif
            } else {
#ifdef PERF
                auto sq0 = take_snapshot();
                std::vector<uint64_t> positions = query_locate(idx, pattern, pattern_len);
                auto sq1 = take_snapshot();
                std::cout << "perf locate: cpu_ms=" << (sq1.cpu_ms - sq0.cpu_ms)
                          << " mem_delta_kb=" << (sq1.peak_rss_kb - sq0.peak_rss_kb)
                          << " pattern=" << argv[i] << " count=" << positions.size() << "\n";
#else
                std::vector<uint64_t> positions = query_locate(idx, pattern, pattern_len);
                std::cout << "count=" << positions.size() << " positions=";
                for (uint64_t p : positions) std::cout << p << " ";
                std::cout << "pattern=" << argv[i] << "\n";
#endif
            }
        }

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
