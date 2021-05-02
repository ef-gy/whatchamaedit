#if !defined(WHATCHAMAEDIT_CHARACTER_MAP_H)
#define WHATCHAMAEDIT_CHARACTER_MAP_H

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace text {
using codepoint = unsigned long;
using charmap = const std::map<codepoint, const std::string_view>;

template <typename W>
using rune = std::pair<W, std::string_view>;

namespace classify {
template <typename W>
class generic {
 public:
  using description = std::optional<std::string_view>;

  static constexpr bool defined(description D) { return bool(D); }

  static constexpr description control(description R) {
    if (R) {
      auto rune = *R;

      if (rune.size() <= 2) {
        return {};
      }

      if (rune.front() == '{' && rune.back() == '}') {
        return rune.substr(1, rune.size() - 2);
      }
    }

    return {};
  }

  static constexpr bool nil(description D) {
    return defined(D) && control(D) && *control(D) == "null";
  }

  static constexpr bool special(description D) { return D && control(D); }

  static constexpr bool glyph(description D) { return D && !special(D); }

  static constexpr bool text(description D) {
    if (glyph(D)) {
      for (auto g : *D) {
        if (g >= 'a' && g <= 'z' || g >= 'A' && g <= 'Z' ||
            g >= '0' && g <= '9' || g >= ' ' && g < '@') {
          // yes this is a very oversimplified test for whether or not a
          // character that you look up is "text" - but really this classifier
          // is just a super simple fallback, so this is fine.
          continue;
        } else {
          return false;
        }
      }
    }

    return false;
  }

  static constexpr bool descriptive = true;
};
}  // namespace classify

template <typename W, size_t len, class C = classify::generic<W>>
class code {
 public:
  using description = std::optional<std::string_view>;
  using set = std::array<rune<W>, len>;

  constexpr code(set s) : data_{s} {}

  constexpr description bisect(W rune, W start, W end) const {
    if (start > end) {
      return {};
    }

    W midpoint = start + (end - start) / 2;

    if (data_[midpoint].first == rune) {
      return data_[midpoint].second;
    } else if (rune < data_[midpoint].first) {
      return bisect(rune, start, midpoint - 1);
    } else {  // rune > *midpoint
      return bisect(rune, midpoint + 1, end);
    }

    return {};
  }

  constexpr description describe(W rune) const {
    return bisect(rune, 0, data_.size() - 1);
  }

  constexpr bool defined(W rune) const {
    if constexpr (C::descriptive) {
      return C::defined(describe(rune));
    } else {
      return C::defined(rune);
    }
  }

  constexpr bool control(W rune) const {
    if constexpr (C::descriptive) {
      return C::control(describe(rune));
    } else {
      return C::control(rune);
    }
  }

  constexpr bool nil(W rune) const {
    if constexpr (C::descriptive) {
      return C::nil(describe(rune));
    } else {
      return C::nil(rune);
    }
  }

  constexpr bool special(W rune) const {
    if constexpr (C::descriptive) {
      return C::special(describe(rune));
    } else {
      return C::special(rune);
    }
  }

  constexpr bool glyph(W rune) const {
    if constexpr (C::descriptive) {
      return C::glyph(describe(rune));
    } else {
      return C::glyph(rune);
    }
  }

  constexpr bool text(W rune) const {
    if constexpr (C::descriptive) {
      return C::text(describe(rune));
    } else {
      return C::text(rune);
    }
  }

 protected:
  set data_;
};

namespace encoding {

static constexpr const code<unsigned long, 0x80> ascii{{
    // 0x00 - 0x1f: control characters
    rune<unsigned long>{0x00, "{null}"},
    {0x01, "{start of heading}"},
    {0x02, "{start of text}"},
    {0x03, "{end of text}"},
    {0x04, "{end of transmission}"},
    {0x05, "{enquiry}"},
    {0x06, "{acknowledge}"},
    {0x07, "{bell}"},
    {0x08, "{backspace}"},
    {0x09, "{horizontal tab}"},
    {0x0a, "{new line}"},
    {0x0b, "{vertical tab}"},
    {0x0c, "{new page}"},
    {0x0d, "{carriage return}"},
    {0x0e, "{shift out}"},
    {0x0f, "{shift in}"},
    {0x10, "{data link escape}"},
    {0x11, "{device control 1}"},
    {0x12, "{device control 2}"},
    {0x13, "{device control 3}"},
    {0x14, "{device control 4}"},
    {0x15, "{negative acknowledge}"},
    {0x16, "{synchronous idle}"},
    {0x17, "{end of transmission block}"},
    {0x18, "{cancel}"},
    {0x19, "{end of medium}"},
    {0x1a, "{substitute}"},
    {0x1b, "{escape}"},
    {0x1c, "{file separator}"},
    {0x1d, "{group separator}"},
    {0x1e, "{record separator}"},
    {0x1f, "{unit separator}"},

    // 0x20 - 0x40: punctuation and numbers
    {0x20, " "},
    {0x21, "!"},
    {0x22, "\""},
    {0x23, "#"},
    {0x24, "$"},
    {0x25, "%"},
    {0x26, "&"},
    {0x27, "'"},
    {0x28, "("},
    {0x29, ")"},
    {0x2a, "*"},
    {0x2b, "+"},
    {0x2c, ","},
    {0x2d, "-"},
    {0x2e, "."},
    {0x2f, "/"},
    {0x30, "0"},
    {0x31, "1"},
    {0x32, "2"},
    {0x33, "3"},
    {0x34, "4"},
    {0x35, "5"},
    {0x36, "6"},
    {0x37, "7"},
    {0x38, "8"},
    {0x39, "9"},
    {0x3a, ":"},
    {0x3b, ";"},
    {0x3c, "<"},
    {0x3d, "="},
    {0x3e, ">"},
    {0x3f, "?"},
    {0x40, "@"},

    // 0x41 - 0x5a: uppercase letters
    {0x41, "A"},
    {0x42, "B"},
    {0x43, "C"},
    {0x44, "D"},
    {0x45, "E"},
    {0x46, "F"},
    {0x47, "G"},
    {0x48, "H"},
    {0x49, "I"},
    {0x4a, "J"},
    {0x4b, "K"},
    {0x4c, "L"},
    {0x4d, "M"},
    {0x4e, "N"},
    {0x4f, "O"},
    {0x50, "P"},
    {0x51, "Q"},
    {0x52, "R"},
    {0x53, "S"},
    {0x54, "T"},
    {0x55, "U"},
    {0x56, "V"},
    {0x57, "W"},
    {0x58, "X"},
    {0x59, "Y"},
    {0x5a, "Z"},

    // 0x5b - 0x60: special characters
    {0x5b, "["},
    {0x5c, "\\"},
    {0x5d, "]"},
    {0x5e, "^"},
    {0x5f, "_"},
    {0x60, "`"},

    // 0x61 - 0x7a: lowercase letters
    {0x61, "a"},
    {0x62, "b"},
    {0x63, "c"},
    {0x64, "d"},
    {0x65, "e"},
    {0x66, "f"},
    {0x67, "g"},
    {0x68, "h"},
    {0x69, "i"},
    {0x6a, "j"},
    {0x6b, "k"},
    {0x6c, "l"},
    {0x6d, "m"},
    {0x6e, "n"},
    {0x6f, "o"},
    {0x70, "p"},
    {0x71, "q"},
    {0x72, "r"},
    {0x73, "s"},
    {0x74, "t"},
    {0x75, "u"},
    {0x76, "v"},
    {0x77, "w"},
    {0x78, "x"},
    {0x79, "y"},
    {0x7a, "z"},

    // 0x7b - 0x7e: special characters
    {0x7b, "{"},
    {0x7c, "|"},
    {0x7d, "}"},
    {0x7e, "~"},

    // 0x7f: delete
    {0x7f, "{delete}"},
}};

static_assert(ascii.nil(0), "ASCII char 0 is nil");
static_assert(ascii.special(0x1f) && !ascii.glyph(0x1f),
              "ASCII 0x1f is a special character");
static_assert(ascii.special(0x7f) && !ascii.glyph(0x7f),
              "ASCII 0x7f is a special character");
static_assert(!ascii.special(0x64) && ascii.glyph(0x64),
              "ASCII 0x64 is a printable glyph");
static_assert(!ascii.defined(0x80), "ASCII is only valid betweem 0x00 - 0x7f");
}  // namespace encoding

namespace pokemon {
namespace bgry {

/* The BGRY section used Bulbapedia extensively for character translations.
 *
 * See this link for ALL the detail:
 * https://bulbapedia.bulbagarden.net/wiki/Character_encoding_in_Generation_I
 */

static const constexpr uint8_t end = 0x50;

static charmap english({
    // NULL
    {0x00, ""},

    // 0x01 - 0xa7: non-text data

    // 0xa9 - 0x5f: dialogue control codes
    {0x49, "{page+}"},
    {0x4b, "{_cont}"},
    {0x4c, "{autocont}"},
    {0x4e, "{line+}"},
    {0x4f, "{line++}"},  // move to bottom line
    {0x50, "{end}"},
    {0x51, "{para}"},
    {0x55, "{+cont}"},
    {0x57, "{done}"},
    {0x58, "{$prompt}"},
    {0x5f, "{dex-}"},

    // variable control codes
    {0x52, "{player}"},
    {0x53, "{rival}"},
    {0x59, "{target}"},
    {0x5a, "{user}"},

    // always this text
    {0x54, "POKé"},

    // 0x60 - 0x7f: mirror characters
    {0x60, "A"},
    {0x61, "B"},
    {0x62, "C"},
    {0x63, "D"},
    {0x64, "E"},
    {0x65, "F"},
    {0x66, "G"},
    {0x67, "H"},
    {0x68, "I"},
    {0x69, "V"},
    {0x6a, "S"},
    {0x6b, "L"},
    {0x6c, "M"},
    {0x6d, ":"},
    {0x6e, "ぃ"},
    {0x6f, "ぅ"},

    {0x70, "‘"},
    {0x71, "’"},
    {0x72, "“"},
    {0x73, "”"},
    {0x74, "・"},
    {0x75, "⋯"},
    {0x76, "ぁ"},
    {0x77, "ぇ"},
    {0x78, "ぉ"},
    {0x7f, " "},

    // 0x80 - 0xbf: normal text characters
    {0x80, "A"},
    {0x81, "B"},
    {0x82, "C"},
    {0x83, "D"},
    {0x84, "E"},
    {0x85, "F"},
    {0x86, "G"},
    {0x87, "H"},
    {0x88, "I"},
    {0x89, "J"},
    {0x8a, "K"},
    {0x8b, "L"},
    {0x8c, "M"},
    {0x8d, "N"},
    {0x8e, "O"},
    {0x8f, "P"},
    {0x90, "Q"},
    {0x91, "R"},
    {0x92, "S"},
    {0x93, "T"},
    {0x94, "U"},
    {0x95, "V"},
    {0x96, "W"},
    {0x97, "X"},
    {0x98, "Y"},
    {0x99, "Z"},
    {0x9a, "("},
    {0x9b, ")"},
    {0x9c, ":"},
    {0x9d, ";"},
    {0x9e, "["},
    {0x9f, "]"},
    {0xa0, "a"},
    {0xa1, "b"},
    {0xa2, "c"},
    {0xa3, "d"},
    {0xa4, "e"},
    {0xa5, "f"},
    {0xa6, "g"},
    {0xa7, "h"},
    {0xa8, "i"},
    {0xa9, "j"},
    {0xaa, "k"},
    {0xab, "l"},
    {0xac, "m"},
    {0xad, "n"},
    {0xae, "o"},
    {0xaf, "p"},
    {0xb0, "q"},
    {0xb1, "r"},
    {0xb2, "s"},
    {0xb3, "t"},
    {0xb4, "u"},
    {0xb5, "v"},
    {0xb6, "w"},
    {0xb7, "x"},
    {0xb8, "y"},
    {0xb9, "z"},
    {0xba, "é"},
    {0xbb, "'d"},
    {0xbc, "'l"},
    {0xbd, "'s"},
    {0xbe, "'t"},
    {0xbf, "'v"},

    // 0xc0 - 0xdf: non-text data

    // 0xe0 - 0xff: numbers and special symbols
    {0xe0, "'"},
    {0xe1, "PK"},
    {0xe2, "MN"},
    {0xe3, "-"},
    {0xe4, "'r"},
    {0xe5, "'m"},
    {0xe6, "?"},
    {0xe7, "!"},
    {0xe8, "."},
    {0xe9, "ァ"},
    {0xea, "ゥ"},
    {0xeb, "ェ"},
    {0xec, "▷"},
    {0xed, "▶"},
    {0xee, "▼"},
    {0xef, "♂"},
    {0xf0, "¥"},
    {0xf1, "×"},
    {0xf2, "."},
    {0xf3, "/"},
    {0xf4, ","},
    {0xf5, "♀"},
    {0xf6, "0"},
    {0xf7, "1"},
    {0xf8, "2"},
    {0xf9, "3"},
    {0xfa, "4"},
    {0xfb, "5"},
    {0xfc, "6"},
    {0xfd, "7"},
    {0xfe, "8"},
    {0xff, "9"},
});

static constexpr bool isText(uint8_t b) {
  return (0x80 <= b && b <= 0xbf) || (0xf6 <= b && b <= 0xff);
};

static uint8_t toROMFormat(std::string &s) {
  std::set<uint8_t> ids;
  std::size_t longest = 1;

  for (const auto &p : text::pokemon::bgry::english) {
    if (p.second.size() >= longest) {
      if (s.rfind(p.second) == 0) {
        if (p.second.size() > longest) {
          ids.clear();
          longest = p.second.size();
        }
        ids.insert(p.first);
      }
    }
  }

  if (ids.size() > 0) {
    s.erase(0, longest);
    return *std::max_element(ids.begin(), ids.end());
  }

  return 0;
}

static uint8_t toROMFormat(char c) {
  // TODO: this function needs to be modified so that the reverse of this is
  // always the same as the source text - or at the very least the shortest
  // subset, if it starts with P. :)
  for (const auto &p : text::pokemon::bgry::english) {
    if (p.second.size() > 0) {
      if (c == p.second[0]) {
        return p.first;
      }
    }
  }

  return 0x50;
}

}  // namespace bgry
}  // namespace pokemon
}  // namespace text

#endif
