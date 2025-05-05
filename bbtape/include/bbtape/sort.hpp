#ifndef BBTAPE_SORT_HPP
#define BBTAPE_SORT_HPP

#include <filesystem>

#include <bbtape/config.hpp>
#include <bbtape/unit.hpp>
#include <bbtape/tape_handler.hpp>
#include <bbtape/sort_impl.hpp>

namespace bb
{
  namespace fs = std::filesystem;

  struct sort_params
  {
    std::size_t file_amount;
    std::size_t thread_amount;
    std::size_t block_size;
  };

  sort_params
  get_sort_params(std::size_t tape_size, std::size_t ram_size, std::size_t conv_amount);

  template< unit_type T >
  void
  external_merge_sort(config ft_config, const fs::path & src, const fs::path & dst);
}

template< bb::unit_type T >
void
bb::external_merge_sort(config ft_config, const fs::path & src, const fs::path & dst)
{
  auto src_tape = std::make_unique< unit< T > >(read_tape_from_file< T >(src));
  auto dst_tape = std::make_unique< unit< T > >(unit< T >(src_tape->size()));

  const std::size_t ram_size = ft_config.phlimit.ram / sizeof(T);
  auto ram = std::make_unique< std::vector< T > >(ram_size);

  sort_params pm = get_sort_params(src_tape->size(), ram_size, ft_config.phlimit.conv);
  if (pm.thread_amount == 0)
  {
    throw std::runtime_error("conv amount is zero!");
  }
  
  std::vector< tape_handler< T > > ths;
  for (std::size_t i = 0; i < ft_config.phlimit.conv; ++i)
  {
    ths.emplace_back(ft_config);
  }

  auto files_tape_ram = split_src_unit< T >(std::move(src_tape), ths[0], pm.file_amount, std::move(ram));
  file_handler tmp_files = std::move(std::get< 0 >(files_tape_ram));
  src_tape = std::move(std::get< 1 >(files_tape_ram));
  ram = std::move(std::get< 2 >(files_tape_ram));

  std::size_t block_size = pm.block_size;
  std::size_t thread_amount = pm.thread_amount;
  while (tmp_files.size() > 1)
  {
    auto merge = strategy< T >(tmp_files, ths, std::move(ram), block_size, thread_amount);
    tmp_files = std::move(std::get< 0 >(merge));
    ram = std::move(std::get< 1 >(merge));

    thread_amount = std::min(thread_amount, tmp_files.size() / 2);
    block_size = ram_size / thread_amount;
  }

  auto tape = read_tape_from_file< T >(tmp_files[0]);
  write_tape_to_file(dst, tape);
}

#endif
