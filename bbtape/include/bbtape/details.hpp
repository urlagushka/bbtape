#ifndef BBTAPE_DETAILS_HPP
#define BBTAPE_DETAILS_HPP

#include <filesystem>
#include <vector>

namespace bb::details
{
  class tmp_file_handler
  {
    public:
      tmp_file_handler() = default;
      tmp_file_handler(const tmp_file_handler &) = delete;
      tmp_file_handler(tmp_file_handler &&) = default;
      tmp_file_handler & operator=(const tmp_file_handler &) = delete;
      tmp_file_handler & operator=(tmp_file_handler &&) = default;
      ~tmp_file_handler();

      std::filesystem::path create_file();
      void delete_file(const std::filesystem::path & file);

    private:
      std::vector< std::filesystem::path > __files;
  };
}

#endif
