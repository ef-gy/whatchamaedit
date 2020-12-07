#if !defined(POKEMON_RANDOMISER_POINTER_H)
#define POKEMON_RANDOMISER_POINTER_H

#include <vector>

namespace gameboy {
namespace rom {
template <typename B = int8_t, typename W = int16_t, W bankSize_ = 0x4000>
class pointer {
 public:
  pointer(B bank, W offset) : bank_(bank), offset_(offset) {}
  pointer(size_t linear) : linear_(linear) {}

  const B bank(void) const {
    if (bank_) {
      return *bank_;
    }

    return *linear_ / bankSize_;
  }

  const W offset(void) const {
    if (offset_) {
      return *offset_;
    }

    return *linear_ % bankSize_ + (*linear_ >= bankSize_ ? bankSize_ : 0);
  }

  const size_t linear(void) const {
    if (linear_) {
      return *linear_;
    }

    return *bank_ * bankSize_ + *offset_ % bankSize_;
  }

  static constexpr const W bankSize(void) { return bankSize_; }

  const B operator*(const std::vector<B> &data) const { return data[linear()]; }

  pointer &operator++(void) {
    if (linear_) {
      linear_ = *linear_ + 1;
    }
    if (offset_) {
      offset_ = *offset_ + 1;

      if (*offset_ % bankSize_ == 0) {
        offset_ = bankSize_;
        bank_ = *bank_ + 1;
      }
    }

    return *this;
  }

  pointer operator+(const size_t &offset) const {
    return pointer{linear() + offset};
  }

  pointer operator++(int) {
    pointer r = *this;
    ++(*this);
    return r;
  }

  const bool operator<(const pointer &b) const { return linear() < b.linear(); }

  const bool operator<=(const pointer &b) const {
    return linear() <= b.linear();
  }

  const bool operator==(const pointer &b) const {
    return linear() == b.linear();
  }

 protected:
  std::optional<B> bank_;
  std::optional<W> offset_;
  std::optional<size_t> linear_;
};
}  // namespace rom
}  // namespace gameboy

#endif
