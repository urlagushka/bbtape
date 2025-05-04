#ifndef BBTAPE_SORT_IMPL_HPP
#define BBTAPE_SORT_IMPL_HPP

#include <vector>
#include <filesystem>
#include <utility>
#include <tuple>

#include <bbtape/tape.hpp>
#include <bbtape/physical.hpp>
#include <bbtape/utils.hpp>

namespace bb::sort
{
  namespace fs = std::filesystem;
  using c_file_vec = const std::vector< fs::path >;
  using u_vec = std::unique_ptr< std::vector< int32_t > >;
  using u_tape = std::unique_ptr< tape_unit >;
  using th_span = std::span< tape_handler >;

  std::pair< utils::tmp_file_handler, u_vec >
  strategy_1(utils::tmp_file_handler & src, th_span ths, u_vec ram, std::size_t block_size);

  std::pair< utils::tmp_file_handler, u_vec >
  strategy_2(utils::tmp_file_handler & src, th_span ths, u_vec ram, std::size_t block_size, std::size_t thread_amount);

  std::pair< utils::tmp_file_handler, u_vec >
  strategy_3(utils::tmp_file_handler & src, th_span ths, u_vec ram, std::size_t block_size, std::size_t thread_amount);

  std::tuple< bb::utils::tmp_file_handler, u_tape, u_vec >
  split_src_file(u_tape src, tape_handler & th, std::size_t file_amount, u_vec ram);

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
    tape_handler & th_1,
    tape_handler & th_2,
    tape_handler & th_3,
    const fs::path & lhs,
    const fs::path & rhs,
    std::span< int32_t > ram_link
  );
}

#endif
