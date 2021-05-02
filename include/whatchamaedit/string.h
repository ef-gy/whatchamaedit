#if !defined(WHATCHAMAEDIT_STRING_H)
#define WHATCHAMAEDIT_STRING_H

#include <whatchamaedit/character-map.h>
#include <whatchamaedit/view.h>

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
      if (b == text::pokemon::bgry::end) {
        break;
      }

      if (text::pokemon::bgry::english.count(b) == 0) {
        break;
      }

      const std::string_view v = text::pokemon::bgry::english.at(b);

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
      if (b == 0 || text::pokemon::bgry::english.count(b) == 0 ||
          b == text::pokemon::bgry::end) {
        if (text > 4 && text * 12 / 11 < length) {
          rv.insert(start);
        }

        length = 0;
        text = 0;
      } else {
        length++;
        if (text::pokemon::bgry::isText(b)) {
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
