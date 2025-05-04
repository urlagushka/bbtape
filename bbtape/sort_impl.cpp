#include <bbtape/sort_impl.hpp>

#include <algorithm>
#include <future>
#include <queue>
#include <iostream>

#include <bbtape/utils.hpp>

namespace
{
  std::size_t
  read_from_tape_to_ram_without_roll(
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

  std::size_t
  read_from_tape_to_ram(
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

  bb::tape_handler &
  take_tape_handler(std::span< bb::tape_handler > src)
  {
    auto dst = std::find_if(src.begin(), src.end(), [](bb::tape_handler & th)
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

std::pair< bb::utils::tmp_file_handler, bb::sort::u_vec >
bb::sort::strategy_1(utils::tmp_file_handler & src, th_span ths, u_vec ram, std::size_t block_size)
{
  if (ths.size() < 1)
  {
    throw std::runtime_error("strategy_1: tape handlers amount is too small!");
  }

  utils::tmp_file_handler dst;
  ram_manager ram_m(std::move(ram), block_size);

  for (std::size_t i = 0; i < src.size(); i = i + 2)
  {
    auto & lhs = src[i];
    auto & rhs = src[i + 1];
    auto block = ram_m.take_ram_block();
    dst.push_back(sort_handler_1(ths[0], lhs, rhs, block));
    ram_m.free_ram_block(block);
  }

  if (dst.size() % 2 != 0 && dst.size() != 1)
  {
    auto tmp_file = utils::create_tmp_file();
    tape_unit tmp_tape;
    write_tape_to_file(tmp_file, tmp_tape); 
    dst.push_back(tmp_file);
  }

  return std::make_pair(std::move(dst), std::move(ram_m.pick_ram()));
}

std::pair< bb::utils::tmp_file_handler, bb::sort::u_vec >
bb::sort::strategy_2(utils::tmp_file_handler & src, th_span ths, u_vec ram, std::size_t block_size, std::size_t thread_amount)
{
  if (ths.size() < 2)
  {
    throw std::runtime_error("strategy_2: tape handlers amount is too small!");
  }

  utils::tmp_file_handler dst;
  ram_manager ram_m(std::move(ram), block_size);

  using th_rw = std::reference_wrapper< tape_handler >;
  using sort_tuple = std::tuple< std::future< fs::path >, th_rw, th_rw, std::span< int32_t > >;
  std::queue< sort_tuple > sort_queue;
  const std::size_t sort_queue_limit = thread_amount;
  for (std::size_t i = 0; i < src.size(); i = i + 2)
  {
    if (sort_queue.size() == sort_queue_limit)
    {
      auto future_2th = std::move(sort_queue.front());
      auto file = std::move(std::get< 0 >(future_2th));
      auto & th_1 = std::get< 1 >(future_2th);
      auto & th_2 = std::get< 2 >(future_2th);
      auto block = std::get< 3 >(future_2th);

      std::cout << "\nFREE RESOURCE\n";
      std::cout << "th_1: " << &th_1 << std::endl;
      std::cout << "th_2: " << &th_2 << std::endl; 
      dst.push_back(file.get());
      th_1.get().free();
      th_2.get().free();
      ram_m.free_ram_block(block);

      sort_queue.pop();
    }
    auto & lhs = src[i];
    auto & rhs = src[i + 1];
    std::cout << "\nLOCK RESOURCE\n";
    std::cout << "lhs: " << lhs << std::endl;
    std::cout << "rhs: " << rhs << std::endl; 
    auto & th_1 = take_tape_handler(ths);
    auto & th_2 = take_tape_handler(ths);
    std::cout << "th_1: " << &th_1 << std::endl;
    std::cout << "th_2: " << &th_2 << std::endl;
    auto block = ram_m.take_ram_block();
    auto tmp_future = std::async(std::launch::async, sort_handler_2,
      std::ref(th_1),
      std::ref(th_2),
      std::cref(lhs),
      std::cref(rhs),
      block
    );

    auto to_push = std::make_tuple(std::move(tmp_future), std::ref(th_1), std::ref(th_2), block);
    sort_queue.push(std::move(to_push));
  }

  while (!sort_queue.empty())
  {
    auto future_2th = std::move(sort_queue.front());
    auto file = std::move(std::get< 0 >(future_2th));
    auto & th_1 = std::get< 1 >(future_2th);
    auto & th_2 = std::get< 2 >(future_2th);
    auto block = std::get< 3 >(future_2th);

    dst.push_back(file.get());
    th_1.get().free();
    th_2.get().free();
    ram_m.free_ram_block(block);

    sort_queue.pop();
  }

  if (dst.size() % 2 != 0 && dst.size() != 1)
  {
    auto tmp_file = utils::create_tmp_file();
    tape_unit tmp_tape;
    write_tape_to_file(tmp_file, tmp_tape); 
    dst.push_back(tmp_file);
  }

  return std::make_pair(std::move(dst), std::move(ram_m.pick_ram()));
}

std::pair< bb::utils::tmp_file_handler, bb::sort::u_vec >
bb::sort::strategy_3(utils::tmp_file_handler & src, th_span ths, u_vec ram, std::size_t block_size, std::size_t thread_amount)
{
  if (ths.size() < 3)
  {
    throw std::runtime_error("strategy_3: tape handlers amount is too small!");
  }

  utils::tmp_file_handler dst;
  ram_manager ram_m(std::move(ram), block_size);

  using th_rw = std::reference_wrapper< tape_handler >;
  using sort_tuple = std::tuple< std::future< fs::path >, th_rw, th_rw, th_rw, std::span< int32_t > >;
  std::queue< sort_tuple > sort_queue;
  const std::size_t sort_queue_limit = thread_amount;
  for (std::size_t i = 0; i < src.size(); i = i + 2)
  {
    if (sort_queue.size() == sort_queue_limit)
    {
      auto future_3th = std::move(sort_queue.front());
      auto file = std::move(std::get< 0 >(future_3th));
      auto & th_1 = std::get< 1 >(future_3th);
      auto & th_2 = std::get< 2 >(future_3th);
      auto & th_3 = std::get< 3 >(future_3th);
      auto block = std::get< 4 >(future_3th);

      dst.push_back(file.get());
      th_1.get().free();
      th_2.get().free();
      th_3.get().free();
      ram_m.free_ram_block(block);

      sort_queue.pop();
    }
    auto & lhs = src[i];
    auto & rhs = src[i + 1];
    auto & th_1 = take_tape_handler(ths);
    auto & th_2 = take_tape_handler(ths);
    auto & th_3 = take_tape_handler(ths);
    auto block = ram_m.take_ram_block();
    auto tmp_future = std::async(std::launch::async, sort_handler_3,
      std::ref(th_1),
      std::ref(th_2),
      std::ref(th_3),
      std::cref(lhs),
      std::cref(rhs),
      block
    );
    
    auto to_push = std::make_tuple(std::move(tmp_future), std::ref(th_1), std::ref(th_2), std::ref(th_3), block);
    sort_queue.push(std::move(to_push));
  }

  while (!sort_queue.empty())
  {
    auto future_3th = std::move(sort_queue.front());
    auto file = std::move(std::get< 0 >(future_3th));
    auto & th_1 = std::get< 1 >(future_3th);
    auto & th_2 = std::get< 2 >(future_3th);
    auto & th_3 = std::get< 3 >(future_3th);
    auto block = std::get< 4 >(future_3th);

    dst.push_back(file.get());
    th_1.get().free();
    th_2.get().free();
    th_3.get().free();
    ram_m.free_ram_block(block);

    sort_queue.pop();
  }

  if (dst.size() % 2 != 0 && dst.size() != 1)
  {
    auto tmp_file = utils::create_tmp_file();
    tape_unit tmp_tape;
    write_tape_to_file(tmp_file, tmp_tape); 
    dst.push_back(tmp_file);
  }

  return std::make_pair(std::move(dst), std::move(ram_m.pick_ram()));
}

std::tuple< bb::utils::tmp_file_handler, bb::sort::u_tape, bb::sort::u_vec >
bb::sort::split_src_file(u_tape src, tape_handler & th, std::size_t file_amount, u_vec ram)
{
  if (!th.is_available())
  {
    throw std::runtime_error("split_src_file: tape_handler is unavailable!");
  }

  utils::tmp_file_handler dst;
  const std::size_t ram_size = ram->size();

  std::size_t src_offset = 0;
  for (std::size_t i = 0; i < file_amount; ++i)
  {
    auto tmp_file = utils::create_tmp_file();
    dst.push_back(tmp_file);

    if (src_offset == src->size())
    {
      write_tape_to_file(tmp_file, {});
      continue;
    }
    th.setup_tape(std::move(src));
    std::size_t was_read = read_from_tape_to_ram(th, 0, ram_size, src_offset, *ram);
    src_offset = src_offset + was_read;
    src = th.release_tape();

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

  return std::make_tuple(std::move(dst), std::move(src), std::move(ram));
}

bb::sort::fs::path
bb::sort::sort_handler_1(
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

bb::sort::fs::path
bb::sort::sort_handler_2(
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

bb::sort::fs::path
bb::sort::sort_handler_3(
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
