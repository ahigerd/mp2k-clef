#include "romfile.h"
#include "songtable.h"
#include <algorithm>
#include <iostream>
#include <sstream>

std::string ROMFile::BadAccess::message(uint32_t addr)
{
  std::ostringstream ss;
  ss << "Address out of range: 0x" << std::hex << addr;
  return ss.str();
}

ROMFile::BadAccess::BadAccess(uint32_t addr)
: std::out_of_range(ROMFile::BadAccess::message(addr)), addr(addr)
{
  // initializers only
}

ROMFile::ROMFile()
: sampleRate(13379)
{
  // initializers only
}

uint32_t ROMFile::cleanPointer(uint32_t addr, uint32_t size, bool align) const
{
  uint32_t mask = align ? 0xFE000003 : 0xFE000000;
  if ((addr & mask) != 0x08000000) return BAD_PTR;
  addr &= 0x07FFFFFF;
  if (addr < 0x200 || addr > rom.size() - size) return BAD_PTR;
  return addr;
}

uint32_t ROMFile::cleanDeref(uint32_t addr, uint32_t size, bool alignTarget, bool alignPointer) const
{
  addr = cleanPointer(addr | 0x08000000, 4, alignPointer);
  if (addr == BAD_PTR) return BAD_PTR;
  return cleanPointer(parseInt<uint32_t>(rom, addr), size, alignTarget);
}

SongTable ROMFile::findSongTable(int minSongs, uint32_t offset) const
{
  size_t size = rom.size() - 8;
  SongTable result(this);
  uint32_t tableStart = 0;
  std::vector<uint32_t> songs;
  offset -= 4;
  while ((offset += 4) < size) {
    uint32_t addr = cleanDeref(offset | 0x08000000, 12);
    if (addr != BAD_PTR && !checkSong(addr, false)) {
      addr = BAD_PTR;
    }
    if (addr == BAD_PTR) {
      if (tableStart) {
        if (songs.size() > result.songs.size()) {
          result.tableStart = tableStart;
          result.tableEnd = offset;
          result.songs = songs;
          if (minSongs > 0 && result.songs.size() > minSongs) {
            return result;
          }
        }
        songs.clear();
      }
      tableStart = 0;
      continue;
    }
    if (!tableStart) {
      tableStart = offset;
    }
    if (std::find(songs.begin(), songs.end(), addr) == songs.end() && checkSong(addr)) {
      // Song is valid and is not a duplicate
      songs.push_back(addr);
    }
    offset += 4;
  }
  if (songs.size() > result.songs.size()) {
    result.tableStart = tableStart;
    result.tableEnd = offset;
    result.songs = songs;
  }
  return result;
}

SongTable ROMFile::findAllSongs() const
{
  SongTable result(this);
  int size = rom.size() - 12;
  for (int offset = 0x200; offset < size; offset += 4) {
    if (checkSong(offset)) {
      result.songs.push_back(offset);
      offset += 4;
    }
  }
  return result;
}

bool ROMFile::checkSong(uint32_t addr, bool deep) const
{
  try {
    uint8_t numTracks = rom[addr];
    if (!numTracks) return !deep;
    uint32_t end = addr + 8 + numTracks * 4;
    if (/*(deep && !rom[addr + 1]) || */ end >= rom.size()) {
      return false;
    }
    for (int p = addr + 4; p < end; p += 4) {
      if ((rom[p + 3] & 0xFE) != 0x08) {
        return false;
      }
      uint32_t data = cleanDeref(p, 12, false);
      if (data == BAD_PTR) return false;
      if (!deep) continue;
      if (p == addr + 4) {
        // tone data
        if (rom[data + 2]) return false;
        uint8_t inst = rom[data];
        if (inst > 12 && inst != 16 && inst != 32 && inst != 64 && inst != 128) return false;
        uint32_t wave = parseInt<uint32_t>(rom, data + 4);
        if (inst & 0x7) {
          if (inst & 0x7 == 4) {
            if (wave > 1) return false;
          } else if (inst & 0x7 != 3) {
            if (wave > 3) return false;
          }
          if (rom[data + 8] > 7) return false;
          if (rom[data + 9] > 7) return false;
          if (rom[data + 10] > 15) return false;
          if (rom[data + 11] > 7) return false;
        }
        if (inst == 0 || inst == 8 || inst == 3 || inst == 11 || inst == 16 || inst == 32) {
          wave = cleanPointer(wave, 16);
          if (wave == BAD_PTR) return false;
          if (inst & 0x7 == 0) {
            if (rom[wave] || rom[wave + 1] || rom[wave + 2] || rom[wave + 3] & ~0x40) return false;
          }
        } else if (inst == 64 || inst == 128) {
          if (rom[data + 1] || rom[data + 2] || rom[data + 3]) return false;
          if (cleanPointer(wave, 16) == BAD_PTR) return false;
          if (inst == 64) {
            if (cleanDeref(data + 8, 128) == BAD_PTR) return false;
          } else {
            if (parseInt<uint32_t>(rom, data + 8)) return false;
          }
        }
      } else {
        // track data
        uint8_t cmd = rom[data];
        // track has no events
        if (deep && cmd == 0xB1) return false;
        // track starts with running status
        if (cmd < 0x80) return false;
        // track starts with unknown command
        if (cmd == 0xC8 || cmd == 0xC9 || cmd == 0xCA || cmd == 0xCB || cmd == 0xCC) return false;
      }
    }
    return true;
  } catch (...) {
    return false;
  }
}
