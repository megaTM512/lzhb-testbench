#include <iostream>
#include <chrono>
#include <vector>
#include <fstream>

#include "lzf.hpp"

uint8_t getBit(const uint8_t& block, uint8_t pos) {
  return (block >> pos) & 1;
}

void decodeBlock(std::vector<uint8_t>& bin_blocks,
                            std::vector<uint32_t>& val_blocks, uint32_t bitsize,
                            uint32_t tail) {
  uint32_t val = 0;
  uint32_t buff = 0;
  for (size_t i = 0; i < bin_blocks.size(); i++) {
    if (i != bin_blocks.size() - 1) {
      for (size_t j = 8; j > 0; j--) {
        val |=
            (getBit(bin_blocks[i], j - 1) << (bitsize - buff - 1));
        buff++;
        if (buff == bitsize) {
          val_blocks.push_back(val);
          buff = 0;
          val = 0;
        }
      }
    } else {
      for (size_t j = 8; j > 8 - tail; j--) {
        val |=
            (getBit(bin_blocks[i], j - 1) << (bitsize - buff - 1));
        buff++;
        if (buff == bitsize) {
          val_blocks.push_back(val);
          buff = 0;
          val = 0;
        }
      }
    }
  }
}

std::vector<PhraseC> decodeToPhraseC(const std::string& filename, bool position = true){
  std::vector<PhraseC> output;

  std::ifstream fs(filename, std::ifstream::binary);

  uint32_t phrase_size = 1;
  uint32_t bitsize_lp = 1;
  uint32_t bitsize_src = 1;

  fs.read((char*)&phrase_size, sizeof(uint32_t));
  fs.read((char*)&bitsize_lp, sizeof(uint32_t));
  fs.read((char*)&bitsize_src, sizeof(uint32_t));

  size_t tail_lp = 1 + ((phrase_size * bitsize_lp - 1) % 8);
  size_t tail_src = 1 + ((phrase_size * bitsize_src - 1) % 8);
  size_t size_lp = 1 + (phrase_size * bitsize_lp - 1) / 8;
  size_t size_src = 1 + (phrase_size * bitsize_src - 1) / 8;

  std::vector<uint8_t> bin_lp(size_lp);
  std::vector<uint8_t> bin_src(size_src);
  std::vector<char> phrase_c(phrase_size);
  fs.read((char*)&bin_lp[0], bin_lp.size());
  fs.read((char*)&bin_src[0], bin_src.size());
  fs.read((char*)&phrase_c[0], phrase_c.size());
  fs.close();

  std::vector<uint32_t> phrase_lp;
  std::vector<uint32_t> phrase_src;
  decodeBlock(bin_lp, phrase_lp, bitsize_lp, tail_lp);
  decodeBlock(bin_src, phrase_src, bitsize_src, tail_src);

  if(position){
    for (size_t i = 0; i < phrase_lp.size(); i++) {
      if (i == 0) {
        output.push_back(
            PhraseC{.len = 1, .pos = phrase_src[i], .nextChar = phrase_c[i]});
      } else {
        if (phrase_lp[i] - phrase_lp[i - 1] == 1) {
          output.push_back(
              PhraseC{.len = 1, .pos = phrase_src[i], .nextChar = phrase_c[i]});
        } else {
          output.push_back(PhraseC{.len = phrase_lp[i] - phrase_lp[i - 1],
                                         .pos = phrase_src[i],
                                         .nextChar = phrase_c[i]});
        }
      }
    }
  } else {
    for (size_t i = 0; i < phrase_lp.size(); i++) {
      output.push_back(PhraseC{
          .len = phrase_lp[i], .pos = phrase_src[i], .nextChar = phrase_c[i]});
    }
  }
  return output;
}

std::string decodePhrasesToString(const std::vector<PhraseC>& phrases, bool position = false){
  std::string output;
  for(const auto& phrase : phrases){
    int cuPos = output.length();
    for(int i = 0; i < phrase.len - 1 - cuPos; i++){
      output.push_back(output[phrase.pos + i]);
    }
    output.push_back(phrase.nextChar);
  }
  return output;
}

uint32_t binSearchPredecessor(const std::vector<PhraseC>& phrases, uint32_t position) {
    if (phrases.empty()) return UINT32_MAX;

    uint32_t low = 0;
    uint32_t high = phrases.size() - 1;
    uint32_t result = UINT32_MAX;

    while (low <= high) {
        uint32_t mid = low + ((high - low) / 2);

        if (phrases[mid].len <= position) {
          
          result = mid; // this is a candidate predecessor
          low = mid + 1; // try to find a larger one still < position
        } else {
            high = mid - 1;
        }
    }
    return result;
}


char getPositionFromPhrases(const std::vector<PhraseC>& phrases, uint32_t position){ // Works when Position = false on generation. Else we need predecessor table

}

void printPhrase(const PhraseC& phrase){
  std::cout << "(" << phrase.len << "," << phrase.pos << "," << phrase.nextChar << ")" << std::endl;
}