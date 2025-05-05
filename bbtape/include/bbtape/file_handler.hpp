#ifndef BBTAPE_FILE_HANDLER_HPP
#define BBTAPE_FILE_HANDLER_HPP

#include <filesystem>
#include <vector>

namespace bb
{
  namespace fs = std::filesystem;

  class file_handler
  {
    public:
      file_handler() = default;
      file_handler(const file_handler &) = delete;
      file_handler(file_handler &&);
      file_handler & operator=(const file_handler &) = delete;
      file_handler & operator=(file_handler &&);
      ~file_handler();

      void push_back(const fs::path & path);

      fs::path & operator[](std::size_t i);
      const fs::path & operator[](std::size_t i) const;

      std::size_t size() const;

    private:
      std::vector< fs::path > __files;
  };
}

#endif
