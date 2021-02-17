#include "romfile.h"
#include "songtable.h"
#include "songdata.h"
#include "utility.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <cstdlib>

int main(int argc, char** argv)
{
  if (argc < 2) return 1;
  ROMFile rom;
  uint8_t buffer[1024];
  std::ifstream f(argv[1]);
  while (f) {
    f.read(reinterpret_cast<char*>(buffer), sizeof(buffer));
    rom.rom.insert(rom.rom.end(), buffer, buffer + f.gcount());
  }
  std::cout << "Read " << argv[1] << ": " << rom.rom.size() << " bytes." << std::endl;
  if (argc > 2) {
    SongTable st(rom.findSongTable(-1));
    std::string tag(argv[2]);
    std::unique_ptr<SongData> sd;
    uint32_t index = ~0;
    if (tag.size() > 2 && tag[1] == 'x') {
      uint32_t addr = std::strtol(argv[2] + 2, nullptr, 16);
      sd = std::make_unique<SongData>(st.songAt(addr));
    } else {
      index = std::atoi(argv[2]);
      sd = std::make_unique<SongData>(st.songFromTable(index));
    }
    bool ok = rom.checkSong(sd->addr, true);
    if (!ok) {
      std::cout << "Check failed." << std::endl;
      return 1;
    }
    std::cout << "Validated." << std::endl;
    if (index == ~0) {
      std::cout << "Song @";
    } else {
      std::cout << "Song #" << index << ":";
    }
    std::cout << " 0x" << std::hex << sd->addr << std::endl;
    std::cout << std::dec << "\tTracks: " << sd->numTracks() << std::endl;
    return 0;
  }
  //SongTable st(rom.findSongTable(-1));
  SongTable st(rom.findAllSongs());
  std::cout << "Song table: " << st.songs.size() << std::endl;
  int i = 0;
  //for (uint32_t song : st.songs) {
  uint32_t song;
  //std::cout << "..." << std::dec << ((st.tableEnd - st.tableStart) / 8) << std::endl;
  //for (uint32_t song : st.songs) std::cout << song << std::endl;
  //for (uint32_t ptr = st.tableStart; ptr < st.tableEnd; ptr += 8, i++) {
  //  song = rom.readPointer(ptr | 0x08000000, false);
  for (uint32_t song : st.songs) {
    std::cout << std::dec << "(" << i << ") " << std::hex << /*ptr <<*/ " -> " << song << std::endl;
    if (rom.checkSong(song)) {
      std::cout << "\t(" << std::dec << i << ") " << std::hex << song << std::dec << ": " << int(rom.rom[song]) << " " << (rom.checkSong(song) ? "+" : "-") << std::endl;
    }
    ++i;
    try {
      st.songAt(song);
    } catch (std::runtime_error& e) {
      std::cout << "\t\t" << e.what() << std::endl;
    }
  }

  return 0;
}
