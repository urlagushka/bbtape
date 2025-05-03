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
  namespace fs = std::filesystem;

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
    std::size_t conv;
  };

  using tape_unit = std::vector< int32_t >;

  struct config
  {
    delay_part delay;
    phlimit_part phlimit;
  };

  fs::path
  get_path_from_string(std::string_view path);

  config
  read_config_from_file(const fs::path & path);

  tape_unit
  read_tape_from_file(const fs::path & path);

  void
  write_tape_to_file(const fs::path & path, const tape_unit & rhs);
}

#endif
