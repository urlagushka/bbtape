#ifndef BBTAPE_TAPE_HANDLER_HPP
#define BBTAPE_TAPE_HANDLER_HPP

#include <cstddef>
#include <memory>
#include <mutex>
#include <fstream>

#include <bbtape/config.hpp>
#include <bbtape/utils.hpp>
#include <bbtape/unit.hpp>
#include <bbtape/json.hpp>

namespace
{
  void
  verify_tape_field(const nlohmann::json & file)
  {
    if (!file.contains("tape"))
    {
      throw std::runtime_error("source file is bad! (field tape missed)");
    }

    if (!file["tape"].is_array())
    {
      throw std::runtime_error("source file is bad! (field tape must be array)");
    }
  }
}

namespace bb
{
  template< unit_type T >
  class tape_handler
  {
    public:
      tape_handler() = delete;
      tape_handler(config ft_config);

      T read();
      void write(T new_data);
      void roll(std::size_t new_pos);
      void offset(int direction);
      void offset_if_possible(int direction);

      void setup_tape(std::unique_ptr< unit< T > > rhs);
      std::unique_ptr< unit< T > > release_tape();
      bool is_available() const;

      void take();
      void free();
      bool is_reserved() const;

      std::size_t get_pos_vl() const;
      std::size_t size() const;

    private:
      std::unique_ptr< std::mutex > __mutex;

      std::unique_ptr< unit< T > > __tape;
      std::size_t __pos;

      std::size_t __delay_on_read;
      std::size_t __delay_on_write;
      std::size_t __delay_on_roll;
      std::size_t __delay_on_offset;

      bool __is_reserved;
  };

  template< unit_type T >
  unit< T >
  read_tape_from_file(const fs::path & path);

  template< unit_type T >
  void
  write_tape_to_file(const fs::path & path, const unit< T > & rhs);
}

template< bb::unit_type T >
bb::tape_handler< T >::tape_handler(config rhs):
  __mutex(std::make_unique< std::mutex >()),
  __tape(nullptr),
  __pos(0),

  __delay_on_read(rhs.delay.on_read),
  __delay_on_write(rhs.delay.on_write),
  __delay_on_roll(rhs.delay.on_roll),
  __delay_on_offset(rhs.delay.on_offset),
  __is_reserved(false)
{}

template< bb::unit_type T >
T
bb::tape_handler< T >::read()
{
  std::lock_guard< std::mutex > lock(*__mutex);
  std::this_thread::sleep_for(std::chrono::milliseconds(__delay_on_read));

  if (!__tape)
  {
    throw std::runtime_error("can't read tape value! (no tape)");
  }
  if (__tape->size() == 0)
  {
    throw std::runtime_error("can't read tape value! (empty tape)");
  }
  if (__pos >= __tape->size())
  {
    throw std::runtime_error("can't read tape value! (bad position)");
  }

  return (*__tape)[__pos];
}

template< bb::unit_type T >
void
bb::tape_handler< T >::write(T new_data)
{
  std::lock_guard< std::mutex > lock(*__mutex);
  std::this_thread::sleep_for(std::chrono::milliseconds(__delay_on_write));

  if (!__tape)
  {
    throw std::runtime_error("can't write tape value! (no tape)");
  }
  if (__tape->size() == 0)
  {
    throw std::runtime_error("can't write tape value! (empty tape)");
  }
  if (__pos >= __tape->size())
  {
    throw std::runtime_error("can't write tape value! (bad position)");
  }

  (*__tape)[__pos] = new_data;
}

template< bb::unit_type T >
void
bb::tape_handler< T >::roll(std::size_t new_pos)
{
  std::lock_guard< std::mutex > lock(*__mutex);
  std::this_thread::sleep_for(std::chrono::milliseconds(__delay_on_roll));

  if (new_pos > __tape->size())
  {
    throw std::runtime_error("can't roll tape! (new position is greater than tape size)");
  }

  __pos = new_pos;
}

template< bb::unit_type T >
void
bb::tape_handler< T >::offset(int direction)
{
  std::lock_guard< std::mutex > lock(*__mutex);
  std::this_thread::sleep_for(std::chrono::milliseconds(__delay_on_offset));

  if (__pos == 0 && direction < 0)
  {
    throw std::runtime_error("can't offset tape! (new position is less than 0)");
  }
  if (__pos == __tape->size() - 1 && direction > 0)
  {
    throw std::runtime_error("can't offset tape! (new position is greater than tape size)");
  }

  __pos = __pos + direction;
}

template< bb::unit_type T >
void
bb::tape_handler< T >::offset_if_possible(int direction)
{
  std::lock_guard< std::mutex > lock(*__mutex);
  std::this_thread::sleep_for(std::chrono::milliseconds(__delay_on_offset));

  if (__pos == 0 && direction < 0)
  {
    return;
  }
  if (__pos == __tape->size() - 1 && direction > 0)
  {
    return;
  }

  __pos = __pos + direction;
}

template< bb::unit_type T >
void
bb::tape_handler< T >::setup_tape(std::unique_ptr< unit< T > > rhs)
{
  std::lock_guard< std::mutex > lock(*__mutex);
  __tape = std::move(rhs);
  __pos = 0;
}

template< bb::unit_type T >
std::unique_ptr< bb::unit< T > >
bb::tape_handler< T >::release_tape()
{
  std::lock_guard< std::mutex > lock(*__mutex);
  return std::move(__tape);
}

template< bb::unit_type T >
bool
bb::tape_handler< T >::is_available() const
{
  return (__tape) ? false : true;
}

template< bb::unit_type T >
void
bb::tape_handler< T >::take()
{
  std::lock_guard< std::mutex > lock(*__mutex);

  if (__is_reserved || !is_available())
  {
    throw std::runtime_error("can not take tape handler!");
  }

  __is_reserved = true;
}

template< bb::unit_type T >
void
bb::tape_handler< T >::free()
{
  std::lock_guard< std::mutex > lock(*__mutex);
  if (!is_available())
  {
    throw std::runtime_error("can not free tape handler!");
  }

  __is_reserved = false;
}

template< bb::unit_type T >
bool
bb::tape_handler< T >::is_reserved() const
{
  return __is_reserved;
}

template< bb::unit_type T >
std::size_t
bb::tape_handler< T >::get_pos_vl() const
{
  return __pos;
}

template< bb::unit_type T >
std::size_t
bb::tape_handler< T >::size() const
{
  return __tape->size();
}

template< bb::unit_type T >
bb::unit< T >
bb::read_tape_from_file(const fs::path & path)
{
  utils::verify_file_path(path);

  std::ifstream in(path);
  nlohmann::json tmp;
  in >> tmp;
  in.close();

  verify_tape_field(tmp);

  unit< T > valid_tape;
  std::copy(tmp["tape"].begin(), tmp["tape"].end(), std::back_inserter(valid_tape));

  return valid_tape;
}

template< bb::unit_type T >
void
bb::write_tape_to_file(const fs::path & path, const unit< T > & rhs)
{
  utils::verify_file_path(path);

  std::ofstream out(path);
  nlohmann::json tmp = {{"tape", rhs}};
  out << tmp.dump(2);
}

#endif
