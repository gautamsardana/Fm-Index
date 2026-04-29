#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
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
  if (!f)
    throw runtime_error("Cannot open file: " + filepath);

  vector<uint8_t> packed;

  if (max_mb > 0) {
    uint64_t max_bytes = max_mb * 1024 * 1024;
    packed.resize(max_bytes);
    f.read(reinterpret_cast<char *>(packed.data()), max_bytes);
    uint64_t bytes_read = f.gcount();
    packed.resize(bytes_read);
  } else {
    packed.assign(istreambuf_iterator<char>(f), {});
  }

  uint64_t n_bits = packed.size() * 8;

  return {packed, n_bits};
}

string encode_binary_to_alphabet(const vector<uint8_t> &packed,
                                 uint64_t n_bits) {
  string encoded;
  encoded.reserve(n_bits);

  for (uint64_t i = 0; i < n_bits; i++) {
    uint8_t bit = (packed[i / 8] >> (7 - (i % 8))) & 1;
    encoded.push_back(bit ? 'B' : 'A');
  }

  return encoded;
}

string extract_pattern(const vector<uint8_t> &packed, uint64_t offset,
                       uint64_t size) {
  string pattern;
  pattern.reserve(size);

  for (uint64_t i = 0; i < size; i++) {
    uint64_t bit_pos = offset + i;
    uint8_t bit = (packed[bit_pos / 8] >> (7 - (bit_pos % 8))) & 1;
    pattern.push_back(bit ? 'B' : 'A');
  }

  return pattern;
}

long get_rss_kb() {
  struct rusage r;
  getrusage(RUSAGE_SELF, &r);
#ifdef __APPLE__
  return r.ru_maxrss / 1024;
#else
  return r.ru_maxrss;
#endif
}

struct QuerySpec {
  uint64_t offset;
  uint64_t size;
};

vector<QuerySpec> read_query_file(const string &filepath) {
  ifstream f(filepath);
  if (!f)
    throw runtime_error("Cannot open query file: " + filepath);

  vector<QuerySpec> queries;
  string line;
  while (getline(f, line)) {
    stringstream ss(line);
    uint64_t offset, size;
    char comma;
    if (ss >> offset >> comma >> size && comma == ',') {
      queries.push_back({offset, size});
    }
  }
  return queries;
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    cerr << "Usage: " << argv[0]
         << " <query_file> <build_out> <count_out> <locate_out>\n";
    return 1;
  }

  string query_file = argv[1];
  string build_output = argv[2];
  string count_output = argv[3];
  string locate_output = argv[4];
  string input_file = "experiments/datasets/english_50MB";

  vector<uint64_t> input_sizes = {1, 5, 10}; // MB

  try {
    ofstream build_out(build_output);
    ofstream count_out(count_output);
    ofstream locate_out(locate_output);

    build_out << "input_size_mb,build_time_ms,peak_memory_kb\n";
    count_out << "input_size_mb,query_size,avg_time_us,avg_memory_kb\n";
    locate_out << "input_size_mb,query_size,avg_time_us,avg_memory_kb\n";

    for (uint64_t size_mb : input_sizes) {
      auto [packed, n_bits] = read_binary_input(input_file, size_mb);

      string encoded_text = encode_binary_to_alphabet(packed, n_bits);

      long mem_before = get_rss_kb();
      auto build_start = chrono::high_resolution_clock::now();

      csa_wt<> fm_index;
      construct_im(fm_index, encoded_text, 1);

      auto build_end = chrono::high_resolution_clock::now();
      long mem_after = get_rss_kb();

      auto build_time_ms =
          chrono::duration_cast<chrono::milliseconds>(build_end - build_start)
              .count();

      build_out << size_mb << "," << build_time_ms << "," << mem_after << "\n";
      build_out.flush();

      vector<QuerySpec> queries = read_query_file(query_file);

      vector<QuerySpec> valid_queries;
      for (const auto &q : queries) {
        if (q.offset + q.size <= n_bits) {
          valid_queries.push_back(q);
        }
      }

      map<uint64_t, vector<QuerySpec>> queries_by_size;
      for (const auto &q : valid_queries) {
        queries_by_size[q.size].push_back(q);
      }

      for (const auto &[query_size, size_queries] : queries_by_size) {
        double count_total_time_us = 0;
        long count_total_mem_kb = 0;

        double locate_total_time_us = 0;
        long locate_total_mem_kb = 0;

        for (const auto &q : size_queries) {
          string pattern = extract_pattern(packed, q.offset, q.size);

          long mem_before = get_rss_kb();
          auto query_start = chrono::high_resolution_clock::now();

          size_t count_result = count(fm_index, pattern);

          auto query_end = chrono::high_resolution_clock::now();
          long mem_after = get_rss_kb();

          auto query_time_us = chrono::duration_cast<chrono::microseconds>(
                                   query_end - query_start)
                                   .count();
          long mem_delta = mem_after - mem_before;

          count_total_time_us += query_time_us;
          count_total_mem_kb += mem_delta;

          mem_before = get_rss_kb();
          query_start = chrono::high_resolution_clock::now();

          auto locations = locate(fm_index, pattern);

          query_end = chrono::high_resolution_clock::now();
          mem_after = get_rss_kb();

          query_time_us = chrono::duration_cast<chrono::microseconds>(
                              query_end - query_start)
                              .count();
          mem_delta = mem_after - mem_before;

          locate_total_time_us += query_time_us;
          locate_total_mem_kb += mem_delta;

          (void)count_result;
          (void)locations;
        }

        double count_avg_time_us = count_total_time_us / size_queries.size();
        double count_avg_mem_kb =
            static_cast<double>(count_total_mem_kb) / size_queries.size();

        double locate_avg_time_us = locate_total_time_us / size_queries.size();
        double locate_avg_mem_kb =
            static_cast<double>(locate_total_mem_kb) / size_queries.size();

        count_out << size_mb << "," << query_size << "," << count_avg_time_us
                  << "," << count_avg_mem_kb << "\n";
        locate_out << size_mb << "," << query_size << "," << locate_avg_time_us
                   << "," << locate_avg_mem_kb << "\n";
      }

      count_out.flush();
      locate_out.flush();
    }

    build_out.close();
    count_out.close();
    locate_out.close();

  } catch (const exception &e) {
    cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
