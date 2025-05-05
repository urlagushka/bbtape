#include <iostream>
#include <string>
#include <format>
#include <filesystem>
#include <stdexcept>

#include <bbtape/sort.hpp>

#include <memory>

int main(int argc, char ** argv)
{
  if (argc != 3)
  {
    std::cout << "input args is bad!\n";
    return 1;
  }

  std::string src_path = argv[1];
  std::string dst_path = argv[2];

  try
  {
    auto valid_src_path = bb::utils::get_path_from_string(src_path);
    auto valid_dst_path = bb::utils::get_path_from_string(dst_path);
    auto valid_config = bb::read_config_from_file(valid_src_path);

    bb::external_merge_sort< int32_t >(valid_config, valid_src_path, valid_dst_path);
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
