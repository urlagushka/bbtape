#ifndef BBTAPE_PHYSICAL_HPP
#define BBTAPE_PHYSICAL_HPP

#include <cstdint>
#include <cstddef>
#include <vector>

namespace bb::physical
{
  struct ram
  {
    std::vector< uint32_t > buffer;
    std::size_t limit_in_fields;
    std::size_t limit_in_bytes;
  };
}

#endif
