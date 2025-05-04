#include <bbtape/physical.hpp>

#include <algorithm>
#include <stdexcept>
#include <ranges>
#include <iostream>

bb::ram_manager::ram_manager(std::unique_ptr< std::vector< int32_t > > ram, std::size_t block_size):
  __ram(std::move(ram)),
  __blocks(__ram->size() / block_size),
  __block_size(block_size)
{
  for (auto& flag : __blocks)
  {
    flag.clear();
  }
  std::cout << "\n----------------------------\n";
  std::cout << "RAM SIZE " << __ram->size() << "\n";
  std::cout << "BLOCKS SIZE " << __blocks.size() << "\n";
}

std::span< int32_t >
bb::ram_manager::take_ram_block()
{
  auto tmp = std::find_if(__blocks.begin(), __blocks.end(), [](const std::atomic_flag & flag)
  {
    return !flag.test();
  });
  if (tmp == __blocks.end())
  {
    throw std::runtime_error("no available ram!");
  }
  tmp->test_and_set();

  std::size_t offset = std::distance(__blocks.begin(), tmp);
  auto to_ret = std::span< int32_t >(*__ram | std::views::drop(offset) | std::views::take(__block_size));
  std::cout << "TAKE\n";
  std::cout << to_ret.data() << std::endl;
  return to_ret;
}

void
bb::ram_manager::free_ram_block(std::span< int32_t > block)
{
  std::cout << "FREE\n";
  std::cout << block.data() << std::endl;

  if (block.data() - __ram->data() < 0)
  {
    throw std::runtime_error("block is not owned by ram!");
  }

  __blocks[(block.data() - __ram->data())].clear();
}

std::unique_ptr< std::vector< int32_t > >
bb::ram_manager::pick_ram()
{
  __blocks.clear();
  __block_size = 0;
  return std::move(__ram);
}

std::size_t
bb::balance_ram_block(std::size_t ram_size, std::size_t lhs_size, std::size_t rhs_size)
{
  if (lhs_size + rhs_size <= ram_size)
  {
    return lhs_size;
  }

  const bool lhs_is_larger = lhs_size > ram_size;
  const size_t larger_size = (lhs_is_larger) ? lhs_size : rhs_size;
  const size_t smaller_size = (lhs_is_larger) ? rhs_size : lhs_size;

  if (larger_size <= ram_size)
  {
    const size_t remaining = ram_size - larger_size;
    const size_t min_acceptable = ram_size / 5;
    
    if (remaining >= smaller_size)
    {
      return (lhs_is_larger) ? lhs_size : remaining;
    }
    else if (remaining >= min_acceptable)
    {
      return lhs_is_larger ? lhs_size : min_acceptable;
    }
    else
    {
      const double ratio = static_cast< double >(larger_size) / (lhs_size + rhs_size);
      const size_t for_larger = static_cast< size_t >(ram_size * ratio);
      return (lhs_is_larger) ? for_larger : ram_size - for_larger;
    }
  }

  const double ratio = static_cast< double >(rhs_size) / (lhs_size + rhs_size);
  size_t for_rhs = static_cast< size_t >(ram_size * ratio);

  if (for_rhs == 0)
  {
    for_rhs = 1;
  }
  else if (for_rhs == rhs_size)
  {
    for_rhs = rhs_size - 1;
  }

  return ram_size - for_rhs;
}
