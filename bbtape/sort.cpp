#include <bbtape/sort.hpp>

#include <cstdint>
#include <memory>
#include <iostream>

#include <bbtape/details.hpp>
#include <bbtape/physical.hpp>

std::filesystem::path
bb::external_merge_sort(config ft_config, const std::filesystem::path & src)
{
  auto src_tape = std::make_unique< tape_unit >(read_tape_from_file(src));
  std::size_t chunk_size = ((src_tape->size() * sizeof(uint32_t)) % ft_config.phlimit.ram) / 2;
  bb::physical::ram tmp_ram = {{}, chunk_size, sizeof(uint32_t)};

  tape_handler tph(ft_config);
  details::tmp_file_handler tfh;

  tape_unit dst_tape;
  std::size_t pos_vl = 0;
  while (pos_vl < src_tape->size())
  {
    tmp_ram.buffer.clear();
    tph.setup_tape(std::move(src_tape));
    tph.roll(pos_vl);
    for (std::size_t i = pos_vl; i < pos_vl + chunk_size; ++i)
    {
      tmp_ram.buffer.push_back(tph.read());
      tph.offset(1);
    }
    src_tape = tph.release_tape();

    auto tmp_tape = std::make_unique< tape_unit >(tape_unit(chunk_size));
    tph.setup_tape(std::move(tmp_tape));
    for (std::size_t i = 0; i < chunk_size; ++i)
    {
      tph.write(tmp_ram.buffer[i]);
      tph.offset(1);
    }
    std::cout << tfh.create_file() << "\n";

    pos_vl = pos_vl + chunk_size;
  }
}
