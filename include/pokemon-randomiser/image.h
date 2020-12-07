#if !defined(POKEMON_RANDOMISER_IMAGE_H)
#define POKEMON_RANDOMISER_IMAGE_H

#include <pokemon-randomiser/view.h>

namespace gameboy {
namespace rom {
template <typename B = uint8_t, typename W = uint16_t>
class image {
 public:
  using pointer = gameboy::rom::pointer<B, W>;
  using view = gameboy::rom::view<B, W>;

  image(const std::string &file) : loadOK(load(file)) {}

  bool load(const std::string &file) {
    bool ok = false;

    std::ifstream rom(std::string(file), std::ios::in | std::ios::binary);

    data = std::vector<B>((std::istreambuf_iterator<char>(rom)),
                          std::istreambuf_iterator<char>());

    return (ok = data.size() > 0);
  }

  bool save(const std::string &file) const {
    std::ofstream rom(file, std::ios::binary | std::ios::ate);
    std::streamsize size = data.size();

    return bool(rom.write((char *)data.data(), size));
  }

  operator bool(void) { return loadOK; }

  std::vector<B> data;

  const std::size_t size(void) const { return data.size(); }

  operator view(void) const { return view{data}; }

 protected:
  bool loadOK;
};
}  // namespace rom
}  // namespace gameboy

#endif
