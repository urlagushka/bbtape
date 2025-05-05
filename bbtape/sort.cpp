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


