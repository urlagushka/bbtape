#include <bbtape/ram_handler.hpp>

std::size_t
bb::balance_ram_block(std::size_t ram_size, std::size_t lhs_size, std::size_t rhs_size)
{
  if (lhs_size == 0)
  {
    return 0;
  }
  if (rhs_size == 0)
  {
    return ram_size;
  }
  
  if (lhs_size + rhs_size <= ram_size)
  {
    return lhs_size;
  }

  double ratio = static_cast< double >(lhs_size) / (lhs_size + rhs_size);
  std::size_t for_lhs = static_cast< std::size_t >(ram_size * ratio);

  const std::size_t min_size = 1;
  if (for_lhs < min_size)
  {
    for_lhs = min_size;
  }
  if (for_lhs > ram_size - min_size)
  {
    for_lhs = ram_size - min_size;
  }

  return for_lhs;
}
