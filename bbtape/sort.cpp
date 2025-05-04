#include <bbtape/sort.hpp>

#include <cstdint>
#include <memory>
#include <utility>
#include <future>
#include <thread>
#include <stop_token>
#include <chrono>
#include <semaphore>
#include <tuple>

#include <iostream>
#include <format>

#include <bbtape/utils.hpp>
#include <bbtape/physical.hpp>
#include <bbtape/sort_impl.hpp>

bb::sort::params
bb::sort::get_params(std::size_t tape_size, std::size_t ram_size, std::size_t conv_amount)
{
  std::size_t file_amount = tape_size / ram_size;
  if (tape_size % ram_size != 0)
  {
    ++file_amount;
  }
  if (file_amount % 2 != 0)
  {
    ++file_amount;
  }

  std::size_t sort_method = 0;
  std::size_t thread_amount = 0;
  std::size_t block_size = 0;
  if (conv_amount < 4)
  {
    sort_method = conv_amount;
    thread_amount = 1;
    block_size = ram_size;
  }
  else if (conv_amount == 4 || conv_amount == 5)
  {
    sort_method = 2;
    thread_amount = 2;
    block_size = ram_size / 2;
  }
  else
  {
    sort_method = 3;
    thread_amount = conv_amount / 3;
    block_size = (thread_amount > ram_size / 2) ? 2 : ram_size / thread_amount;
  }

  return {file_amount, sort_method, thread_amount, block_size};
}

void
bb::sort::external_merge(config ft_config, const fs::path & src, const fs::path & dst)
{
  auto src_tape = std::make_unique< tape_unit >(read_tape_from_file(src));
  auto dst_tape = std::make_unique< tape_unit >(tape_unit(src_tape->size()));

  const std::size_t ram_size = ft_config.phlimit.ram / sizeof(int32_t);
  auto ram = std::make_unique< std::vector< int32_t > >(ram_size);

  params pm = get_params(src_tape->size(), ram_size, ft_config.phlimit.conv);
  std::cout << "ram: " << ram_size << "\n";
  std::cout << "files: " << pm.file_amount << "\n";
  std::cout << "sort: " << pm.sort_method << "\n";
  std::cout << "block: " << pm.block_size << "\n";
  std::cout << "thread: " << pm.thread_amount << "\n";
  
  std::vector< tape_handler > ths;
  for (std::size_t i = 0; i < ft_config.phlimit.conv; ++i)
  {
    ths.emplace_back(ft_config);
  }

  auto files_tape_ram = split_src_file(std::move(src_tape), ths[0], pm.file_amount, std::move(ram));
  utils::tmp_file_handler tmp_files = std::move(std::get< 0 >(files_tape_ram));
  src_tape = std::move(std::get< 1 >(files_tape_ram));
  ram = std::move(std::get< 2 >(files_tape_ram));

  std::cout << "//////////////////////////// SORTING\n";

  std::size_t block_size = pm.block_size;
  while (tmp_files.size() > 1)
  {
    switch (pm.sort_method)
    {
      case 0:
      {
        throw std::runtime_error("bad sort method!");
        break;
      }

      case 1:
      {
        auto merge = strategy_1(tmp_files, ths, std::move(ram), block_size);
        tmp_files = std::move(std::get< 0 >(merge));
        ram = std::move(std::get< 1 >(merge));
        break;
      }

      case 2:
      {
        auto merge = strategy_2(tmp_files, ths, std::move(ram), block_size, pm.thread_amount);
        tmp_files = std::move(std::get< 0 >(merge));
        ram = std::move(std::get< 1 >(merge));
        break;
      }

      case 3:
      {
        auto merge = strategy_3(tmp_files, ths, std::move(ram), block_size, pm.thread_amount);
        tmp_files = std::move(std::get< 0 >(merge));
        ram = std::move(std::get< 1 >(merge));
        break;
      }
      
      default:
      {
        auto merge = strategy_3(tmp_files, ths, std::move(ram), block_size, pm.thread_amount);
        tmp_files = std::move(std::get< 0 >(merge));
        ram = std::move(std::get< 1 >(merge));
        block_size = block_size * 2;
        break;
      }
    }
  }

  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "sorting time: " << duration.count() << " ms\n";
}
