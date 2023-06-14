#include "plugin/baseplugin.h"
#include "codec/sampledata.h"
#include "romfile.h"
#include "songtable.h"
#include "songdata.h"
#include <sstream>
#include <iomanip>
#include <map>

// In the functions below, openFile() is provided by the plugin interface. Use this
// instead of standard library functions to open additional files in order to use
// the host's virtual filesystem.

static SynthContext* openBySubsong(S2WContext* ctx, std::unique_ptr<ROMFile>& rom, std::unique_ptr<SongData>& songData, const std::string& filename, std::istream& file)
{
  SynthContext* synth = nullptr;
  try {
    synth = new SynthContext(ctx, 32768);
    rom.reset(new ROMFile(ctx));
    if (file) {
      rom->load(file);
    } else {
      size_t qpos = filename.rfind('?');
      auto newFile(ctx->openFile(filename.substr(0, qpos)));
      rom->load(*newFile);
    }
    std::cerr << "loaded " << rom->rom.size() << " bytes " << filename << std::endl;

    size_t qpos = filename.rfind('?');
    std::string subsong;
    if (qpos != std::string::npos) {
      subsong = filename.substr(qpos + 1);
    }
    if (subsong.substr(0, 2) == "0x") {
      uint32_t addr = std::stoi(subsong, nullptr, 0);
      std::cerr << "loading " << std::hex << addr << std::endl;
      songData.reset(new SongData(rom.get(), addr));
    } else {
      SongTable st = rom->findSongTable(-1);
      int index = std::stoi(subsong.empty() ? "0" : subsong);
      std::cerr << "loading index " << index << std::endl;
      songData.reset(st.songFromTable(index));
    }

    for (int i = 0; i < songData->numTracks(); i++) {
      synth->addChannel(songData->getTrack(i));
    }

    return synth;
  } catch (...) {
    delete synth;
    return nullptr;
  }
}

static std::map<std::string, double> durationCache;
static std::map<std::string, std::vector<std::string>> subsongCache;

struct S2WPluginInfo {
  S2WPLUGIN_STATIC_FIELDS

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
    std::unique_ptr<SongData> lengthSong;
    std::unique_ptr<ROMFile> lengthRom;
    std::unique_ptr<SynthContext> synth(openBySubsong(ctx, lengthRom, lengthSong, filename, file));
    double length = 0;
    if (synth) {
      length = synth->maximumTime();
    }
    durationCache[filename] = length;
    return length;
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

    std::vector<std::string> subsongs;
    ROMFile rom(s2w);
    rom.load(file);
    SongTable st = rom.findAllSongs();

    for (uint32_t song : st.songs) {
      std::ostringstream ss;
      ss << base << "?0x" << std::hex << std::setw(6) << std::setfill('0') << song;
      subsongs.push_back(ss.str());
    }

    subsongCache[base] = subsongs;
    return subsongs;
  }

  SynthContext* prepare(S2WContext* ctx, const std::string& filename, std::istream& file) {
    // Prepare to play the file. Load any necessary data into memory and store any
    // applicable state in members on this plugin object.

    // Be sure to call this to clear the sample cache:
    ctx->purgeSamples();

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
ConstPairList S2WPluginInfo::extensions = { { "gba", "GBA ROM images (*.gba)" } };
const std::string S2WPluginInfo::about =
  "gbamp2wav copyright (C) 2020-2023 Adam Higerd\n"
  "Distributed under the MIT license.";

SEQ2WAV_PLUGIN(S2WPluginInfo);
