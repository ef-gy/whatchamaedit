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
        entry{view::from(start).length(0x0004)},
        logo{view::after(entry).to(0x0133)},
        title{view::after(logo).to(0x0143)},
        manufacturer{view::from(0x013f).to(0x0142)},
        gbcolor{view::from(0x0143).length(0x0001)},
        licensee{view::after(title).length(0x0002)},
        supergb{view::after(licensee).length(0x0001)},
        cartridge{view::after(supergb).length(0x0001)},
        rom{view::after(cartridge).length(0x0001)},
        ram{view::after(rom).length(0x0001)},
        region{view::after(ram).length(0x0001)},
        oldLicensee{view::after(region).length(0x0001)},
        version{view::after(oldLicensee).length(0x0001)},
        headerChecksum{view::after(version).length(0x0001)},
        globalChecksum{view::after(headerChecksum).length(0x0002)} {}

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
  view headerChecksum;
  view globalChecksum;

  operator bool(void) const {
    return view::from(start).to(end).isFullCover(entry, globalChecksum);
  }
};
}  // namespace rom
}  // namespace gameboy

#endif
