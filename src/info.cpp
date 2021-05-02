#include <ef.gy/cli.h>
#include <whatchamaedit/debug.h>
#include <whatchamaedit/rom.h>

static efgy::cli::flag<std::string> romFile("rom-file", "the ROM to load");

static efgy::cli::flag<std::string> output(
    "output", "the name of the file to write the changed ROM to");

static efgy::cli::flag<bool> showHeader("show-header",
                                        "dump full header information");

static efgy::cli::flag<bool> fixChecksum("fix-checksum",
                                         "fix up checksum in output ROM");

static efgy::cli::flag<bool> getStrings("strings",
                                        "like 'strings's for pokemon text");

int main(int argc, char *argv[]) {
  efgy::cli::options opts(argc, argv);

  if (std::string{::romFile} != "") {
    whatchamaedit::rom::gb<> rom(romFile);

    if (rom) {
      if (::showHeader) {
        std::cout << debug::dump(rom.header) << "\n";
      } else {
        std::cout << rom.title() << "\n";
      }

      if (::getStrings) {
        const auto strs = rom.getStrings();

        for (const auto &str : strs) {
          std::cout << "0x" << std::hex << std::setw(6) << std::setfill('0')
                    << str.first.linear() << " " << str.second << "\n";
        }
      }

      if (::fixChecksum) {
        rom.fixChecksum();
      }

      if (!std::string(output).empty()) {
        rom.save(output);
      }
    } else {
      std::cerr << "NOT LOADED\n";

      if (rom.checksum()) {
        std::cout << "CHECKSUM OK\n";
      } else {
        std::cout << "CHECKSUM NOT OK (" << rom.romChecksum() << " vs "
                  << rom.headerChecksum() << ")\n";
      }
    }
  } else {
    std::cerr << "NOT LOADED\n";
    std::cout << "no ROM file specified\n";
  }

  return 0;
}
