#ifndef GBAMP2WAV_SONGDATA_H
#define GBAMP2WAV_SONGDATA_H

#include "seq/isequence.h"
#include "seq/itrack.h"
#include "instrumentdata.h"
#include <unordered_map>
class ROMFile;
class SongData;
class SynthContext;

struct RawEvent {
  uint32_t addr;
  uint8_t opcode;
  std::vector<uint32_t> args;

  std::string render() const;
};

struct Mp2kEvent {
  enum Type {
    Note,
    Rest,
    Param,
    Goto,
    Stop,
  };

  uint64_t effAddr;
  Type type;
  uint16_t value;
  uint8_t param;
  uint8_t duration;
  RawEvent raw;
};

class TrackData : public ITrack {
public:
  struct ActiveNote {
    uint64_t playbackID;
    double releaseTime, endTime;
    bool released;
  };

  TrackData(SongData* song, int index, uint32_t addr, MpInstrument* defaultInst);
  ~TrackData();

  virtual bool isFinished() const;

  virtual double length() const;

  int trackIndex;
  SongData* const song;
  const uint32_t addr;
  bool hasLoop;

  void showParsed(std::ostream& out);

protected:
  virtual std::shared_ptr<SequenceEvent> readNextEvent();
  virtual void internalReset();

  size_t playIndex;
  double playTime;
  double secPerTick;
  mutable double lengthCache;
  MpInstrument* currentInstrument;
  std::vector<RawEvent> rawEvents;
  std::vector<Mp2kEvent> events;
  std::vector<std::shared_ptr<SequenceEvent>> pendingEvents;
  std::unordered_map<uint8_t, ActiveNote> activeNotes;
  double bendRange;
  double releaseTime;
  uint8_t transpose;
  double tuning;
  double volume;
  bool stopped : 1;
};

class SongData : public BaseSequence<TrackData> {
public:
  SongData(const ROMFile* rom, uint32_t addr);
  SongData(const SongData& other) = delete;
  SongData(SongData&& other) = delete;
  SongData& operator=(const SongData& other) = delete;
  SongData& operator=(SongData&& other) = delete;

  virtual bool canLoop() const;
  MpInstrument* getInstrument(uint8_t id) const;

  const ROMFile* const rom;
  const uint32_t addr;
  InstrumentData instruments;

  void showParsed(std::ostream& out);

  std::vector<std::pair<double, double>> tempos;
  std::unordered_map<uint8_t, TrackData::ActiveNote> activePsg;

  double tickLengthAt(double timestamp) const;

private:
  bool hasLoop;
};

#endif
