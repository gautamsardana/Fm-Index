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
#ifdef __APPLE__
#include <mach/mach.h>
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

struct PerfSnapshot {
    double cpu_ms;
    long   peak_rss_kb;
    long   current_rss_kb;
};

static long get_current_rss_kb() {
#ifdef __APPLE__
    malloc_statistics_t stats;
    malloc_zone_statistics(nullptr, &stats);
    return (long)(stats.size_in_use / 1024);
#else
    struct mallinfo2 mi = mallinfo2();
    return (long)(mi.uordblks / 1024);
#endif
}

static PerfSnapshot take_snapshot() {
    struct rusage r;
    getrusage(RUSAGE_SELF, &r);
    double cpu_ms = (r.ru_utime.tv_sec + r.ru_stime.tv_sec) * 1000.0
                  + (r.ru_utime.tv_usec + r.ru_stime.tv_usec) / 1000.0;
#ifdef __APPLE__
    long peak_kb = r.ru_maxrss / 1024;
#else
    long peak_kb = r.ru_maxrss;
#endif
    return {cpu_ms, peak_kb, get_current_rss_kb()};
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
    if (idx.use_sparse_sa)
        sample_suffix_array(idx, idx.sa_sampling_rate);
    debug_print(idx);
}

void print_usage(const char *prog) {
    std::cerr << "Usage:\n"
              << "  " << prog << " [--jacobson] [--ssa <k>] --input <file>\n"
              << "     [--num-runs <n>] --count|--locate [--pattern-file <file> | <pattern>]\n"
              << "     --extract <start> <end>\n";
}

int main(int argc, char *argv[]) {
    if (argc < 3) { print_usage(argv[0]); return 1; }

    bool use_jacobson = false;
    bool use_sparse_sa = false;
    uint64_t sa_sampling_rate = 1;
    int i = 1;

    while (i < argc && std::string(argv[i]).rfind("--", 0) == 0) {
        std::string flag = argv[i];
        if (flag == "--jacobson") {
            use_jacobson = true; i++;
        } else if (flag == "--ssa") {
            if (i + 1 >= argc) { print_usage(argv[0]); return 1; }
            use_sparse_sa = true;
            sa_sampling_rate = std::stoull(argv[i + 1]);
            i += 2;
        } else {
            break;
        }
    }

    if (i >= argc || std::string(argv[i]) != "--input") { print_usage(argv[0]); return 1; }
    std::string input_file = argv[++i];
    i++;

    try {
        FmIndex idx;

#ifdef PERF
        auto s0 = take_snapshot();
        build_index(idx, input_file, use_jacobson, use_sparse_sa, sa_sampling_rate);
        auto s1 = take_snapshot();
        std::cout << "perf build: cpu_ms=" << (s1.cpu_ms - s0.cpu_ms)
                  << " peak_rss_kb=" << s1.peak_rss_kb
                  << " post_build_rss_kb=" << s1.current_rss_kb << "\n";
#else
        build_index(idx, input_file, use_jacobson, use_sparse_sa, sa_sampling_rate);
#endif

        if (i >= argc) {
            std::cout << "build: n=" << (idx.n - 1) << " bits\n";
            return 0;
        }

        // --extract <start> <end>
        if (std::string(argv[i]) == "--extract") {
            if (i + 2 >= argc) { print_usage(argv[0]); return 1; }
            uint64_t start = std::stoull(argv[i + 1]);
            uint64_t end   = std::stoull(argv[i + 2]);
            std::string extracted = extract_substring(idx, start, end);
            std::cout << "extract=" << extracted << " start=" << start << " end=" << end << "\n";
            return 0;
        }

        // --num-runs <n> (optional)
        uint64_t num_runs = 1;
        if (i < argc && std::string(argv[i]) == "--num-runs") {
            i++;
            if (i >= argc) { print_usage(argv[0]); return 1; }
            num_runs = std::stoull(argv[i++]);
        }

        // --count|--locate [--pattern-file <file> | <pattern>]
        if (i >= argc) { print_usage(argv[0]); return 1; }
        std::string mode = argv[i++];
        if (mode != "--count" && mode != "--locate") { print_usage(argv[0]); return 1; }

        std::vector<uint8_t> pattern;
        uint64_t pattern_len;
        std::string label;

        if (i < argc && std::string(argv[i]) == "--pattern-file") {
            i++;
            if (i >= argc) { print_usage(argv[0]); return 1; }
            label = argv[i++];
            auto [p, pl] = read_input(label);
            pattern = p; pattern_len = pl;
        } else {
            if (i >= argc) { print_usage(argv[0]); return 1; }
            label = argv[i];
            auto [p, pl] = parse_pattern(argv[i++]);
            pattern = p; pattern_len = pl;
        }

#ifdef PERF
        auto sq0 = take_snapshot();
#endif
        for (uint64_t r = 0; r < num_runs; r++) {
            if (mode == "--count") {
                uint64_t cnt = query_count(idx, pattern, pattern_len);
                if (num_runs == 1)
                    std::cout << "count=" << cnt << " pattern=" << label << "\n";
            } else {
                std::vector<uint64_t> positions = query_locate(idx, pattern, pattern_len);
                if (num_runs == 1) {
                    std::cout << "count=" << positions.size() << " positions=";
                    for (uint64_t p : positions) std::cout << p << " ";
                    std::cout << "pattern=" << label << "\n";
                }
            }
        }
#ifdef PERF
        auto sq1 = take_snapshot();
        std::cout << "perf queries: cpu_ms=" << (sq1.cpu_ms - sq0.cpu_ms)
                  << " mem_delta_kb=" << (sq1.peak_rss_kb - sq0.peak_rss_kb)
                  << " n_queries=" << num_runs << "\n";
#endif

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
