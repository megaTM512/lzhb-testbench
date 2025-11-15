#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

#include "cxxopts.hpp"
#include "lzf.hpp"
#include "lzhb-decode.hpp"

int main(int argc, char* argv[]) {
  // IMPORTANT position = false.
  // TODO Otherwise we need predecessor table
  // This currently only works with lzcp files,
  // create using ./lzhb3 -a -z -s -f "./banana.txt" -o "banana"

  cxxopts::Options options("LZHB-Testbench",
                           "Testbench for LZHB factorizations.");
  options.add_options()(
      "i,inputfile", "Input file",
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
  try
  {
    inputFile = result["inputfile"].as<std::string>();
  }
  catch(const cxxopts::exceptions::option_has_no_value& e)
  {
    std::cerr << "Error: Input file is required." << std::endl;
    std::cerr << "Use -i or --inputfile to specify the input file." << std::endl;
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

  std::string output = decodePhrasesToString(phrases);
  if(verbose) std::cout << output << std::endl;

  std::vector<std::chrono::duration<double, std::micro>> timings;
  std::cout << "Decoding successful" << std::endl;
  std::cout << "Starting random access test with " << repeats
            << " queries." << std::endl;
  srand(time(0));
  auto predecessortable = buildPredecessorTable(phrases);
  for (int i = 0; i < repeats; i++) {
    long long int randomPos = (rand() % output.size()) + 1;
    auto start = std::chrono::high_resolution_clock::now();
    char c = getPositionFromPhrasesT(phrases, predecessortable, randomPos);
    auto end = std::chrono::high_resolution_clock::now();
    // std::cout << "Character at position " << randomPos << ": " << c <<
    // std::endl; std::cout << "Expected character: " << output[randomPos - 1]
    // << std::endl;
    assert(output[randomPos - 1] == c &&
           "character mismatch between output and decoded phrases");
    std::chrono::duration<double, std::micro> elapsed = end - start;
    timings.push_back(elapsed);
  }
  double totalTime = 0.0;
  for (auto t : timings) {
    totalTime += t.count();
  }
  std::cout << "Total time for " << repeats << " queries: " << totalTime
            << " microseconds" << std::endl;
  std::cout << "Average time per query: " << (totalTime / repeats)
            << " microseconds" << std::endl;
  return 0;
}