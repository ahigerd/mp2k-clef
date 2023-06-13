#ifndef GBAMP2WAV_SONGTABLE_H
#define GBAMP2WAV_SONGTABLE_H

#include <vector>
#include <cstdint>
class ROMFile;
class SongData;

using std::size_t;

class SongTable {
public:
  SongTable();
  SongTable(const ROMFile* rom);
  SongTable(const SongTable& other) = default;
  SongTable(SongTable&& other) = default;
  SongTable& operator=(const SongTable& other) = default;
  SongTable& operator=(SongTable&& other) = default;

  const ROMFile* rom;
  uint32_t tableStart, tableEnd;
  std::vector<uint32_t> songs;

  SongData* songAt(uint32_t addr) const;
  SongData* song(size_t index) const;
  SongData* songFromTable(size_t index) const;
};

#endif
