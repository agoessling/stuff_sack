#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "src/log_reader.h"

int CReadLog(const std::string& log, size_t chunk_size) {
  FILE *file = fopen(log.c_str(), "r");
  if (!file) return -1;

  std::vector<char> buf(chunk_size);

  int total_bytes = 0;
  while (true) {
    size_t num_bytes = fread(buf.data(), 1, chunk_size, file);
    total_bytes += num_bytes;
    if (num_bytes != chunk_size) break;
  }

  std::cout << "Bytes Read: " << static_cast<double>(total_bytes) << std::endl;

  int ret = 0;
  if (ferror(file)) ret = -1;

  fclose(file);
  return ret;
}

int CppReadLog(const std::string& log, size_t chunk_size) {
  std::ifstream file(log, std::ios::in | std::ios::binary);
  if (!file.is_open()) return -1;

  std::vector<char> buf(chunk_size);

  int total_bytes = 0;
  while (true) {
    file.read(buf.data(), chunk_size);
    size_t num_bytes = file.gcount();
    total_bytes += num_bytes;
    if (num_bytes != chunk_size) break;
  }

  std::cout << "Bytes Read: " << static_cast<double>(total_bytes) << std::endl;

  if (file.bad()) return -1;

  return 0;
}

int ExtractToVector(const std::string& log, size_t chunk_size) {
  std::ifstream file(log, std::ios::in | std::ios::binary);
  if (!file.is_open()) return -1;

  std::vector<char> buf(chunk_size);

  std::vector<uint8_t> data;
  int total_bytes = 0;
  while (true) {
    file.read(buf.data(), buf.size());

    size_t num_bytes = file.gcount();
    total_bytes += num_bytes;

    data.push_back(buf[2]);

    if (num_bytes != buf.size()) break;
  }

  std::cout << "Bytes Read: " << static_cast<double>(total_bytes) << std::endl;
  std::cout << "Bytes Pushed: " << static_cast<double>(data.size()) << std::endl;

  if (file.bad()) return -1;

  return 0;
}

int ExtractToVectorBuffered(const std::string& log, size_t chunk_size) {
  std::ifstream file(log, std::ios::in | std::ios::binary);
  if (!file.is_open()) return -1;

  constexpr size_t kBufMultiple = 10;

  std::vector<char> buf(kBufMultiple * chunk_size);

  std::vector<uint8_t> data;
  int total_bytes = 0;
  while (true) {
    file.read(buf.data(), buf.size());

    size_t num_bytes = file.gcount();
    total_bytes += num_bytes;

    for (size_t i = 0; i < kBufMultiple; ++i) {
      data.push_back(buf[i * chunk_size + 2]);
    }

    if (num_bytes != buf.size()) break;
  }

  std::cout << "Bytes Read: " << static_cast<double>(total_bytes) << std::endl;
  std::cout << "Bytes Pushed: " << static_cast<double>(data.size()) << std::endl;

  if (file.bad()) return -1;

  return 0;
}

int LoadAll(const std::string& log) {
  using namespace ss;

  LogReader reader(log);
  std::unordered_map<std::string, TypeBox> types = reader.LoadAll();
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Test argument required." << std::endl;
    return -1;
  }

  const std::string log = "/home/agoessling/Desktop/profile_log.ss";

  while (const char c = *argv[1]++) {
    std::cout << c << ":" << std::endl;

    using namespace std::chrono;
    time_point start_time = system_clock::now();

    constexpr size_t kChunkSize = 5;

    int ret;
    switch (c) {
      case 'A':
        ret = CReadLog(log, kChunkSize);
        break;
      case 'B':
        ret = CppReadLog(log, kChunkSize);
        break;
      case 'C':
        ret = ExtractToVector(log, kChunkSize);
        break;
      case 'D':
        ret = ExtractToVectorBuffered(log, kChunkSize);
        break;
      case 'E':
        ret = LoadAll(log);
        break;
      default:
        std::cerr << "Unrecognized argument" << std::endl;
        return -1;
    }

    if (ret) {
      std::cerr << "ERROR" << std::endl;
      return -1;
    }

    time_point end_time = system_clock::now();

    duration<double> run_time = end_time - start_time;
    std::cout << "Run Time: " << run_time.count() << std::endl;
  }

  return 0;
}
