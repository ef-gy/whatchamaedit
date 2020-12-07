#if !defined(POKEMON_RANDOMISER_VIEW_H)
#define POKEMON_RANDOMISER_VIEW_H

#include <pokemon-randomiser/pointer.h>

#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>

namespace gameboy {
namespace rom {
enum type {
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

template <typename B = int8_t, typename W = int16_t>
class annotations {
 public:
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
  using bytes = std::vector<B>;

  view(const std::vector<B> &data)
      : data_{data},
        start_{0},
        end_{data.size() - 1},
        cur_{0},
        annotations_{} {}
  view(const std::vector<B> &data, const pointer &start)
      : data_{data},
        start_{start},
        end_{start.bank(), W(start.bankSize() - 1)},
        cur_{start},
        annotations_{} {}
  view(const std::vector<B> &data, const pointer &start, const pointer &end)
      : data_{data}, start_{start}, end_{end}, cur_{start}, annotations_{} {}

  // this is the full constructor used by the verbose constructors
  view(const bytes &data, const pointer &start, const pointer &end,
       const annotations &annotations)
      : data_{data},
        start_{start},
        end_{end},
        cur_{start},
        annotations_{annotations} {}

  // chainable, verbose constructors
  view from(const pointer &p) const {
    return view{data_, p, end_, annotations_};
  }

  view to(const pointer &p) const {
    return view{data_, start_, p, annotations_};
  }

  view length(const W &l) const {
    return view{data_, start_, start_ + (l - 1), annotations_};
  }

  view start(void) const { return view{data_, start_, end_, annotations_}; }

  view after(const view &v) const {
    return view{data_, v.end_ + 1, end_, annotations_};
  }

  view before(const view &v) const {
    return view{data_, start_, v.start_ + -1, annotations_};
  }

  view expect(const annotations &a) const {
    return view{data_, start_, end_, annotations_ | a};
  }

  view is(const annotations &a) const {
    annotations b = annotations_ | a;

    if (a.type) {
      switch (*a.type) {
        case dt_byte:
          return view{data_, start_, start_, b};
        case dt_word:
          return view{data_, start_, start_ + 1, b};
        default:
          break;
      }
    }

    return view{data_, start_, end_, b};
  }

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

  const B byte(void) { return cur_++ * data_; }
  const B byte(void) const { return cur_ * data_; }

  const B byte(const pointer &p) { return (cur_ = p) * data_; }
  const B byte(const pointer &p) const { return p * data_; }

  const W word_be(const pointer &p) const {
    const B b[2]{byte(p), byte(p + 1)};

    return W(b[0] << 8 | b[1]);
  }

  const W word_le(const pointer &p) const {
    const B b[2]{byte(p), byte(p + 1)};

    return W(b[0] | b[1] << 8);
  }

  const W word(const pointer &p) const {
    W r;

    if (annotations_.endianness) {
      switch (*annotations_.endianness) {
        case e_big_endian:
          r = word_be(p);
          break;
        case e_little_endian:
          r = word_le(p);
          break;
        default:
          break;
      }
    }

    return r;
  }

  const W word(void) const { return word(cur_); }

  const W word(pointer &p) {
    const auto t = *this;

    W r = t.word(p);
    cur_ += 2;
    return r;
  }

  const W word(void) {
    const auto t = *this;

    W r = t.word();
    cur_ += 2;
    return r;
  }

  ssize_t size(void) const { return end_ - start_ + 1; }

  const bool isFullCover(const view &start, const view &end) const {
    return start.start_ == start_ && end.end_ == end_;
  }

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

  operator std::vector<B>(void) const { return std::vector<B>{begin(), end()}; }

  bool operator==(const view &b) const {
    return &data_ == &b.data_ && start_ == b.start_ && end_ == b.end_ &&
           cur_ == b.cur_;
  }

  iterator begin(void) const { return iterator{*this, start_}; };
  iterator end(void) const { return iterator{*this, end_ + 1}; };

  operator bool(void) const { return checkLength() && checkEndianness(); }

 protected:
  const std::vector<B> &data_;
  const pointer start_;
  const pointer end_;
  pointer cur_;
  annotations annotations_;

  bool checkLength(void) const {
    size_t minLength = 0;
    size_t maxLength = data_.size();

    if (annotations_.type) {
      switch (*annotations_.type) {
        case dt_code:
          break;
        case dt_byte:
          minLength = 1;
          maxLength = 1;
          break;
        case dt_bytes:
          minLength = 0;
        case dt_word:
          minLength = 1;
          maxLength = 1;
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
};

template <typename B = uint8_t, typename W = uint16_t>
class image {
 public:
  using pointer = gameboy::rom::pointer<B, W>;
  using view = gameboy::rom::view<B, W>;

  image(const std::string &file) : loadOK(load(file)) {}

  bool load(const std::string &file) {
    bool ok = false;

    std::ifstream rom(std::string(file), std::ios::in | std::ios::binary);

    data = std::vector<B>((std::istreambuf_iterator<char>(rom)),
                          std::istreambuf_iterator<char>());

    return (ok = data.size() > 0);
  }

  bool save(const std::string &file) const {
    std::ofstream rom(file, std::ios::binary | std::ios::ate);
    std::streamsize size = data.size();

    return bool(rom.write((char *)data.data(), size));
  }

  operator bool(void) { return loadOK; }

  std::vector<B> data;

  const std::size_t size(void) const { return data.size(); }

  operator view(void) const { return view{data}; }

 protected:
  bool loadOK;
};
}  // namespace rom
}  // namespace gameboy

#endif
