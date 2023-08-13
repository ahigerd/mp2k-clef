#ifndef GBAMP2WAV_INSTRUMENTDATA_H
#define GBAMP2WAV_INSTRUMENTDATA_H

#include <vector>
#include <cstdint>
#include <memory>
#include "synth/iinstrument.h"
class ROMFile;
class SampleData;
struct BaseNoteEvent;
class SynthContext;

class MpInstrument: public IInstrument {
public:
  static MpInstrument* load(const ROMFile* rom, uint32_t addr, bool isSplit = false);
  MpInstrument(const ROMFile* rom, uint32_t addr);
  virtual ~MpInstrument() {}

  const ROMFile* rom;
  uint32_t addr;

  enum Type {
    Sample = 0,
    Square1 = 1,
    Square2 = 2,
    GBSample = 3,
    Noise = 4,
    FixedSample = 8,
    KeySplit = 0x40,
    Percussion = 0x80,
  };
  Type type;

  double attack, decay, sustain, release;
  bool forcePan;
  uint8_t pan;
  double gate;
  virtual BaseNoteEvent* makeEvent(double volume, uint8_t key, uint8_t vel, double len) const = 0;
  virtual Channel::Note* noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event) = 0;

protected:
  Channel::Note* addEnvelope(Channel* channel, Channel::Note* event, double factor) const;
};

class SampleInstrument : public MpInstrument {
public:
  SampleInstrument(const ROMFile* rom, uint32_t addr);

  SampleData* sample;

  virtual BaseNoteEvent* makeEvent(double volume, uint8_t key, uint8_t vel, double len) const;
  virtual Channel::Note* noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event);
};

class PSGInstrument : public MpInstrument {
public:
  PSGInstrument(const ROMFile* rom, uint32_t addr);

  uint8_t mode, sweep;

  virtual BaseNoteEvent* makeEvent(double volume, uint8_t key, uint8_t vel, double len) const;
  virtual Channel::Note* noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event);
};

class SplitInstrument : public MpInstrument {
public:
  SplitInstrument(const ROMFile* rom, uint32_t addr);

  std::vector<std::shared_ptr<MpInstrument>> splits;

  virtual BaseNoteEvent* makeEvent(double volume, uint8_t key, uint8_t vel, double len) const;
  virtual Channel::Note* noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event);
};

class InstrumentData {
public:
  InstrumentData(const ROMFile* rom, uint32_t addr);

  std::vector<MpInstrument*> instruments;
};

#endif
