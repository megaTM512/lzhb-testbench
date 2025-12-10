
#include "lzhb-testbench.hpp"

#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

#include "cxxopts.hpp"
#include "lzf.hpp"
#include "lzhb-decode.hpp"

int main(int argc, char* argv[]) {
  // This currently only works with lzcp files,
  // create using ./lzhb3 -a -z -s -f "./banana.txt" -o "banana"

  cxxopts::Options options("LZHB-Testbench",
                           "Testbench for LZHB factorizations.");
  options.add_options()("i,inputfile", "Input file",
                        cxxopts::value<std::string>())(
      "r,repeats", "Number of repeats",
      cxxopts::value<int>()->default_value("1000"))(
      "o,outputfile", "Output file",
      cxxopts::value<std::string>()->default_value(""))(
      "v,verbose", "Verbose output",
      cxxopts::value<bool>()->default_value("false"))(
      "b,batchsize", "Batch size for random access benchmark",
      cxxopts::value<int>()->default_value("1000"))(
        "c,comparefile", "Factorization to compare against",
        cxxopts::value<std::string>()->default_value("") );

  auto result = options.parse(argc, argv);
  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }
  auto batchSize = result["batchsize"].as<int>();

  int repeats = result["repeats"].as<int>();
  std::string inputFile;
  try {
    inputFile = result["inputfile"].as<std::string>();
  } catch (const cxxopts::exceptions::option_has_no_value& e) {
    std::cerr << "Error: Input file is required." << std::endl;
    std::cerr << "Use -i or --inputfile to specify the input file."
              << std::endl;
    return 1;
  }
  std::string compareFile = result["comparefile"].as<std::string>();
  if (!compareFile.empty()) {
    auto phrasesA = decodeToPhraseC(inputFile);
    auto phrasesB = decodeToPhraseC(compareFile);
    double similarity =
        getSimilarityBetweenFactorizations(phrasesA, phrasesB);
    if (similarity >= 0.0) {
      std::cout << "Similarity between factorizations: " << similarity
                << std::endl;
    }
    return 0;
  }

  std::string outputFile = result["outputfile"].as<std::string>();
  bool verbose = result["verbose"].as<bool>();

  auto phrases = decodeToPhraseC(inputFile);

  if (verbose) {
    for (auto phrase : phrases) {
      printPhrase(phrase);
    }
  }

  std::cout << "Input file: " << inputFile << std::endl;
  std::cout << "Number of phrases: " << phrases.size() << std::endl;
  std::string output = decodePhrasesToString(phrases);
  if (verbose) std::cout << output << std::endl;
  std::cout << "Decoding successful" << std::endl;
  std::cout << "--- Random Access Decoding Benchmark ---" << std::endl;
  std::cout << "Starting random access test with " << repeats << " queries."
            << std::endl;
  randomAccessBenchmark(repeats, output, phrases, batchSize);

  std::cout << "--- Consecutive Random Access Decoding Benchmark ---"
            << std::endl;
  std::cout << "Starting consecutive random access test with " << repeats
            << " batches of" << batchSize << "." << std::endl;
  std::cout
      << "The range of consecutive accesses is from 1 up to 20 characters."
      << std::endl;
  randomAccessConsecutiveBenchmark(repeats, output, phrases);
  std::cout << "--- Height Analysis ---" << std::endl;
  heightAnalysis(phrases);
  return 0;
}

void randomAccessBenchmark(const int repeats, std::string& output,
                           std::vector<PhraseC>& phrases, int batchSize) {
  std::vector<std::chrono::duration<double, std::nano>> timings;
  timings.reserve(repeats);

  // Pre-generate random positions
  srand(time(0));
  std::vector<int> positions(repeats);
  for (int i = 0; i < repeats; i++) {
    positions[i] = (rand() % output.size()) + 1;
  }

  auto predecessortable = buildPredecessorTable(phrases);

  // Warming up cache
  for (int i = 0; i < std::min(1000, repeats); i++) {
    getPositionFromPhrasesT(phrases, predecessortable, positions[i]);
  }

  // Benchmark
  for (int i = 0; i < repeats; i++) {
    volatile char c;
    auto start = std::chrono::high_resolution_clock::now();
    for(int k = 0; k < batchSize; k++) c = getPositionFromPhrasesT(phrases, predecessortable, positions[i]);
    auto end = std::chrono::high_resolution_clock::now();
    (void)c;
    double dt = std::chrono::duration<double, std::nano>(end - start).count();
    dt /= batchSize; // average over batchSize runs
    timings.push_back(std::chrono::duration<double, std::nano>(dt));
  }
  double totalTime = 0.0;
  for (auto t : timings) {
    totalTime += t.count();
  }
  std::cout << "Total time for " << repeats << " batches: " << totalTime
            << " nanoseconds" << std::endl;
  std::cout << "Average time per batch: " << (totalTime / repeats)
            << " nanoseconds" << std::endl;

    std::vector<std::chrono::duration<double, std::nano>> stringTimings;
    stringTimings.reserve(repeats);
    // Compare to string access
    for(int i = 0; i < repeats; i++) {
        volatile char c;
        auto start = std::chrono::high_resolution_clock::now();
        for(int k = 0; k < batchSize; k++) c = output[positions[i]-1];
        auto end = std::chrono::high_resolution_clock::now();
        (void)c;
        double dt = std::chrono::duration<double, std::nano>(end - start).count();
        dt /= batchSize; // average over batchSize runs
        stringTimings.push_back(std::chrono::duration<double, std::nano>(dt));
    }
    double totalStringTime = 0.0;
    for (auto t : stringTimings) {
        totalStringTime += t.count();
    }
    std::cout << "Total time for " << repeats << " batches (string access): " << totalStringTime
              << " nanoseconds" << std::endl;
    std::cout << "Average time per batch (string access): " << (totalStringTime / repeats)
              << " nanoseconds" << std::endl;
}

void randomAccessConsecutiveBenchmark(const int repeats, std::string& output,
                                      std::vector<PhraseC>& phrases) {
  std::vector<std::chrono::duration<double, std::nano>> timings;
  timings.reserve(repeats);

  auto predecessortable = buildPredecessorTable(phrases);

  srand(time(0));
  std::vector<int> positions(repeats);
  for (int i = 0; i < repeats; i++) {
    positions[i] = (rand() % output.size()) + 1;
  }
  srand(42);
  std::vector<ulong> lengths(repeats);
  for (int i = 0; i < repeats; i++) {
    lengths[i] = (rand() % 20) + 1;
  }

  // Warming up cache
  for (int i = 0; i < std::min(1000, repeats); i++) {
    for (size_t j = 0; j < std::min(lengths[i], output.size() - positions[i]);
         j++) {
      getPositionFromPhrasesT(phrases, predecessortable, positions[i] + j);
    }
  }
  // Benchmark
  volatile char c;
  for (int i = 0; i < repeats; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    for (long unsigned int j = 0; j < std::min(lengths[i], output.size() - positions[i]);
         j++) {
      c =
          getPositionFromPhrasesT(phrases, predecessortable, positions[i] + j);
        }
        auto end = std::chrono::high_resolution_clock::now();
    (void)c;
    double dt = std::chrono::duration<double, std::nano>(end - start).count();
    dt /= repeats;
    timings.push_back(std::chrono::duration<double, std::nano>(dt));
  }
  double totalTime = 0.0;
  for (auto t : timings) {
    totalTime += t.count();
  }
  std::cout << "Total time for " << repeats << " queries: " << totalTime
            << " nanoseconds" << std::endl;
  std::cout << "Average time per query: " << (totalTime / repeats)
            << " nanoseconds" << std::endl;
}

void heightAnalysis(const std::vector<PhraseC>& phrases) {
  std::vector<int> heights;

  for (uint32_t i = 0; i < phrases.size(); i++) {
    const auto& p = phrases[i];

    // Literal
    if (p.len == 1) {
      heights.push_back(0);
      continue;
    }

    auto phrase_start = heights.size();
    for (uint32_t j = 0; j < p.len - 1; j++) {
      if(p.pos + j >= phrase_start) {
        heights.push_back(heights[p.pos + j]); // Self-Reference
      }
      else heights.push_back(heights[p.pos + j] + 1);
    }

    // appended literal
    heights.push_back(0);
  }

  assert(decodePhrasesToString(phrases).size() == heights.size() &&
         "Height analysis size mismatch");

  long double totalHeight = 0.0;
  int maxHeight = 0;
  for (auto h : heights) {
    totalHeight += h;
    if (h > maxHeight) {
      maxHeight = h;
    }
  }

  double avgHeight = totalHeight / heights.size();
  std::cout << "Average height: " << avgHeight << std::endl;
  std::cout << "Maximum height: " << maxHeight << std::endl;
  std::cout << "Variance of heights: ";
  double variance = 0.0;
  for (auto h : heights) {
    variance += (h - avgHeight) * (h - avgHeight);
  }
  variance /= heights.size();
  std::cout << variance << std::endl;
}

double getSimilarityBetweenFactorizations(const std::vector<PhraseC>& phrasesA,
                             const std::vector<PhraseC>& phrasesB) {
  if (phrasesA.size() != phrasesB.size()) {
    std::cout << "Factorizations have different number of phrases: "
              << phrasesA.size() << " vs " << phrasesB.size() << std::endl;
    return -1.0;
  }
  int common = 0;
  for (size_t i = 0; i < phrasesA.size(); i++) {
    if (phrasesA[i].len == phrasesB[i].len &&
        phrasesA[i].pos == phrasesB[i].pos &&
        phrasesA[i].nextChar == phrasesB[i].nextChar) {
      common++;
    }
  }
  return static_cast<double>(common) / phrasesA.size();
}