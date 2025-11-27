
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
      cxxopts::value<bool>()->default_value("false"));

  auto result = options.parse(argc, argv);

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
            randomAccessBenchmark(repeats, output, phrases);

  std::cout << "--- Consecutive Random Access Decoding Benchmark ---" << std::endl;
  std::cout << "Starting consecutive random access test with " << repeats << " queries."
            << std::endl;
  std::cout << "The range of consecutive accesses is from 1 up to 20 characters." << std::endl;
  randomAccessConsecutiveBenchmark(repeats, output, phrases);
  std::cout << "--- Height Analysis ---" << std::endl;
  heightAnalysis(phrases);
  return 0;
}

void randomAccessBenchmark(const int repeats, std::string& output,
                           std::vector<PhraseC>& phrases) {
  std::vector<std::chrono::duration<double, std::micro>> timings;
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
    auto start = std::chrono::high_resolution_clock::now();
    char c = getPositionFromPhrasesT(phrases, predecessortable, positions[i]);
    auto end = std::chrono::high_resolution_clock::now();

    assert(output[positions[i] - 1] == c &&
           "character mismatch between output and decoded phrases");
    timings.push_back(end - start);
  }
  double totalTime = 0.0;
  for (auto t : timings) {
    totalTime += t.count();
  }
  std::cout << "Total time for " << repeats << " queries: " << totalTime
            << " microseconds" << std::endl;
  std::cout << "Average time per query: " << (totalTime / repeats)
            << " microseconds" << std::endl;
}

void randomAccessConsecutiveBenchmark(const int repeats, std::string& output,
                                    std::vector<PhraseC>& phrases) {
  std::vector<std::chrono::duration<double, std::micro>> timings;
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
    for (int j = 0; j < std::min(lengths[i], output.size() - positions[i]); j++) {
      getPositionFromPhrasesT(phrases, predecessortable, positions[i] + j);
    }
  }
  // Benchmark
  char sink;
  for (int i = 0; i < repeats; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int j = 0; j < std::min(lengths[i], output.size() - positions[i]); j++) {
      char c = getPositionFromPhrasesT(phrases, predecessortable, positions[i] + j);
      sink ^= c;
    }
    auto end = std::chrono::high_resolution_clock::now();

    timings.push_back(end - start);
  }
  double totalTime = 0.0;
  for (auto t : timings) {
    totalTime += t.count();
  }
  std::cout << "Total time for " << repeats << " queries: " << totalTime
            << " microseconds" << std::endl;
  std::cout << "Average time per query: " << (totalTime / repeats)
            << " microseconds" << std::endl;
}


void heightAnalysis(const std::vector<PhraseC>& phrases) {
  std::vector<int> heights;
  heights.reserve(phrases.size());

  for (uint32_t i = 0; i < phrases.size(); i++) {
    if( phrases[i].len == 1 ) {
      heights.push_back(0);
      continue;
    }
    for(uint32_t j = 0; j < phrases[i].len - 1; j++) {
      if(phrases[i].pos + j >= i) { // Self reference doesn't increase height
        heights.push_back(heights[phrases[i].pos + j]);
      }
      else heights.push_back(heights[phrases[i].pos + j] + 1);
    }
    heights.push_back(0); // for the appended char
  }

  double totalHeight = 0.0;
  int maxHeight = 0;
  for (auto h : heights) {
    totalHeight += h;
    if (h > maxHeight) {
      maxHeight = h;
    }
  }
  for(auto h : heights) {
    std::cout << h << std::endl;
  }
  double avgHeight = totalHeight / heights.size();
  std::cout << "Average height: " << avgHeight
            << std::endl;
  std::cout << "Maximum height: " << maxHeight << std::endl;
  std::cout << "Variance of heights: ";
  double variance = 0.0;
  for (auto h : heights) {
    variance += (h - avgHeight) * (h - avgHeight);
  }
  variance /= heights.size();
  std::cout << variance << std::endl;
}