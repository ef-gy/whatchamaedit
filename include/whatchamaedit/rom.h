#if !defined(WHATCHAMAEDIT_ROM_H)
#define WHATCHAMAEDIT_ROM_H

#include <whatchamaedit/header.h>
#include <whatchamaedit/image.h>
#include <whatchamaedit/string.h>

#include <sstream>

namespace whatchamaedit {
namespace rom {
template <typename B = char>
class gb : public gameboy::rom::image<> {
 public:
  using image = gameboy::rom::image<>;
  using pointer = typename image::pointer;
  using view = typename image::view;
  using string = gameboy::rom::string<>;

  gb(const std::string &file) : image(file), header{*this} {}

  std::string getString(long start, long end) const {
    return string{view{*this}.from(start).to(end)}.translated();
  }

  std::map<pointer, std::string> getStrings(void) const {
    std::map<pointer, std::string> rv;

    for (const auto &p : string{view{*this}}.scan()) {
      rv[p] = string{view{*this}.from(p)}.translated();
    }

    return rv;
  }

  std::string title(void) const { return std::string(header.title); }

  long romChecksum(void) const { return header.checksumR(true); }

  long headerChecksum(void) const { return header.checksumR(false); }

  bool checksum(void) const {
    return header.checksumR(true) == header.checksumR(false);
  }

  bool fixChecksum(void) {
    uint16_t checksum = romChecksum();

    static const long high = 0x14e;
    static const long low = 0x14f;

    data_[high] = checksum >> 8;
    data_[low] = checksum & 0xff;

    return this->checksum();
  }

  gameboy::rom::header<> header;

  operator bool(void) const { return loadOK && header; }
};
}  // namespace rom
}  // namespace whatchamaedit

#endif
