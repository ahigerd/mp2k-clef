#include "plugin/baseplugin.h"
#include "codec/sampledata.h"
#include "romfile.h"
#include "songtable.h"
#include "songdata.h"
#include <sstream>
#include <iomanip>
#include <map>

#ifdef BUILD_CLAP
#include "plugin/clapplugin.h"
#endif

// In the functions below, openFile() is provided by the plugin interface. Use this
// instead of standard library functions to open additional files in order to use
// the host's virtual filesystem.

static SynthContext* openBySubsong(S2WContext* ctx, std::unique_ptr<ROMFile>& rom, std::unique_ptr<SongData>& songData, const std::string& filename, std::istream& file)
{
  SynthContext* synth = nullptr;
  try {
    synth = new SynthContext(ctx, 32768);
    size_t qpos = filename.rfind('?');
    std::string baseFile = filename.substr(0, qpos);
    bool alreadyLoaded = rom && rom->filename == baseFile;
    if (!alreadyLoaded) {
      rom.reset(new ROMFile(ctx));
    }
    if (file) {
      rom->load(synth, file, baseFile);
    } else {
      auto newFile(ctx->openFile(baseFile));
      rom->load(synth, *newFile, baseFile);
    }

    if (ctx->isDawPlugin) {
      SongTable st = rom->findSongTable(-1);
      int numSongs = st.songs.size();
      for (int index = 0; index < numSongs; index++) {
        // initialize instruments
        try {
          st.songFromTable(index);
        } catch (...) {
          // ignore
        }
      }
    } else {
      std::string subsong;
      if (qpos != std::string::npos) {
        subsong = filename.substr(qpos + 1);
      }
      if (subsong.substr(0, 2) == "0x") {
        uint32_t addr = std::stoi(subsong, nullptr, 0);
        songData.reset(new SongData(rom.get(), addr));
      } else {
        SongTable st = rom->findSongTable(-1);
        int index = std::stoi(subsong.empty() ? "0" : subsong);
        do {
          try {
            songData.reset(st.songFromTable(index));
            break;
          } catch (std::exception& e) {
            ++index;
            if (index >= st.songs.size()) {
              throw;
            }
          }
        } while (!songData);
      }

      for (int i = 0; i < songData->numTracks(); i++) {
        synth->addChannel(songData->getTrack(i));
      }
    }

    return synth;
  } catch (std::exception& e) {
    std::cerr << "error in openBySubsong " << e.what() << std::endl;
    delete synth;
    throw;
  }
}

static std::map<std::string, double> durationCache;
static std::map<std::string, std::vector<std::string>> subsongCache;

struct S2WPluginInfo {
  S2WPLUGIN_STATIC_FIELDS
#ifdef BUILD_CLAP
  using ClapPlugin = S2WClapPlugin<S2WPluginInfo>;
#endif

  std::unique_ptr<ROMFile> rom;
  std::unique_ptr<SongData> songData;

  static bool isPlayable(std::istream& file) {
    // Implementations should check to see if the file is supported.
    // Return false or throw an exception to report failure.
    return true;
  }

  static int sampleRate(S2WContext* ctx, const std::string& filename, std::istream& file) {
    // Implementations should return the sample rate of the file.
    // This can be hard-coded if the plugin always uses the same sample rate.
    return 32768;
  }

  static double length(S2WContext* ctx, const std::string& filename, std::istream& file) {
    // Implementations should return the length of the file in seconds.
    auto iter = durationCache.find(filename);
    if (iter != durationCache.end()) {
      return iter->second;
    }
    size_t qpos = filename.rfind('?');
    std::string base = filename.substr(0, qpos);
    std::unique_ptr<ROMFile> lengthRom(new ROMFile(ctx));
    lengthRom->load(nullptr, file, base);
    SongTable st = lengthRom->findAllSongs();
    std::vector<std::string> subsongs;
    bool first = true;
    for (uint32_t song : st.songs) {
      std::ostringstream ss;
      ss << base << "?0x" << std::hex << std::setw(6) << std::setfill('0') << song;
      std::string subsong = ss.str();

      std::unique_ptr<SongData> lengthSong;
      double length = 0;
      try {
        std::unique_ptr<SynthContext> synth(openBySubsong(ctx, lengthRom, lengthSong, subsong, file));
        if (synth) {
          length = synth->maximumTime();
          subsongs.push_back(subsong);
        }
      } catch (...) {
        // ignore
      }
      durationCache[subsong] = length;
      if (first && filename != subsong) {
        durationCache[filename] = length;
      }
    }
    subsongCache[base] = subsongs;
    return durationCache[filename];
  }

  static TagMap readTags(S2WContext* ctx, const std::string& filename, std::istream& file) {
    // Implementations should read the tags from the file.
    // If the file format does not support embedded tags, consider
    // inheriting from TagsM3UMixin and removing this function.
    return TagMap();
  }

  static std::vector<std::string> getSubsongs(S2WContext* s2w, const std::string& filename, std::istream& file)
  {
    std::string base = filename.substr(0, filename.rfind('?'));

    auto iter = subsongCache.find(base);
    if (iter != subsongCache.end()) {
      return iter->second;
    }
    subsongCache[base] = std::vector<std::string>();
    length(s2w, base, file);
    return subsongCache.at(base);
  }

  SynthContext* prepare(S2WContext* ctx, const std::string& filename, std::istream& file) {
    // Prepare to play the file. Load any necessary data into memory and store any
    // applicable state in members on this plugin object.

    // Be sure to call this to clear the sample cache:
    ctx->purgeSamples();
    rom.reset(nullptr);

    return openBySubsong(ctx, rom, songData, filename, file);
  }

  void release() {
    // Release any retained state allocated in prepare().
    songData.reset(nullptr);
    rom.reset(nullptr);
  }
};

const std::string S2WPluginInfo::version = "0.0.1";
const std::string S2WPluginInfo::pluginName = "gbamp2wav";
const std::string S2WPluginInfo::pluginShortName = "gbamp2wav";
const std::string S2WPluginInfo::author = "Adam Higerd";
const std::string S2WPluginInfo::url = "https://bitbucket.org/ahigerd/gbamp2wav";
ConstPairList S2WPluginInfo::extensions = { { "gba", "GBA ROM images (*.gba)" } };
const std::string S2WPluginInfo::about =
  "gbamp2wav copyright (C) 2020-2023 Adam Higerd\n"
  "Distributed under the MIT license.";

SEQ2WAV_PLUGIN(S2WPluginInfo);
