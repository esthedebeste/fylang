#pragma once
#include "utils.cpp"
class CharReader {
  char buf[4096];
  char *p;
  size_t n;
  bool ended;

public:
  char *file_path;
  FILE *file;
  CharReader(FILE *file, char *file_path) : file(file), file_path(file_path) {
    p = buf;
    n = 0;
  }
  char next_char() {
    if (ended) {
      if (!QUIET)
        fputs("[EOF]", stderr);
      return EOF;
    }
    if (n == 0) {
      n = fread(buf, 1, sizeof(buf), file);
      p = buf;
    }
    char ret = n-- > 0 ? *p++ : EOF;
    if (ret == EOF) {
      ended = true;
      return ' ';
    } else if (!QUIET)
      fputc(ret, stderr);
    return ret;
  }
};

char **visited_paths = new char *[1];
size_t visited_paths_len;
CharReader **queue = new CharReader *[1];
size_t queue_len = 0;
static int next_char() {
  char ret = EOF;
  while (ret == EOF && queue_len > 0) {
    ret = queue[queue_len - 1]->next_char();
    if (ret == EOF)
      queue_len--;
  }
  return ret;
}

char *join(char *base, char *rel) {
  char *buf = alloc_c(strlen(base) + strlen(rel) + 2 /* path sep and NUL */);
  strcpy(buf, base);
  strcat(buf, "/");
  strcat(buf, rel);
  return buf;
}
char *get_relative_path(char *base_path, char *relative_path) {
  return join(dirname(strdup(base_path)), relative_path);
}

char *add_fy_ext(char *str) {
  char *buf = alloc_c(strlen(str) + 4 /* .fy and NUL */);
  strcpy(buf, str);
  strcat(buf, ".fy");
  return buf;
}

CharReader *get_file(char *base_path, char *relative_path) {
  for (unsigned int i = 0; i < 3; i++) {
    char *abs;
    if (i == 0)
      abs = get_relative_path(base_path, relative_path);
    else if (i == 1)
      abs = join(get_relative_path(base_path, (char *)"../lib"), relative_path);
    else if (i == 2)
      abs = join(std_dir, relative_path);
    FILE *file = fopen(abs, "r");
    if (file)
      return new CharReader(file, abs);
    abs = add_fy_ext(abs);
    file = fopen(abs, "r");
    if (file)
      return new CharReader(file, abs);
  }
  error("File \'%s\' can't be resolved", relative_path);
}

static void add_file_to_queue(char *base_path, char *relative_path) {
  static size_t queue_allocated = 1;
  if (queue_len >= queue_allocated) {
    queue_allocated *= 2;
    queue = realloc_arr<CharReader *>(queue, queue_allocated);
  }
  CharReader *file = get_file(base_path, relative_path);
  char *path = realpath(file->file_path, nullptr);
  for (size_t i = 0; i < visited_paths_len; i++)
    if (streq(visited_paths[i], path))
      return; // similar to C #pragma once
  static size_t visited_allocated = 1;
  if (visited_paths_len >= visited_allocated) {
    visited_allocated *= 2;
    visited_paths = realloc_arr<char *>(visited_paths, visited_allocated);
  }
  visited_paths[visited_paths_len++] = path;
  queue[queue_len++] = file;
}