#include "fm_index.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <mach/mach.h>
#include <sstream>
#include <string>
#include <sys/resource.h>
#include <vector>

using namespace std;

pair<vector<uint8_t>, uint64_t> read_binary_input(const string &filepath,
                                                  uint64_t max_mb = 0) {
    ifstream f(filepath, ios::binary);
    if (!f) throw runtime_error("Cannot open file: " + filepath);

    vector<uint8_t> packed;
    if (max_mb > 0) {
        uint64_t max_bytes = max_mb * 1024 * 1024;
        packed.resize(max_bytes);
        f.read(reinterpret_cast<char *>(packed.data()), max_bytes);
        packed.resize(f.gcount());
    } else {
        packed.assign(istreambuf_iterator<char>(f), {});
    }

    return {packed, packed.size() * 8};
}

vector<uint8_t> extract_pattern(const vector<uint8_t> &packed, uint64_t offset,
                                uint64_t size) {
    vector<uint8_t> pattern((size + 7) / 8, 0);
    for (uint64_t i = 0; i < size; i++) {
        uint8_t bit = (packed[(offset + i) / 8] >> (7 - ((offset + i) % 8))) & 1;
        pattern[i / 8] |= (bit << (7 - (i % 8)));
    }
    return pattern;
}

long get_phys_footprint_kb() {
    task_vm_info_data_t info;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO,
                  (task_info_t)&info, &count) == KERN_SUCCESS)
        return (long)(info.phys_footprint / 1024);
    return -1;
}

double get_cpu_ms() {
    struct rusage r;
    getrusage(RUSAGE_SELF, &r);
    return (r.ru_utime.tv_sec + r.ru_stime.tv_sec) * 1000.0
         + (r.ru_utime.tv_usec + r.ru_stime.tv_usec) / 1000.0;
}

uint64_t index_size_bytes(const FmIndex &idx) {
    uint64_t total = 0;
    total += idx.bwt.capacity();
    total += idx.suffix_array.capacity()              * sizeof(uint64_t);
    total += idx.rank_table.capacity()                * sizeof(uint64_t);
    total += idx.jacobson_rank.chunk_rank.capacity()  * sizeof(uint32_t);
    total += idx.jacobson_rank.relative_ranks.capacity() * sizeof(uint8_t);
    return total;
}

struct QuerySpec { uint64_t offset, size; };

vector<QuerySpec> read_query_file(const string &filepath) {
    ifstream f(filepath);
    if (!f) throw runtime_error("Cannot open query file: " + filepath);
    vector<QuerySpec> queries;
    string line;
    while (getline(f, line)) {
        stringstream ss(line);
        uint64_t offset, size;
        char comma;
        if (ss >> offset >> comma >> size && comma == ',')
            queries.push_back({offset, size});
    }
    return queries;
}

void build_fm_index(FmIndex &idx, const vector<uint8_t> &data, uint64_t n_bits,
                    bool use_jacobson, bool use_ssa, uint64_t ssa_rate) {
    idx.n = n_bits + 1;
    idx.use_jacobson = use_jacobson;
    idx.use_sparse_sa = use_ssa;
    idx.sa_sampling_rate = use_ssa ? ssa_rate : 1;
    build_suffix_array(idx, data);
    build_bwt(idx, data);
    build_rank(idx);
    if (use_ssa)
        sample_suffix_array(idx, ssa_rate);
}

int main(int argc, char *argv[]) {
    if (argc < 6) {
        cerr << "Usage: " << argv[0]
             << " <input_file> <query_file> <build_out> <count_out> <locate_out>"
             << " [--jacobson] [--ssa <k>]\n";
        return 1;
    }

    string input_file   = argv[1];
    string query_file   = argv[2];
    string build_output = argv[3];
    string count_output = argv[4];
    string locate_output = argv[5];

    bool use_jacobson = false;
    bool use_ssa = false;
    uint64_t ssa_rate = 32;
    for (int i = 6; i < argc; i++) {
        if (string(argv[i]) == "--jacobson") use_jacobson = true;
        else if (string(argv[i]) == "--ssa" && i + 1 < argc) {
            use_ssa = true;
            ssa_rate = stoull(argv[++i]);
        }
    }

    vector<uint64_t> input_sizes_mb = {1, 2, 5, 10, 20};

    try {
        ofstream build_out(build_output);
        ofstream count_out(count_output);
        ofstream locate_out(locate_output);

        build_out << "input_size_mb,build_time_ms,peak_memory_kb,index_size_kb\n";
        count_out << "input_size_mb,query_size_bits,avg_time_us,avg_result_count\n";
        locate_out << "input_size_mb,query_size_bits,avg_time_us,avg_result_count\n";

        for (uint64_t size_mb : input_sizes_mb) {
            cerr << "[custom] building index for " << size_mb << " MB..." << flush;
            auto [packed, n_bits] = read_binary_input(input_file, size_mb);

            long mem_before = get_phys_footprint_kb();
            double cpu_before = get_cpu_ms();

            FmIndex fm_index;
            build_fm_index(fm_index, packed, n_bits, use_jacobson, use_ssa, ssa_rate);

            double build_time_ms = get_cpu_ms() - cpu_before;
            long peak_mem_kb = get_phys_footprint_kb();
            long index_kb = (long)(index_size_bytes(fm_index) / 1024);

            cerr << " done (" << (long)build_time_ms << " ms, index " << index_kb << " KB)\n";

            build_out << size_mb << "," << build_time_ms << ","
                      << (peak_mem_kb - mem_before) << "," << index_kb << "\n";
            build_out.flush();

            vector<QuerySpec> all_queries = read_query_file(query_file);
            vector<QuerySpec> valid_queries;
            for (const auto &q : all_queries)
                if (q.offset + q.size <= n_bits)
                    valid_queries.push_back(q);

            map<uint64_t, vector<QuerySpec>> by_size;
            for (const auto &q : valid_queries)
                by_size[q.size].push_back(q);

            for (const auto &[query_size, size_queries] : by_size) {
                cerr << "[custom] " << size_mb << " MB  query_size=" << query_size
                     << " bits  (" << size_queries.size() << " queries)..." << flush;

                double count_total_us = 0;
                double locate_total_us = 0;
                uint64_t count_total_results = 0;
                uint64_t locate_total_results = 0;

                for (const auto &q : size_queries) {
                    vector<uint8_t> pattern = extract_pattern(packed, q.offset, q.size);

                    double t0 = get_cpu_ms();
                    uint64_t cnt = query_count(fm_index, pattern, q.size);
                    count_total_us += (get_cpu_ms() - t0) * 1000.0;
                    count_total_results += cnt;

                    t0 = get_cpu_ms();
                    auto locs = query_locate(fm_index, pattern, q.size);
                    locate_total_us += (get_cpu_ms() - t0) * 1000.0;
                    locate_total_results += locs.size();
                }

                double n = size_queries.size();
                cerr << " count " << (long)(count_total_us / n) << " us"
                     << "  locate " << (long)(locate_total_us / n) << " us\n";

                count_out  << size_mb << "," << query_size << ","
                           << count_total_us / n << "," << count_total_results / n << "\n";
                locate_out << size_mb << "," << query_size << ","
                           << locate_total_us / n << "," << locate_total_results / n << "\n";
            }

            count_out.flush();
            locate_out.flush();
        }

    } catch (const exception &e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
