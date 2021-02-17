#ifndef GBAMP2WAV_SONGDATA_H
#define GBAMP2WAV_SONGDATA_H

#include "seq/isequence.h"
#include "seq/itrack.h"
class ROMFile;

class GotoEvent : public BaseEvent<SequenceEvent::UserBase + 0> {
public:
  GotoEvent(size_t index);
  size_t index;
};

class RestEvent : public BaseEvent<SequenceEvent::UserBase + 1> {
public:
  RestEvent(uint8_t ticks);
  uint8_t ticks;
};

class ParamEvent : public BaseEvent<SequenceEvent::UserBase + 2> {
public:
  ParamEvent(uint8_t param, uint8_t value);
  uint8_t param, value;
};

class TrackData : public BasicTrack {
public:
  TrackData(const ROMFile* rom, uint32_t addr);
  ~TrackData();

  //virtual bool isFinished() const;
  //virtual std::shared_ptr<SequenceEvent> nextEvent();

  //virtual void reset();
  //virtual void seek(double timestamp);
  virtual double length() const;

  const ROMFile* const rom;
  const uint32_t addr;
  bool hasLoop;

protected:
  virtual std::shared_ptr<SequenceEvent> readNextEvent();
  virtual void internalReset();

  void addNoteEvent(uint8_t instrument, uint8_t noteKey, uint8_t noteVel, uint8_t noteLen);

  size_t playIndex;
  double playTime;
  double secPerTick;
};

class SongData : public BaseSequence<TrackData> {
public:
  SongData(const ROMFile* rom, uint32_t addr);

  virtual bool canLoop() const;

  const ROMFile* const rom;
  const uint32_t addr;

private:
  bool hasLoop;
};

#endif
