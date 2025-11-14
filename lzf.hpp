#ifndef LZFACTORIZATION
#define LZFACTORIZATION

#include <vector>

struct Phrase{
  size_t len;
  size_t pos; // char when len <= 1
};

struct PhraseC{ //
  size_t endPos;
  size_t pos;
  char nextChar;
};

class lzf{
  public:
    std::vector<Phrase> phrases;
    std::vector<int> predecessors;
};

#endif