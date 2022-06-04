#include "reader.h"
#include "utils.h"

CharReader::CharReader(std::string file_path)
    : file_path(file_path), file(file_path) {}
CharReader::~CharReader() { file.close(); }
char CharReader::next_char() {
  if (ended) {
    if (DEBUG)
      fputs("[EOF]", stderr);
    return EOF;
  }
  if (n == 0) {
    file.read(buf, sizeof(buf));
    n = file.gcount();
    p = buf;
  }
  char ret = n-- > 0 ? *p++ : EOF;
  if (ret == EOF) {
    ended = true;
    return ' ';
  } else if (DEBUG)
    fputc(ret, stderr);
  return ret;
}

std::vector<std::string> visited_paths;
std::vector<CharReader *> queue;
int next_char() {
  char ret = EOF;
  while (ret == EOF && queue.size() > 0) {
    ret = queue.back()->next_char();
    if (ret == EOF)
      queue.pop_back();
  }
  return ret;
}

std::string dirname(std::string path) {
  size_t last_slash = path.find_last_of('/');
  if (last_slash == std::string::npos)
    return ".";
  return path.substr(0, last_slash);
}

CharReader *get_file(std::string base_path, std::string relative_path) {
  for (unsigned int i = 0; i < 3; i++) {
    std::string abs;
    if (i == 0)
      abs = dirname(base_path) + '/' + relative_path;
    else if (i == 1)
      abs = dirname(base_path) + "/../lib/" + relative_path;
    else if (i == 2)
      abs = std_dir + '/' + relative_path;
    if (std::filesystem::exists(abs))
      return new CharReader(abs);
    abs += ".fy";
    if (std::filesystem::exists(abs))
      return new CharReader(abs);
  }
  error("File '" + relative_path + "' can't be resolved");
}

void add_file_to_queue(std::string base_path, std::string relative_path) {
  CharReader *file = get_file(base_path, relative_path);
  std::string path = std::filesystem::canonical(file->file_path).string();
  for (auto &visited_path : visited_paths)
    if (visited_path == path)
      return; // only include a file once
  debug_log("Including file '" + path + "'");
  visited_paths.push_back(path);
  queue.push_back(file);
}