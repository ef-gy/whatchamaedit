#if !defined(WHATCHAMAEDIT_DEBUG_H)
#define WHATCHAMAEDIT_DEBUG_H

#include <ef.gy/range.h>
#include <whatchamaedit/header.h>
#include <whatchamaedit/pointer.h>
#include <whatchamaedit/view.h>

#include <iomanip>
#include <sstream>

namespace debug {
template <typename B, typename W, W bankSize_>
static std::string dump(const gameboy::rom::pointer<B, W, bankSize_> &ptr) {
  std::ostringstream os{};

  os << "*{" << std::hex << std::setfill('0');
  if (ptr.isLinear()) {
    os << "[0x" << std::setw(6) << ptr.linear() << "]";
  } else {
    os << " 0x" << std::setw(6) << ptr.linear() << " ";
  }
  os << "=" << std::hex << std::setfill('0');
  if (ptr.isBanked()) {
    os << "[0x" << std::setw(2) << W(ptr.bank()) << ":" << std::setw(4)
       << ptr.offset() << "]";
  } else {
    os << " 0x" << std::setw(2) << W(ptr.bank()) << ":" << std::setw(4)
       << ptr.offset() << " ";
  }
  os << "}";

  return os.str();
}

template <typename B, typename W>
static std::string dump(const gameboy::rom::view<B, W> view) {
  std::ostringstream os{};
  const auto a = view.expected();

  std::string prefix{""};

  if (a.label) {
    if (*a.label == "__ignore") {
      // explicitly ignored internal label for weird cases like sprites with
      // variable field numbers
      return "";
    }

    if (*a.label == "__transitive_hull") {
      prefix = "; ";
    }

    if (*a.label == "__scope") {
      prefix = "; ";
    }

    os << prefix << *a.label;

    if (!((*a.label).rfind("__", 0) == 0)) {
      os << "_" << std::hex << std::setfill('0') << std::setw(6)
         << view.startPtr().linear() << "_" << std::dec << view.size();
    }

    os << ":\t";
  }

  static const std::size_t hexBytesPerLine = 16;
  static const std::size_t hexByteLimit = 120;

  os << "\t; $" << std::hex << std::setfill('0') << std::setw(6)
     << view.startPtr().linear() << ": " << std::dec << view.size() << " bytes";

  auto it = view.begin();

  for (std::size_t c = 0, l = 0; c < hexByteLimit && c < view.size();
       c++, l = (l < (hexBytesPerLine + 1) ? l + 1 : 0)) {
    B val = (it == view.end()) ? 0 : *(it++);

    if (l == 0) {
      os << "\n"
         << prefix << "\tdb\t$" << std::hex << std::setfill('0') << std::setw(2)
         << W(val);
    } else {
      os << ", $" << std::hex << std::setfill('0') << std::setw(2) << W(val);
    }
  }

  if (hexByteLimit < view.size()) {
    os << "\n"
       << prefix << "\t; " << std::dec << (view.size() - hexByteLimit)
       << " bytes omitted in preview";
  }

  os << "\n" << prefix << "\t; type metadata";

  if (a.endianness) {
    switch (*a.endianness) {
      case gameboy::e_little_endian:
        os << " LE";
        break;
      case gameboy::e_big_endian:
        os << " BE";
        break;
      default:
        os << " ?E";
    }
  } else {
    os << " ?E";
  }

  if (a.type) {
    switch (*a.type) {
      case gameboy::dt_rom_bank:
        os << " BANK    ";
        break;
      case gameboy::dt_rom_offset:
        os << " OFFSET  ";
        break;
      case gameboy::dt_code:
        os << " CODE    ";
        break;
      case gameboy::dt_byte:
        os << " BYTE    ";
        break;
      case gameboy::dt_bytes:
        os << " BYTE[]  ";
        break;
      case gameboy::dt_word:
        os << " WORD    ";
        break;
      case gameboy::dt_words:
        os << " WORD[]  ";
        break;
      case gameboy::dt_text:
        os << " TEXT    ";
        break;
      default:
        os << " UNTYPED ";
    }

    switch (*a.type) {
      case gameboy::dt_rom_bank:
      case gameboy::dt_byte:
      case gameboy::dt_bytes:
        os << "$" << std::hex << std::setfill('0') << std::setw(2)
           << W(view.byte()) << " (" << std::dec << W(view.byte()) << ")";
        break;
      case gameboy::dt_rom_offset:
      case gameboy::dt_word:
      case gameboy::dt_words:
        os << "$" << std::hex << std::setfill('0') << std::setw(4)
           << W(view.word()) << " (" << std::dec << W(view.word()) << ")";
        break;
      case gameboy::dt_text:
        os << "\"" << std::string(view) << "\"";
        break;
      default:
        break;
    }

    switch (*a.type) {
      case gameboy::dt_bytes:
      case gameboy::dt_words:
        os << "\t[...]";
        break;
      default:
        break;
    }

    os << "\n";
  }

  return os.str();
}

template <typename B, typename W, std::size_t count>
static std::string dump(const std::array<gameboy::rom::view<B, W>, count> views,
                        std::string_view section = "UNNAMED") {
  std::ostringstream os{};

  auto hull{gameboy::rom::view<B, W>::hull(views)};

  os << "\nSECTION \"" << section << " 0x" << std::hex
     << hull.startPtr().linear() << "\", ";
  if (hull.startPtr().bank() == 0) {
    os << "ROM0[$" << std::hex << hull.startPtr().offset() << "]";
  } else {
    os << "ROMX[$" << std::hex << hull.startPtr().offset() << "], BANK["
       << W(hull.startPtr().bank()) << "]";
  }
  os << "\n";

  os << "\n" << dump(hull) << "\n";

  for (const auto v : views) {
    if (!bool(v)) {
      os << " ! ERR view not valid\n";
    } else {
      std::string d = dump(v);

      if (d != "") {
        os << d << "\n";
      }
    }
  }

  os << "; ===== END SECTION: " << section << " ======\n\n\n";

  return os.str();
}

template <typename B, typename W>
static std::string dump(const gameboy::rom::header<B, W> header) {
  std::ostringstream os{};

  if (!bool(header)) {
    os << " ! ERR header is not valid\n";
  } else {
    os << dump(header.fields(), "GameBoy ROM Header");
  }

  return os.str();
}

template <typename view, typename B, typename W, W bankSize_>
static std::string dump(const gameboy::rom::lazy<view, B, W, bankSize_> &lazy) {
  using pointer = gameboy::rom::pointer<B, W, bankSize_>;

  std::ostringstream os{};

  os << "?{bank@ " << dump(lazy.bank()) << " offset@ " << dump(lazy.offset())
     << "]";
  if (lazy) {
    os << " =OK=> " << dump(pointer(lazy));
  } else {
    os << " =NOK_";
  }
  os << "}";

  return os.str();
}

template <typename B, typename W>
static std::string dump(const gameboy::rom::view<B, W> &view,
                        const std::set<gameboy::rom::view<B, W> *> &subviews) {
  std::ostringstream os{};

  os << "SUBVIEWS\n"
     << " * par " << dump(view) << "\n";

  for (const auto &v : subviews) {
    os << " * sub " << dump(*v) << "\n";
    if (!bool(*v)) {
      os << " ! ERR view not valid\n";
    }
    if (!view.within(*v)) {
      os << " ! ERR view is not contained within given parent\n";
    }
  }

  return os.str();
}
}  // namespace debug

#endif
