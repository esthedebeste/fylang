#pragma once
#include "consts.h"
#include <fstream>
#include <string>
#include <vector>

class CharReader {
  char buf[4096];
  char *p = buf;
  size_t n = 0;
  bool ended = false;

public:
  std::string file_path;
  std::ifstream file;
  CharReader(std::string file_path);
  ~CharReader();
  char next_char();
};

extern std::vector<std::string> visited_paths;
extern std::vector<CharReader *> queue;
int next_char();

CharReader *get_file(std::string base_path, std::string relative_path);

void add_file_to_queue(std::string base_path, std::string relative_path);