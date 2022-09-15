#include "plugin/baseplugin.h"
#include "codec/sampledata.h"

// In the functions below, openFile() is provided by the plugin interface. Use this
// instead of standard library functions to open additional files in order to use
// the host's virtual filesystem.

struct S2WPluginInfo {
  S2WPLUGIN_STATIC_FIELDS

  static bool isPlayable(std::istream& file) {
    // Implementations should check to see if the file is supported.
    // Return false or throw an exception to report failure.
    return false;
  }

  static int sampleRate(S2WContext* ctx, const std::string& filename, std::istream& file) {
    // Implementations should return the sample rate of the file.
    // This can be hard-coded if the plugin always uses the same sample rate.
    return 48000;
  }

  static double length(S2WContext* ctx, const std::string& filename, std::istream& file) {
    // Implementations should return the length of the file in seconds.
    return 0;
  }

  static TagMap readTags(S2WContext* ctx, const std::string& filename, std::istream& file) {
    // Implementations should read the tags from the file.
    // If the file format does not support embedded tags, consider
    // inheriting from TagsM3UMixin and removing this function.
    return TagMap();
  }

  SynthContext* prepare(S2WContext* ctx, const std::string& filename, std::istream& file) {
    // Prepare to play the file. Load any necessary data into memory and store any
    // applicable state in members on this plugin object.

    // Be sure to call this to clear the sample cache:
    ctx->purgeSamples();

    return nullptr;
  }

  void release() {
    // Release any retained state allocated in prepare().
  }
};

const std::string S2WPluginInfo::version = "0.0.1";
const std::string S2WPluginInfo::pluginName = "Template Plugin";
const std::string S2WPluginInfo::pluginShortName = "template";
ConstPairList S2WPluginInfo::extensions = { { "dummy", "Dummy files (*.dummy)" } };
const std::string S2WPluginInfo::about =
  "Template Plugin copyright (C) 2020-2022 Adam Higerd\n"
  "Distributed under the MIT license.";

SEQ2WAV_PLUGIN(S2WPluginInfo);
