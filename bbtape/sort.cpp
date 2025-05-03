#include <bbtape/sort.hpp>

#include <cstdint>
#include <memory>
#include <utility>
#include <future>
#include <thread>
#include <stop_token>
#include <chrono>
#include <semaphore>

#include <iostream>
#include <format>

#include <bbtape/utils.hpp>
#include <bbtape/physical.hpp>

namespace
{
  std::size_t read_from_tape_to_ram_without_roll(
    bb::tape_handler & th,
    std::size_t lhs_ram_pos,
    std::size_t rhs_ram_pos,
    std::span< int32_t > ram_link
  )
  {
    std::size_t was_read = 0;
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

  std::size_t read_from_tape_to_ram(
    bb::tape_handler & th,
    std::size_t lhs_ram_pos,
    std::size_t rhs_ram_pos,
    std::size_t tape_offset,
    std::span< int32_t > ram_link
  )
  {
    th.roll(tape_offset);
    return read_from_tape_to_ram_without_roll(th, lhs_ram_pos, rhs_ram_pos, ram_link);
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
    block_size = (thread_amount > ram_size / 2) ? 2 : ram_size / thread_amount;
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

    std::sort(ram->begin(), ram->end());

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
  ram_manager ram_h(std::move(ram), params.block_size);
  
  // std::vector< tape_handler > handler(ft_config.phlimit.conv);

  auto lhs = tf_h[0];
  auto rhs = tf_h[1];
  auto block = ram_h.take_ram_block();
  tape_handler th0(ft_config);
  tape_handler th1(ft_config);

  auto start = std::chrono::high_resolution_clock::now();

  auto merge = sort_handler_1(th, lhs, rhs, block);
  // auto merge = sort_handler_2(th, th0, lhs, rhs, block);
  //auto merge = sort_handler_3(th, th0, th1, lhs, rhs, block);

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "sorting time: " << duration.count() << " ms\n";
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
  auto merge_tape = std::make_unique< tape_unit >(lhs_tape->size() + rhs_tape->size());

  const std::size_t lhs_tape_size = lhs_tape->size();
  const std::size_t rhs_tape_size = rhs_tape->size();
  const std::size_t merge_tape_size = merge_tape->size();

  std::size_t lhs_tape_pos = 0;
  std::size_t rhs_tape_pos = 0;
  std::size_t merge_tape_pos = 0;

  std::size_t was_read_lhs = 0;
  std::size_t was_read_rhs = 0;

  std::size_t was_write_lhs = 0;
  std::size_t was_write_rhs = 0;

  std::size_t ram_mid = 0;
  auto lhs_ram = ram_link.subspan(0, 0);
  auto rhs_ram = ram_link.subspan(0, 0);
  while (merge_tape_pos + 1 < merge_tape_size)
  {
    // balance ram block for read tapes
    auto valid_lhs_size = lhs_tape_size - lhs_tape_pos;
    auto valid_rhs_size = rhs_tape_size - rhs_tape_pos;
    if (was_read_lhs == was_write_lhs && was_read_rhs == was_write_rhs)
    {
      ram_mid = balance_ram_block(ram_link.size(), valid_lhs_size, valid_rhs_size);
      lhs_ram = ram_link.subspan(0, ram_mid);
      rhs_ram = ram_link.subspan(ram_mid, ram_link.size() - ram_mid);
    }

    // read if there are elements && tape is not empty
    if (valid_lhs_size > 0 && was_read_lhs == was_write_lhs)
    {
      th_1.setup_tape(std::move(lhs_tape));
      was_read_lhs = read_from_tape_to_ram(th_1, 0, lhs_ram.size(), lhs_tape_pos, lhs_ram);
      lhs_tape_pos = lhs_tape_pos + was_read_lhs;
      lhs_tape = th_1.release_tape();
      was_write_lhs = 0;
    }
    if (valid_rhs_size > 0 && was_read_rhs == was_write_rhs)
    {
      th_1.setup_tape(std::move(rhs_tape));
      was_read_rhs = read_from_tape_to_ram(th_1, 0, rhs_ram.size(), rhs_tape_pos, rhs_ram);
      rhs_tape_pos = rhs_tape_pos + was_read_rhs;
      rhs_tape = th_1.release_tape();
      was_write_rhs = 0;
    }
 
    th_1.setup_tape(std::move(merge_tape));
    th_1.roll(merge_tape_pos);
    while (
      (was_write_lhs < was_read_lhs && was_write_rhs < was_read_rhs)
      ||
      ((was_write_lhs < was_read_lhs || was_write_rhs < was_read_rhs) && (valid_lhs_size == 0 || valid_rhs_size == 0))
    )
    {
      // only rhs has element
      if (was_write_lhs == was_read_lhs)
      {
        th_1.write(rhs_ram[was_write_rhs++]);
        if (merge_tape_pos + 1 == th_1.size())
        {
          break;
        }
        th_1.offset(1);
        ++merge_tape_pos;
        continue;
      }

      // only lhs has element
      if (was_write_rhs == was_read_rhs)
      {
        th_1.write(lhs_ram[was_write_lhs++]);
        if (merge_tape_pos + 1 == th_1.size())
        {
          break;
        }
        th_1.offset(1);
        ++merge_tape_pos;
        continue;
      }

      // both has element
      auto lhs_data = lhs_ram[was_write_lhs];
      auto rhs_data = rhs_ram[was_write_rhs];
      if (lhs_data < rhs_data)
      {
        th_1.write(lhs_data);
        ++was_write_lhs;
      }
      else
      {
        th_1.write(rhs_data);
        ++was_write_rhs;
      }
      if (merge_tape_pos + 1 == th_1.size())
      {
        ++merge_tape_pos;
        break;
      }
      th_1.offset(1);
      ++merge_tape_pos;
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
    throw std::runtime_error("sort_handler_1: tape_handler is unavailable!");
  }

  auto lhs_tape = std::make_unique< tape_unit >(read_tape_from_file(lhs));
  auto rhs_tape = std::make_unique< tape_unit >(read_tape_from_file(rhs));
  auto merge_tape = std::make_unique< tape_unit >(lhs_tape->size() + rhs_tape->size());

  const std::size_t lhs_tape_size = lhs_tape->size();
  const std::size_t rhs_tape_size = rhs_tape->size();
  const std::size_t merge_tape_size = merge_tape->size();

  std::atomic< std::size_t > lhs_tape_pos = 0;
  std::atomic< std::size_t > rhs_tape_pos = 0;
  std::atomic< std::size_t > merge_tape_pos = 0;

  std::atomic< std::size_t > was_read_lhs = 0;
  std::atomic< std::size_t > was_read_rhs = 0;

  std::atomic< std::size_t > was_write_lhs = 0;
  std::atomic< std::size_t > was_write_rhs = 0;

  std::size_t ram_mid = 0;
  auto lhs_ram = ram_link.subspan(0, 0);
  auto rhs_ram = ram_link.subspan(0, 0);

  std::atomic_flag need_lhs_read;
  std::atomic_flag need_rhs_read;
  std::mutex th_1_mutex;

  auto lhs_thread = std::jthread([&](std::stop_token stop)
  {
    while (!stop.stop_requested())
    {
      if (need_lhs_read.test())
      {
        std::lock_guard< std::mutex > lock(th_1_mutex);
        th_1.setup_tape(std::move(lhs_tape));
        was_read_lhs = read_from_tape_to_ram(th_1, 0, lhs_ram.size(), lhs_tape_pos, lhs_ram);
        lhs_tape_pos = lhs_tape_pos + was_read_lhs;
        lhs_tape = th_1.release_tape();
        was_write_lhs = 0;
        need_lhs_read.clear();
      }
    }
  });

  auto rhs_thread = std::jthread([&](std::stop_token stop)
  {
    while (!stop.stop_requested())
    {
      if (need_rhs_read.test())
      {
        std::lock_guard< std::mutex > lock(th_1_mutex);
        th_1.setup_tape(std::move(rhs_tape));
        was_read_rhs = read_from_tape_to_ram(th_1, 0, rhs_ram.size(), rhs_tape_pos, rhs_ram);
        rhs_tape_pos = rhs_tape_pos + was_read_rhs;
        rhs_tape = th_1.release_tape();
        was_write_rhs = 0;
        need_rhs_read.clear();
      }
    }
  });

  th_2.setup_tape(std::move(merge_tape));
  while (merge_tape_pos + 1 < merge_tape_size)
  {
    std::size_t valid_lhs_size = 0;
    std::size_t valid_rhs_size = 0;
    {
      std::lock_guard< std::mutex > lock(th_1_mutex);
      // balance ram block for read tapes
      valid_lhs_size = lhs_tape_size - lhs_tape_pos;
      valid_rhs_size = rhs_tape_size - rhs_tape_pos;
      if (was_read_lhs == was_write_lhs && was_read_rhs == was_write_rhs)
      {
        ram_mid = balance_ram_block(ram_link.size(), valid_lhs_size, valid_rhs_size);
        lhs_ram = ram_link.subspan(0, ram_mid);
        rhs_ram = ram_link.subspan(ram_mid, ram_link.size() - ram_mid);
      }

      // read if there are elements && tape is not empty
      if (valid_lhs_size > 0 && was_read_lhs == was_write_lhs)
      {
        need_lhs_read.test_and_set();
      }
      if (valid_rhs_size > 0 && was_read_rhs == was_write_rhs)
      {
        need_rhs_read.test_and_set();
      }
    }
    while (
      (was_write_lhs < was_read_lhs && was_write_rhs < was_read_rhs)
      ||
      ((was_write_lhs < was_read_lhs || was_write_rhs < was_read_rhs) && (valid_lhs_size == 0 || valid_rhs_size == 0))
    )
    {
      std::lock_guard< std::mutex > lock(th_1_mutex);
      // only rhs has element
      if (was_write_lhs == was_read_lhs)
      {
        th_2.write(rhs_ram[was_write_rhs++]);
        if (merge_tape_pos + 1 == th_2.size())
        {
          break;
        }
        th_2.offset(1);
        ++merge_tape_pos;
        continue;
      }

      // only lhs has element
      if (was_write_rhs == was_read_rhs)
      {
        th_2.write(lhs_ram[was_write_lhs++]);
        if (merge_tape_pos + 1 == th_2.size())
        {
          break;
        }
        th_2.offset(1);
        ++merge_tape_pos;
        continue;
      }

      // both has element
      auto lhs_data = lhs_ram[was_write_lhs];
      auto rhs_data = rhs_ram[was_write_rhs];
      if (lhs_data < rhs_data)
      {
        th_2.write(lhs_data);
        ++was_write_lhs;
      }
      else
      {
        th_2.write(rhs_data);
        ++was_write_rhs;
      }
      if (merge_tape_pos + 1 == th_2.size())
      {
        ++merge_tape_pos;
        break;
      }
      th_2.offset(1);
      ++merge_tape_pos;
    }
  }

  lhs_thread.request_stop();
  rhs_thread.request_stop();
  merge_tape = th_2.release_tape();
  auto merge_file = utils::create_tmp_file();
  write_tape_to_file(merge_file, *merge_tape);
  return merge_file;
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
    throw std::runtime_error("sort_handler_1: tape_handler is unavailable!");
  }

  auto lhs_tape = std::make_unique< tape_unit >(read_tape_from_file(lhs));
  auto rhs_tape = std::make_unique< tape_unit >(read_tape_from_file(rhs));
  auto merge_tape = std::make_unique< tape_unit >(lhs_tape->size() + rhs_tape->size());

  const std::size_t lhs_tape_size = lhs_tape->size();
  const std::size_t rhs_tape_size = rhs_tape->size();
  const std::size_t merge_tape_size = merge_tape->size();

  std::atomic< std::size_t > lhs_tape_pos = 0;
  std::atomic< std::size_t > rhs_tape_pos = 0;
  std::atomic< std::size_t > merge_tape_pos = 0;

  std::atomic< std::size_t > was_read_lhs = 0;
  std::atomic< std::size_t > was_read_rhs = 0;

  std::atomic< std::size_t > was_write_lhs = 0;
  std::atomic< std::size_t > was_write_rhs = 0;

  std::size_t ram_mid = 0;
  auto lhs_ram = ram_link.subspan(0, 0);
  auto rhs_ram = ram_link.subspan(0, 0);

  std::atomic_flag need_lhs_read;
  std::atomic_flag need_rhs_read;
  std::mutex th_1_mutex;
  std::mutex th_2_mutex;

  th_1.setup_tape(std::move(lhs_tape));
  auto lhs_thread = std::jthread([&](std::stop_token stop)
  {
    while (!stop.stop_requested())
    {
      if (need_lhs_read.test())
      {
        std::lock_guard< std::mutex > lock(th_1_mutex);
        was_read_lhs = read_from_tape_to_ram_without_roll(th_1, 0, lhs_ram.size(), lhs_ram);
        lhs_tape_pos = lhs_tape_pos + was_read_lhs;
        was_write_lhs = 0;
        need_lhs_read.clear();
      }
    }
    lhs_tape = th_1.release_tape();
  });

  th_2.setup_tape(std::move(rhs_tape));
  auto rhs_thread = std::jthread([&](std::stop_token stop)
  {
    while (!stop.stop_requested())
    {
      if (need_rhs_read.test())
      {
        std::lock_guard< std::mutex > lock(th_2_mutex);
        was_read_rhs = read_from_tape_to_ram_without_roll(th_2, 0, rhs_ram.size(), rhs_ram);
        rhs_tape_pos = rhs_tape_pos + was_read_rhs;
        was_write_rhs = 0;
        need_rhs_read.clear();
      }
    }
    rhs_tape = th_2.release_tape();
  });

  th_3.setup_tape(std::move(merge_tape));
  while (merge_tape_pos + 1 < merge_tape_size)
  {
    std::size_t valid_lhs_size = 0;
    std::size_t valid_rhs_size = 0;
    {
      std::lock_guard< std::mutex > lock_th_1(th_1_mutex);
      std::lock_guard< std::mutex > lock_th_2(th_2_mutex);
      // balance ram block for read tapes
      valid_lhs_size = lhs_tape_size - lhs_tape_pos;
      valid_rhs_size = rhs_tape_size - rhs_tape_pos;
      if (was_read_lhs == was_write_lhs && was_read_rhs == was_write_rhs)
      {
        ram_mid = balance_ram_block(ram_link.size(), valid_lhs_size, valid_rhs_size);
        lhs_ram = ram_link.subspan(0, ram_mid);
        rhs_ram = ram_link.subspan(ram_mid, ram_link.size() - ram_mid);
      }

      // read if there are elements && tape is not empty
      if (valid_lhs_size > 0 && was_read_lhs == was_write_lhs)
      {
        need_lhs_read.test_and_set();
      }
      if (valid_rhs_size > 0 && was_read_rhs == was_write_rhs)
      {
        need_rhs_read.test_and_set();
      }
    }

    while (
      (was_write_lhs < was_read_lhs && was_write_rhs < was_read_rhs)
      ||
      ((was_write_lhs < was_read_lhs || was_write_rhs < was_read_rhs) && (valid_lhs_size == 0 || valid_rhs_size == 0))
    )
    {
      std::lock_guard< std::mutex > lock(th_1_mutex);
      // only rhs has element
      if (was_write_lhs == was_read_lhs)
      {
        th_3.write(rhs_ram[was_write_rhs++]);
        if (merge_tape_pos + 1 == th_3.size())
        {
          break;
        }
        th_3.offset(1);
        ++merge_tape_pos;
        continue;
      }

      // only lhs has element
      if (was_write_rhs == was_read_rhs)
      {
        th_3.write(lhs_ram[was_write_lhs++]);
        if (merge_tape_pos + 1 == th_3.size())
        {
          break;
        }
        th_3.offset(1);
        ++merge_tape_pos;
        continue;
      }

      // both has element
      auto lhs_data = lhs_ram[was_write_lhs];
      auto rhs_data = rhs_ram[was_write_rhs];
      if (lhs_data < rhs_data)
      {
        th_3.write(lhs_data);
        ++was_write_lhs;
      }
      else
      {
        th_3.write(rhs_data);
        ++was_write_rhs;
      }
      if (merge_tape_pos + 1 == th_3.size())
      {
        ++merge_tape_pos;
        break;
      }
      th_3.offset(1);
      ++merge_tape_pos;
    }
  }

  lhs_thread.request_stop();
  rhs_thread.request_stop();
  merge_tape = th_3.release_tape();
  auto merge_file = utils::create_tmp_file();
  write_tape_to_file(merge_file, *merge_tape);
  return merge_file;
}
