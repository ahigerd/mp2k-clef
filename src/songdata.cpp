#include "songdata.h"
#include "romfile.h"
#include <unordered_map>

static const uint8_t noteLength[48] = {
   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 28, 30, 32, 36, 40, 42, 44, 48,
  52, 54, 56, 60, 64, 66, 68, 72, 76, 78, 80, 84, 88, 90, 92, 96
};

GotoEvent::GotoEvent(size_t index) : index(index) {}
RestEvent::RestEvent(uint8_t ticks) : ticks(ticks) {}
ParamEvent::ParamEvent(uint8_t param, uint8_t value) : param(param), value(value) {}

TrackData::TrackData(const ROMFile* rom, uint32_t addr)
: BasicTrack(), rom(rom), addr(addr), hasLoop(true), playIndex(0), playTime(0), secPerTick(1.0 / 60.0)
{
  std::unordered_map<uint64_t, size_t> addrToIndex;
  std::unordered_map<size_t, uint64_t> indexToAddr;
  const ROMFile& r = *rom;
  uint32_t pos = addr - 1;
  uint8_t running = 0;
  uint8_t cmd, arg1, arg2, arg3;
  uint8_t instrument = 0, noteKey, noteVel, noteLen;
  uint32_t argAddr, argPos;
  uint32_t timestamp = 0;
  uint32_t returnAddr = 0;
  uint32_t repeatAddr = 0;
  uint8_t repeatCount = 1;
  while (true) {
    ++pos;
    size_t index = events.size();
    uint64_t effAddr = (uint64_t(returnAddr) << 32) | pos;
    addrToIndex[effAddr] = index;
    if (!indexToAddr.count(index)) {
      indexToAddr[index] = effAddr;
    }
    uint8_t cmd = r[pos];
    if (cmd < 0x80) {
      cmd = running;
      argPos = pos;
      arg1 = cmd;
    } else if (cmd == 0x80) {
      addEvent(new RestEvent(0));
      continue;
    } else if (cmd < 0xB1) {
      addEvent(new RestEvent(noteLength[cmd - 0x81]));
      continue;
    } else if (cmd >= 0xB5) {
      argPos = pos + 1;
      arg1 = r[pos + 1];
    }
    running = cmd;
    if (cmd >= 0xD0) { // NOTE
      noteKey = arg1;
      noteLen = noteLength[cmd - 0xD0];
      arg2 = r[argPos + 1];
      arg3 = r[argPos + 2];
      if (arg2 < 128) {
        noteVel = arg2;
        pos++;
        if (arg3 < 128) {
          noteLen += arg3;
          pos++;
        }
      }
      addNoteEvent(instrument, noteKey, noteVel, noteLen);
      continue;
    }
    //std::cout << std::hex << pos << ": " << (int)cmd << std::endl;
    switch (cmd) {
      case 0xB1: // FINE
        return;
      case 0xB2: // GOTO
        argAddr = r.readPointer(pos + 1, false);
        effAddr = (uint64_t(returnAddr) << 32) | argAddr;
        if (addrToIndex.count(effAddr)) {
          // Jump to an address we've already seen
          hasLoop = true;
          addEvent(new GotoEvent(addrToIndex.at(effAddr)));
          //std::cout << std::hex << pos << ": LOOP " << argAddr << std::endl;
          return;
        }
        //std::cout << std::hex << pos << ": GOTO " << argAddr << std::endl;
        pos = argAddr - 1;
        break;
      case 0xB3: // PATT
        if (returnAddr) {
          throw std::runtime_error("nested pattern detected");
        }
        argAddr = r.readPointer(pos + 1, false);
        returnAddr = pos + 5;
        //std::cout << std::hex << returnAddr << ": PATT " << argAddr << std::endl;
        pos = argAddr - 1;
        break;
      case 0xB4: // PEND
        if (!returnAddr) {
          //throw std::runtime_error("PEND without PATT detected");
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
        //std::cout << std::hex << returnAddr << ": REPT " << (int)repeatCount << ", " << repeatAddr << std::endl;
        pos = repeatAddr - 1;
        break;
      case 0xBA: // PRIO
        // ignore
        pos++;
        break;
      case 0xBB: // TEMPO
      case 0xBC: // KEYSH
      case 0xBD: // VOICE
      case 0xBE: // VOL
      case 0xBF: // PAN
      case 0xC0: // BEND
      case 0xC1: // BENDR
      case 0xC2: // LFOS
      case 0xC3: // LFODL
      case 0xC4: // MOD
      case 0xC5: // MODT
      case 0xC8: // TUNE
        // TODO: secPerTick = arg1 / 4500.0; // simplification of arg1 / 75.0 * (1.0 / 60.0)
        addEvent(new ParamEvent(cmd, arg1));
        pos++;
        break;
      case 0xB6: // Unknown
      case 0xB9: // Unknown
      case 0xCB: // Unknown
      case 0xCC: // Unknown
        // ???
        break;
      case 0xCD: // XCMD
      case 0xCE: // EOT
      case 0xCF: // TIE
        // TODO
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
  return -1;
}

void TrackData::addNoteEvent(uint8_t instrument, uint8_t noteKey, uint8_t noteVel, uint8_t noteLen)
{
}

std::shared_ptr<SequenceEvent> TrackData::readNextEvent()
{
  return BasicTrack::readNextEvent();
}

SongData::SongData(const ROMFile* rom, uint32_t addr)
: rom(rom), addr(addr), hasLoop(false)
{
  int numTracks = rom->read<uint8_t>(addr);
  for (int i = 0; i < numTracks; i++) {
    //std::cout << "Track " << std::dec << i << ": " << std::hex << (addr + 8 + i*4) << " - " << rom->read<uint32_t>(addr + 8 + i*4) << std::endl;
    TrackData* track = new TrackData(rom, rom->readPointer(addr + 8 + i * 4, false));
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
