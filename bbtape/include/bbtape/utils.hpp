#ifndef BBTAPE_UTILS_HPP
#define BBTAPE_UTILS_HPP

#include <filesystem>
#include <vector>

namespace bb::utils
{
  namespace fs = std::filesystem;

  class tmp_file_handler
  {
    public:
      tmp_file_handler() = default;
      tmp_file_handler(const tmp_file_handler &) = delete;
      tmp_file_handler(tmp_file_handler &&) = default;
      tmp_file_handler & operator=(const tmp_file_handler &) = delete;
      tmp_file_handler & operator=(tmp_file_handler &&) = default;
      ~tmp_file_handler();

      fs::path create_file();
      void delete_file(const fs::path & path);

      void push_back(const fs::path & path);
      fs::path & operator[](std::size_t i);

    private:
      std::vector< fs::path > __files;
  };

  fs::path
  create_tmp_file();

  void
  remove_file(const fs::path & path);
}

#endif
