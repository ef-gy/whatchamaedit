#if !defined(POKEMON_RANDOMISER_HEADER_H)
#define POKEMON_RANDOMISER_HEADER_H

#include <pokemon-randomiser/view.h>
#include <string.h>

namespace gameboy {
namespace rom {
template <typename B = uint8_t, typename W = uint16_t>
class header : view<B, W> {
 public:
  using view = view<B, W>;
  using pointer = typename view::pointer;

 protected:
  const pointer start{0x0100};
  const pointer end{0x014f};

 public:
  header(view v)
      : view{v},
        entry{view::from(start).length(0x0004).expect(dt_code)},
        logo{view::after(entry).to(0x0133).is(dt_bytes)},
        title{view::after(logo).to(0x0143).is(dt_text)},
        manufacturer{view::from(0x013f).to(0x0142).is(dt_text)},
        gbcolor{view::from(0x0143).is(dt_byte)},
        licensee{view::after(title).is(dt_text).length(0x0002)},
        supergb{view::after(licensee).is(dt_byte)},
        cartridge{view::after(supergb).is(dt_byte)},
        rom{view::after(cartridge).is(dt_byte)},
        ram{view::after(rom).is(dt_byte)},
        region{view::after(ram).is(dt_byte)},
        oldLicensee{view::after(region).is(dt_byte)},
        version{view::after(oldLicensee).is(dt_byte)},
        headerChecksum_{view::after(version).is(dt_byte)},
        globalChecksum_{
            view::after(headerChecksum_).is(dt_word).expect(e_big_endian)} {}

  B checksumH(bool calculate) const {
    if (calculate) {
      W c = 0;

      for (const auto b : view::after(logo).before(headerChecksum_)) {
        c = c - b - 1;
      }

      return B(c);
    }

    return headerChecksum_.byte();
  }

  W checksumR(bool calculate) const {
    if (calculate) {
      W chk = globalChecksum_.word();
      W c = 0;

      // remove current checksum bytes before a full sum
      c -= chk >> 8;
      c -= chk & 0xff;

      for (const auto b : view(*this)) {
        c += b;
      }

      return c;
    }

    return globalChecksum_.word();
  }

  view entry;
  view logo;
  view title;
  view manufacturer;
  view gbcolor;
  view licensee;
  view supergb;
  view cartridge;
  view rom;
  view ram;
  view region;
  view oldLicensee;
  view version;

  operator bool(void) const {
    return view(*this) &&
           view::from(start).to(end).isFullCover(entry, globalChecksum_) &&
           checksumH(true) == checksumH(false) &&
           checksumR(true) == checksumR(false);
  }

 protected:
  view headerChecksum_;
  view globalChecksum_;
};
}  // namespace rom
}  // namespace gameboy

#endif
