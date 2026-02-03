#ifndef LZHB_TESTBENCH_HPP
#define LZHB_TESTBENCH_HPP

#include <string>
#include <vector>
#include "lzf.hpp"

struct HeightResults {
    uint64_t maxHeight;
    double avgHeight;
    double varHeight;
};

struct LengthResults {
    uint64_t maxLength;
    double avgLength;
    double varLength;
};

struct AccessResults {
    double totalAccessTimeNs;
    double averageAccessBatchTimeNs;
    double averageAccessCharTimeNs;
    double totalStringAccessTimeNs;
    double averageStringAccessTimeNs;
    double averageStringCharTimeNs;
    uint64_t totalChars;
};

struct ConsecutiveResults {
    double totalTimeNs;
    double averageTimeNsPerQuery;
    double averageNsPerChar;
    uint64_t totalChars;
    double totalStringTimeNs;
    double averageStringTimeNsPerQuery;
    double averageStringTimeNsPerChar;
};


double getSimilarityBetweenFactorizations(const std::vector<PhraseC>& phrasesA,
                             const std::vector<PhraseC>& phrasesB);
HeightResults heightAnalysis(const std::vector<PhraseC>& phrases);
LengthResults lengthAnalysis(const std::vector<PhraseC>& phrases);
AccessResults randomAccessBenchmark(const int repeats, std::string& output,
                                    std::vector<PhraseC>& phrases, int batchSize);
ConsecutiveResults randomAccessConsecutiveBenchmark(const int repeats, const int maxRunLength, std::string& output,
                                    std::vector<PhraseC>& phrases);
#endif  // LZHB_TESTBENCH_HPP