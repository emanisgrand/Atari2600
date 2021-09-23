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

#include "M6532.hxx"
#include "System.hxx"
#include "CartFE.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFE::CartridgeFE(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings,
                         size_t bsSize)
  : CartridgeEnhanced(image, size, md5, settings, bsSize)
{
  myDirectPeek = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFE::reset()
{
  CartridgeEnhanced::reset();
  myLastAccessWasFE = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFE::install(System& system)
{
  CartridgeEnhanced::install(system);

  // The hotspot $01FE is in a mirror of zero-page RAM
  // We need to claim access to it here, and deal with it in peek/poke below
  System::PageAccess access(this, System::PageAccessType::READWRITE);
  for(uInt16 addr = 0x180; addr < 0x200; addr += System::PAGE_SIZE)
    mySystem->setPageAccess(addr, access);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::checkSwitchBank(uInt16 address, uInt8 value)
{
  if(myLastAccessWasFE)
  {
    bank((value & 0x20) ? 0 : 1);
    myLastAccessWasFE = false; // was: address == 0x01FE;
    return true;
  }
  myLastAccessWasFE = address == 0x01FE;
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeFE::peek(uInt16 address)
{
  uInt8 value = (address < 0x200) ? mySystem->m6532().peek(address) :
    myImage[myCurrentSegOffset[(address & myBankMask) >> myBankShift] + (address & myBankMask)];

  // Check if we hit hotspot
  checkSwitchBank(address, value);

  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::poke(uInt16 address, uInt8 value)
{
  if(address < 0x200)
    mySystem->m6532().poke(address, value);

  // Check if we hit hotspot
  checkSwitchBank(address, value);

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::save(Serializer& out) const
{
  CartridgeEnhanced::save(out);
  try
  {
    out.putBool(myLastAccessWasFE);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeFE::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::load(Serializer& in)
{
  CartridgeEnhanced::load(in);
  try
  {
    myLastAccessWasFE = in.getBool();
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeFE::load" << endl;
    return false;
  }

  return true;
}
