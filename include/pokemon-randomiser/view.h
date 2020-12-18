#if !defined(POKEMON_RANDOMISER_VIEW_H)
#define POKEMON_RANDOMISER_VIEW_H

#include <pokemon-randomiser/pointer.h>

#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <set>
#include <sstream>

namespace gameboy {
enum type {
  dt_rom_bank,
  dt_rom_offset,
  dt_code,
  dt_byte,
  dt_bytes,
  dt_word,
  dt_words,
  dt_text,
};

enum endianness {
  e_little_endian,
  e_big_endian,
};

namespace rom {

template <typename B = int8_t, typename W = int16_t>
class annotations {
 public:
  using pointer = gameboy::rom::pointer<B, W>;

  annotations(void) {}
  annotations(const type &a) : type{a} {}
  annotations(const endianness &a) : endianness{a} {}

  std::optional<type> type;
  std::optional<endianness> endianness;

  annotations operator|(const annotations &b) const {
    annotations a = *this;

    if (b.type) {
      a.type = b.type;
    }
    if (b.endianness) {
      a.endianness = b.endianness;
    }

    return a;
  }

  annotations &operator|=(const annotations &b) {
    *this = *this | b;
    return *this;
  }
};

template <typename B = int8_t, typename W = int16_t>
class view {
 public:
  using annotations = annotations<B, W>;
  using pointer = gameboy::rom::pointer<B, W>;
  using lazy = typename pointer::template lazy<view>;
  using bytes = std::vector<B>;

  using subviews = std::set<view *>;
  using lazies = std::set<lazy *>;

  view(const bytes &data)
      : data_{data},
        start_{0},
        end_{data.size() - 1},
        cur_{0},
        annotations_{} {}
  view(const bytes &data, const pointer &start)
      : data_{data},
        start_{start},
        end_{start.bank(), W(start.bankSize() - 1)},
        cur_{start},
        annotations_{} {}
  view(const bytes &data, const pointer &start, const pointer &end)
      : data_{data}, start_{start}, end_{end}, cur_{start}, annotations_{} {}

  // this is the full constructor used by the verbose constructors
  view(const view *parent, const pointer &start, const pointer &end,
       const annotations &annotations)
      : data_{parent->data_},
        start_{start},
        end_{end},
        cur_{start},
        annotations_{annotations} {}

  // chainable, verbose constructors
  view from(const pointer &p) const {
    return view{this, p, end_, annotations_};
  }

  view to(const pointer &p) const {
    return view{this, start_, p, annotations_};
  }

  view length(const W &l) const {
    return view{this, start_, start_ + (l - 1), annotations_};
  }

  view start(void) const { return view{this, start_, end_, annotations_}; }

  view after(const view &v) const {
    return view{this, v.end_ + 1, end_, annotations_};
  }

  view before(const view &v) const {
    return view{this, start_, v.start_ + -1, annotations_};
  }

  // data type annotations

  view expect(const annotations &a) const {
    return view{this, start_, end_, annotations_ | a};
  }

  view is(const annotations &a) const {
    annotations b = annotations_ | a;

    if (a.type) {
      switch (*a.type) {
        case dt_rom_bank:
        case dt_byte:
          return view{this, start_, start_, b};
        case dt_rom_offset:
        case dt_word:
          return view{this, start_, start_ + 1, b};
        default:
          break;
      }
    }

    return view{this, start_, end_, b};
  }

  // shortcuts
  view asLittleEndian(void) const { return expect({e_little_endian}); }

  view asBigEndian(void) const { return expect({e_big_endian}); }

  view asByte(void) const { return is({dt_byte}); }

  view asWord(void) const { return is({dt_word}); }

  view asROMBank(void) const { return is({dt_rom_bank}); }

  view asROMOffset(void) const { return is({dt_rom_offset}); }

  // iterator
  class iterator : public std::iterator<std::input_iterator_tag, B> {
   public:
    explicit iterator(const view &v, const pointer p)
        : view_(v), position_(p) {}

    iterator &operator++(void) {
      position_++;
      return *this;
    }
    iterator operator++(int) {
      auto r = *this;
      ++(*this);
      return r;
    };
    bool operator==(const iterator &b) {
      return view_ == b.view_ && position_ == b.position_;
    }
    bool operator!=(const iterator &b) { return !(*this == b); }
    B operator*(void) const { return view_.byte(position_); };

   protected:
    const view &view_;
    pointer position_;
  };

  // read bytes
  const B byte(void) const { return cur_ * data_; }

  const B byte(const pointer &p) { return (cur_ = p) * data_; }
  const B byte(const pointer &p) const { return p * data_; }

  // read words
  const W word_be(const pointer &p) const {
    const B b[2]{byte(p), byte(p + 1)};

    return W(b[0] << 8 | b[1]);
  }

  const W word_le(const pointer &p) const {
    const B b[2]{byte(p), byte(p + 1)};

    return W(b[0] | b[1] << 8);
  }

  const W word(const pointer &p) const {
    if (annotations_.endianness) {
      switch (*annotations_.endianness) {
        case e_big_endian:
          return word_be(p);
        case e_little_endian:
          return word_le(p);
        default:
          break;
      }
    }

    std::cerr << "WARNING: endianness not declared for word read\n";
    return word_le(p);
  }

  const W word(void) const { return word(cur_); }

  const W word(const pointer &p) {
    cur_ = p;
    return word();
  }

  ssize_t size(void) const { return end_ - start_ + 1; }

  operator const std::string(void) const {
    std::string r;

    for (const auto c : *this) {
      if (c == 0) {
        break;
      }

      r.push_back(c);
    }

    return r;
  }

  operator bytes(void) const { return bytes{begin(), end()}; }

  bool operator==(const view &b) const {
    return &data_ == &b.data_ && start_ == b.start_ && end_ == b.end_ &&
           cur_ == b.cur_;
  }

  iterator begin(void) const { return iterator{*this, start_}; };
  iterator end(void) const { return iterator{*this, end_ + 1}; };

  pointer last(void) const { return end_; }

  operator bool(void) const {
    return checkRange() && checkLength() && checkEndianness() && checkValue();
  }

  B banks(void) const { return pointer::banks(data_.size()); }

  bool within(const pointer &s, const pointer &e) const {
    return start_ <= s && e <= end_;
  }

  bool within(const view &b) const { return within(b.start_, b.end_); }

  template <typename V>
  std::vector<V> repeated(const view &cnt) const {
    std::vector<V> r{};
    pointer st = start_;

    if (cnt) {
      auto c = cnt.byte();

      while (c-- > 0) {
        V item{from(st)};
        auto size = item.size();
        if (item) {
          r.push_back(item);
        }
        if (size <= 0) {
          break;
        }
        st = st + size;
      }
    }

    return r;
  }

  template <typename V>
  pointer last(const std::vector<V> &r) {
    pointer l = start_;
    for (const auto &v : r) {
      if (l < v.last()) {
        l = v.last();
      }
    }
    return l;
  }

 protected:
  const bytes &data_;
  const pointer start_;
  const pointer end_;
  pointer cur_;
  annotations annotations_;

  bool checkLength(void) const {
    size_t minLength = 0;
    size_t maxLength = size();

    if (annotations_.type) {
      switch (*annotations_.type) {
        case dt_code:
          break;
        case dt_rom_bank:
        case dt_byte:
          minLength = 1;
          maxLength = 1;
          break;
        case dt_bytes:
          minLength = 0;
          break;
        case dt_rom_offset:
        case dt_word:
          minLength = 2;
          maxLength = 2;
          break;
        case dt_words:
          minLength = 0;
          break;
        case dt_text:
          minLength = 0;
          break;
      }
    }

    return minLength <= size() && size() <= maxLength;
  }

  bool checkEndianness(void) const {
    if (annotations_.type) {
      switch (*annotations_.type) {
        case dt_rom_offset:
        case dt_word:
        case dt_words:
          if (!annotations_.endianness) {
            return false;
          }
        default:
          break;
      }
    }

    return true;
  }

  bool check(const pointer &p) const {
    const pointer start{0};
    const pointer end{data_.size()};

    return start <= p && p <= end;
  }

  bool checkRange(void) const {
    return start_ <= cur_ && cur_ <= end_ && check(start_) && check(cur_) &&
           check(end_);
  }

  bool checkValue(void) const {
    if (annotations_.type) {
      switch (*annotations_.type) {
        case dt_rom_bank:
          if (byte() >= banks()) {
            return false;
          }
          break;
        case dt_rom_offset:
          if (word() >= pointer::bankSize() * 2) {
            return false;
          }
          break;
        default:
          break;
      }
    }

    return true;
  }

  bool check(const subviews &subs) const {
    bool r = true;

    for (const auto &v : subs) {
      r = r && bool(*v) && within(*v);
      if (!r) {
        std::cerr << "CHECK FAILED: subview test:\n"
                  << " * par " << this->debug() << "\n"
                  << " * sub " << v->debug() << "\n";
        if (!bool(*v)) {
          std::cerr << " ! ERR sub view is not valid\n";
        }
        if (!within(*v)) {
          std::cerr << " ! ERR sub view is not contained within parent\n";
        }
        break;
      }
    }

    return r;
  }

  bool check(const lazies &lazs) const {
    bool r = true;

    for (const auto &v : lazs) {
      r = r && bool(*v) && check(pointer(*v));
    }

    return r;
  }

 public:
  std::string debug(void) const {
    std::ostringstream os{};

    os << "[view:"
       << " window=[" << start_.debug() << " - " << end_.debug() << "]"
       << " length=[0x" << std::hex << std::setw(4) << std::setfill('0')
       << size() << "]"
       << " cursor=[" << cur_.debug() << "]"
       << "]";

    return os.str();
  }
};
}  // namespace rom
}  // namespace gameboy

#endif
