#ifndef LZHB_TESTBENCH_HPP
#define LZHB_TESTBENCH_HPP

#include <string>
#include <vector>
#include "lzf.hpp"

void randomAccessBenchmark(const int repeats, std::string& output,
                           std::vector<PhraseC>& phrases, int batchSize);
void heightAnalysis(const std::vector<PhraseC>& phrases);
void randomAccessConsecutiveBenchmark(const int repeats, std::string& output,
                                    std::vector<PhraseC>& phrases);
#endif  // LZHB_TESTBENCH_HPP