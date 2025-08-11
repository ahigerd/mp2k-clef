#ifndef GBAMP2WAV_INSTRUMENTDATA_H
#define GBAMP2WAV_INSTRUMENTDATA_H

#include <vector>
#include <cstdint>
#include <memory>
#include <string>
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
    Square1_Alt = 9,
    Square2_Alt = 10,
    GBSample_Alt = 11,
    Noise_Alt = 12,
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

  inline bool operator==(const MpInstrument& other) const { return *this == &other; }
  bool operator==(const MpInstrument* other) const;

  virtual void showParsed(std::ostream& out, std::string indent = std::string()) const;

protected:
  Channel::Note* addEnvelope(Channel* channel, Channel::Note* event, double factor) const;
};

class SampleInstrument : public MpInstrument {
public:
  SampleInstrument(const ROMFile* rom, uint32_t addr);

  SampleData* sample;

  virtual BaseNoteEvent* makeEvent(double volume, uint8_t key, uint8_t vel, double len) const;
  virtual Channel::Note* noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event);
  virtual std::string displayName() const;

  //virtual void showParsed(std::ostream& out, std::string indent = std::string()) const;
};

class PSGInstrument : public MpInstrument {
public:
  PSGInstrument(const ROMFile* rom, uint32_t addr);

  uint8_t mode, sweep;

  virtual BaseNoteEvent* makeEvent(double volume, uint8_t key, uint8_t vel, double len) const;
  virtual Channel::Note* noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event);
  virtual std::string displayName() const;

  //virtual void showParsed(std::ostream& out, std::string indent = std::string()) const;
};

class SplitInstrument : public MpInstrument {
public:
  SplitInstrument(const ROMFile* rom, uint32_t addr);

  std::vector<std::shared_ptr<MpInstrument>> splits;

  virtual BaseNoteEvent* makeEvent(double volume, uint8_t key, uint8_t vel, double len) const;
  virtual Channel::Note* noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event);
  virtual std::string displayName() const;

  virtual void showParsed(std::ostream& out, std::string indent = std::string()) const;
};

class InstrumentData {
public:
  InstrumentData(const ROMFile* rom, uint32_t addr);

  uint32_t instruments[128];

private:
  MpInstrument* findDupe(SynthContext* synth, const MpInstrument& inst) const;
};

#endif
