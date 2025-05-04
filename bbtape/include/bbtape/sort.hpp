#ifndef BBTAPE_SORT_HPP
#define BBTAPE_SORT_HPP

#include <filesystem>
#include <span>

#include <bbtape/tape.hpp>
#include <bbtape/files.hpp>
#include <bbtape/utils.hpp>

namespace bb::sort
{
  namespace fs = std::filesystem;

  struct params
  {
    std::size_t file_amount;
    std::size_t sort_method;
    std::size_t thread_amount;
    std::size_t block_size;
  };

  params
  get_params(std::size_t tape_size, std::size_t ram_size, std::size_t conv_amount);

  void
  external_merge(config ft_config, const fs::path & src, const fs::path & dst);
}

#endif
