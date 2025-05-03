#include <bbtape/tape.hpp>

#include <stdexcept>
#include <thread>

bb::tape_handler::tape_handler(config rhs):
  __tape(nullptr),
  __pos_vl(0),

  __delay_on_read(rhs.delay.on_read),
  __delay_on_write(rhs.delay.on_write),
  __delay_on_roll(rhs.delay.on_roll),
  __delay_on_offset(rhs.delay.on_offset)
{}

int32_t
bb::tape_handler::read()
{
  std::lock_guard< std::mutex > lock(__tape_mutex);
  std::this_thread::sleep_for(std::chrono::milliseconds(__delay_on_read));

  if (!__tape)
  {
    throw std::runtime_error("can't read tape value! (no tape)");
  }
  if (__tape->size() == 0)
  {
    throw std::runtime_error("can't read tape value! (empty tape)");
  }
  if (__pos_vl >= __tape->size())
  {
    throw std::runtime_error("can't read tape value! (bad position)");
  }

  return (*__tape)[__pos_vl];
}

void
bb::tape_handler::write(int32_t new_data)
{
  std::lock_guard< std::mutex > lock(__tape_mutex);
  std::this_thread::sleep_for(std::chrono::milliseconds(__delay_on_write));

  if (!__tape)
  {
    throw std::runtime_error("can't write tape value! (no tape)");
  }
  if (__tape->size() == 0)
  {
    throw std::runtime_error("can't write tape value! (empty tape)");
  }
  if (__pos_vl >= __tape->size())
  {
    throw std::runtime_error("can't write tape value! (bad position)");
  }

  (*__tape)[__pos_vl] = new_data;
}

void
bb::tape_handler::roll(std::size_t new_pos)
{
  std::lock_guard< std::mutex > lock(__tape_mutex);
  std::this_thread::sleep_for(std::chrono::milliseconds(__delay_on_roll));

  if (new_pos > __tape->size())
  {
    throw std::runtime_error("can't roll tape! (new position is greater than tape size)");
  }

  __pos_vl = new_pos;
}

void
bb::tape_handler::offset(int direction)
{
  std::lock_guard< std::mutex > lock(__tape_mutex);
  std::this_thread::sleep_for(std::chrono::milliseconds(__delay_on_offset));

  if (__pos_vl == 0 && direction < 0)
  {
    throw std::runtime_error("can't offset tape! (new position is less than 0)");
  }
  if (__pos_vl == __tape->size() - 1 && direction > 0)
  {
    throw std::runtime_error("can't offset tape! (new position is greater than tape size)");
  }

  __pos_vl = __pos_vl + direction;
}

void
bb::tape_handler::setup_tape(std::unique_ptr< tape_unit > rhs)
{
  __tape = std::move(rhs);
  __pos_vl = 0;
}

std::unique_ptr< bb::tape_unit >
bb::tape_handler::release_tape()
{
  return std::move(__tape);
}

bool
bb::tape_handler::is_available() const
{
  return (__tape) ? false : true;
}

std::size_t
bb::tape_handler::get_pos_vl() const
{
  return __pos_vl;
}

std::size_t
bb::tape_handler::size() const
{
  return __tape->size();
}
