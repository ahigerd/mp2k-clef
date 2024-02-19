#include "romfile.h"
#include "songtable.h"
#include "songdata.h"
#include "instrumentdata.h"
#include "utility.h"
#include "clefcontext.h"
#include "synth/synthcontext.h"
#include "riffwriter.h"
#include "commandargs.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <sstream>

static int scanSongTables(const std::string& path, bool doValidate)
{
  ClefContext clef;
  ROMFile rom(&clef);
  rom.load(nullptr, path);

  std::vector<SongTable> sts = rom.findSongTables();
  if (sts.empty()) {
    std::cerr << "No song tables found." << std::endl;
    return 1;
  }

  for (const auto& st : rom.findSongTables()) {
    std::cerr << "Song table @ 0x" << std::hex << st.tableStart << std::dec << ": " << st.songs.size() << " songs (table size " << ((st.tableEnd - st.tableStart) / 8) << ")" << std::endl;
    for (uint32_t addr = st.tableStart; addr < st.tableEnd; addr += 8) {
      uint32_t song = rom.readPointer(addr);
      if (!doValidate && std::find(st.songs.begin(), st.songs.end(), song) == st.songs.end()) {
        continue;
      }
      int idx = (addr - st.tableStart) / 8;
      std::cerr << "\t" << std::setfill(' ') << std::setw(4) << idx << " Song @ 0x" << std::hex << std::setw(8) << std::setfill('0') << song << std::dec << " - ";
      try {
        std::unique_ptr<SongData> sd(st.songAt(song));
        std::cerr << sd->numTracks() << " tracks" << std::endl;
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
      } catch (...) {
        std::cerr << "unknown error" << std::endl;
      }
    }
  }
  return 0;
}

static int scanAllSongs(const std::string& path)
{
  ClefContext clef;
  ROMFile rom(&clef);
  rom.load(nullptr, path);

  SongTable st = rom.findAllSongs();
  if (st.songs.empty()) {
    std::cerr << "No songs found." << std::endl;
    return 1;
  }

  for (uint32_t song : st.songs) {
    std::unique_ptr<SongData> sd(st.songAt(song));
    std::cerr << "Song @ 0x" << std::hex << song << std::dec << " - " << sd->numTracks() << " tracks" << std::endl;
    std::cerr << std::endl;
  }
  return 0;
}

int main(int argc, char** argv)
{
  CommandArgs args({
    { "help", "h", "", "Show this help text" },
    { "output", "o", "filename", "Specify the output filename" },
    { "scan", "s", "", "Scan for song tables" },
    { "scan-songs", "S", "", "Scan for songs, even without tables" },
    { "validate", "V", "", "Validate songs when scanning" },
    { "table", "t", "location", "Use a specific song table" },
    { "parse", "p", "", "Output parsed sequence data instead of audio" },
    { "instruments", "i", "", "Output parsed instrument data instead of audio" },
    { "", "", "input", "Path to the input file" },
    { "", "", "song", "Song index or sequence offset" },
  });

  std::string argError = args.parse(argc, argv);
  if (!argError.empty()) {
    std::cerr << argError << std::endl;
    return 1;
  }

  if (args.hasKey("help") || args.positional().empty()) {
    std::cerr << args.usageText(argv[0]) << std::endl;
    return 0;
  }

  std::string src = args.positional()[0];

  if (args.hasKey("scan")) {
    return scanSongTables(src, args.hasKey("validate"));
  }

  if (args.hasKey("scan-songs")) {
    return scanAllSongs(src);
  }

  if (args.positional().size() < 2) {
    std::cerr << args.usageText(argv[0]) << std::endl;
    return 1;
  }

  ClefContext clef;
  SynthContext ctx(&clef, 32768);
  ROMFile rom(&clef);
  rom.load(&ctx, src);

  std::string songSelection = args.positional()[1];

  SongTable songTable;
  bool byAddr = songSelection.substr(0, 2) == "0x";
  if (args.hasKey("table")) {
    std::string tbl(args.getString("table"));
    uint32_t songTableAddr = 0;
    if (tbl.size() > 2 && tbl[1] == 'x') {
      songTableAddr = args.getInt("table");
      songTable = rom.findSongTable(1, songTableAddr);
      if (songTable.songs.empty()) {
        std::cerr << "Song table " << args.getInt("table") << " not found" << std::endl;
        return 1;
      }
    } else {
      int songTableIdx = args.getInt("table");
      do {
        songTable = rom.findSongTable(1, songTableAddr);
        if (songTable.songs.empty()) {
          std::cerr << "Song table " << args.getInt("table") << " not found" << std::endl;
          return 1;
        }
        songTableIdx--;
        songTableAddr = songTable.tableEnd;
      } while (songTableIdx > 0);
    }
  } else if (byAddr) {
    songTable = rom.findAllSongs();
  } else {
    songTable = rom.findSongTable(-1);
    if (songTable.songs.empty()) {
      std::cerr << "No song table found" << std::endl;
      return 1;
    }
  }

  std::unique_ptr<SongData> sd;
  try {
    if (byAddr) {
      uint32_t addr = 0;
      addr = std::stoi(songSelection, nullptr, 16);
      sd.reset(songTable.songAt(addr));
    } else {
      uint32_t index = ~0;
      index = std::stoi(songSelection);
      sd.reset(songTable.songFromTable(index));
    }
    if (!sd) {
      std::cerr << "Could not load song " << songSelection << std::endl;
      return 1;
    }
  } catch (std::exception& e) {
    std::cerr << "An error occurred while loading song " << songSelection << std::endl;
    std::cerr << "\t" << e.what() << std::endl;
    return 1;
  }

  std::string filename = args.getString("output");
  if (args.hasKey("parse")) {
    if (filename.empty()) {
      sd->showParsed(std::cout);
    } else {
      std::ofstream out(filename);
      sd->showParsed(out);
    }
    return 0;
  } else if (args.hasKey("instruments")) {
    std::ofstream out;
    if (!filename.empty()) {
      out.open(filename);
    }
    std::ostream& oout = filename.empty() ? std::cout : out;
    for (int i = 0; i < 128; i++) {
      auto inst = sd->getInstrument(i);
      if (inst) {
        oout << "Instrument " << i << " - ";
        inst->showParsed(oout);
        oout << std::endl;
      }
    }
    return 0;
  }

  for (int i = 0; i < sd->numTracks(); i++) {
    ctx.addChannel(sd->getTrack(i));
  }

  if (filename.empty()) {
    std::ostringstream fnss;
    fnss << src << "." << songSelection << ".wav";
    filename = fnss.str();
  }
  std::cerr << "Writing " << (int(ctx.maximumTime() * 10) * .1) << " seconds to \"" << filename << "\"..." << std::endl;
  RiffWriter riff(ctx.sampleRate, true);
  riff.open(filename);
  ctx.save(&riff);
  riff.close();
  return 0;
}
