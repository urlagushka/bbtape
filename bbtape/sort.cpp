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
#include <bbtape/sort_impl.hpp>

bb::sort_params
bb::get_sort_params(std::size_t tape_size, std::size_t ram_size, std::size_t conv_amount)
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

  std::size_t thread_amount = std::min(conv_amount, file_amount / 2);
  std::size_t block_size = ram_size / thread_amount;

  return {file_amount, thread_amount, block_size};
}


