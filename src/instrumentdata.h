#ifndef GBAMP2WAV_INSTRUMENTDATA_H
#define GBAMP2WAV_INSTRUMENTDATA_H

#include <vector>
#include <cstdint>
#include <memory>
class ROMFile;
class SampleData;
class SequenceEvent;

class MpInstrument {
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
  virtual SequenceEvent* makeEvent(double volume, uint8_t key, uint8_t vel, double len) const = 0;

protected:
  SequenceEvent* addEnvelope(SequenceEvent* event, double factor) const;
};

class SampleInstrument : public MpInstrument {
public:
  SampleInstrument(const ROMFile* rom, uint32_t addr);

  SampleData* sample;

  virtual SequenceEvent* makeEvent(double volume, uint8_t key, uint8_t vel, double len) const;
};

class PSGInstrument : public MpInstrument {
public:
  PSGInstrument(const ROMFile* rom, uint32_t addr);

  uint8_t mode, sweep;

  virtual SequenceEvent* makeEvent(double volume, uint8_t key, uint8_t vel, double len) const;
};

class SplitInstrument : public MpInstrument {
public:
  SplitInstrument(const ROMFile* rom, uint32_t addr);

  std::vector<std::shared_ptr<MpInstrument>> splits;

  virtual SequenceEvent* makeEvent(double volume, uint8_t key, uint8_t vel, double len) const;
};

class InstrumentData {
public:
  InstrumentData(const ROMFile* rom, uint32_t addr);

  std::vector<std::shared_ptr<MpInstrument>> instruments;
};

#endif
