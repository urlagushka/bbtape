#include <iostream>
#include <string>

#include <bbtape/files.hpp>
#include <bbtape/sort.hpp>

int main(int argc, char ** argv)
{
  std::string path = "/Users/urlagushka/Documents/bbtape/tape.template.json";

  try
  {
    auto valid_src_path = bb::get_config_path_with_verify(path);
    auto only_config = bb::read_config_from_file(valid_src_path);

    auto valid_dst_path = bb::external_merge_sort(only_config, valid_src_path);
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
