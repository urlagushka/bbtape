#ifndef BBTAPE_PHYSICAL_HPP
#define BBTAPE_PHYSICAL_HPP

#include <memory>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <span>
#include <atomic>

#include <bbtape/tape.hpp>

namespace bb
{
  class ram_handler
  {
    public:
      ram_handler() = delete;
      ram_handler(std::unique_ptr< std::vector< int32_t > > ram, std::size_t block_size);

      std::span< int32_t > take_ram_block();
      void free_ram_block(std::span< int32_t > block);

    private:
      std::unique_ptr< std::vector< int32_t > > __ram;
      std::vector< std::atomic_flag > __blocks;
      std::size_t __block_size;
  };

  std::size_t
  balance_ram_block(std::size_t ram_size, std::size_t lhs_size, std::size_t rhs_size);

  class conv_handler
  {
    public:
    private:
      std::unique_ptr< tape_handler > __conv;
  };
}

#endif
