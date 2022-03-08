#pragma once
#include "utils.cpp"
class CharReader {
#define BUF_SIZE 4096
  char buf[BUF_SIZE];
  char *p;
  unsigned int n;

public:
  char *file_path;
  FILE *file;
  CharReader(FILE *file, char *file_path) : file(file), file_path(file_path) {
    p = buf;
    n = 0;
  }
  char next_char() {
    if (n == 0) {
      n = fread(buf, 1, sizeof(buf), file);
      p = buf;
    }
    char ret = n-- > 0 ? *p++ : EOF;
    if (ret == EOF)
      fputs("[EOF]", stderr);
    else
      fputc(ret, stderr);
    return ret;
  }
};

CharReader **queue;
unsigned int queue_len = 0;
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
  return join(dirname(clone_str(base_path)), relative_path);
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
  fprintf(stderr, "File \'%s\' can't be resolved", relative_path);
  exit(1);
}

static void add_file_to_queue(char *base_path, char *relative_path) {
  static unsigned int queue_allocated = 1;
  if (queue_len == queue_allocated)
    queue_allocated *= 2;
  queue = realloc_arr<CharReader *>(queue, queue_allocated);
  queue[queue_len] = get_file(base_path, relative_path);
  queue_len++;
}