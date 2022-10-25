#include "reader.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
class CharReader {
  char buf[4096];
  char *p = buf;
  size_t n = 0;
  bool ended = false;

public:
  fs::path file_path;
  std::ifstream file;
  CharReader(fs::path file_path)
      : file_path(file_path), file(std::ifstream(file_path)) {}
  void close() { file.close(); }
  char next_char() {
    if (n == 0) {
      if (file.eof()) {
        if (DEBUG)
          std::cerr << "[EOF]";
        return EOF;
      }
      n = file.read(buf, sizeof(buf)).gcount();
      p = buf;
    }
    char ret = n-- > 0 ? *p++ : EOF;
    if (DEBUG)
      std::cerr << ret;
    return ret;
  }
};

std::vector<fs::path> visited_paths;
std::vector<CharReader *> queue;
int next_char() {
  if (queue.size() == 0)
    return EOF;
  char ret = queue.back()->next_char();
  if (ret == EOF) {
    queue.back()->close();
    queue.pop_back();
    return ' ';
  }
  return ret;
}

fs::path get_fy_file(fs::path base_path, std::string relative_path) {
  for (unsigned int i = 0; i < 3; i++) {
    fs::path path;
    if (i == 0)
      path = base_path.parent_path() / relative_path;
    else if (i == 1)
      path = fs::path("./lib") / relative_path;
    else if (i == 2)
      path = std_dir / relative_path;
    if (fs::exists(path))
      return path;
    path += ".fy";
    if (fs::exists(path))
      return path;
  }
  error("File '" + relative_path + "' can't be resolved");
}

void add_file_to_queue(fs::path path) {
  path = fs::canonical(path);
  for (auto &visited_path : visited_paths)
    if (visited_path == path)
      return; // only include a file once
  debug_log("Including file '" + fs::relative(path).string() + "'");
  visited_paths.push_back(path);
  queue.push_back(new CharReader(path));
}

void add_file_to_queue(fs::path base_path, std::string relative_path) {
  add_file_to_queue(get_fy_file(base_path, relative_path));
}
void add_file_to_queue(std::string relative_path) {
  add_file_to_queue(queue.back()->file_path, relative_path);
}
