#ifndef BBTAPE_RAM_HANDLER_HPP
#define BBTAPE_RAM_HANDLER_HPP

#include <memory>
#include <vector>
#include <cstddef>
#include <span>
#include <atomic>
#include <mutex>

#include <bbtape/unit.hpp>

namespace bb
{
  template< unit_type T >
  class ram_handler
  {
    public:
      ram_handler() = delete;
      ram_handler(std::unique_ptr< std::vector< T > > ram, std::size_t block_size);

      std::span< T > take_ram_block();
      void free_ram_block(std::span< T > block);
      std::unique_ptr< std::vector< T > > pick_ram();

    private:
      std::unique_ptr< std::mutex > __mutex;
      std::unique_ptr< std::vector< T > > __ram;
      std::vector< std::atomic_flag > __blocks;
      std::size_t __block_size;
  };

  std::size_t
  balance_ram_block(std::size_t ram_size, std::size_t lhs_size, std::size_t rhs_size);
}

template< bb::unit_type T >
bb::ram_handler< T >::ram_handler(std::unique_ptr< std::vector< T > > ram, std::size_t block_size):
  __mutex(std::make_unique< std::mutex >()),
  __ram(std::move(ram)),
  __blocks(__ram->size() / block_size),
  __block_size(block_size)
{
  for (auto & flag : __blocks)
  {
    flag.clear();
  }
}

template< bb::unit_type T >
std::span< T >
bb::ram_handler< T >::take_ram_block()
{
  std::lock_guard< std::mutex > lock(*__mutex);
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
  auto start = __ram->data() + offset * __block_size;
  return std::span< T >{start, start + __block_size};
}

template< bb::unit_type T >
void
bb::ram_handler< T >::free_ram_block(std::span< T > block)
{
  std::lock_guard< std::mutex > lock(*__mutex);
  if (block.data() - __ram->data() < 0)
  {
    throw std::runtime_error("block is not owned by ram!");
  }

  __blocks[(block.data() - __ram->data()) / __block_size].clear();
}

template< bb::unit_type T >
std::unique_ptr< std::vector< T > >
bb::ram_handler< T >::pick_ram()
{
  std::lock_guard< std::mutex > lock(*__mutex);
  __blocks.clear();
  __block_size = 0;
  return std::move(__ram);
}

#endif
