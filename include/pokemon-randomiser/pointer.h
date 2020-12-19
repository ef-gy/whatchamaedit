#if !defined(POKEMON_RANDOMISER_POINTER_H)
#define POKEMON_RANDOMISER_POINTER_H

#include <vector>

namespace gameboy {
namespace rom {
template <typename B = int8_t, typename W = int16_t, W bankSize_ = 0x4000>
class pointer {
 public:
  constexpr pointer(B bank, W offset)
      : bank_(bank), offset_(normaliseOffset(bank, offset)) {}
  constexpr pointer(size_t linear) : linear_(linear) {}

  /* copy a pointer, enforce a linear address reference.
   *
   * Will return ptr but always as a linear address, regardless of what the
   * original pointer was set up as.
   *
   * You probably won't need to ever use this, this is for library functions
   * that manipulat pointers and want to match the style of another for whatever
   * reason there might be.
   */
  static constexpr pointer asLinear(const pointer &ptr) {
    return pointer{ptr.linear()};
  }

  /* copy the pointer, enforce a bank/offset address reference.
   *
   * Kind of the same but opposite of asLinear() - does pretty much what you
   * think.
   */
  static constexpr pointer asBanked(const pointer &ptr) {
    return pointer{ptr.bank(), ptr.offset()};
  }

  /* copy the given pointer, enforce the address reference style of the object
   * you call this function on.
   *
   * Useful when you manipulate pointers but want to keep the style they were
   * presented as initially, for style reasons.
   */
  constexpr pointer asMatched(const pointer &ptr) const {
    if (isLinear()) {
      return asLinear(ptr);
    }

    return asBanked(ptr);
  }

  constexpr bool isLinear(void) const { return bool(linear_); }

  constexpr bool isBanked(void) const { return bool(bank_) && bool(offset_); }

  constexpr const B bank(void) const {
    if (bank_) {
      return *bank_;
    }

    return banks(*linear_);
  }

  constexpr const W offset(void) const {
    if (offset_) {
      return *offset_;
    }

    return normaliseOffset(bank(), *linear_);
  }

  constexpr const size_t linear(void) const {
    if (linear_) {
      return *linear_;
    }

    /* we reuse the normaliseOffset() function and pretend this is for bank 0,
     * so that we get a 0x0000-based offset, which we can simply add to the
     * multiple of the bank size and have everything magically check out.
     *
     * This wouldn't work if we had a more complicated MBC... which I'm not sure
     * exists for GameBoys. And, like, why would anyone create a memory map with
     * non-constant bank sizes?
     */
    return *bank_ * bankSize_ + normaliseOffset(0, *offset_);
  }

  /* Bank size asserted by this pointer.
   *
   * All common MBCs seem to use a 0x4000 bank size for the ROM, so we could
   * just use a few magic values instead, but writing it out like this actually
   * serves as convenient documentation for how we get to the values we get. And
   * those are both useful and rare.
   */
  static constexpr const W bankSize(void) {
    /* this mirrors the template parameter of almost the same name -
     * we cannot make these available in any other way than a variable or a
     * function like this, however it is conceivable that a future version will
     * try to determine the actual bank size based on the cartridge headers and
     * may therefore be dynamic. So using a call here will future-proof us a
     * little bit. */
    return bankSize_;
  }

  /* The number of 'banks' in a linear address.
   *
   * Feeding in an offset will also work and so what one would expect - simply
   * returning a small number of banks.
   */
  static constexpr const B banks(std::size_t s) { return s / bankSize_; }

  constexpr pointer operator+(const ssize_t &d) const {
    return asMatched(pointer{linear() + d});
  }

  constexpr pointer operator-(const ssize_t &d) const {
    return asMatched(pointer{linear() - d});
  }

  const B operator*(const std::vector<B> &data) const { return data[linear()]; }

  pointer &operator++(void) { return *this += 1; }

  pointer operator++(int) {
    pointer r = *this;
    ++(*this);
    return r;
  }

  pointer &operator+=(const ssize_t &d) {
    const pointer lin{linear() + d};

    if (isLinear()) {
      linear_ = lin.linear();
    }
    if (isBanked()) {
      bank_ = lin.bank();
      offset_ = lin.offset();
    }

    return *this;
  }

  constexpr ssize_t operator-(const pointer &p) const {
    return linear() - p.linear();
  }

  constexpr const bool operator<(const pointer &b) const {
    return linear() < b.linear();
  }

  constexpr const bool operator<=(const pointer &b) const {
    return linear() <= b.linear();
  }

  /* Test for equality with another pointer.
   *
   * We're being a bit paranoid here by testing all of the potential components
   * individually - but better safe than sorry to be honest. This does actually
   * end up in invalid states in memory corruption cases.
   */
  constexpr const bool operator==(const pointer &b) const {
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

  static constexpr W normaliseOffset(B bank, W offset) {
    /* we assume the following about banks and offsets in a GameBoy ROM:
     *   - ROM bank 0 is always mapped to 0x0000 of the address space
     *   - ROM bank 1+ are only mapped at bankSize() and bank 0 cannot be mapped
     * there
     *   - these are the only ROM map ranges that are valid
     *   - offsets are stored and generally most useful in the address space map
     * ranges - because then the GameBoy doesn't need to translate between
     * offsets as read out of the ROM and as it would need to be used as a
     * memory address
     *
     * The first two are actually based on technical limitations of certain
     * common MBC chips - definitely the MBC3s used in Pokemon, which we're
     * focussing on, and the MBC1s are equally restricted.
     *
     * With this we can normalise the offsets we deal with. A mismatch in offset
     * to bank will be automatically corrected. This will always lead to a
     * single preferred notation for {bank:offset} addresses, that should be
     * provably correctly identifying a single, specific memory location. //
     * TODO: prove this transform is non-destructive and non-aliasing.
     */
    if (bank == 0) {
      /* for bank 0, simply calculate the remainder of the offset to the bank
       * size.
       *
       * We discard any overflow in the normalisation, because we assumet the
       * bank size is correct and authoritative, i.e. if we were to have an
       * offset of {1 bank, 2k bytes}, and we are called with bank=0, then we
       * take the bank=0 as truth and simply discard the 1 bank offset. This is
       * because theoretically, it's up to the MBC chip to map banks to wherever
       * it pleases, but the complexity of any map that isn't trivially a
       * multiple of the bank size as the start of a bank's address space is
       * simply not worth the extra cycles spent on every memory access, and it
       * is thinkable someone would have an MBC that has, say, the normal
       * cartridge RAM area actually shadowing ROM banks.
       *
       * Since this is confusing and we normally actually really want to use
       * linear addresses for manipulating the ROM in a non-GameBoy runtime
       * environment, it's better to simply assert the most likely map and cut
       * out any confusion by finding a single preferred notation and running
       * with it. */
      return offset % bankSize_;
    }

    /* if we reach here, then the bank wasn't 0, so we use the algorithm for
     * bank=0 (i.e.: "clip to remainder against bank size") and simply add one
     * bank size.
     *
     * This is simple and straightforward and produces stable results.
     */
    return normaliseOffset(0, offset) + bankSize_;
  }
};
}  // namespace rom
}  // namespace gameboy

#endif
