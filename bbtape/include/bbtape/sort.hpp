#ifndef BBTAPE_SORT_HPP
#define BBTAPE_SORT_HPP

#include <filesystem>
#include <span>

#include <bbtape/tape.hpp>
#include <bbtape/files.hpp>
#include <bbtape/utils.hpp>

namespace bb
{
  namespace fs = std::filesystem;

  struct sort_params
  {
    std::size_t file_amount;
    std::size_t sort_method;
    std::size_t thread_amount;
    std::size_t block_size;
  };

  sort_params
  get_sort_params(std::size_t tape_size, std::size_t ram_size, std::size_t conv_amount);

  void
  external_merge_sort(config ft_config, const fs::path & src, const fs::path & dst);

  fs::path
  sort_handler_1(
    tape_handler & th_1,
    const fs::path & lhs,
    const fs::path & rhs,
    std::span< int32_t > ram_link
  );

  fs::path
  sort_handler_2(
    tape_handler & th_1,
    tape_handler & th_2,
    const fs::path & lhs,
    const fs::path & rhs,
    std::span< int32_t > ram_link
  );

  fs::path
  sort_handler_3(
    std::span< tape_handler > th_link,
    const fs::path & lhs,
    const fs::path & rhs,
    std::span< int32_t > ram_link
  );
}

#endif
