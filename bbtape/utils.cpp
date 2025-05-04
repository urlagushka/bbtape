#include <bbtape/utils.hpp>

#include <chrono>
#include <string>
#include <format>
#include <stdexcept>
#include <algorithm>
#include <fstream>

bb::utils::tmp_file_handler &
bb::utils::tmp_file_handler::operator=(tmp_file_handler && rhs)
{
  if (std::addressof(rhs) == this)
  {
    return * this;
  }

  for (auto & file : __files)
  {
    // remove_file(file);
  }

  __files = rhs.__files;
  rhs.__files = {};

  return * this;
}

bb::utils::tmp_file_handler::~tmp_file_handler()
{
  for (auto & file : __files)
  {
    // std::filesystem::remove(file);
  }
}

bb::utils::fs::path
bb::utils::tmp_file_handler::create_file()
{
  std::filesystem::path tmp_path = std::filesystem::temp_directory_path();
  std::filesystem::path filename = std::format("bbtape_{}.json", std::chrono::system_clock::now());
  std::filesystem::path full_path = tmp_path / filename;

  std::ofstream tmp(full_path);
  if (!tmp.is_open())
  {
    throw std::runtime_error("can not create file!");
  }
  tmp.close();

  __files.push_back(full_path);
  return full_path;
}

void
bb::utils::tmp_file_handler::delete_file(const fs::path & path)
{
  if (std::find(__files.begin(), __files.end(), path) == __files.end())
  {
    throw std::runtime_error("file is not owned by this handler!");
  }
  if (!fs::exists(path))
  {
    throw std::runtime_error("file does not exist!");
  }

  fs::remove(path);
  auto to_del = std::remove(__files.begin(), __files.end(), path);
  __files.erase(to_del, __files.end());
}

void
bb::utils::tmp_file_handler::push_back(const fs::path & path)
{
  __files.push_back(path);
}

bb::utils::fs::path &
bb::utils::tmp_file_handler::operator[](std::size_t i)
{
  return __files[i];
}

std::size_t
bb::utils::tmp_file_handler::size() const
{
  return __files.size();
}

bb::utils::fs::path
bb::utils::create_tmp_file()
{
  std::filesystem::path tmp_path = std::filesystem::temp_directory_path();
  std::filesystem::path filename = std::format("bbtape_{}.json", std::chrono::system_clock::now());
  std::filesystem::path full_path = tmp_path / filename;

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
    throw std::runtime_error("file does not exist!");
  }

  fs::remove(path);
}
