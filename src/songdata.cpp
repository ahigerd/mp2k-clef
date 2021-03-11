#include "songdata.h"
#include "romfile.h"
#include "synth/audionode.h"
#include <unordered_map>
#include <sstream>
#include <iomanip>

static const uint8_t noteLength[50] = {
   0, 0xFF,
   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 28, 30, 32, 36, 40, 42, 44, 48,
  52, 54, 56, 60, 64, 66, 68, 72, 76, 78, 80, 84, 88, 90, 92, 96
};

namespace EventType {
  enum Opcode {
    FINE = 0xB1,
    GOTO,
    PATT,
    PEND,
    REPT,
    STOP,
    MEMACC = 0xB9,
    PRIO,
    TEMPO,
    KEYSH,
    VOICE,
    VOL,
    PAN,
    BEND,
    BENDR,
    LFOS,
    LFODL,
    MOD,
    MODT,
    TUNE = 0xC8,
    XCMD = 0xCD,
    EOT,
    TIE,
  };

  // starts at 0xB1
  static std::string names[] = {
    "FINE", "GOTO", "PATT", "PEND", "REPT", "STOP", "", "", "MEMACC", "PRIO",
    "TEMPO", "KEYSH", "VOICE", "VOL", "PAN", "BEND", "BENDR", "LFOS", "LFODL",
    "MOD", "MODT", "", "", "TUNE", "", "", "", "", "XCMD", "EOT", "TIE",
  };

  static std::string name(uint8_t opcode)
  {
    if (opcode < 0x80) return "";
    if (opcode >= 0xD0) return "NOTE";
    if (opcode <= 0xB0) return "REST";
    return names[opcode - 0xB1];
  }

  static int size(uint8_t opcode)
  {
    // return 0 = running status
    // return -1 = variable length
    switch (opcode) {
      case FINE:
      case STOP:
      case PEND: return 1;
      case GOTO:
      case PATT: return 5;
      case REPT: return 6;
      case MEMACC:
      case EOT:
      case TIE: return -1;
    }
    if (opcode < 0x80) {
      return 0;
    } else if (opcode < FINE) {
      return 1;
    } else if (opcode >= PRIO && opcode <= XCMD) {
      return 2;
    } else {
      return -1;
    }
  }
}

struct RawEvent {
  uint32_t addr;
  uint8_t opcode;
  std::vector<uint32_t> args;

  std::string render() const {
    std::ostringstream ss;
    ss << "0x" << std::hex << addr << " " << std::dec;
    if (opcode < 0x80) {
      ss << "      ";
    } else {
      ss << std::setw(6) << EventType::name(opcode);
    }
    bool first = true;
    if (opcode >= 0x80 && opcode <= 0xB0) {
      ss << " " << (opcode - 0x80);
      first = false;
    } else if (opcode >= 0xD0) {
      ss << " " << (opcode - 0xCF);
      first = false;
    }
    for (uint32_t arg : args) {
      if (first) {
        first = false;
        ss << " ";
      } else {
        ss << ", ";
      }
      if (arg > 0xFF) {
        ss << std::hex << "0x" << arg << std::dec;
      } else {
        ss << arg;
      }
    }
    return ss.str();
  }
};

TrackData::TrackData(SongData* song, int index, uint32_t addr, MpInstrument* defaultInst)
: trackIndex(index), song(song), addr(addr), hasLoop(true), playIndex(0), playTime(0), secPerTick(1.0 / 75.0),
  lengthCache(-1), currentInstrument(defaultInst), bendRange(2), transpose(0), tuning(0), stopped(false)
{
  std::unordered_map<uint64_t, size_t> addrToIndex;
  std::unordered_map<size_t, uint64_t> indexToAddr;
  const ROMFile& r = *song->rom;
  uint32_t pos = addr;
  uint8_t running = 0;
  uint8_t instrument = 0, noteVel = 127, noteLen = 96;
  RawEvent raw;
  int8_t noteKey = 60;
  uint32_t argAddr, argPos;
  uint32_t timestamp = 0;
  uint32_t returnAddr = 0;
  uint32_t repeatAddr = 0;
  uint8_t repeatCount = 1;
  int ts = 0;
  while (true) {
    raw.addr = pos;
    raw.opcode = r[pos];
    raw.args.clear();
    int eventSize = EventType::size(raw.opcode);
    uint32_t argOffset = 1;
    if (eventSize == 0) {
      // running status
      raw.opcode = running;
      eventSize = EventType::size(raw.opcode) - 1;
      argOffset = 0;
    }
    if (eventSize < 0) {
      if (raw.opcode == EventType::MEMACC) {
        if (r[pos + 1] > 5) {
          eventSize = 8;
        } else {
          eventSize = 4;
        }
      } else {
        eventSize += 2;
        int maxSize;
        switch (raw.opcode) {
          case EventType::EOT:
            maxSize = argOffset;
            break;
          case EventType::TIE:
            maxSize = argOffset + 1;
            break;
          default:
            maxSize = argOffset + 2;
        }
        while (r[pos + eventSize] < 0x80 && eventSize <= maxSize) {
          eventSize++;
        }
      }
    }
    if (raw.opcode == EventType::MEMACC) {
      for (int i = 1; i < 4; i++) {
        raw.args.push_back(r[pos + i]);
      }
      if (eventSize > 4) {
        raw.args.push_back(r.read<uint32_t>(pos + 4));
      }
    } else if (raw.opcode == EventType::GOTO || raw.opcode == EventType::PATT) {
      raw.args.push_back(r.readPointer(pos + 1, false));
    } else if (raw.opcode == EventType::REPT) {
      raw.args.push_back(r[pos + 1]);
      raw.args.push_back(r.readPointer(pos + 2, false));
    } else if (eventSize > argOffset) {
      for (int i = argOffset; i < eventSize; i++) {
        raw.args.push_back(r[pos + i]);
      }
    }
    std::cout << "[" << trackIndex << "] " << std::setw(6) << ts << " | " << raw.render() << std::endl;
    pos += eventSize;
    Mp2kEvent ev;
    ev.effAddr = (uint64_t(returnAddr) << 32) | raw.addr;
    ev.duration = 0;
    size_t index = events.size();
    addrToIndex[ev.effAddr] = index;
    if (!indexToAddr.count(index)) {
      indexToAddr[index] = ev.effAddr;
    }
    if (raw.opcode < 0xB1) {
      ev.type = Mp2kEvent::Rest;
      ev.duration = noteLength[raw.opcode - 0x81 + 2];
      ts += ev.duration;
      events.push_back(ev);
    } else if (raw.opcode >= 0xCE) { // EOT / TIE / NOTE
      running = raw.opcode;
      uint8_t noteLen = noteLength[raw.opcode - 0xCE];
      int numArgs = raw.args.size();
      if (numArgs > 0) noteKey = raw.args[0];
      if (numArgs > 1) noteVel = raw.args[1];
      if (numArgs > 2) noteLen += raw.args[2];
      ev.type = Mp2kEvent::Note;
      ev.param = noteKey;
      if (raw.opcode == EventType::EOT) {
        ev.value = 0;
        ev.duration = 0xFF;
      } else {
        ev.value = noteVel;
        ev.duration = noteLen;
      }
      events.push_back(ev);
    } else switch (raw.opcode) {
      using namespace EventType;
      case FINE:
      case STOP:
        ev.type = Mp2kEvent::Stop;
        events.push_back(ev);
        return;
      case GOTO:
        {
          uint64_t effAddr = (uint64_t(returnAddr) << 32) | raw.args[0];
          if (addrToIndex.count(effAddr)) {
            // Jump to an address we've already seen
            hasLoop = true;
            ev.type = Mp2kEvent::Goto;
            ev.value = addrToIndex.at(effAddr);
            events.push_back(ev);
            return;
          }
        }
        // Skip decoding forward to the goto target
        pos = raw.args[0];
        break;
      case PATT:
        if (returnAddr) {
          throw std::runtime_error("nested pattern detected");
        }
        repeatCount = 1;
        returnAddr = pos;
        pos = raw.args[0];
        break;
      case PEND:
        if (!returnAddr) {
          // PEND without a call stack is ignored
          continue;
        }
        repeatCount--;
        pos = repeatCount > 0 ? repeatAddr : returnAddr;
        returnAddr = 0;
        break;
      case REPT:
        if (returnAddr) {
          throw std::runtime_error("nested pattern detected");
        }
        repeatCount = raw.args[0];
        if (repeatCount > 0) {
          repeatAddr = raw.args[1];
          returnAddr = pos;
          pos = repeatAddr;
        }
        break;
      case PRIO:
        // ignore
        break;
      case VOICE:
      case VOL:
      case PAN:
      case BEND:
      case BENDR:
      case MOD:
      case TUNE:
        running = raw.opcode;
        // fallthrough;
      case TEMPO:
      case KEYSH:
      case LFOS:
      case LFODL:
      case MODT:
        ev.type = Mp2kEvent::Param;
        ev.param = raw.opcode;
        ev.value = raw.args[0];
        events.push_back(ev);
        break;
      case XCMD:
        // TODO
      case 0xB9: // Unknown
      case 0xCB: // Unknown
      case 0xCC: // Unknown
        // ???
        //break;
        std::cout << "unknown " << (int)raw.opcode << std::endl;
        break;
      default:
        std::cout << "XXX " << std::hex << (int)raw.opcode << std::endl;
        throw std::runtime_error("unknown MP2K command");
    }
  }
}

TrackData::~TrackData()
{
}

void TrackData::internalReset()
{
  playIndex = 0;
  playTime = 0;
  secPerTick = 1.0 / 60.0;
}

double TrackData::length() const
{
  // TODO
  return 20;
}

bool TrackData::isFinished() const
{
  return stopped || playIndex >= events.size() || playTime > length();
}

std::shared_ptr<SequenceEvent> TrackData::readNextEvent()
{
  bool didGoto = false;
  while (!isFinished() && !pendingEvents.size()) {
    const Mp2kEvent& event = events[playIndex++];
    for (int i = song->tempos.size() - 1; i >= 0; --i) {
      const auto& tempo = song->tempos[i];
      if (tempo.first <= playTime) {
        if (secPerTick != tempo.second) {
          // TODO: adjust remaining note play time
          secPerTick = tempo.second;
        }
        break;
      }
    }
    double duration = event.duration == 0xFF ? -1 : event.duration * secPerTick;
    if (event.type == Mp2kEvent::Stop) {
      stopped = true;
    } else if (event.type == Mp2kEvent::Rest) {
      playTime += duration;
    } else if (event.type == Mp2kEvent::Goto) {
      if (didGoto) {
        // loop never produces an event: abort
        playIndex = events.size();
        std::cout << "abort" << std::endl;
      } else {
        playIndex = event.value;
        std::cout << "goto" << std::endl;
        didGoto = true;
      }
    } else if (event.type == Mp2kEvent::Param) {
      switch (event.param) {
        using namespace EventType;
        case TEMPO:
          // simplification of 1.0 / (value / 75.0 * 60.0)
          secPerTick = 0.8 * 1.6 / event.value; // TODO: fix base rate
          song->tempos.emplace_back(playTime, secPerTick);
          break;
        case KEYSH:
          transpose = event.value;
          break;
        case TUNE:
          tuning = (int(event.value) - 64) / 64.0;
          break;
        case VOICE:
          currentInstrument = song->getInstrument(event.value);
          std::cout << trackIndex << ": Using instrument " << std::dec << (int)event.value << " (" << (currentInstrument ? (int)currentInstrument->type : -1) << ") " << std::endl;
          if (currentInstrument) {
            releaseTime = currentInstrument->release;
          }
          break;
        case PAN:
          pendingEvents.emplace_back(new ChannelEvent(AudioNode::Pan, event.value / 128.0));
          pendingEvents.back()->timestamp = playTime;
          break;
        case VOL:
          pendingEvents.emplace_back(new ChannelEvent(AudioNode::Gain, 4 * event.value / 127.0));
          pendingEvents.back()->timestamp = playTime;
          volume = event.value / 127.0;
          break;
        case BENDR:
          bendRange = event.value;
          break;
        case BEND:
          {
            double bend = noteToFreq(69 + bendRange * (event.value - 64.0) / 64.0) / 440.0;
            for (const auto& it : activeNotes) {
              ModulatorEvent* modEvent = new ModulatorEvent(it.second.playbackID, 'bend', bend);
              modEvent->timestamp = playTime;
              pendingEvents.emplace_back(modEvent);
            }
          }
          break;
        default:
          // TODO
          std::cout << "unknown param " << std::hex << (int)event.param << std::dec << std::endl;
      }
    } else if (event.type == Mp2kEvent::Note && currentInstrument) {
      uint8_t note = event.param + transpose;
      // PSG instruments are mutually exclusive
      bool psg = currentInstrument->type & 0x7;
      uint8_t noteID = psg ? 0x80 + currentInstrument->type : note;
      auto& active = psg ? song->activePsg : activeNotes;
      auto iter = active.find(noteID);
      if (iter != active.end() && (psg || iter->second.releaseTime == 0 || iter->second.endTime > playTime)) {
        KillEvent* killEvent = new KillEvent(iter->second.playbackID, playTime);
        if (event.value > 0 || iter->second.endTime == 0 || iter->second.endTime > playTime) {
          // note replaced with another note or past its release time
          killEvent->immediate = true;
          active.erase(iter);
        } else if (!iter->second.released) {
          iter->second.released = true;
          iter->second.endTime = playTime + (iter->second.endTime - iter->second.releaseTime);
          iter->second.releaseTime = playTime;
        }
        pendingEvents.emplace_back(killEvent);
      }
      if (duration != 0) {
        SequenceEvent* noteEvent = currentInstrument->makeEvent(volume, note + tuning, event.value, duration);
        if (noteEvent) {
          noteEvent->timestamp = playTime;
          double noteReleaseTime = duration >= 0 ? playTime + duration : 0;
          double endTime = noteReleaseTime + releaseTime;
          active[noteID] = (ActiveNote){
            dynamic_cast<BaseNoteEvent*>(noteEvent)->playbackID,
            noteReleaseTime,
            endTime,
            false,
          };
          if (psg) {
            activeNotes[noteID] = active[noteID];
          }
          pendingEvents.emplace_back(noteEvent);
        }
      }
    }
  }
  if (isFinished()) {
    while (activeNotes.size()) {
      auto iter = activeNotes.begin();
      KillEvent* killEvent = new KillEvent(iter->second.playbackID, playTime);
      killEvent->immediate = true;
      pendingEvents.emplace_back(killEvent);
      activeNotes.erase(iter);
    }
  }
  if (pendingEvents.size()) {
    auto result = pendingEvents.at(0);
    pendingEvents.erase(pendingEvents.begin());
    return result;
  }
  return nullptr;
}

SongData::SongData(const ROMFile* rom, uint32_t addr)
: rom(rom), addr(addr), hasLoop(false), instruments(rom, rom->readPointer(addr + 4))
{
  int numTracks = rom->read<uint8_t>(addr);
  MpInstrument* defaultInst = nullptr;
  for (int i = 0; !defaultInst && i < instruments.instruments.size(); i++) {
    defaultInst = getInstrument(i);
  }
  for (int i = 0; i < numTracks; i++) {
    TrackData* track = new TrackData(this, i, rom->readPointer(addr + 8 + i * 4, false), defaultInst);
    addTrack(track);
    if (track->hasLoop) {
      hasLoop = true;
    }
  }
}

bool SongData::canLoop() const
{
  return false;
}

MpInstrument* SongData::getInstrument(uint8_t id) const
{
  return instruments.instruments.at(id).get();
}
