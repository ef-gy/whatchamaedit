#if !defined(POKEMON_RANDOMISER_CHARACTER_MAP_H)
#define POKEMON_RANDOMISER_CHARACTER_MAP_H

#include <algorithm>
#include <map>
#include <string>

namespace pokemon {
namespace text {
using charmap = std::map<uint8_t, std::string>;

namespace bgry {

/* The BGRY section used Bulbapedia extensively for character translations.
 *
 * See this link for ALL the detail:
 * https://bulbapedia.bulbagarden.net/wiki/Character_encoding_in_Generation_I
 */

static const constexpr uint8_t end = 0x50;

static charmap english({
    // NULL
    {0x00, std::string()},

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

  for (const auto &p : pokemon::text::bgry::english) {
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
  for (const auto &p : pokemon::text::bgry::english) {
    if (p.second.size() > 0) {
      if (c == p.second[0]) {
        return p.first;
      }
    }
  }

  return 0x50;
}

}  // namespace bgry
}  // namespace text
}  // namespace pokemon

#endif
