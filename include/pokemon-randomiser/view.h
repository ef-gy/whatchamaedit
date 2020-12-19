#if !defined(POKEMON_RANDOMISER_VIEW_H)
#define POKEMON_RANDOMISER_VIEW_H

#include <pokemon-randomiser/pointer.h>

#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <set>

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
    return view{this, start_, v.start_ - 1, annotations_};
  }

  view toBankEnd(void) const {
    return view{this, start_, pointer{start_.bank(), pointer::bankSize() - 1},
                annotations_};
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
  const B byte(void) const {
    if (!checkUnitReadable()) {
      /* explicitly check that the current block of data is readable before
       * trying to read anyting - this includes things such as the cursor being
       * in range of data_. It's enough to check this with byte() reads, because
       * higher level constructs all use byte() reads.
       *
       * We check this here, because this test should easily be inlined and
       * memoized by any compiler (it's just a few less-than-or-equals) and
       * unbounded access will actually cause memory access faults in this
       * program, which isn't great with input data like Pokemon where the game
       * is known to crash in hilarious ways.
       *
       * This approach allows relatively safe data type scanning through the
       * whole ROM using a simple linear search, since individual data type
       * constructors will simply accept a view and normally are thoroughly
       * testing the validity of the data block they would read at this pointer.
       */
      return 0;
    }

    return cur_ * data_;
  }

  const B byte(const pointer &p) { return (cur_ = p), byte(); }

  const B byte(const pointer &p) const {
    /* note: this access function is not as tightly protected as the non-const
     * and the cursor variant - it's expected that accessors of this function
     * will have run a proper check themselves or their accessors would,
     * otherwise the access safety doesn't work and you'll likely have a bad
     * day. */
    return p * data_;
  }

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
    return checkRange() && checkUnitReadable() && checkLength() &&
           checkEndianness() && checkValue();
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

  W unit(void) const {
    if (annotations_.type) {
      switch (*annotations_.type) {
        case dt_code:
        case dt_rom_bank:
        case dt_byte:
        case dt_bytes:
        case dt_text:
          return 1;
        case dt_rom_offset:
        case dt_word:
        case dt_words:
          return 2;
      }
    }

    return 1;
  }

  const pointer startPtr(void) const { return start_; }
  const pointer endPtr(void) const { return end_; }
  const pointer curPtr(void) const { return cur_; }

 protected:
  const bytes &data_;
  const pointer start_;
  const pointer end_;
  pointer cur_;
  annotations annotations_;

  bool checkUnitReadable(void) const {
    /* check that a full unit of data is currently readable - this requires as
     * many bytes as unit() indicates on top of the cursor, -1 because the end
     * is defined via the high byte of the compound (e.g. base + 1 for a word,
     * instead of one after). */
    const pointer high = cur_ + unit() - 1;

    return check(start_) && check(cur_) && check(high) && check(end_) &&
           start_ <= cur_ && cur_ <= high && high <= end_;
  }

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
    if (unit() > 1) {
      /* for any data type with a unit size larger than a byte, e.g. a word or a
       * quad, insist on having the endianness set explicitly.
       *
       * Since this is GameBoy, we could reasonably default to little endian,
       * which is the default for the architecture, but seeing as how even the
       * cartridge header itself has an example fo a big endian 16 bit checksum,
       * let's just not and always be explicit. The endianness is inherited just
       * like any other annotation, so it's easy to set at the top level view of
       * a data structure if all the values in the same share that attribute.
       */
      if (!annotations_.type || !annotations_.endianness) {
        /* only check for the case that's explicitly disallowed, and allow
         * everything else. */
        return false;
      }
    }

    return true;
  }

  bool check(const pointer &p) const {
    /* check a pointer for being valid in this view, that is, the pointer is in
     * the range possible for the size of data_. */
    const pointer start{0};
    const pointer end{data_.size()};

    /* note that the start and end are both inclusive, thus the
     * less-than-or-equal on both sides of the equation. */
    return start <= p && p <= end;
  }

  bool checkRange(void) const {
    /* check that the cursor in this view is valid and in range between the
     * start and end of the window that was set. */
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
        break;
      }
    }

    return r;
  }

  bool check(const lazies &lazs) const {
    bool r = true;

    for (const auto &v : lazs) {
      r = r && bool(*v) && check(pointer(*v));

      if (!r) {
        break;
      }
    }

    return r;
  }
};
}  // namespace rom
}  // namespace gameboy

#endif
