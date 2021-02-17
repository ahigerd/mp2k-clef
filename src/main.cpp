#include "romfile.h"
#include "songtable.h"
#include "songdata.h"
#include "utility.h"
#include <iostream>
#include <fstream>
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
    uint32_t index = std::atoi(argv[2]);
    SongTable st(rom.findSongTable(-1));
    SongData sd = st.songFromTable(index);
    bool ok = rom.checkSong(sd.addr, true);
    if (!ok) {
      std::cout << "Check failed." << std::endl;
      return 1;
    }
    std::cout << "Validated." << std::endl;
    std::cout << "Song #" << index << ": 0x" << std::hex << sd.addr << std::endl;
    std::cout << std::dec << "\tTracks: " << sd.numTracks() << std::endl;
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
