#if !defined(POKEMON_RANDOMISER_VIEW_H)
#define POKEMON_RANDOMISER_VIEW_H

#include <pokemon-randomiser/pointer.h>

#include <array>
#include <fstream>
#include <iostream>
#include <optional>

namespace gameboy {
namespace rom {
template <typename B = int8_t, typename W = int16_t>
class view {
 public:
  using pointer = gameboy::rom::pointer<B, W>;

  view(const std::vector<B> &data)
      : data_{data}, start_{0}, end_{data.size()}, cur_{0} {}
  view(const std::vector<B> &data, const pointer &start)
      : data_{data},
        start_{start},
        end_{start.bank(), W(start.bankSize() - 1)},
        cur_{start} {}
  view(const std::vector<B> &data, const pointer &start, const pointer &end)
      : data_{data}, start_{start}, end_{end}, cur_{start} {}

  const B byte(void) { return cur_++ * data_; }
  const B byte(void) const { return cur_ * data_; }

  const B byte(const pointer &p) { return (cur_ = p) * data_; }
  const B byte(const pointer &p) const { return p * data_; }

  template <std::size_t N>
  const std::array<B, N> array(void) {
    std::array<B, N> r{};

    for (std::size_t i = 0; i < N; i++) {
      r[i] = byte();
    }

    return r;
  }

  const std::vector<B> vector(void) {
    std::vector<B> r{};

    for (; cur_ <= end_;) {
      r.push_back(byte());
    }

    return r;
  }

  const W word_be(void) {
    const auto b = array<2>();

    return W(b[0] << 8 | b[1]);
  }

  const W word_le(void) {
    const auto b = array<2>();

    return W(b[0] | b[1] << 8);
  }

  view from(const pointer &p) const { return view{data_, p, end_}; }

  view to(const pointer &p) const { return view{data_, start_, p}; }

  view length(const W &l) const {
    return view{data_, start_, start_ + (l - 1)};
  }

  view start(void) const { return view{data_, start_, end_}; }

  view after(const view &v) const { return view{data_, v.end_ + 1}; }

  const bool isFullCover(const view &start, const view &end) const {
    return start.start_ == start_ && end.end_ == end_;
  }

  operator const std::string(void) {
    std::string r;

    for (pointer cur_ = start_; cur_ <= end_;) {
      const B b = byte();
      if (b == 0) {
        break;
      }

      r.push_back(b);
    }

    return r;
  }

  operator const std::string(void) const { return std::string(start()); }

 protected:
  const std::vector<B> &data_;
  const pointer start_;
  const pointer end_;
  pointer cur_;
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
