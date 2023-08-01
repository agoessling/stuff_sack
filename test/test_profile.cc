#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/log_reader.h"

using namespace ss;

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

int MapLookup(const std::vector<uint32_t>& keys, const std::unordered_map<uint32_t, TypeBox>& map,
              int iterations) {
  for (int i = 0; i < iterations; ++i) {
    const uint32_t key = keys[rand() % keys.size()];
    auto lookup = map.find(key);
    if (lookup == map.end()) continue;
    const TypeBox& type = lookup->second;
    (void)type;
  }

  return 0;
}

int VectorLookup(const std::vector<uint32_t>& keys, const std::vector<uint32_t>& key_lookup,
                 const std::vector<TypeBox>& type_lookup, int iterations) {
  for (int i = 0; i < iterations; ++i) {
    const uint32_t key = keys[rand() % keys.size()];
    auto lookup = std::lower_bound(key_lookup.begin(), key_lookup.end(), key);
    if (lookup == key_lookup.end() || *lookup != key) continue;
    const TypeBox& type = type_lookup[lookup - key_lookup.begin()];
    (void)type;
  }

  return 0;
}

int LoadAll(const std::string& log) {
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

  std::vector<uint32_t> keys;
  std::unordered_map<uint32_t, TypeBox> map;
  std::vector<uint32_t> key_lookup;
  std::vector<TypeBox> type_lookup;
  for (int i = 0; i < 10; ++i) {
    uint32_t key = rand();
    keys.push_back(key);
    if (i % 2) {
      TypeBox type = Primitive<uint16_t>();

      map.insert({key, type});

      key_lookup.push_back(key);
      type_lookup.push_back(type);
    }
  }
  std::sort(key_lookup.begin(), key_lookup.end());

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
      case 'F':
        ret = MapLookup(keys, map, 2e7);
        break;
      case 'G':
        ret = VectorLookup(keys, key_lookup, type_lookup, 2e7);
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
