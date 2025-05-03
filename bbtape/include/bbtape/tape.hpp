#ifndef BBTAPE_TAPE_HPP
#define BBTAPE_TAPE_HPP

#include <mutex>
#include <list>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <bbtape/files.hpp>

namespace bb
{
  class tape_handler
  {
    public:
      tape_handler() = delete;
      tape_handler(config ft_config);

      int32_t read();
      void write(int32_t new_data);
      void roll(std::size_t new_pos);
      void offset(int direction);

      void setup_tape(std::unique_ptr< tape_unit > rhs);
      std::unique_ptr< tape_unit > release_tape();
      bool is_available() const;

      std::size_t get_pos_vl() const;
      std::size_t size() const;

    private:
      std::mutex __tape_mutex;

      std::unique_ptr< tape_unit > __tape;
      std::size_t __pos_vl;

      std::size_t __delay_on_read;
      std::size_t __delay_on_write;
      std::size_t __delay_on_roll;
      std::size_t __delay_on_offset;
  };
}

#endif
