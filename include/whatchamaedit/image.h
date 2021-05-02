#if !defined(WHATCHAMAEDIT_IMAGE_H)
#define WHATCHAMAEDIT_IMAGE_H

#include <whatchamaedit/view.h>

namespace gameboy {
namespace rom {
template <std::size_t count, typename B = uint8_t, typename W = uint16_t>
class blank : public gameboy::rom::generic::blank<count, B, W> {
 public:
  constexpr bool load(const std::string &) const { return false; }
  constexpr bool save(const std::string &) const { return false; }
};

template <typename B = uint8_t, typename W = uint16_t>
class image {
 public:
  using pointer = gameboy::rom::pointer<B, W>;
  using view = gameboy::rom::view<B, W>;

  image(const std::string &file) : data_{}, loadOK(load(file)) {}

  bool load(const std::string &file) {
    bool ok = false;

    std::ifstream rom(std::string(file), std::ios::in | std::ios::binary);

    data_ = std::vector<B>((std::istreambuf_iterator<char>(rom)),
                           std::istreambuf_iterator<char>());

    return (ok = data_.size() > 0);
  }

  bool save(const std::string &file) const {
    std::ofstream rom(file, std::ios::binary | std::ios::ate);
    std::streamsize size = data_.size();

    return bool(rom.write((char *)data_.data(), size));
  }

  constexpr operator bool(void) const { return loadOK; }

  constexpr std::basic_string_view<B> readonly(void) const {
    return {data_.data(), data_.size()};
  }

  constexpr const std::size_t size(void) const { return readonly().size(); }

  constexpr operator view(void) const { return view{readonly()}; }

 protected:
  std::vector<B> data_;
  bool loadOK;
};
}  // namespace rom
}  // namespace gameboy

#endif
