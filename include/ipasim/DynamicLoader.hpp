// DynamicLoader.hpp: Definition of class `DynamicLoader` and some smaller
// classes it uses.

#ifndef IPASIM_DYNAMIC_LOADER_HPP
#define IPASIM_DYNAMIC_LOADER_HPP

#include "ipasim/Common.hpp"
#include "ipasim/Emulator.hpp"
#include "ipasim/LoadedLibrary.hpp"
#include "ipasim/Logger.hpp"
#include "ipasim/TextBlockStream.hpp"

#include <functional>
#include <map>
#include <stack>
#include <string>
#include <unicorn/unicorn.h>
#include <vector>

namespace ipasim {

// Represents a path to a binary file. It can be both `.dll` and `.dylib`. It
// can also be both user and "our system" binary.
struct BinaryPath {
  std::string Path;
  bool Relative; // `true` iff `Path` is relative to install dir

  // Checks whether the binary exists.
  bool isFileValid() const;
};

// Pair of a `LoadedLibrary` and its path.
struct LibraryInfo {
  const std::string *LibPath;
  LoadedLibrary *Lib;
};

// Used for dyld-objc integration.
using _dyld_objc_notify_mapped = void (*)(unsigned count,
                                          const char *const paths[],
                                          const void *const mh[]);
using _dyld_objc_notify_init = void (*)(const char *path, const void *mh);
using _dyld_objc_notify_unmapped = void (*)(const char *path, const void *mh);

// Represents our dynamic loader. It tries to resemble the behavior of iOS's
// `dyld`. The dynamic loader retains information about loaded libraries.
class DynamicLoader {
public:
  DynamicLoader(Emulator &Emu);
  LoadedLibrary *load(const std::string &Path);
  // Used for dyld-objc integration. Notifies registered listeners that a new
  // library was loaded into memory. Objective-C runtime uses this to initialize
  // the library's classes.
  void registerMachO(const void *Hdr);
  // Registers listeners for dyld-objc integration. See also `registerMachO`.
  void registerHandler(_dyld_objc_notify_mapped Mapped,
                       _dyld_objc_notify_init Init,
                       _dyld_objc_notify_unmapped Unmapped);
  // Finds a library that `Addr` is mapped inside.
  LibraryInfo lookup(uint64_t Addr);
  // Logging helpers
  LogStream::Handler dumpAddr(uint64_t Addr);
  LogStream::Handler dumpAddr(uint64_t Addr, const LibraryInfo &LI);
  LogStream::Handler dumpAddr(uint64_t Addr, const LibraryInfo &LI,
                              ObjCMethod M);
  uint64_t getKernelAddr() { return KernelAddr; }
  static constexpr uint64_t alignToPageSize(uint64_t Addr) {
    return Addr & (-PageSize);
  }
  static constexpr uint64_t roundToPageSize(uint64_t Addr) {
    return alignToPageSize(Addr + PageSize - 1);
  }

  static constexpr int PageSize = 4096;

private:
  struct MachOHandler {
    _dyld_objc_notify_mapped Mapped;
    _dyld_objc_notify_init Init;
    _dyld_objc_notify_unmapped Unmapped;
  };

  bool canSegmentsSlide(LIEF::MachO::Binary &Bin);
  BinaryPath resolvePath(const std::string &Path);
  LoadedLibrary *loadMachO(const std::string &Path);
  LoadedLibrary *loadPE(const std::string &Path);
  void handleMachOs(size_t HdrOffset, size_t HandlerOffset);

  static constexpr int R_SCATTERED = 0x80000000; // From `<mach-o/reloc.h>`
  Emulator &Emu;
  uint64_t KernelAddr;
  // Loaded libraries and their paths
  std::map<std::string, std::unique_ptr<LoadedLibrary>> LLs;
  // These are used for dyld-objc integration:
  std::vector<const void *> Hdrs; // Registered headers
  std::set<uintptr_t> HdrSet;     // Set of registered headers for faster lookup
  std::vector<MachOHandler> Handlers; // Registered handlers
};

} // namespace ipasim

// !defined(IPASIM_DYNAMIC_LOADER_HPP)
#endif
