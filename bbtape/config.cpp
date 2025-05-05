#include <bbtape/config.hpp>

#include <fstream>

#include <bbtape/json.hpp>

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
    if (!file["physical_limit"].contains("conv"))
    {
      throw std::runtime_error("source file is bad! (field physical_limit.conv missed)");
    }

    if (!file["physical_limit"]["ram"].is_number_integer())
    {
      throw std::runtime_error("source file is bad! (field physical_limit.ram must be integer number)");
    }
    if (!file["physical_limit"]["conv"].is_number_integer())
    {
      throw std::runtime_error("source file is bad! (field physical_limit.conv must be integer number)");
    }
  }
}

bb::config
bb::read_config_from_file(const fs::path & path)
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
    tmp["physical_limit"]["ram"],
    tmp["physical_limit"]["conv"]
  };

  return valid_config;
}
