#include <iostream>
#include <string>

#include <bbtape/files.hpp>

int main(int argc, char ** argv)
{
  std::string path = "/Users/urlagushka/Documents/bbtape/tape.template.json";
  auto valid_path = bb::get_config_path_with_verify(path);
  bb::config tape_config = bb::parse_config_file(valid_path);
  return 0;
}
