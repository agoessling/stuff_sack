#include <cstdio>
#include <fstream>
#include <iostream>

#include <benchmark/benchmark.h>

static void BM_ReadLogC(benchmark::State& state, const std::string& log) {
  FILE *file = fopen(log.c_str(), "r");
  if (!file) {
    state.SkipWithError("Could not open file.");;
    return;
  }

  std::vector<uint8_t> buf(state.range(0));

  for (auto _ : state) {
    fseek(file, 0, SEEK_SET);
    while (true) {
      const size_t num_bytes = fread(buf.data(), 1, buf.size(), file);
      if (num_bytes != buf.size()) break;
    }

    if (ferror(file)) {
      state.SkipWithError("Read error.");
      break;
    }
  }

  fclose(file);
}
BENCHMARK_CAPTURE(BM_ReadLogC, Log, "/home/agoessling/Desktop/test_log.ss")->Range(32, 8128);

void BM_ReadLogCpp(benchmark::State& state, const std::string& log) {
  std::ifstream file(log, std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    state.SkipWithError("Could not open file.");
    return;
  }

  std::vector<uint8_t> buf(state.range(0));

  for (auto _ : state) {
    file.clear();
    file.seekg(0);
    while (true) {
      file.read(reinterpret_cast<char *>(buf.data()), buf.size());
      if (static_cast<size_t>(file.gcount()) != buf.size()) break;
    }

    if (file.bad()) {
      state.SkipWithError("Read error.");
      break;
    }
  }
}
BENCHMARK_CAPTURE(BM_ReadLogCpp, Log, "/home/agoessling/Desktop/test_log.ss")->Range(32, 8128);

BENCHMARK_MAIN();
