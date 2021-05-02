#if !defined(WHATCHAMAEDIT_HEADER_H)
#define WHATCHAMAEDIT_HEADER_H

#include <string.h>
#include <whatchamaedit/view.h>

#include <functional>
#include <numeric>

namespace gameboy {
namespace rom {
template <typename B = uint8_t, typename W = uint16_t>
class header : public view<B, W> {
 public:
  using view = view<B, W>;
  using pointer = typename view::pointer;

 protected:
  static constexpr pointer start{0x0100};
  static constexpr pointer end{0x014f};

 public:
  /** @constructor
   *
   * @v a view over the whole ROM for which to initialise this ROM header from.
   *
   * The given view has to be over the full ROM because this object also
   * calculates checksums to see if the ROM was read properly and hasn't been
   * tampered with by tools that don't update checksums. This is slightly
   * counter-intuitive but this feels like the best place to put the checksum
   * calculations.
   *
   * All other fields are initialised with a view recipe for mapping ROM header
   * locations to their fields, and can therefore be read directly.
   */
  constexpr header(view v)
      : view{v.asLittleEndian()},
        entry{view::from(start).length(0x0004).expect(dt_code).label(
            "__gb_entry_point")},
        logo{view::after(entry).to(0x0133).is(dt_bytes).label(
            "__gb_nintendo_logo")},
        title{view::after(logo).to(0x0143).is(dt_text).label(
            "__gb_cartridge_title")},
        manufacturer{view::from(0x013f).to(0x0142).is(dt_text).label(
            "__gb_manufacturer_code")},
        gbcolor{view::from(0x0143).asByte().label("__gb_color_flags")},
        licensee{view::after(title).is(dt_text).length(0x0002).label(
            "__gb_licensee_code")},
        supergb{view::after(licensee).asByte().label("__gb_super_flags")},
        cartridge{view::after(supergb).asByte().label("__gb_cartridge_flags")},
        rom{view::after(cartridge).asByte().label("__gb_rom_size_flags")},
        ram{view::after(rom).asByte().label("__gb_ram_size_flags")},
        region{view::after(ram).asByte().label("__gb_region_code")},
        oldLicensee{
            view::after(region).asByte().label("__gb_old_licensee_code")},
        version{
            view::after(oldLicensee).asByte().label("__gb_cartridge_version")},
        headerChecksum_{
            view::after(version).asByte().label("__gb_header_chksum")},
        globalChecksum_{view::after(headerChecksum_)
                            .asWord()
                            .asBigEndian()
                            .label("__gb_global_chksum")} {}

  constexpr B checksumH(bool calculate) const {
    if (calculate) {
      // the checksum is a check-difference, with an extra -1 per item, thus the
      // custom lambda to calculate it.
      return std::accumulate(headerChecksumRange().begin(),
                             headerChecksumRange().end(), W(0),
                             [](const W a, const W b) { return a - b - 1; });
    }

    return headerChecksum_.byte();
  }

  constexpr W checksumR(bool calculate) const {
    if (calculate) {
      // the checksum is a literal sum, except we need to skip the recorded
      // checksum, so we divide it up into two ranges and accumulate with the
      // standard library.
      return std::accumulate(globalChecksumHeaderRange().begin(),
                             globalChecksumHeaderRange().end(), W(0)) +
             std::accumulate(globalChecksumDataRange().begin(),
                             globalChecksumDataRange().end(), W(0));
    }

    // read current ROM checksum from header
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

  constexpr operator bool(void) const {
    return view(*this) && view::check(fields()) &&
           checksumH(true) == checksumH(false) &&
           checksumR(true) == checksumR(false);
  }

  constexpr std::array<view, 14> fields(void) const {
    return {entry,
            logo,
            title,
            manufacturer,
            gbcolor,
            licensee,
            supergb,
            cartridge,
            rom,
            ram,
            oldLicensee,
            version,
            headerChecksum_,
            globalChecksum_};
  }

 protected:
  view headerChecksum_;
  view globalChecksum_;

  constexpr view headerChecksumRange(void) const {
    return view::after(logo).before(headerChecksum_);
  }

  constexpr view globalChecksumHeaderRange(void) const {
    return view::start().before(globalChecksum_);
  }

  constexpr view globalChecksumDataRange(void) const {
    return view::after(globalChecksum_);
  }
};
}  // namespace rom
}  // namespace gameboy

#endif
