#include "songdata.h"
#include "romfile.h"
#include <unordered_map>

static const uint8_t noteLength[50] = {
   0, 0xFF,
   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 28, 30, 32, 36, 40, 42, 44, 48,
  52, 54, 56, 60, 64, 66, 68, 72, 76, 78, 80, 84, 88, 90, 92, 96
};

TrackData::TrackData(SongData* song, uint32_t addr, MpInstrument* defaultInst)
: song(song), addr(addr), hasLoop(true), playIndex(0), playTime(0), secPerTick(1.0 / 75.0),
  lengthCache(-1), currentInstrument(defaultInst), bendRange(2), transpose(0)
{
  std::unordered_map<uint64_t, size_t> addrToIndex;
  std::unordered_map<size_t, uint64_t> indexToAddr;
  const ROMFile& r = *song->rom;
  uint32_t pos = addr - 1;
  uint8_t running = 0;
  uint8_t cmd, arg1, arg2, arg3;
  uint8_t instrument = 0, noteVel, noteLen;
  int8_t noteKey;
  uint32_t argAddr, argPos;
  uint32_t timestamp = 0;
  uint32_t returnAddr = 0;
  uint32_t repeatAddr = 0;
  uint8_t repeatCount = 1;
  int restSum = 0;
  while (true) {
    ++pos;
    size_t index = events.size();
    uint64_t effAddr = (uint64_t(returnAddr) << 32) | pos;
    addrToIndex[effAddr] = index;
    if (!indexToAddr.count(index)) {
      indexToAddr[index] = effAddr;
    }
    uint8_t cmd = r[pos];
    Mp2kEvent ev;
    ev.effAddr = effAddr;
    ev.duration = 0;
    if (cmd < 0x80) {
      arg1 = cmd;
      cmd = running;
      argPos = pos;
    } else if (cmd == 0x80) {
      continue;
    } else if (cmd < 0xB1) {
      ev.type = Mp2kEvent::Rest;
      ev.duration = noteLength[cmd - 0x81 + 2];
      restSum += ev.duration;
      events.push_back(ev);
      continue;
    } else if (cmd >= 0xB5) {
      argPos = pos + 1;
      arg1 = r[pos + 1];
    }
    if (cmd >= 0xCE) { // EOT / TIE / NOTE
      running = cmd;
      noteLen = noteLength[cmd - 0xCE];
      uint8_t totalNoteLen = noteLen;
      arg2 = r[argPos + 1];
      arg3 = r[argPos + 2];
      if (arg1 < 128) {
        noteKey = arg1;
        if (arg2 < 128) {
          noteVel = arg2;
          argPos++;
          if (arg3 < 128) {
            totalNoteLen += arg3;
            argPos++;
          }
        }
      }
      ev.type = Mp2kEvent::Note;
      ev.param = noteKey;
      ev.value = noteVel;
      ev.duration = totalNoteLen;
      events.push_back(ev);
      if (restSum > 0) {
        restSum = 0;
      }
      pos = argPos;
      continue;
    }
    switch (cmd) {
      case 0xB1: // FINE
        return;
      case 0xB2: // GOTO
        argAddr = r.readPointer(pos + 1, false);
        effAddr = (uint64_t(returnAddr) << 32) | argAddr;
        if (addrToIndex.count(effAddr)) {
          // Jump to an address we've already seen
          hasLoop = true;
          ev.type = Mp2kEvent::Goto;
          ev.value = addrToIndex.at(effAddr);
          events.push_back(ev);
          return;
        }
        // Skip decoding forward to the goto target
        pos = argAddr - 1;
        break;
      case 0xB3: // PATT
        if (returnAddr) {
          throw std::runtime_error("nested pattern detected");
        }
        argAddr = r.readPointer(pos + 1, false);
        returnAddr = pos + 5;
        pos = argAddr - 1;
        break;
      case 0xB4: // PEND
        if (!returnAddr) {
          // PEND without a call stack is ignored
          continue;
        }
        if (repeatAddr) {
          repeatCount--;
          if (repeatCount > 0) {
            pos = repeatAddr - 1;
          } else {
            pos = returnAddr - 1;
            repeatAddr = 0;
            returnAddr = 0;
          }
        } else {
          pos = returnAddr - 1;
          returnAddr = 0;
        }
        break;
      case 0xB5: // REPT
        if (returnAddr) {
          throw std::runtime_error("nested pattern detected");
        }
        repeatCount = arg1;
        repeatAddr = r.readPointer(pos + 2, false);
        returnAddr = pos + 5;
        pos = repeatAddr - 1;
        break;
      case 0xBA: // PRIO
        // ignore
        pos++;
        break;
      case 0xBD: // VOICE
      case 0xBE: // VOL
      case 0xBF: // PAN
      case 0xC0: // BEND
      case 0xC1: // BENDR
      case 0xC4: // MOD
      case 0xC8: // TUNE
        running = cmd;
        // fallthrough;
      case 0xBB: // TEMPO
      case 0xBC: // KEYSH
      case 0xC2: // LFOS
      case 0xC3: // LFODL
      case 0xC5: // MODT
        ev.type = Mp2kEvent::Param;
        ev.param = cmd;
        ev.value = arg1;
        events.push_back(ev);
        pos = argPos;
        break;
      case 0xB6: // Unknown
      case 0xB9: // Unknown
      case 0xCB: // Unknown
      case 0xCC: // Unknown
        // ???
        //break;
      case 0xCD: // XCMD
        // TODO
        std::cout << "unknown " << (int)cmd << std::endl;
        break;
      default:
        std::cout << "XXX " << std::hex << (int)cmd << std::endl;
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
  return 10;
}

bool TrackData::isFinished() const
{
  return playIndex >= events.size() || playTime > length();
}

std::shared_ptr<SequenceEvent> TrackData::readNextEvent()
{
  SequenceEvent* result = nullptr;
  bool didGoto = false;
  while (!result && !isFinished()) {
    const Mp2kEvent& event = events[playIndex++];
    for (int i = song->tempos.size() - 1; i >= 0; --i) {
      const auto& tempo = song->tempos[i];
      if (tempo.first < playTime) {
        if (secPerTick != tempo.second) {
          // TODO: adjust remaining note play time
          secPerTick = tempo.second;
          std::cout << (intptr_t)this << ": got tempo " << secPerTick << " " << i << std::endl;
        }
        break;
      }
    }
    double duration = event.duration == 0xFF ? -1 : event.duration * secPerTick;
    if (event.type == Mp2kEvent::Rest) {
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
        case 0xBB: // TEMPO
          // simplification of value / 75.0 * (1.0 / 60.0)
          secPerTick = event.value / 4500.0 * 2; // TODO: the 2 factor is a hack, fix when engine rate is available
          song->tempos.emplace_back(playTime, secPerTick);
          std::cout << (intptr_t)this << ": set tempo " << secPerTick << " @ " << playTime << std::endl;
          break;
        case 0xBC: // KEYSH
          transpose = event.value;
          break;
        case 0xBD: // VOICE
          currentInstrument = song->getInstrument(event.value);
          if (currentInstrument) {
            releaseTime = currentInstrument->release;
          }
          break;
        case 0xBE: // VOL
          result = new ChannelEvent('gain', event.value / 255.0);
          //std::cout << "VOL " << (int)event.value << std::endl;
          break;
        case 0xC1: // BENDR
          bendRange = double(event.value);
          break;
        default:
          // TODO
          std::cout << "unknown param " << std::hex << (int)event.param << std::dec << std::endl;
      }
    } else if (event.type == Mp2kEvent::Note && currentInstrument) {
      uint8_t note = event.param + transpose;
      auto iter = activeNotes.find(note);
      if (iter != activeNotes.end() && (iter->second.releaseTime == 0 || iter->second.endTime > playTime)) {
        KillEvent* killEvent = new KillEvent(iter->second.playbackID, playTime);
        if (event.value > 0 || iter->second.endTime == 0 || iter->second.endTime > playTime) {
          // note replaced with another note or past its release time
          killEvent->immediate = true;
          activeNotes.erase(iter);
        } else if (!iter->second.released) {
          iter->second.released = true;
          iter->second.endTime = playTime + (iter->second.endTime - iter->second.releaseTime);
          iter->second.releaseTime = playTime;
        }
        result = killEvent;
        --playIndex;
      }
      if (!result) {
        result = currentInstrument->makeEvent(1 /* volume */, note, event.value, duration);
        if (result) {
          double noteReleaseTime = duration >= 0 ? playTime + duration : 0;
          double endTime = noteReleaseTime + releaseTime;
          activeNotes[note] = (ActiveNote){
            dynamic_cast<BaseNoteEvent*>(result)->playbackID,
            noteReleaseTime,
            endTime,
            false,
          };
        }
      }
    }
    if (result) {
      result->timestamp = playTime;
    }
  }
  if (!result && isFinished()) {
    if (activeNotes.size()) {
      auto iter = activeNotes.begin();
      result = new KillEvent(iter->second.playbackID, playTime);
      activeNotes.erase(iter);
    }
  }
  return std::shared_ptr<SequenceEvent>(result);
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
    TrackData* track = new TrackData(this, rom->readPointer(addr + 8 + i * 4, false), defaultInst);
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
