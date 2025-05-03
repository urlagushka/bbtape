#include <bbtape/physical.hpp>

#include <algorithm>
#include <stdexcept>
#include <ranges>

bb::ram_handler::ram_handler(std::unique_ptr< std::vector< int32_t > > ram, std::size_t block_size):
  __ram(std::move(ram)),
  __blocks(__ram->size() / block_size),
  __block_size(block_size)
{}

std::span< int32_t >
bb::ram_handler::take_ram_block()
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

  return std::span< int32_t >(*__ram | std::views::drop(offset) | std::views::take(__block_size));
}

void
bb::ram_handler::free_ram_block(std::span< int32_t > block)
{
  if (block.data() < __ram->data() || block.data() + __ram->size() > __ram->data() + __ram->size())
  {
    throw std::runtime_error("block is not owned by ram!");
  }

  __blocks[(__ram->data() - block.data()) / __block_size].clear();
}
