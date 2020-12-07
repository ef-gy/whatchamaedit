#if !defined(POKEMON_RANDOMISER_ROM_H)
#define POKEMON_RANDOMISER_ROM_H

#include <pokemon-randomiser/character-map.h>
#include <pokemon-randomiser/header.h>

#include <sstream>

namespace pokemon {

static const std::string listPokemon(
    const std::map<std::string, uint8_t> &ids) {
  std::ostringstream os;
  os.clear();

  os << ids.size() << "\n";
  for (const auto &pk : ids) {
    int16_t v = uint8_t(pk.second);
    os << "0x" << std::hex << std::setw(2) << std::setfill('0') << v << " ["
       << pk.first << "]\n";
  }

  return os.str();
}

static const std::string listPokemon(
    const std::map<uint8_t, std::string> &ids) {
  std::ostringstream os;
  os.clear();

  for (const auto &pk : ids) {
    int16_t v = uint8_t(pk.first);
    os << "0x" << std::hex << std::setw(2) << std::setfill('0') << v << " ["
       << pk.second << "]\n";
  }

  return os.str();
}

namespace rom {
template <typename B = char>
class bgry : public gameboy::rom::image<> {
 public:
  using image = gameboy::rom::image<>;

  bgry(const std::string &file) : image(file), header{*this} {}

  std::string getString(long start, long end) const {
    std::string rv = "";

    for (long i = start; i <= end; i++) {
      const int8_t b = byte(i);
      if (b == pokemon::text::bgry::end) {
        // 0x50 is the string terminator symbol (it's NOT 0x00).
        break;

        // TODO: potentially verify invariants of text strings, such as that all
        // characters after 0x50 ought to be blanks in a fixed-width string.
      }

      const std::string v = pokemon::text::bgry::english[b];

      if (v.empty()) {
        break;
      }

      rv += v;
    }

    return rv;
  }

  std::map<long, std::string> getStrings(void) const {
    std::map<long, std::string> rv;
    std::string line = "";
    unsigned long start = 0;
    unsigned long normal = 0;

    for (unsigned long i = 0; i <= size(); i++) {
      uint8_t b = byte(i);
      const std::string v = pokemon::text::bgry::english[b];

      if (v.empty() || b == pokemon::text::bgry::end) {
        if (normal > 3) {
          static const double normalRatio = 0.8;

          if (double(normal) / double(line.size()) >= normalRatio) {
            rv[start] = line;
          }
        }
        line.clear();
        normal = 0;
        start = 0;
      } else {
        if (line.empty()) {
          start = i;
        }
        line += v;

        if (0x80 <= b && b <= 0xbf) {
          normal++;
        }
      }
    }

    return rv;
  }

  const std::string dump(long start, long end, int alignment) const {
    std::ostringstream os;
    os.clear();

    for (long i = start; i <= end; i++) {
      int16_t v = byte(i);

      if ((i - start) % alignment == 0) {
        os << "\n";
      }

      os << " \t0x" << std::hex << std::setw(2) << std::setfill('0') << v;

      if (pokemon::text::bgry::english.count(v) == 1) {
        os << " " << std::setw(4) << std::setfill('.')
           << pokemon::text::bgry::english[v];
      } else {
        os << " ....";
      }
    }

    os << "\n";

    return os.str();
  }

  const uint8_t byte(long start) const { return data[start]; }

  const uint16_t word_be(long start) const {
    return uint16_t(byte(start) << 8 | byte(start + 1));
  }

  const uint16_t word_le(long start) const {
    return uint16_t(byte(start + 1) << 8 | byte(start));
  }

  static const long address(uint8_t bank, uint16_t off) {
    return bank * 0x4000 + off - (off >= 0x4000 ? 0x4000 : 0);
  }

  const long address(const std::pair<uint8_t, uint16_t> &hw) const {
    return address(hw.first, hw.second);
  }

  std::string getPokemonName(uint8_t id) const {
    static long nameBase = 0x1c228;

    return getString(nameBase + (id - 2) * 0x0a,
                     nameBase + (id - 1) * 0x0a - 1);
  }

  const std::set<uint8_t> getTitleScreenPokemon(void) const {
    std::set<uint8_t> rv{};

    for (long i = 0x4588; i < 0x4597; i++) {
      rv.insert(byte(i));
    }

    return rv;
  }

  const std::set<uint8_t> getStarterPokemon(void) const {
    std::set<uint8_t> rv{};

    for (const auto p : getStarterPointers()) {
      for (const auto pn : p.second) {
        rv.insert(byte(pn));
      }
    }

    return rv;
  }

  std::map<long, std::set<long>> getStarterPointers() const {
    std::map<long, std::set<long>> rv{};

    if (title() == "POKEMON RED") {
      // TODO: research these pointers and figure out what they actually belong
      // to, then derive the addresses "properly" - suspect most of these are
      // encounter data.
      rv[0] = {0x1d126, 0x1cc84, 0x1d10E, 0x39cf8, 0x3a1eb, 0x50fb3, 0x510dd};
      rv[1] = {0x1d104, 0x19591, 0x1cc88, 0x1cdc8, 0x1d11f, 0x3a1e5,
               0x50faf, 0x510d9, 0x51caf, 0x6060e, 0x61450, 0x75f9e};
      rv[2] = {0x1d115, 0x19599, 0x1cdd0, 0x1d130, 0x39cf2, 0x3a1e8,
               0x50fb1, 0x510db, 0x51cb7, 0x60616, 0x61458, 0x75fa6};
    }

    return rv;
  }

  std::map<long, std::set<long>> getStarterTextPointers() const {
    std::map<long, std::set<long>> rv{};

    if (title() == "POKEMON RED") {
      rv[0] = {0x94e23};
      rv[1] = {0x94e4d};
      rv[2] = {0x94e69};
    }

    return rv;
  }

  bool setStarterPokemon(const std::set<std::string> &starter) {
    bool ok = true;

    auto ids = getAllPokemonIds();

    long n = 0;
    auto sps = getStarterPointers();
    auto spt = getStarterTextPointers();

    for (const auto st : starter) {
      if (sps.count(n) == 1) {
        for (const auto i : sps[n]) {
          data[i] = ids[st];
        }
      }
      if (spt.count(n) == 1) {
        for (auto p : spt[n]) {
          for (long pn = 0; pn <= 0x9; p++, pn++) {
            uint16_t r = text::bgry::toROMFormat(st[pn]);
            data[p] = r;
          }
        }
      }
      n++;
    }

    long i = 0x4588;
    for (const auto &st : starter) {
      if (i <= 0x458a) {
        data[i] = ids[st];
        i += 1;
      }
    }

    return ok;
  }

  const std::map<uint8_t, std::string> getPokemonNames(
      const std::set<uint8_t> &r) const {
    std::map<uint8_t, std::string> rv{};
    for (uint8_t v : r) {
      rv[v] = getPokemonName(v);
    }
    return rv;
  }

  const std::map<std::string, uint8_t> getPokemonIds(
      const std::set<uint8_t> &r) const {
    std::map<std::string, uint8_t> rv{};
    for (uint8_t v : r) {
      const std::string n = getPokemonName(v);

      if (!n.empty() && n != "MISSINGNO.") {
        rv[n] = v;
      }
    }
    return rv;
  }

  static const constexpr uint8_t pokemonCutoff = 190;

  const std::map<std::string, uint8_t> getAllPokemonIds() const {
    std::set<uint8_t> r{};
    for (unsigned i = 1; i <= pokemonCutoff; i++) {
      r.insert(i);
    }
    return getPokemonIds(r);
  }

  void clearTitleScreenPokemon() {
    long n = 0;
    for (long i = 0x4588; i <= 0x4597; i++) {
      switch (n) {
        case 0:
          data[i] = 0xb1;
          break;
        case 1:
          data[i] = 0x99;
          break;
        case 2:
          data[i] = 0xb0;
          break;
        default:
          data[i] = 0x00;
          break;
      }
      n++;
    }

    data[0x4399] = 0xb1;
  }

  std::string title(void) const { return std::string(header.title); }

  long romChecksum(void) const {
    uint32_t checksum = 0;

    static const long high = 0x14e;
    static const long low = 0x14f;

    for (long i = 0; i < size(); i++) {
      if (i != high && i != low) {
        checksum += byte(i);
      }
    }

    return uint16_t(checksum);
  }

  long headerChecksum(void) const {
    uint16_t checksum = 0;

    static const long high = 0x14e;
    static const long low = 0x14f;

    checksum = uint8_t(data[high]) << 8;
    checksum |= uint8_t(data[low]);

    return checksum;
  }

  bool checksum(void) const { return romChecksum() == headerChecksum(); }

  bool fixChecksum(void) {
    uint16_t checksum = romChecksum();

    static const long high = 0x14e;
    static const long low = 0x14f;

    data[high] = checksum >> 8;
    data[low] = checksum & 0xff;

    return this->checksum();
  }

  gameboy::rom::header<> header;

  operator bool(void) const { return loadOK && header; }
};

}  // namespace rom
}  // namespace pokemon

#endif
