#include <bbtape/files.hpp>
#include <bbtape/json.hpp>

#include <fstream>
#include <stdexcept>

namespace
{
  void
  verify_delay_field(const nlohmann::json & file)
  {
    if (!file.contains("delay"))
    {
      throw std::runtime_error("source file is bad! (field delay missed)");
    }

    if (!file["delay"].contains("on_read"))
    {
      throw std::runtime_error("source file is bad! (field delay.on_read missed)");
    }
    if (!file["delay"].contains("on_write"))
    {
      throw std::runtime_error("source file is bad! (field delay.on_write missed)");
    }
    if (!file["delay"].contains("on_roll"))
    {
      throw std::runtime_error("source file is bad! (field delay.on_roll missed)");
    }
    if (!file["delay"].contains("on_offset"))
    {
      throw std::runtime_error("source file is bad! (field delay.on_offset missed)");
    }

    if (!file["delay"]["on_read"].is_number_integer())
    {
      throw std::runtime_error("source file is bad! (field delay.on_read must be integer number)");
    }
    if (!file["delay"]["on_write"].is_number_integer())
    {
      throw std::runtime_error("source file is bad! (field delay.on_write must be integer number)");
    }
    if (!file["delay"]["on_roll"].is_number_integer())
    {
      throw std::runtime_error("source file is bad! (field delay.on_roll must be integer number)");
    }
    if (!file["delay"]["on_offset"].is_number_integer())
    {
      throw std::runtime_error("source file is bad! (field delay.on_offset must be integer number)");
    }
  }

  void
  verify_phlimit_field(const nlohmann::json & file)
  {
    if (!file.contains("physical_limit"))
    {
      throw std::runtime_error("source file is bad! (field physical_limit missed)");
    }

    if (!file["physical_limit"].contains("ram"))
    {
      throw std::runtime_error("source file is bad! (field physical_limit.ram missed)");
    }

    if (!file["physical_limit"]["ram"].is_number_integer())
    {
      throw std::runtime_error("source file is bad! (field physical_limit.ram must be integer number)");
    }
  }

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

  void
  verify_source_file(const nlohmann::json & file)
  {
    verify_delay_field(file);
    verify_phlimit_field(file);
    verify_tape_field(file);
  }
}

std::filesystem::path
bb::get_config_path_with_verify(std::string_view path)
{
  std::ifstream tmp(path);
  if (!tmp.is_open())
  {
    throw std::runtime_error("source file is unavailable!");
  }
  tmp.close();

  std::filesystem::path valid_path = path;
  if (valid_path.extension() != ".json")
  {
    throw std::runtime_error("source file extension is bad! (.json required)");
  }

  return valid_path;
}

bb::config
bb::read_config_from_file(const std::filesystem::path & path)
{
  std::ifstream in(path);
  nlohmann::json tmp;
  in >> tmp;
  in.close();

  verify_delay_field(tmp);
  verify_phlimit_field(tmp);

  config valid_config;

  valid_config.delay = {
    tmp["delay"]["on_read"],
    tmp["delay"]["on_write"],
    tmp["delay"]["on_roll"],
    tmp["delay"]["on_offset"]
  };

  valid_config.phlimit = {
    tmp["physical_limit"]["ram"]
  };

  return valid_config;
}

bb::tape_unit
bb::read_tape_from_file(const std::filesystem::path & path)
{
  std::ifstream in(path);
  nlohmann::json tmp;
  in >> tmp;
  in.close();

  verify_tape_field(tmp);

  tape_unit valid_tape;
  std::copy(tmp["tape"].begin(), tmp["tape"].end(), std::back_inserter(valid_tape));

  return valid_tape;
}

void
bb::dump_tape(std::ofstream & out, const tape_unit & rhs)
{
  nlohmann::json tmp = {{"tape", rhs}};
  out << tmp.dump(2);
}
