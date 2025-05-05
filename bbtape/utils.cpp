#include <bbtape/utils.hpp>

#include <chrono>
#include <string>
#include <format>
#include <stdexcept>
#include <algorithm>
#include <fstream>

bb::utils::fs::path
bb::utils::create_tmp_file()
{
  fs::path tmp_path = fs::temp_directory_path();
  fs::path filename = std::format("bbtape_{}.json", std::chrono::system_clock::now());
  fs::path full_path = tmp_path / filename;

  std::ofstream tmp(full_path);
  if (!tmp.is_open())
  {
    throw std::runtime_error("can not create file!");
  }
  tmp.close();

  return full_path;
}

void
bb::utils::remove_file(const fs::path & path)
{
  if (!fs::exists(path))
  {
    throw std::runtime_error("remove_file: file does not exist!");
  }

  fs::remove(path);
}

bb::utils::fs::path
bb::utils::get_path_from_string(std::string_view path)
{
  std::filesystem::path valid_path = path;
  verify_file_path(valid_path);

  return valid_path;
}

void
bb::utils::verify_file_path(const fs::path & path)
{
  if (!fs::exists(path))
  {
    throw std::runtime_error("verify_file_path: file does not exist!");
  }
  if (path.extension() != ".json")
  {
    throw std::runtime_error("file extension is bad! (.json required)");
  }
}
