#include "songtable.h"
#include "songdata.h"
#include "romfile.h"

SongTable::SongTable(const ROMFile* rom)
: rom(rom), tableStart(0), tableEnd(0)
{
  // initializers only
}

SongData SongTable::songAt(uint32_t addr) const
{
  return SongData(rom, addr);
}

SongData SongTable::song(size_t index) const
{
  return songAt(songs.at(index));
}

SongData SongTable::songFromTable(size_t index) const
{
  uint32_t ptr = tableStart + 8 * index;
  if (ptr >= tableEnd) {
    throw std::out_of_range("song index out of range");
  }
  return songAt(rom->readPointer(tableStart + 8 * index));
}
