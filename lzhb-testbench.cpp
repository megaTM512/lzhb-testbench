#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

#include "lzf.hpp"
#include "lzhb-decode.hpp"
#include "cxxopts.hpp"

int main(int argc, char* argv[]) {
  // IMPORTANT position = false.
  // TODO Otherwise we need predecessor table
  // This currently only works with lzcp files, 
  // create using ./lzhb3 -a -z -s -f "./banana.txt" -o "banana"

  cxxopts::Options options("LZHB-Testbench", "Testbench for LZHB factorizations.");
  options.add_options()
    ("i,inputfile", "Input file", cxxopts::value<std::string>()->default_value("input.lzcp"))
    ("r,repeats", "Number of repeats", cxxopts::value<int>()->default_value("100"))
    ("o,outputfile", "Output file", cxxopts::value<std::string>()->default_value(""));

  auto result = options.parse(argc, argv);

  int repeats = result["repeats"].as<int>();
  std::string inputFile = result["inputfile"].as<std::string>();
  std::string outputFile = result["outputfile"].as<std::string>();

  auto phrases = decodeToPhraseC(inputFile, true);

  for (auto phrase : phrases) {
    printPhrase(phrase);
  }

  std::string output = decodePhrasesToString(phrases, true);
  std::cout << output << std::endl;

  std::vector<std::chrono::duration<double, std::micro>> timings;

  for (size_t i = 0; i < repeats; i++)
  {
    int randomPos = (rand() % output.size()) + 1;
    auto start = std::chrono::high_resolution_clock::now();
    char c = getPositionFromPhrases(phrases, randomPos);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Character at position " << randomPos << ": " << c << std::endl;
    std::cout << "Expected character: " << output[randomPos - 1] << std::endl;
    assert(output[randomPos-1] == c &&
           "character mismatch between output and decoded phrases");
    std::chrono::duration<double, std::micro> elapsed = end - start;
    timings.push_back(elapsed);
  }
  double totalTime = 0.0;
  for (auto t : timings) {
    totalTime += t.count();
  }
  std::cout << "Average time per query: " << (totalTime / repeats) << " microseconds" << std::endl;
  return 0;
}