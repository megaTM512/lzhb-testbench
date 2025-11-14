#include <iostream>
#include <chrono>
#include <vector>
#include <fstream>

#include "lzf.hpp"
#include "lzhb-decode.cpp"

int main(){
  auto phrases = decodeToPhraseC("rama.lzcp", false);
  for(auto phrase : phrases){
    printPhrase(phrase);
  }
  std::cout << decodePhrasesToString(phrases, true) << std::endl;
  printPhrase(phrases[binSearchPredecessor(phrases, 10)]);
  std::cout << getPositionFromPhrases(phrases, 10) << std::endl;
  return 0;
}