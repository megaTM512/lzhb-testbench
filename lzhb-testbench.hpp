#ifndef LZHB_TESTBENCH_HPP
#define LZHB_TESTBENCH_HPP

#include <string>
#include <vector>
#include "lzf.hpp"

void randomAccessBenchmark(const int repeats, std::string& output,
                           std::vector<PhraseC>& phrases);

#endif  // LZHB_TESTBENCH_HPP