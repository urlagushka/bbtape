#ifndef BBTAPE_FILES_HPP
#define BBTAPE_FILES_HPP

#include <cstddef>
#include <list>
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

  using tape = std::list< long long int >;

  struct config
  {
    delay_part delay;
    phlimit_part phlimit;
    tape src;
  };

  std::filesystem::path get_config_path_with_verify(std::string_view path);
  config parse_config_file(const std::filesystem::path & path);
}

#endif
