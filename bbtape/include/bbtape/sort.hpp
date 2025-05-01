#ifndef BBTAPE_SORT_HPP
#define BBTAPE_SORT_HPP

#include <filesystem>

#include <bbtape/tape.hpp>
#include <bbtape/files.hpp>
#include <bbtape/details.hpp>

namespace bb
{
  std::filesystem::path
  external_merge_sort(config ft_config, const std::filesystem::path & src);
}

#endif
