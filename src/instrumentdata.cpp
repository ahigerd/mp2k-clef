#include "instrumentdata.h"
#include "romfile.h"
#include "utility.h"
#include "clefcontext.h"
#include "codec/sampledata.h"
#include "codec/pcmcodec.h"
#include "seq/sequenceevent.h"
#include "synth/synthcontext.h"
#include "synth/oscillator.h"
#include "synth/sampler.h"
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
  if (type == Square1) {
    if (rom->read<uint64_t>(addr) == 0x0000000200003c01ULL && rom->read<uint32_t>(addr + 8) == 0x000f0000) {
      // unused instrument
      return nullptr;
    }
  }

  uint8_t normType = type;
  try {
    switch (normType) {
      case GBSample:
      case GBSample_Alt:
      case Sample:
      case FixedSample:
        return new SampleInstrument(rom, addr);
      case Square1:
      case Square1_Alt:
      case Square2:
      case Square2_Alt:
      case Noise:
      case Noise_Alt:
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
    //std::cerr << "Bad pointer loading " << int(type) << " instrument at 0x" << std::hex << addr << std::dec << std::endl;
    return nullptr;
  } catch (std::exception& e) {
    //std::cerr << "Error loading " << int(type) << " instrument at 0x" << std::hex << addr << std::dec << ": " << e.what() << std::endl;
    return nullptr;
  }
}

MpInstrument::MpInstrument(const ROMFile* rom, uint32_t addr)
: rom(rom), addr(addr), type(Type(rom->read<uint8_t>(addr))), forcePan(false), pan(0), gate(0)
{
  if (type < 16 && type != 0 && type != 8) {
    type = Type(type);
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

Channel::Note* MpInstrument::addEnvelope(Channel* channel, Channel::Note* note, double factor) const
{
  double startGain = 1.0;
  double eAttack = 1.0;
  if (attack) {
    startGain = 1.0 - (60 * attack) / 255;
    eAttack = attack * factor;
  }

  bool expDecay = (type & 0x7) == 0;
  double eDecay = decay * factor;
  double eRelease = release * factor;
  if (expDecay) {
    // fitted using gradient descent
    // TODO: use DiscreteEnvelope
    static const double COEF = 64.9707;
    static const double ADJ = 1.4875;
    eDecay = eDecay ? std::log(eDecay) * COEF - ADJ : 0;
    eRelease = eRelease ? std::log(eRelease) * COEF - ADJ : 0;
  }

  Envelope* env = new Envelope(channel->ctx, eAttack, 0, eDecay, sustain, 0, eRelease);
  env->expAttack = false;
  env->expDecay = expDecay;
  env->param(Envelope::StartGain)->setConstant(startGain);
  env->connect(note->source);
  note->source.reset(env);
  return note;
}

bool MpInstrument::operator==(const MpInstrument* other) const
{
  if (!other) {
    return false;
  }
  if (other == this) {
    // reflexive identity
    return true;
  }
  if (other->type != type || other->attack != attack || other->decay != decay ||
      other->sustain != sustain || other->release != release || other->gate != gate) {
    return false;
  }
  if (type == Sample || type == GBSample || type == FixedSample) {
    const SampleInstrument* s1 = static_cast<const SampleInstrument*>(this);
    const SampleInstrument* s2 = static_cast<const SampleInstrument*>(other);
    return s1->sample == s2->sample;
  } else if (type == Square1 || type == Square2 || type == Noise) {
    const PSGInstrument* s1 = static_cast<const PSGInstrument*>(this);
    const PSGInstrument* s2 = static_cast<const PSGInstrument*>(other);
    return s1->mode == s2->mode && s1->sweep == s2->sweep;
  } else if (type == KeySplit || type == Percussion) {
    const SplitInstrument* s1 = static_cast<const SplitInstrument*>(this);
    const SplitInstrument* s2 = static_cast<const SplitInstrument*>(other);
    return s1->splits == s2->splits;
  }
  return false;
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

    /*
    std::ostringstream fnss;
    fnss << "dump/sample-" << std::hex << (sampleAddr) << ".wav";
    RiffWriter dump(sampleRate, false);
    dump.open(fnss.str());
    dump.write(sample->channels[0]);
    dump.close();
    */
  }
}

std::string SampleInstrument::displayName() const
{
  std::ostringstream ss;
  if (type == GBSample) {
    ss << "Waveform";
  } else if (type == GBSample_Alt) {
    ss << "Waveform (Alt)";
  } else {
    ss << "Sample";
  }
  uint32_t sampleAddr = (sample->sampleID & 0xFFFFFFFF);
  ss << " (0x" << std::hex << sampleAddr << ")";
  return ss.str();
}

BaseNoteEvent* SampleInstrument::makeEvent(double, uint8_t key, uint8_t vel, double len) const
{
  if (key & 0x80) return nullptr;
  InstrumentNoteEvent* event = new InstrumentNoteEvent;
  event->duration = len;
  event->pitch = key;
  // TODO: velocity accuracy
  event->volume = (vel / 127.0);
  return event;
}

Channel::Note* SampleInstrument::noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event)
{
  static const double cFreq = 261.6256;
  double ratio = 1.0;
  if (type != FixedSample) {
    double freq = noteToFreq(int8_t(static_cast<InstrumentNoteEvent*>(event.get())->pitch));
    ratio = freq / cFreq;
  }
  std::shared_ptr<AudioNode> node(new Sampler(channel->ctx, sample, ratio));
  node->param(AudioNode::Gain)->setConstant(event->volume);
  node->param(AudioNode::Pan)->setConstant(event->pan);
  double duration = event->duration;
  if (!duration) {
    duration = sample->duration();
  }
  Channel::Note* note = channel->allocNote(event, node, duration);
  return addEnvelope(channel, note, 1.0);
}

PSGInstrument::PSGInstrument(const ROMFile* rom, uint32_t addr)
: MpInstrument(rom, addr), sweep(0)
{
  if (type == Square1 || type == Square1_Alt) {
    sweep = rom->read<uint8_t>(addr + 3);
  }
  mode = rom->read<uint8_t>(addr + 4);
  bool err = rom->read<uint8_t>(addr + 5) || rom->read<uint8_t>(addr + 6) || rom->read<uint8_t>(addr + 7);
  if (type == Noise) {
    err = err || mode > 1;
  } else {
    err = err || mode > 3;
  }
  if (err) {
    throw std::runtime_error("invalid PSG instrument type " + std::to_string(mode));
  }
}

std::string PSGInstrument::displayName() const
{
  std::ostringstream ss;
  if (type == Square1 || type == Square1_Alt || type == Square2 || type == Square2_Alt) {
    if (type == Square1) {
      ss << "Square 1 ";
    } else if (type == Square1_Alt) {
      ss << "Square 1 (Alt) ";
    } else if (type == Square2) {
      ss << "Square 2 ";
    } else if (type == Square2_Alt) {
      ss << "Square 2 (Alt) ";
    }
    if (mode == 0) {
      ss << "(12.5%)";
    } else if (mode == 1) {
      ss << "(25%)";
    } else if (mode == 2) {
      ss << "(50%)";
    } else if (mode == 3) {
      ss << "(75%)";
    }
  } else if (type == Noise) {
    ss << "Noise (type " << int(mode) << ")";
  } else if (type == Noise_Alt) {
    ss << "Noise (Alt) (type " << int(mode) << ")";
  } else {
    ss << "PSG (type " << int(type) << ")";
  }
  return ss.str();
}

BaseNoteEvent* PSGInstrument::makeEvent(double volume, uint8_t key, uint8_t vel, double len) const
{
  InstrumentNoteEvent* event = new InstrumentNoteEvent;
  event->duration = (gate && len > gate) ? gate : len;
  event->pitch = key;
  event->volume = (vel / 127.0);
  return event;
}

Channel::Note* PSGInstrument::noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event)
{
  BaseOscillator::WaveformPreset waveformID = BaseOscillator::Square50;
  if (type == Noise) {
    if (mode & 1) {
      waveformID = BaseOscillator::GBNoise127;
    } else {
      waveformID = BaseOscillator::GBNoise;
    }
  } else if (mode == 0) {
    waveformID = BaseOscillator::Square125;
  } else if (mode == 1) {
    waveformID = BaseOscillator::Square25;
  /*
  } else if (mode == 2) {
    waveformID = BaseOscillator::Square50;
  */
  } else if (mode == 3) {
    waveformID = BaseOscillator::Square75;
  }
  // TODO: velocity accuracy
  double volume = event->volume * 0.3;
  std::shared_ptr<AudioNode> node(BaseOscillator::create(
      channel->ctx,
      waveformID,
      noteToFreq(static_cast<InstrumentNoteEvent*>(event.get())->pitch),
      volume,
      event->pan
  ));
  Channel::Note* note = channel->allocNote(event, node, event->duration);
  /*
  if (sweep) {
    node = new SweepNode(ctx, node, sweep);
  }
  */
  return addEnvelope(channel, note, volume);
}

SplitInstrument::SplitInstrument(const ROMFile* rom, uint32_t addr)
: MpInstrument(rom, addr)
{
  attack = -1;
  uint32_t splitAddr = rom->readPointer(rom->baseAddr | (addr + 4));
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

std::string SplitInstrument::displayName() const
{
  std::ostringstream ss;
  if (type == KeySplit) {
    ss << "Split";
  } else {
    ss << "Percussion";
  }
  ss << " (0x" << std::hex << addr << ")";
  return ss.str();
}

BaseNoteEvent* SplitInstrument::makeEvent(double volume, uint8_t key, uint8_t vel, double len) const
{
  auto split = splits.at(key).get();
  if (!split) {
    return nullptr;
  }
  BaseNoteEvent* event = split->makeEvent(volume, key, vel, len);
  if (split->forcePan && split->pan != 64) {
    event->pan = split->pan;
  }
  return event;
}

Channel::Note* SplitInstrument::noteEvent(Channel* channel, std::shared_ptr<BaseNoteEvent> event)
{
  auto split = splits.at(size_t(static_cast<InstrumentNoteEvent*>(event.get())->pitch));
  if (!split) {
    return nullptr;
  }
  return split->noteEvent(channel, event);
}

InstrumentData::InstrumentData(const ROMFile* rom, uint32_t addr)
{
  for (int i = 0; i < 128; i++) {
    instruments[i] = 0;
  }
  SynthContext* synth = rom->synthContext();
  for (int instId = 0; addr < rom->rom.size() && instId < 128; addr += 12, instId++) {
    MpInstrument* inst = synth ? static_cast<MpInstrument*>(synth->getInstrument(addr)) : nullptr;
    if (inst) {
      instruments[instId] = addr;
      continue;
    }
    inst = MpInstrument::load(rom, addr);
    if (!inst) {
      //std::cout << instId << ": unknown/bad instrument @ 0x" << std::hex << addr << std::endl;
      instruments[instId] = 0;
      continue;
    }
    //std::cerr << instId << ": Loaded 0x" << std::hex << addr << " of type " << std::dec << inst->type << std::endl;
    MpInstrument* dupe = findDupe(synth, *inst);
    if (dupe) {
      instruments[instId] = dupe->addr;
      delete inst;
    } else if (synth) {
      instruments[instId] = addr;
      synth->registerInstrument(addr, std::unique_ptr<IInstrument>(inst));
    } else {
      instruments[instId] = instId;
    }
  }
}

MpInstrument* InstrumentData::findDupe(SynthContext* synth, const MpInstrument& inst) const
{
  if (!synth) {
    return nullptr;
  }
  int numInsts = synth->numInstruments();
  for (int i = 0; i < numInsts; i++) {
    uint64_t otherID = synth->instrumentID(i);
    MpInstrument* other = static_cast<MpInstrument*>(synth->getInstrument(otherID));
    if (inst == other) {
      return other;
    }
  }
  return nullptr;
}

void MpInstrument::showParsed(std::ostream& out, std::string indent) const
{
  out << indent << displayName() << ":" << std::endl;
  out << indent << "  Base address: 0x" << std::hex << addr << std::dec << std::endl;
  if (attack >= 0) {
    out << indent << "  Base key=" << int(rom->read<uint8_t>(addr + 1)) << std::endl;
    if (type == Square1 || type == Square1_Alt || type == Square2 || type == Square2_Alt || type == GBSample || type == GBSample_Alt
      || type == Noise || type == Noise_Alt) {
        out << indent << "  Gate=" << int(rom->read<uint8_t>(addr + 2)) << std::endl;
        if (type == Square1 || type == Square1_Alt) {
          out << indent << "  Sweep=" << int(rom->read<uint8_t>(addr + 3)) << std::endl;
        }
    } else {
      int pan = rom->read<uint8_t>(addr + 2);
      if (pan == 0) {
        out << indent << "  Pan=" << pan << std::endl;
      } else {
        out << indent << "  Pan=" << (pan | 0x80) << std::endl;
      }
    }
    out << indent << "  A=" << int(rom->read<uint8_t>(addr + 8)) << " (" << attack << ")" << std::endl;
    out << indent << "  D=" << int(rom->read<uint8_t>(addr + 9)) << " (" << decay << ")" << std::endl;
    out << indent << "  S=" << int(rom->read<uint8_t>(addr + 10)) << " (" << sustain << ")" << std::endl;
    out << indent << "  R=" << int(rom->read<uint8_t>(addr + 11)) << " (" << release << ")" << std::endl;
  }
}

void SplitInstrument::showParsed(std::ostream& out, std::string) const
{
  out << displayName() << ":" << std::endl;
  out << "  Base address: 0x" << std::hex << addr << std::dec << std::endl;
  int note = 0;
  for (const auto& split : splits) {
    if (split.get()) {
      out << "  " << note << ": " << split->displayName() << std::endl;
    }
    note++;
  }
}
