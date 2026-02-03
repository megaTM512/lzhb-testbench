
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
      "r,repeats", "Number of repeats (single accesses) for random access benchmark",
      cxxopts::value<int>()->default_value("100000"))(
      "s,consecutiverepeats", "Number of repeats (consecutive accesses) for random access benchmark",
      cxxopts::value<int>()->default_value("10000"))(
      "o,outputfile", "Output file",
      cxxopts::value<std::string>()->default_value(""))(
      "v,verbose", "Verbose output",
      cxxopts::value<bool>()->default_value("false"))(
      "b,batchsize", "Batch size for random access benchmark",
      cxxopts::value<int>()->default_value("1000"))(
        "c,comparefile", "Factorization to compare against",
        cxxopts::value<std::string>()->default_value("") )(
      "h,help", "Print usage");
  ;

  auto result = options.parse(argc, argv);
  if (result.count("help")) {
    std::cout << options.help() << std::endl;
    return 0;
  }
  auto batchSize = result["batchsize"].as<int>();

  int repeats = result["repeats"].as<int>();
  int consecutiverepeats = result["consecutiverepeats"].as<int>();
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
    double similarity = getSimilarityBetweenFactorizations(phrasesA, phrasesB);
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
  std::cout << "--- Height Analysis ---" << std::endl;
  HeightResults heightResults = heightAnalysis(phrases);
  std::cout << "--- Length Analysis ---" << std::endl;
  LengthResults lengthResults = lengthAnalysis(phrases);
  std::cout << "--- Random Access Decoding Benchmark ---" << std::endl;
  std::cout << "Starting random access test with " << repeats << " batches of "
            << batchSize << "." << std::endl;
  AccessResults randomAccessResults =
      randomAccessBenchmark(repeats, output, phrases, batchSize);
  std::cout << "--- Consecutive Random Access Decoding Benchmark ---"
            << std::endl;
  std::cout << "Starting consecutive random access test with " << consecutiverepeats
            << " runs of " << batchSize << " characters." << std::endl;
  ConsecutiveResults randomAccessConsecutiveResults =
      randomAccessConsecutiveBenchmark(consecutiverepeats,batchSize, output, phrases);

  uint64_t sizeInBytes = sizeof(PhraseC) * phrases.size() + sizeof(uint32_t) * phrases.size(); // PhraseC + predecessor table
  std::cout << "Factorization size in memory in bytes (including predecessor table): "
            << sizeInBytes << " bytes." << std::endl;

  std::ofstream csv("testbench_results.csv", std::ios::app);
  if (csv.tellp() == 0)
    csv << "TIMESTAMP,FILE_NAME,REPEATS,BATCH_SIZE,PHRASE_NUM,MAX_HEIGHT,AVG_HEIGHT,VAR_HEIGHT,MAX_LENGTH,AVG_LENGTH,VAR_LENGTH,TOTAL_ACCESS_TIME_NS,AVERAGE_ACCESS_BATCH_TIME_NS,AVERAGE_ACCESS_CHAR_TIME,TOTAL_ACCESS_CHARS,TOTAL_CONSEC_ACCESS_TIME_NS,AVERAGE_CONSEC_QUERY_TIME_NS,AVERAGE_CONSEC_CHAR_TIME_NS,TOTAL_CONSEC_CHARS,STRING_TOTAL_ACCESS_TIME_NS,STRING_AVERAGE_ACCESS_TIME_NS,AVERAGE_STRING_CHAR_TIME_NS,STRING_TOTAL_CONSEC_TIME_NS,STRING_AVERAGE_CONSEC_QUERY_TIME_NS,AVERAGE_STRING_CONSEC_CHAR_TIME_NS, BYTES\n";
  auto t = std::time(nullptr);
  csv << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S") << ","
      << inputFile << "," << repeats << "," << batchSize << ","
      << phrases.size() << "," << heightResults.maxHeight << ","
      << heightResults.avgHeight << "," << heightResults.varHeight << ","
      << lengthResults.maxLength << "," << lengthResults.avgLength << ","
      << lengthResults.varLength << "," << randomAccessResults.totalAccessTimeNs
      << "," << randomAccessResults.averageAccessBatchTimeNs << ","
      << randomAccessResults.averageAccessCharTimeNs << ","
      << randomAccessResults.totalChars << ","
      << randomAccessConsecutiveResults.totalTimeNs << ","
      << randomAccessConsecutiveResults.averageTimeNsPerQuery << ","
      << randomAccessConsecutiveResults.averageNsPerChar << ","
      << randomAccessConsecutiveResults.totalChars << ","
      << randomAccessResults.totalStringAccessTimeNs << ","
      << randomAccessResults.averageStringAccessTimeNs << ","
      << randomAccessResults.averageStringCharTimeNs << ","
      << randomAccessConsecutiveResults.totalStringTimeNs << ","
      << randomAccessConsecutiveResults.averageStringTimeNsPerQuery << ","
      << randomAccessConsecutiveResults.averageStringTimeNsPerChar << ","
      << sizeInBytes << "\n";
  csv.close();
  return 0;
}

AccessResults randomAccessBenchmark(const int repeats, std::string& output,
                                    std::vector<PhraseC>& phrases,
                                    int batchSize) {
  std::vector<std::chrono::duration<double, std::nano>> timings;
  timings.reserve(repeats);

  // Pre-generate random positions
  srand(42);
  std::vector<int> positions(repeats * batchSize);
  for (int i = 0; i < repeats * batchSize; i++) {
    positions[i] = (rand() % output.size()) + 1;
  }

  auto predecessortable = buildPredecessorTable(phrases);

  // Warming up cache
  for (int i = 0; i < std::min(1000, repeats * batchSize); i++) {
    getPositionFromPhrasesT(phrases, predecessortable, positions[i]);
  }

  // Benchmark
  for (int i = 0; i < repeats; i++) {
    volatile char c;
    auto start = std::chrono::high_resolution_clock::now();
    for (int k = 0; k < batchSize; k++)
      c = getPositionFromPhrasesT(phrases, predecessortable, positions[i * batchSize + k]);
    auto end = std::chrono::high_resolution_clock::now();
    (void)c;
    double dt = std::chrono::duration<double, std::nano>(end - start).count();
    dt /= batchSize;  // average over batchSize runs
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
  for (int i = 0; i < repeats; i++) {
    volatile char c;
    auto start = std::chrono::high_resolution_clock::now();
    for (int k = 0; k < batchSize; k++) c = output[positions[i * batchSize + k] - 1];
    auto end = std::chrono::high_resolution_clock::now();
    (void)c;
    double dt = std::chrono::duration<double, std::nano>(end - start).count();
    dt /= batchSize;  // average over batchSize runs
    stringTimings.push_back(std::chrono::duration<double, std::nano>(dt));
  }
  double totalStringTime = 0.0;
  for (auto t : stringTimings) {
    totalStringTime += t.count();
  }
  std::cout << "Total time for " << repeats
            << " batches (string access): " << totalStringTime << " nanoseconds"
            << std::endl;
  std::cout << "Average time per batch (string access): "
            << (totalStringTime / repeats) << " nanoseconds" << std::endl;
  return {totalTime, totalTime / repeats, totalTime / (repeats * batchSize),
          totalStringTime, totalStringTime / repeats, totalStringTime / (repeats * batchSize), static_cast<uint64_t>(repeats * batchSize)};
}

ConsecutiveResults randomAccessConsecutiveBenchmark(const int repeats, const int maxRunLength,
                                               std::string& output,
                                               std::vector<PhraseC>& phrases) {
  std::vector<std::chrono::duration<double, std::nano>> timings;
  timings.reserve(repeats);

  auto predecessortable = buildPredecessorTable(phrases);

  uint64_t totalChars = 0;
  
  srand(24);
  std::vector<int> positions(repeats);
  for (int i = 0; i < repeats; i++) {
    positions[i] = (rand() % output.size()) + 1;
  }

  // Warming up cache
  for (int i = 0; i < std::min(1000, repeats); i++) {
    for (size_t j = 0; j < std::min(static_cast<size_t>(maxRunLength), output.size() - positions[i]);
         j++) {
      getPositionFromPhrasesT(phrases, predecessortable, positions[i] + j);
    }
  }
  // Benchmark
  volatile char c;
  for (int i = 0; i < repeats; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    for (long unsigned int j = 0;
         j < std::min(static_cast<size_t>(maxRunLength), output.size() - positions[i]); j++) {
      c = getPositionFromPhrasesT(phrases, predecessortable, positions[i] + j);
    }
    auto end = std::chrono::high_resolution_clock::now();
    totalChars += std::min(static_cast<size_t>(maxRunLength), output.size() - positions[i]);
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

  std::vector<std::chrono::duration<double, std::nano>> stringTimings;
  stringTimings.reserve(repeats);
  // Compare to string access
  for (int i = 0; i < repeats; i++) {
    volatile char c;
    auto start = std::chrono::high_resolution_clock::now();
    for (long unsigned int j = 0;
         j < std::min(static_cast<size_t>(maxRunLength), output.size() - positions[i]); j++) {
      c = output[positions[i] + j - 1];
    }
    auto end = std::chrono::high_resolution_clock::now();
    (void)c;
    double dt = std::chrono::duration<double, std::nano>(end - start).count();
    dt /= repeats;
    stringTimings.push_back(std::chrono::duration<double, std::nano>(dt));
  }

  double totalStringTime = 0.0;
  for (auto t : stringTimings) {
    totalStringTime += t.count();
  }
  std::cout << "Total time for " << repeats
            << " queries (string access): " << totalStringTime << " nanoseconds"
            << std::endl;
  std::cout << "Average time per query (string access): "
            << (totalStringTime / repeats) << " nanoseconds" << std::endl;

  return {totalTime, totalTime / repeats,
          totalTime / static_cast<double>(totalChars), totalChars, totalStringTime, totalStringTime / repeats, totalStringTime / static_cast<double>(totalChars)};
}

HeightResults heightAnalysis(const std::vector<PhraseC>& phrases) {
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
      if (p.pos + j >= phrase_start) {
        heights.push_back(heights[p.pos + j]);  // Self-Reference
      } else
        heights.push_back(heights[p.pos + j] + 1);
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
  return {maxHeight, avgHeight, variance};
}

LengthResults lengthAnalysis(const std::vector<PhraseC>& phrases) {
  std::vector<int> lengths;
  lengths.reserve(phrases.size());

  for (const auto& p : phrases) {
    lengths.push_back(p.len);
  }

  long double totalLength = 0.0;
  int maxLength = 0;
  for (auto l : lengths) {
    totalLength += l;
    if (l > maxLength) {
      maxLength = l;
    }
  }

  double avgLength = totalLength / lengths.size();
  std::cout << "Average phrase length: " << avgLength << std::endl;
  std::cout << "Maximum phrase length: " << maxLength << std::endl;
  std::cout << "Variance of phrase lengths: ";
  double variance = 0.0;
  for (auto l : lengths) {
    variance += (l - avgLength) * (l - avgLength);
  }
  variance /= lengths.size();
  std::cout << variance << std::endl;
  return {maxLength, avgLength, variance};
}

double getSimilarityBetweenFactorizations(
    const std::vector<PhraseC>& phrasesA,
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