#ifndef BBTAPE_FILES_HPP
#define BBTAPE_FILES_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
#include <utility>
#include <filesystem>
#include <string_view>

namespace bb
{
  struct delay_part
  {
    std::size_t on_read;
    std::size_t on_write;
    std::size_t on_roll;
    std::size_t on_offset;
  };

  struct phlimit_part
  {
    std::size_t ram;
  };

  using tape_unit = std::vector< int32_t >;

  struct config
  {
    delay_part delay;
    phlimit_part phlimit;
  };

  std::filesystem::path 
  get_config_path_with_verify(std::string_view path);

  config
  read_config_from_file(const std::filesystem::path & path);

  tape_unit
  read_tape_from_file(const std::filesystem::path & path);

  void
  dump_tape(std::ofstream & out, const tape_unit & rhs);
}

#endif
