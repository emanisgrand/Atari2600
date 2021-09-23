//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "FSNode.hxx"
#include "Settings.hxx"
#include "System.hxx"
#include "MD5.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
  #include "Base.hxx"
#endif

#include "Cart.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge::Cartridge(const Settings& settings, const string& md5)
  : mySettings{settings}
{
  auto to_uInt32 = [](const string& s, uInt32 pos) {
    return uInt32(std::stoul(s.substr(pos, 8), nullptr, 16));
  };

  uInt32 seed = to_uInt32(md5, 0)  ^ to_uInt32(md5, 8) ^
                to_uInt32(md5, 16) ^ to_uInt32(md5, 24);
  Random rand(seed);
  for(uInt32 i = 0; i < 256; ++i)
    myRWPRandomValues[i] = rand.next();

  myRamReadAccesses.reserve(5);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::setAbout(const string& about, const string& type,
                         const string& id)
{
  myAbout = about;
  myDetectedType = type;
  myMultiCartID = id;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::saveROM(const FilesystemNode& out) const
{
  try
  {
    size_t size = 0;
    const ByteBuffer& image = getImage(size);
    if(size == 0)
    {
      cerr << "save not supported" << endl;
      return false;
    }
    out.write(image, size);
  }
  catch(...)
  {
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::bankChanged()
{
  bool changed = myBankChanged;
  myBankChanged = false;
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cartridge::bankSize(uInt16 bank) const
{
  size_t size;
  getImage(size);

  return static_cast<uInt16>(
     std::min(size / romBankCount(), 4_KB)); // assuming that each bank has the same size
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Cartridge::peekRAM(uInt8& dest, uInt16 address)
{
  uInt8 value = myRWPRandomValues[address & 0xFF];

  // Reading from the write port triggers an unwanted write
  // But this only happens when in normal emulation mode
#ifdef DEBUGGER_SUPPORT
  if(!bankLocked() && !mySystem->autodetectMode())
  {
    // Record access here; final determination will happen in ::pokeRAM()
    myRamReadAccesses.push_back(address);
    dest = value;
  }
#else
  if(!mySystem->autodetectMode())
    dest = value;
#endif
  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::pokeRAM(uInt8& dest, uInt16 address, uInt8 value)
{
#ifdef DEBUGGER_SUPPORT
  for(auto i = myRamReadAccesses.begin(); i != myRamReadAccesses.end(); ++i)
  {
    if(*i == address)
    {
      myRamReadAccesses.erase(i);
      break;
    }
  }
#endif
  dest = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::createRomAccessArrays(size_t size)
{
  myAccessSize = uInt32(size);
#ifdef DEBUGGER_SUPPORT
  myRomAccessBase = make_unique<Device::AccessFlags[]>(size);
  std::fill_n(myRomAccessBase.get(), size, Device::ROW);
  myRomAccessCounter = make_unique<Device::AccessCounter[]>(size * 2);
  std::fill_n(myRomAccessCounter.get(), size * 2, 0);
#else
  myRomAccessBase = nullptr;
  myRomAccessCounter = nullptr;
#endif
}

#ifdef DEBUGGER_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cartridge::getAccessCounters() const
{
  ostringstream out;
  uInt32 offset = 0;

  for(uInt16 bank = 0; bank < romBankCount(); ++bank)
  {
    uInt16 origin = bankOrigin(bank);
    uInt16 bankSize = this->bankSize(bank);

    out << "Bank " << Common::Base::toString(bank, Common::Base::Fmt::_10_8) << " / 0.."
      << Common::Base::toString(romBankCount() - 1, Common::Base::Fmt::_10_8) << " reads:\n";
    for(uInt16 addr = 0; addr < bankSize; ++addr)
    {
      out << Common::Base::HEX4 << (addr | origin) << ","
        << Common::Base::toString(myRomAccessCounter[offset + addr], Common::Base::Fmt::_10_8) << ", ";
    }
    out << "\n";
    out << "Bank " << Common::Base::toString(bank, Common::Base::Fmt::_10_8) << " / 0.."
      << Common::Base::toString(romBankCount() - 1, Common::Base::Fmt::_10_8) << " writes:\n";
    for(uInt16 addr = 0; addr < bankSize; ++addr)
    {
      out << Common::Base::HEX4 << (addr | origin) << ","
        << Common::Base::toString(myRomAccessCounter[offset + addr + myAccessSize], Common::Base::Fmt::_10_8) << ", ";
    }
    out << "\n";

    offset += bankSize;
  }

  return out.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cartridge::bankOrigin(uInt16 bank) const
{
  // isolate the high 3 address bits, count them and
  // select the most frequent to define the bank origin
  // TODO: origin for banks smaller than 4K
  const int intervals = 0x8000 / 0x100;
  uInt32 offset = bank * bankSize();
  //uInt16 addrMask = (4_KB - 1) & ~(bankSize(bank) - 1);
  //int addrShift = 0;
  std::array<uInt16, intervals> count; // up to 128 256 byte interval origins


  //if(addrMask)
  //  addrShift = log(addrMask) / log(2);
  //addrMask;

  count.fill(0);
  for(uInt16 addr = 0x0000; addr < bankSize(bank); ++addr)
  {
    Device::AccessFlags flags = myRomAccessBase[offset + addr];
    // only count really accessed addresses
    if(flags & ~Device::ROW)
    {
      //uInt16 addrBit = addr >> addrShift;
      count[(flags & Device::HADDR) >> 13]++;
    }
  }
  uInt16 max = 0, maxIdx = 0;
  for(int idx = 0; idx < intervals; ++idx)
  {
    if(count[idx] > max)
    {
      max = count[idx];
      maxIdx = idx;
    }
  }
  return maxIdx << 13 | 0x1000; //| (offset & 0xfff);
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cartridge::initializeRAM(uInt8* arr, size_t size, uInt8 val) const
{
  if(randomInitialRAM())
    for(size_t i = 0; i < size; ++i)
      arr[i] = mySystem->randGenerator().next();
  else
    std::fill_n(arr, size, val);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cartridge::initializeStartBank(uInt16 defaultBank)
{
  int propsBank = myStartBankFromPropsFunc();

  if(randomStartBank())
    return myStartBank = mySystem->randGenerator().next() % romBankCount();
  else if(propsBank >= 0)
    return myStartBank = BSPF::clamp(propsBank, 0, romBankCount() - 1);
  else
    return myStartBank = BSPF::clamp(int(defaultBank), 0, romBankCount() - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::randomInitialRAM() const
{
  return mySettings.getBool(mySettings.getBool("dev.settings") ? "dev.ramrandom" : "plr.ramrandom");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cartridge::randomStartBank() const
{
  return mySettings.getBool(mySettings.getBool("dev.settings") ? "dev.bankrandom" : "plr.bankrandom");
}
