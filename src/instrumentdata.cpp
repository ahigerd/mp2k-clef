#include "instrumentdata.h"
#include "romfile.h"
#include "utility.h"
#include "s2wcontext.h"
#include "codec/sampledata.h"
#include "codec/pcmcodec.h"
#include "seq/sequenceevent.h"
#include "synth/oscillator.h"
#include "riffwriter.h"
#include <sstream>
#include <cmath>

/*
class SweepNode : public AudioNode
{
public:
  SweepNode(const SynthContext* ctx, AudioNode* source, uint8_t sweep)
  : AudioNode(ctx), source(source), lastTime(0), active(true)
  {
    freq = source->param(BaseOscillator::Frequency).get();
    if (!freq) {
      active = false;
      source = nullptr;
      return;
    }
    sweepShift = sweep & 0x7;
    sweepNegate = sweep & 0x8;
    sweepPeriod = (sweep >> 4) & 0x7;
    if (!sweepShift || !sweepPeriod) {
      sweepShift = 0;
      sweepPeriod = 0;
    }
  }

  bool isActive() const {
    return active;
  }

protected:
  int16_t generateSample(double time, int channel) {
    int16_t sample = source->getSample(time, channel);
    if (!sweepShift) {
      return sample;
    }
    double delta = (time - lastTime) * 128.0;
    if (delta >= sweepPeriod) {
      int16_t timer = 4194304.0 / freq->valueAt(time);
      int16_t adj = timer >> sweepShift;
      timer = sweepNegate ? timer - adj : timer + adj;
      if (timer <= 0) {
        sweepShift = 0;
      } else if (timer >= 2048) {
        active = false;
      } else {
        freq->setConstant(4194304.0 / timer);
      }
      delta -= sweepPeriod;
      lastTime = time - delta;
    }
    return sample;
  }

  AudioNode* source;
  AudioParam* freq;

  double lastTime;
  uint8_t sweepPeriod;
  uint8_t sweepShift;
  bool sweepNegate : 1;
  bool active : 1;
  bool squelch : 1;
};
*/

MpInstrument* MpInstrument::load(const ROMFile* rom, uint32_t addr, bool isSplit)
{
  if (addr == 0x80808080) {
    return nullptr;
  }
  uint8_t type = rom->read<uint8_t>(addr);
  uint8_t normType = type & 0x7 ? type & 0x7 : type;
  try {
    switch (normType) {
      case GBSample:
        return nullptr;
      case Sample:
      case FixedSample:
        return new SampleInstrument(rom, addr);
      case Square1:
      case Square2:
      case Noise:
        return nullptr;
        return new PSGInstrument(rom, addr);
      case KeySplit:
      case Percussion:
        if (!isSplit) {
          return new SplitInstrument(rom, addr);
        }
      default:
        //std::cerr << "0x" << std::hex << addr << ": Unknown instrument type " << std::dec << (int)type << std::endl;
        return nullptr;
    }
  } catch (ROMFile::BadAccess& e) {
    std::cerr << "Bad pointer loading " << int(type) << " instrument at 0x" << std::hex << addr << std::dec << std::endl;
    return nullptr;
  } catch (std::exception& e) {
    std::cerr << "Error loading " << int(type) << " instrument at 0x" << std::hex << addr << std::dec << ": " << e.what() << std::endl;
    return nullptr;
  }
}

MpInstrument::MpInstrument(const ROMFile* rom, uint32_t addr)
: rom(rom), addr(addr), type(Type(rom->read<uint8_t>(addr))), forcePan(false), pan(0), gate(0)
{
  if (type & 0x7) {
    type = Type(type & 0x7);
    attack = (rom->read<uint8_t>(addr + 8) & 0x7) / 7.0;
    decay = rom->read<uint8_t>(addr + 9) / 60.0;
    sustain = rom->read<uint8_t>(addr + 10) / 15.0;
    release = rom->read<uint8_t>(addr + 11) / 60.0;
    gate = rom->read<uint8_t>(addr + 2) / 255.0;
  } else if (type == 0 || type == 8) {
    attack = (255 - rom->read<uint8_t>(addr + 8)) / 60.0;
    decay = rom->read<uint8_t>(addr + 9) / 256.0;
    sustain = rom->read<uint8_t>(addr + 10) / 255.0;
    release = rom->read<uint8_t>(addr + 11) / 256.0;
  }
}

SequenceEvent* MpInstrument::addEnvelope(SequenceEvent* event, double factor) const
{
  BaseNoteEvent* note = dynamic_cast<BaseNoteEvent*>(event);
  if (note) {
    note->useEnvelope = true;
    if (attack) {
      note->startGain = 1.0 - (60 * attack) / 255;
      note->attack = attack * factor;
    } else {
      note->startGain = 1.0;
      note->attack = 0;
    }
    note->sustain = sustain;
    note->expDecay = (type & 0x7) == 0;
    if (note->expDecay) {
      // fitted using gradient descent
      static const double COEF = 64.9707;
      static const double ADJ = 1.4875;
      note->decay = decay ? std::log(decay * factor) * COEF - ADJ : 0;
      note->release = release ? std::log(release * factor) * COEF - ADJ : 0;
    } else {
      note->decay = decay * factor;
      note->release = release * factor;
    }
  }
  return event;
}

SampleInstrument::SampleInstrument(const ROMFile* rom, uint32_t addr)
: MpInstrument(rom, addr)
{
  uint32_t sampleAddr = rom->readPointer(addr + 4);
  uint32_t sampleStart = sampleAddr;
  uint64_t sampleID = (uint64_t(type) << 32) | sampleAddr;
  sample = rom->context()->getSample(sampleID);
  if (!sample) {
    int sampleBits = 4;
    int sampleLen = 16;
    int loopStart = 0;
    int loopEnd = 32;
    double sampleRate = rom->sampleRate;
    if (type == GBSample) {
      sampleRate = 4186.0;
    } else {
      pan = rom->read<uint32_t>(addr + 3) ^ 0x80;
      forcePan = !(pan & 0x80);
      if (pan == 127) {
        // adjust full right panning to make centering easier
        pan = 128;
      }
      sampleBits = 8;
      sampleLen = rom->read<uint32_t>(sampleAddr + 12);
      if (rom->read<uint16_t>(sampleAddr + 2)) {
        loopStart = rom->read<uint32_t>(sampleAddr + 8);
        loopEnd = sampleLen;
      } else {
        loopEnd = 0;
      }
      if (type == Sample) {
        sampleRate = rom->read<uint32_t>(sampleAddr + 4) / 1024.0;
      }
      sampleStart = sampleAddr + 16;
    }
    if (sampleStart + sampleLen > rom->rom.size()) {
      throw ROMFile::BadAccess(sampleStart + sampleLen);
    }
    sample = PcmCodec(rom->context(), type == GBSample ? 4 : 8).decodeRange(rom->rom.begin() + sampleStart, rom->rom.begin() + sampleStart + sampleLen, sampleID);
    sample->sampleRate = sampleRate;
    sample->loopStart = loopStart;
    sample->loopEnd = loopEnd;

    std::ostringstream fnss;
    fnss << "dump/sample-" << std::hex << (sampleAddr) << ".wav";
    RiffWriter dump(sampleRate, false);
    dump.open(fnss.str());
    dump.write(sample->channels[0]);
    dump.close();
  }
}

SequenceEvent* SampleInstrument::makeEvent(double volume, uint8_t key, uint8_t vel, double len) const
{
  if (key & 0x80) return nullptr;
  static const double cFreq = 261.6256;
  double freq = noteToFreq((int8_t)key);
  SampleEvent* event = new SampleEvent;
  event->duration = len;
  event->pitchBend = freq / cFreq;
  event->sampleID = sample->sampleID;
  // TODO: velocity accuracy
  event->volume = (vel / 127.0);
  return addEnvelope(event, 1.0);
}

PSGInstrument::PSGInstrument(const ROMFile* rom, uint32_t addr)
: MpInstrument(rom, addr), sweep(0)
{
  if (type == Square1) {
    sweep = rom->read<uint8_t>(addr + 3);
  }
  mode = rom->read<uint8_t>(addr + 4);
}

SequenceEvent* PSGInstrument::makeEvent(double volume, uint8_t key, uint8_t vel, double len) const
{
  OscillatorEvent* event = new OscillatorEvent;
  event->duration = (gate && len > gate) ? gate : len;
  event->frequency = noteToFreq(key);
  if (type == Noise) {
    if (mode & 1) {
      event->waveformID = BaseOscillator::GBNoise127;
    } else {
      event->waveformID = BaseOscillator::GBNoise;
    }
  } else if (mode == 0) {
    event->waveformID = BaseOscillator::Square125;
  } else if (mode == 1) {
    event->waveformID = BaseOscillator::Square25;
  } else if (mode == 2) {
    event->waveformID = BaseOscillator::Square50;
  } else if (mode == 3) {
    event->waveformID = BaseOscillator::Square75;
  }
  /*
  if (sweep) {
    node = new SweepNode(ctx, node, sweep);
  }
  */
  // TODO: velocity accuracy
  event->volume = (vel / 127.0) * .3;
  return addEnvelope(event, volume);
}

SplitInstrument::SplitInstrument(const ROMFile* rom, uint32_t addr)
: MpInstrument(rom, addr)
{
  uint32_t splitAddr = rom->readPointer(0x08000000 | (addr + 4));
  if (type == Percussion) {
    for (int i = 0; i < 128; i++) {
      splits.emplace_back(load(rom, splitAddr + 12 * i, true));
    }
  } else {
    uint32_t tableAddr = rom->readPointer(addr + 8);
    for (int i = 0; i < 128; i++) {
      int n = rom->read<uint8_t>(tableAddr + i);
      splits.emplace_back(load(rom, splitAddr + 12 * n, true));
    }
  }
}

SequenceEvent* SplitInstrument::makeEvent(double volume, uint8_t key, uint8_t vel, double len) const
{
  auto split = splits.at(key).get();
  if (!split) {
    return nullptr;
  }
  SequenceEvent* event = split->makeEvent(volume, key, vel, len);
  if (split->forcePan && split->pan != 64) {
    BaseNoteEvent* noteEvent = dynamic_cast<BaseNoteEvent*>(event);
    if (noteEvent) {
      noteEvent->pan = split->pan;
    }
  }
  return event;
}

InstrumentData::InstrumentData(const ROMFile* rom, uint32_t addr)
{
  while (addr < rom->rom.size() && instruments.size() < 127) {
    MpInstrument* inst = MpInstrument::load(rom, addr);
    if (inst) {
      instruments.emplace_back(inst);
    } else {
      //std::cout << "unknown/bad instrument @ 0x" << std::hex << addr << std::endl;
      instruments.emplace_back(nullptr);
    }
    addr += 12;
  }
}
