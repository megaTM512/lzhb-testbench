#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

#include "lzf.hpp"
#include "lzhb-decode.hpp"

int main() {
  // IMPORTANT position = false.
  // TODO Otherwise we need predecessor table
  // This currently only works with lzcp files, 
  // create using ./lzhb3 -a -z -s -f "./banana.txt" -o "banana"
  auto phrases = decodeToPhraseC("rama.lzcp", false);

  for (auto phrase : phrases) {
    printPhrase(phrase);
  }
  std::string output = decodePhrasesToString(phrases, true);
  std::cout << output << std::endl;

  int testPosition = 2; // One-based index
  int height = 0;
  char c = getPositionFromPhrases(phrases, testPosition, &height);
  assert(testPosition >= 0 &&
         static_cast<size_t>(testPosition) < output.size() &&
         "testPosition out of range");
  assert(output[testPosition - 1] == c &&
         "character mismatch between output and decoded phrases");
  std::cout << "Character at position " << testPosition << ": " << c
            << " | Found in height " << height << std::endl;
  return 0;
}