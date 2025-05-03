#include <bbtape/sort.hpp>

#include <cstdint>
#include <memory>
#include <utility>

#include <iostream>
#include <format>

#include <bbtape/utils.hpp>
#include <bbtape/physical.hpp>

namespace
{
  std::size_t read_from_tape_to_ram(
    bb::tape_handler & th,
    std::size_t lhs_ram_pos,
    std::size_t rhs_ram_pos,
    std::size_t tape_offset,
    std::span< int32_t > ram_link
  )
  {
    std::size_t was_read = 0;
    th.roll(tape_offset);
    for (std::size_t ram_pos_vl = lhs_ram_pos; ram_pos_vl < rhs_ram_pos; ++ram_pos_vl, ++was_read)
    {
      ram_link[ram_pos_vl] = th.read();
      if (th.get_pos_vl() + 1 >= th.size())
      {
        ++was_read;
        break;
      }
      th.offset(1);
    }

    return was_read;
  }
}

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
    block_size = ram_size / thread_amount;
  }

  return {file_amount, sort_method, thread_amount, block_size};
}

void
bb::external_merge_sort(config ft_config, const fs::path & src, const fs::path & dst)
{
  auto src_tape = std::make_unique< tape_unit >(read_tape_from_file(src));
  auto dst_tape = std::make_unique< tape_unit >(tape_unit(src_tape->size()));

  std::size_t ram_size = ft_config.phlimit.ram / sizeof(int32_t);
  auto ram = std::make_unique< std::vector< int32_t > >(ram_size);

  sort_params params = get_sort_params(src_tape->size(), ram_size, ft_config.phlimit.conv);
  tape_handler th(ft_config);

  utils::tmp_file_handler tf_h;
  std::size_t src_offset = 0;
  for (std::size_t i = 0; i < params.file_amount; ++i)
  {
    auto tmp_file = utils::create_tmp_file();
    tf_h.push_back(tmp_file);

    if (src_offset == src_tape->size())
    {
      write_tape_to_file(tmp_file, {});
      continue;
    }
    th.setup_tape(std::move(src_tape));
    std::size_t was_read = read_from_tape_to_ram(th, 0, ram_size, src_offset, *ram);
    src_offset = src_offset + was_read;
    src_tape = th.release_tape();

    auto tmp_tape = std::make_unique< tape_unit >(was_read);
    th.setup_tape(std::move(tmp_tape));
    for (std::size_t i = 0; i < was_read; ++i)
    {
      th.write((*ram)[i]);
      if (th.get_pos_vl() + 1 >= th.size())
      {
        break;
      }
      th.offset(1);
    }
    tmp_tape = th.release_tape();
    write_tape_to_file(tmp_file, *tmp_tape);
  }


  // sort
  ram_handler ram_h(std::move(ram), params.block_size);
  
  // std::vector< tape_handler > handler(ft_config.phlimit.conv);

  auto lhs = tf_h[0];
  auto rhs = tf_h[1];
  auto block = ram_h.take_ram_block();
  auto merge = sort_handler_1(th, lhs, rhs, block);

}

bb::fs::path
bb::sort_handler_1(
  tape_handler & th_1,
  const fs::path & lhs,
  const fs::path & rhs,
  std::span< int32_t > ram_link
)
{
  if (!th_1.is_available())
  {
    throw std::runtime_error("sort_handler_1: tape_handler is unavailable!");
  }

  auto lhs_tape = std::make_unique< tape_unit >(read_tape_from_file(lhs));
  auto rhs_tape = std::make_unique< tape_unit >(read_tape_from_file(rhs));
  auto merge_tape = std::make_unique< tape_unit >(tape_unit(lhs_tape->size() + rhs_tape->size()));

  const std::size_t lhs_ram_start = 0;
  const std::size_t rhs_ram_start = ram_link.size() / 2;
  std::size_t lhs_tape_off_pos = 0;
  std::size_t rhs_tape_off_pos = 0;
  std::size_t merge_tape_off_pos = 0;
  std::size_t valid_lhs_ram_limit = 0;
  std::size_t valid_rhs_ram_limit = 0;
  std::size_t lhs_pos_vl = 0;
  std::size_t rhs_pos_vl = 0;

  while (merge_tape_off_pos < merge_tape->size())
  {
    if (lhs_pos_vl != rhs_ram_start && lhs_tape_off_pos < lhs_tape->size())
    {
      th_1.setup_tape(std::move(lhs_tape));
      valid_lhs_ram_limit = read_from_tape_to_ram(th_1, lhs_ram_start, rhs_ram_start, lhs_tape_off_pos, ram_link);
      lhs_tape_off_pos = lhs_tape_off_pos + valid_lhs_ram_limit - lhs_ram_start;
      lhs_tape = th_1.release_tape();
    }

    if (rhs_pos_vl != ram_link.size() && rhs_tape_off_pos < rhs_tape->size())
    {
      th_1.setup_tape(std::move(rhs_tape));
      valid_rhs_ram_limit = read_from_tape_to_ram(th_1, rhs_ram_start, ram_link.size(), rhs_tape_off_pos, ram_link);
      rhs_tape_off_pos = rhs_tape_off_pos + valid_rhs_ram_limit - rhs_ram_start;
      rhs_tape = th_1.release_tape();
    }

    lhs_pos_vl = lhs_ram_start;
    rhs_pos_vl = rhs_ram_start;

    th_1.setup_tape(std::move(merge_tape));
    th_1.roll(merge_tape_off_pos);
    while (lhs_pos_vl != valid_lhs_ram_limit || rhs_pos_vl != valid_rhs_ram_limit)
    {
      auto lhs_data = ram_link[lhs_pos_vl];
      auto rhs_data = ram_link[rhs_pos_vl];
      auto min_data = std::min(lhs_data, rhs_data);
      th_1.write(min_data);
      th_1.offset(1);
      (min_data == lhs_data) ? ++lhs_pos_vl : ++rhs_pos_vl;
    }
    merge_tape = th_1.release_tape();
  }

  auto merge_file = utils::create_tmp_file();
  write_tape_to_file(merge_file, *merge_tape);
  return merge_file;
}

bb::fs::path
bb::sort_handler_2(
  tape_handler & th_1,
  tape_handler & th_2,
  const fs::path & lhs,
  const fs::path & rhs,
  std::span< int32_t > ram_link
)
{
  if (!th_1.is_available() || !th_2.is_available())
  {
    throw std::runtime_error("sort_handler_2: tape_handler is unavailable!");
  }
}

bb::fs::path
bb::sort_handler_3(
  tape_handler & th_1,
  tape_handler & th_2,
  tape_handler & th_3,
  const fs::path & lhs,
  const fs::path & rhs,
  std::span< int32_t > ram_link
)
{
  if (!th_1.is_available() || !th_2.is_available() || !th_3.is_available())
  {
    throw std::runtime_error("sort_handler_3: tape_handler is unavailable!");
  }
}
