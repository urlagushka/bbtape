#ifndef BBTAPE_UTILS_HPP
#define BBTAPE_UTILS_HPP

#include <filesystem>
#include <vector>
#include <string_view>

namespace bb::utils
{
  namespace fs = std::filesystem;

  fs::path
  get_path_from_string(std::string_view path);

  fs::path
  create_tmp_file();

  void
  remove_file(const fs::path & path);

  void
  verify_file_path(const std::filesystem::path & path);
}

#endif
