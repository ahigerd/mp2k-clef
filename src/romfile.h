#ifndef GBAMP2WAV_ROMFILE_H
#define GBAMP2WAV_ROMFILE_H

#include <cstdint>
#include <vector>
#include <stdexcept>
#include <iostream>
#include "utility.h"
class S2WContext;
class SynthContext;
class SongTable;

class ROMFile {
public:
  static constexpr uint32_t BAD_PTR = 0xDEADBEEF;

  class BadAccess : public std::out_of_range {
  public:
    static std::string message(uint32_t addr);
    BadAccess(uint32_t addr);
    const uint32_t addr;
  };

  ROMFile(S2WContext* ctx);
  ROMFile(const ROMFile& other) = delete;
  ROMFile(ROMFile&& other) = delete;
  ROMFile& operator=(const ROMFile& other) = delete;
  ROMFile& operator=(ROMFile&& other) = delete;

  void load(SynthContext* synth, const std::string& path);
  void load(SynthContext* synth, std::istream& stream);

  inline S2WContext* context() const { return ctx; }
  inline SynthContext* synthContext() const { return synth; }

  SongTable findSongTable(int minSongs = -1, uint32_t offset = 0x200) const;
  std::vector<SongTable> findSongTables(uint32_t offset = 0x200) const;
  SongTable findAllSongs() const;
  bool checkSong(uint32_t addr, bool deep = true) const;

  std::vector<uint8_t> rom;
  uint32_t sampleRate;

  inline uint8_t operator[](uint32_t addr) const { return read<uint8_t>(addr); }
  template<typename T> inline T read(uint32_t addr) const {
    uint32_t cleaned = cleanPointer(addr | 0x08000000, sizeof(T), false);
    if (cleaned == BAD_PTR) throw BadAccess(addr);
    return parseInt<T>(rom, cleaned);
  }
  inline uint32_t readPointer(uint32_t addr, bool align = true) const {
    uint32_t cleaned = cleanDeref(addr, 4, align, false);
    if (cleaned == BAD_PTR) throw BadAccess(addr);
    return cleaned;
  }
  template<typename T> inline T deref(uint32_t addr) const {
    uint32_t cleaned = cleanDeref(addr, sizeof(T), (sizeof(T) & 3) > 0);
    if (cleaned == BAD_PTR) throw BadAccess(addr);
    return parseInt<T>(rom, cleaned);
  }

private:
  uint32_t cleanPointer(uint32_t addr, uint32_t size = 4, bool align = true) const;
  uint32_t cleanDeref(uint32_t addr, uint32_t size = 4, bool align = true, bool alignPointer = true) const;

  S2WContext* ctx;
  SynthContext* synth;
};

#endif
