#if !defined(POKEMON_RANDOMISER_POINTER_H)
#define POKEMON_RANDOMISER_POINTER_H

#include <iomanip>
#include <sstream>
#include <vector>

namespace gameboy {
namespace rom {
template <typename B = int8_t, typename W = int16_t, W bankSize_ = 0x4000>
class pointer {
 public:
  constexpr pointer(B bank, W offset) : bank_(bank), offset_(offset) {}
  constexpr pointer(size_t linear) : linear_(linear) {}

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
  static constexpr const B banks(std::size_t s) { return s / bankSize_; }

  const B operator*(const std::vector<B> &data) const { return data[linear()]; }

  pointer &operator++(void) { return *this += 1; }

  pointer operator+(const ssize_t &d) const {
    if (linear_) {
      return pointer{*linear_ + d};
    }

    const auto lin = pointer{linear() + d};
    return pointer{lin.bank(), lin.offset()};
  }

  pointer operator++(int) {
    pointer r = *this;
    ++(*this);
    return r;
  }

  pointer &operator+=(const ssize_t &d) {
    if (linear_) {
      linear_ = *linear_ + d;
    }
    if (offset_) {
      const auto lin = pointer{linear() + d};

      bank_ = lin.bank();
      offset_ = lin.offset();
    }

    return *this;
  }

  ssize_t operator-(const pointer &p) const { return linear() - p.linear(); }

  const bool operator<(const pointer &b) const { return linear() < b.linear(); }

  const bool operator<=(const pointer &b) const {
    return linear() <= b.linear();
  }

  const bool operator==(const pointer &b) const {
    return linear() == b.linear() && bank() == b.bank() &&
           offset() == b.offset();
  }

  template <typename V>
  class lazy {
   public:
    lazy(const V &bank, const V &offset) : bank_{bank}, offset_{offset} {}

    operator bool(void) const { return bool(bank_) && bool(offset_); }

    operator pointer(void) const {
      return pointer{bank_.byte(), offset_.word()};
    }

    pointer resolve(void) const {
      return pointer{bank_.byte(), offset_.word()};
    }

   protected:
    const V &bank_;
    const V &offset_;
  };

 protected:
  std::optional<B> bank_;
  std::optional<W> offset_;
  std::optional<size_t> linear_;

 public:
  const std::string debug(void) const {
    std::ostringstream os{};

    os << "*{";
    if (linear_) {
      os << "[0x" << std::hex << std::setw(6) << std::setfill('0') << *linear_
         << "]";
    } else {
      os << " 0x" << std::hex << std::setw(6) << std::setfill('0') << linear()
         << " ";
    }
    os << " = ";
    if (bank_ && offset_) {
      os << "[0x" << std::hex << std::setw(2) << std::setfill('0') << W(*bank_)
         << ":" << std::setw(4) << *offset_ << "]";
    } else {
      os << " 0x" << std::hex << std::setw(2) << std::setfill('0') << W(bank())
         << ":" << std::setw(4) << offset() << " ";
    }
    os << "}";

    return os.str();
  }
};
}  // namespace rom
}  // namespace gameboy

#endif
