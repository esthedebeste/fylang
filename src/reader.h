#pragma once
#include "consts.h"
#include <filesystem>

int next_char();
std::filesystem::path get_fy_file(std::filesystem::path base_path,
                                  std::string relative_path);
void add_file_to_queue(std::filesystem::path relative_path);
void add_file_to_queue(std::filesystem::path base_path,
                       std::string relative_path);
void add_file_to_queue(std::string relative_path);