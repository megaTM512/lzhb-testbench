#ifndef LZHB_DECODE_HPP
#define LZHB_DECODE_HPP

#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstdint>
#include <string>

#include "lzf.hpp"

uint8_t getBit(const uint8_t& block, uint8_t pos);
void decodeBlock(std::vector<uint8_t>& bin_blocks,
                 std::vector<uint32_t>& val_blocks, uint32_t bitsize,
                 uint32_t tail);
std::vector<PhraseC> decodeToPhraseC(const std::string& filename,
                                     bool position = true);
std::string decodePhrasesToString(const std::vector<PhraseC>& phrases,
                                  bool position = false);
uint32_t binSearchPredecessor(const std::vector<PhraseC>& phrases,
                              uint32_t position);
char getPositionFromPhrases(
    const std::vector<PhraseC>& phrases, uint32_t position,
    int* height = nullptr);
void printPhrase(const PhraseC& phrase);

#endif  // LZHB_DECODE_HPP