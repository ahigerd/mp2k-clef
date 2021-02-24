#ifndef GBAMP2WAV_SONGTABLE_H
#define GBAMP2WAV_SONGTABLE_H

#include <vector>
#include <cstdint>
class ROMFile;
class SongData;

class SongTable {
public:
  SongTable(const ROMFile* rom);
  SongTable(SongTable&& other) = default;

  const ROMFile* const rom;
  uint32_t tableStart, tableEnd;
  std::vector<uint32_t> songs;

  SongData* songAt(uint32_t addr) const;
  SongData* song(size_t index) const;
  SongData* songFromTable(size_t index) const;
};

#endif
