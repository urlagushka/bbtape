#include <bbtape/details.hpp>

#include <chrono>
#include <string>
#include <format>
#include <stdexcept>
#include <algorithm>
#include <iostream>

bb::details::tmp_file_handler::~tmp_file_handler()
{
  for (auto & file : __files)
  {
    std::filesystem::remove(file);
  }
}

std::filesystem::path
bb::details::tmp_file_handler::create_file()
{
  std::filesystem::path tmp_path = std::filesystem::temp_directory_path();
  std::cout << tmp_path << "\n";
  std::filesystem::path filename = std::format("bbtape_{}.json", std::chrono::system_clock::now());
  __files.push_back(filename);

  return filename;
}

void
bb::details::tmp_file_handler::delete_file(const std::filesystem::path & file)
{
  if (std::find(__files.begin(), __files.end(), file) == __files.end());
  {
    throw std::runtime_error("file is not owned by this handler!");
  }
  if (!std::filesystem::exists(file))
  {
    throw std::runtime_error("file does not exist!");
  }

  std::filesystem::remove(file);
  auto to_del = std::remove(__files.begin(), __files.end(), file);
  __files.erase(to_del, __files.end());
}
