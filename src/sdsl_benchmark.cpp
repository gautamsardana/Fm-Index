#include <fstream>
#include <iostream>
#include <map>
#include <mach/mach.h>
#include <sdsl/suffix_arrays.hpp>
#include <sstream>
#include <string>
#include <sys/resource.h>
#include <vector>

using namespace sdsl;
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

// encode bits as 'A'/'B' for SDSL (avoids null bytes which confuse construct_im)
string encode_to_alphabet(const vector<uint8_t> &packed, uint64_t n_bits) {
    string s;
    s.reserve(n_bits);
    for (uint64_t i = 0; i < n_bits; i++)
        s.push_back(((packed[i / 8] >> (7 - (i % 8))) & 1) ? 'B' : 'A');
    return s;
}

string extract_pattern(const vector<uint8_t> &packed, uint64_t offset, uint64_t size) {
    string pattern;
    pattern.reserve(size);
    for (uint64_t i = 0; i < size; i++) {
        uint8_t bit = (packed[(offset + i) / 8] >> (7 - ((offset + i) % 8))) & 1;
        pattern.push_back(bit ? 'B' : 'A');
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

int main(int argc, char *argv[]) {
    if (argc != 6) {
        cerr << "Usage: " << argv[0]
             << " <input_file> <query_file> <build_out> <count_out> <locate_out>\n";
        return 1;
    }

    string input_file   = argv[1];
    string query_file   = argv[2];
    string build_output = argv[3];
    string count_output = argv[4];
    string locate_output = argv[5];

    vector<uint64_t> input_sizes_mb = {1, 2, 5, 10, 20};

    try {
        ofstream build_out(build_output);
        ofstream count_out(count_output);
        ofstream locate_out(locate_output);

        build_out << "input_size_mb,build_time_ms,peak_memory_kb,index_size_kb\n";
        count_out << "input_size_mb,query_size_bits,avg_time_us,avg_result_count\n";
        locate_out << "input_size_mb,query_size_bits,avg_time_us,avg_result_count\n";

        for (uint64_t size_mb : input_sizes_mb) {
            cerr << "[sdsl] building index for " << size_mb << " MB..." << flush;
            auto [packed, n_bits] = read_binary_input(input_file, size_mb);
            string encoded = encode_to_alphabet(packed, n_bits);

            long mem_before = get_phys_footprint_kb();
            double cpu_before = get_cpu_ms();

            csa_wt<> fm_index;
            construct_im(fm_index, encoded, 1);

            double build_time_ms = get_cpu_ms() - cpu_before;
            long peak_mem_kb = get_phys_footprint_kb();
            long index_kb = (long)(size_in_bytes(fm_index) / 1024);

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
                cerr << "[sdsl] " << size_mb << " MB  query_size=" << query_size
                     << " bits  (" << size_queries.size() << " queries)..." << flush;

                double count_total_us = 0;
                double locate_total_us = 0;
                uint64_t count_total_results = 0;
                uint64_t locate_total_results = 0;

                for (const auto &q : size_queries) {
                    string pattern = extract_pattern(packed, q.offset, q.size);

                    double t0 = get_cpu_ms();
                    size_t cnt = sdsl::count(fm_index, pattern);
                    count_total_us += (get_cpu_ms() - t0) * 1000.0;
                    count_total_results += cnt;

                    t0 = get_cpu_ms();
                    auto locs = sdsl::locate(fm_index, pattern);
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
