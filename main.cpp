#include <iostream>
#include <string>
#include <format>
#include <filesystem>
#include <stdexcept>
#include <chrono>

#include <bbtape/files.hpp>
#include <bbtape/sort.hpp>

int main(int argc, char ** argv)
{
  std::string src_path = "/Users/urlagushka/Documents/bbtape/tape.src.json";
  std::string dst_path = "/Users/urlagushka/Documents/bbtape/tape.dst.json";

  try
  {
    auto valid_src_path = bb::get_path_from_string(src_path);
    auto valid_dst_path = bb::get_path_from_string(dst_path);
    auto valid_config = bb::read_config_from_file(valid_src_path);

    bb::external_merge_sort(valid_config, valid_src_path, valid_dst_path);
  }
  catch (const std::format_error & error)
  {
    std::cerr << error.what() << "\n";
  }
  catch (const std::filesystem::filesystem_error & error)
  {
    std::cerr << error.what() << "\n";
    std::cerr << error.path1() << "\n";
    std::cerr << error.path2() << "\n";
    std::cerr << error.code() << "\n";
  }
  catch (const std::runtime_error & error)
  {
    std::cerr << error.what() << "\n";
  }

  return 0;
}
