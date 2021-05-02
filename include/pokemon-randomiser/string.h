#if !defined(POKEMON_RANDOMISER_STRING_H)
#define POKEMON_RANDOMISER_STRING_H

#include <pokemon-randomiser/character-map.h>
#include <pokemon-randomiser/view.h>

#include <set>
#include <string>

namespace gameboy {
namespace rom {
template <typename B = uint8_t, typename W = uint16_t>
class string : gameboy::rom::view<B, W> {
 public:
  using pointer = gameboy::rom::pointer<B, W>;
  using view = gameboy::rom::view<B, W>;

  string(view v) : view{v} {}

  const std::string translated(void) const {
    std::string rv{};

    for (const auto b : *this) {
      if (b == pokemon::text::bgry::end) {
        break;
      }

      if (pokemon::text::bgry::english.count(b) == 0) {
        break;
      }

      const std::string_view v = pokemon::text::bgry::english.at(b);

      if (v.empty()) {
        break;
      }

      rv += v;
    }

    return rv;
  }

  const std::set<pointer> scan(void) const {
    std::set<pointer> rv{};
    pointer start = view::start_, cur = view::start_;

    std::size_t length = 0, text = 0;

    for (const auto b : *this) {
      if (b == 0 || pokemon::text::bgry::english.count(b) == 0 ||
          b == pokemon::text::bgry::end) {
        if (text > 4 && text * 12 / 11 < length) {
          rv.insert(start);
        }

        length = 0;
        text = 0;
      } else {
        length++;
        if (pokemon::text::bgry::isText(b)) {
          text++;
        }
      }

      cur++;

      if (length == 0) {
        start = cur;
      }
    }

    return rv;
  }
};
}  // namespace rom
}  // namespace gameboy

#endif
