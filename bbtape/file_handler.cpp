#include <bbtape/file_handler.hpp>

#include <bbtape/utils.hpp>

bb::file_handler::file_handler(file_handler && rhs):
  __files(rhs.__files)
{
  rhs.__files = {};
}

bb::file_handler &
bb::file_handler::operator=(file_handler && rhs)
{
  if (std::addressof(rhs) == this)
  {
    return * this;
  }

  for (auto & file : __files)
  {
    if (fs::exists(file))
    {
      utils::remove_file(file);
    }
  }

  __files = rhs.__files;
  rhs.__files = {};

  return * this;
}

bb::file_handler::~file_handler()
{
  for (auto & file : __files)
  {
    if(fs::exists(file))
    {
      utils::remove_file(file);
    }
  }
}

void
bb::file_handler::push_back(const fs::path & path)
{
  __files.push_back(path);
}

bb::fs::path &
bb::file_handler::operator[](std::size_t i)
{
  return __files[i];
}

const bb::fs::path &
bb::file_handler::operator[](std::size_t i) const
{
  return __files[i];
}

std::size_t
bb::file_handler::size() const
{
  return __files.size();
}
