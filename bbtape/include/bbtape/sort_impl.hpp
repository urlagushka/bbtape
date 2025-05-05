#ifndef BBTAPE_SORT_IMPL_HPP
#define BBTAPE_SORT_IMPL_HPP

#include <vector>
#include <utility>
#include <tuple>
#include <queue>
#include <future>

#include <bbtape/file_handler.hpp>
#include <bbtape/tape_handler.hpp>
#include <bbtape/ram_handler.hpp>
#include <bbtape/unit.hpp>
#include <bbtape/utils.hpp>

namespace
{
  template< bb::unit_type T >
  std::size_t
  read_from_tape_to_ram_without_roll(bb::tape_handler< T > & th, std::size_t lhs, std::size_t rhs, std::span< T > ram)
  {
    std::size_t was_read = 0;
    for (std::size_t pos = lhs; pos < rhs; ++pos, ++was_read)
    {
      ram[pos] = th.read();
      if (th.get_pos_vl() + 1 == th.size())
      {
        ++was_read;
        break;
      }
      th.offset(1);
    }

    return was_read;
  }

  template< bb::unit_type T >
  std::size_t
  read_from_tape_to_ram(bb::tape_handler< T > & th, std::size_t lhs, std::size_t rhs, std::size_t off, std::span< T > ram)
  {
    th.roll(off);
    return read_from_tape_to_ram_without_roll(th, lhs, rhs, ram);
  }

  template< bb::unit_type T >
  bb::tape_handler< T > &
  take_tape_handler(std::span< bb::tape_handler< T > > src)
  {
    auto dst = std::find_if(src.begin(), src.end(), [](bb::tape_handler< T > & th)
    {
      return !th.is_reserved();
    });

    if (dst == src.end())
    {
      throw std::runtime_error("can not find available tape handler!");
    }

    dst->take();

    return *dst;
  }
}

namespace bb
{
  using c_path_l = const fs::path &;

  template< unit_type T >
  using th_l = tape_handler< T > &;

  template< unit_type T >
  using u_vec = std::unique_ptr< std::vector< T > >;

  template< unit_type T >
  using u_unit = std::unique_ptr< unit < T > >;

  template< unit_type T >
  using th_span = std::span< tape_handler< T > >;

  template< unit_type T >
  std::pair< file_handler, u_vec< T > >
  strategy(const file_handler & src, th_span< T > ths, u_vec< T > ram, std::size_t block_size, std::size_t thread_amount);

  template< unit_type T >
  std::tuple< file_handler, u_unit< T >, u_vec< T > >
  split_src_unit(u_unit< T > src, th_l< T > th, std::size_t file_amount, u_vec< T > ram);

  template< unit_type T >
  fs::path
  sort_handler(th_l< T > th_1, c_path_l lhs, c_path_l rhs, std::span< T > ram);
}

template< bb::unit_type T >
std::pair< bb::file_handler, bb::u_vec< T > >
bb::strategy(const file_handler & src, th_span< T > ths, u_vec< T > ram, std::size_t block_size, std::size_t thread_amount)
{
  if (ths.size() < 1)
  {
    throw std::runtime_error("strategy: tape handlers amount is too small!");
  }

  file_handler dst;
  ram_handler rhandler(std::move(ram), block_size);

  using th_rw = std::reference_wrapper< tape_handler< T > >;
  using sort_tuple = std::tuple< std::future< fs::path >, th_rw, std::span< T > >;
  std::queue< sort_tuple > sort_queue;
  for (std::size_t i = 0; i < src.size(); i = i + 2)
  {
    if (sort_queue.size() == thread_amount)
    {
      auto future_2th = std::move(sort_queue.front());
      sort_queue.pop();

      auto file = std::move(std::get< 0 >(future_2th));
      auto & th = std::get< 1 >(future_2th);
      auto block = std::get< 2 >(future_2th);

      dst.push_back(file.get());
      th.get().free();
      rhandler.free_ram_block(block);
    }
    auto & th = take_tape_handler< T >(ths);
    const auto & lhs = src[i];
    const auto & rhs = src[i + 1];
    auto block = rhandler.take_ram_block();
    auto tmp_future = std::async(std::launch::async, sort_handler< T >,
      std::ref(th),
      std::cref(lhs),
      std::cref(rhs),
      block
    );

    auto to_push = std::make_tuple(std::move(tmp_future), std::ref(th), block);
    sort_queue.push(std::move(to_push));
  }

  while (!sort_queue.empty())
  {
    auto future_2th = std::move(sort_queue.front());
    auto file = std::move(std::get< 0 >(future_2th));
    auto & th = std::get< 1 >(future_2th);
    auto block = std::get< 2 >(future_2th);

    dst.push_back(file.get());
    th.get().free();
    rhandler.free_ram_block(block);

    sort_queue.pop();
  }

  if (dst.size() % 2 != 0 && dst.size() != 1)
  {
    auto tmp_file = utils::create_tmp_file();
    write_tape_to_file< T >(tmp_file, {}); 
    dst.push_back(tmp_file);
  }

  return std::make_pair(std::move(dst), std::move(rhandler.pick_ram()));
}

template< bb::unit_type T >
std::tuple< bb::file_handler, bb::u_unit< T >, bb::u_vec< T > >
bb::split_src_unit(u_unit< T > src, th_l< T > th, std::size_t file_amount, u_vec< T > ram)
{
  if (!th.is_available())
  {
    throw std::runtime_error("split_src_file: tape_handler is unavailable!");
  }

  file_handler dst;
  const std::size_t ram_size = ram->size();

  std::size_t src_offset = 0;
  for (std::size_t i = 0; i < file_amount; ++i)
  {
    auto tmp_file = utils::create_tmp_file();
    dst.push_back(tmp_file);

    if (src_offset == src->size())
    {
      write_tape_to_file< T >(tmp_file, {});
      continue;
    }
    th.setup_tape(std::move(src));
    std::size_t was_read = read_from_tape_to_ram< T >(th, 0, ram_size, src_offset, *ram);
    src_offset = src_offset + was_read;
    src = th.release_tape();

    std::sort(ram->begin(), ram->begin() + was_read);

    auto tmp_tape = std::make_unique< unit< T > >(was_read);
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
    write_tape_to_file< T >(tmp_file, *tmp_tape);
  }

  return std::make_tuple(std::move(dst), std::move(src), std::move(ram));
}

template< bb::unit_type T >
bb::fs::path
bb::sort_handler(th_l< T > th_1, c_path_l lhs, c_path_l rhs, std::span< T > ram)
{
  if (!th_1.is_available())
  {
    throw std::runtime_error("sort_handler_1: tape_handler is unavailable!");
  }
  if (ram.size() < 2)
  {
    throw std::runtime_error("sort_handler_1: ram size is too small!");
  }

  auto lhs_tape = std::make_unique< unit< T > >(read_tape_from_file< T >(lhs));
  auto rhs_tape = std::make_unique< unit< T > >(read_tape_from_file< T >(rhs));
  auto dst_tape = std::make_unique< unit< T > >(lhs_tape->size() + rhs_tape->size());

  const std::size_t lhs_size = lhs_tape->size();
  const std::size_t rhs_size = rhs_tape->size();
  const std::size_t dst_size = dst_tape->size();

  std::size_t lhs_pos = 0;
  std::size_t rhs_pos = 0;
  std::size_t dst_pos = 0;

  std::size_t to_write_lhs = 0;
  std::size_t to_write_rhs = 0;

  std::span< T > lhs_ram{};
  std::span< T > rhs_ram{};

  std::size_t lhs_ram_pos = 0;
  std::size_t rhs_ram_pos = 0;

  while (lhs_pos < lhs_size && rhs_pos < rhs_size)
  {
    if (to_write_lhs == 0 && to_write_rhs == 0)
    {
      auto tmp_lhs_size = lhs_size - lhs_pos;
      auto tmp_rhs_size = rhs_size - rhs_pos;
      auto ram_mid = balance_ram_block(ram.size(), tmp_lhs_size, tmp_rhs_size);
      lhs_ram = ram.subspan(0, ram_mid);
      rhs_ram = ram.subspan(ram_mid, ram.size() - ram_mid);
      assert(lhs_ram.size() + rhs_ram.size() == ram.size());
    }


    if (lhs_ram_pos == to_write_lhs)
    {
      th_1.setup_tape(std::move(lhs_tape));
      to_write_lhs = read_from_tape_to_ram< T >(th_1, 0, lhs_ram.size(), lhs_pos, lhs_ram);
      lhs_tape = th_1.release_tape();
      lhs_ram_pos = 0;
    }

    if (rhs_ram_pos == to_write_rhs)
    {
      th_1.setup_tape(std::move(rhs_tape));
      to_write_rhs = read_from_tape_to_ram< T >(th_1, 0, rhs_ram.size(), rhs_pos, rhs_ram);
      rhs_tape = th_1.release_tape();
      rhs_ram_pos = 0;
    }

    th_1.setup_tape(std::move(dst_tape));
    th_1.roll(dst_pos);
    while (lhs_ram_pos < to_write_lhs && rhs_ram_pos < to_write_rhs)
    {
      auto lhs_value = lhs_ram[lhs_ram_pos];
      auto rhs_value = rhs_ram[rhs_ram_pos];
      if (lhs_value < rhs_value)
      {
        th_1.write(lhs_value);
        ++lhs_pos;
        ++lhs_ram_pos;
      }
      else
      {
        th_1.write(rhs_value);
        ++rhs_pos;
        ++rhs_ram_pos;
      }

      th_1.offset_if_possible(1);
      ++dst_pos;
    }
    dst_tape = th_1.release_tape();
  }

  while (lhs_pos < lhs_size)
  {
    th_1.setup_tape(std::move(lhs_tape));
    to_write_lhs = read_from_tape_to_ram< T >(th_1, 0, ram.size(), lhs_pos, ram);
    lhs_tape = th_1.release_tape();
    lhs_ram_pos = 0;

    th_1.setup_tape(std::move(dst_tape));
    th_1.roll(dst_pos);
    while (lhs_ram_pos < to_write_lhs)
    {
      auto value = ram[lhs_ram_pos];
      th_1.write(value);
      ++lhs_pos;
      ++lhs_ram_pos;
      ++dst_pos;
      th_1.offset_if_possible(1);
    }
    dst_tape = th_1.release_tape();
  }

  while (rhs_pos < rhs_size)
  {
    th_1.setup_tape(std::move(rhs_tape));
    to_write_rhs = read_from_tape_to_ram< T >(th_1, 0, ram.size(), rhs_pos, ram);
    rhs_tape = th_1.release_tape();
    rhs_ram_pos = 0;

    th_1.setup_tape(std::move(dst_tape));
    th_1.roll(dst_pos);
    while (rhs_ram_pos < to_write_rhs)
    {
      auto value = ram[rhs_ram_pos];
      th_1.write(value);
      ++rhs_pos;
      ++rhs_ram_pos;
      ++dst_pos;
      th_1.offset_if_possible(1);
    }
    dst_tape = th_1.release_tape();
  }

  assert(dst_pos == dst_size);

  auto dst = utils::create_tmp_file();
  write_tape_to_file< T >(dst, *dst_tape);
  return dst;
}

#endif
