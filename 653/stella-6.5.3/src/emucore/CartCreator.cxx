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

#include "bspf.hxx"
#include "Cart.hxx"
#include "Cart0840.hxx"
#include "Cart2K.hxx"
#include "Cart3E.hxx"
#include "Cart3EX.hxx"
#include "Cart3EPlus.hxx"
#include "Cart3F.hxx"
#include "Cart4A50.hxx"
#include "Cart4K.hxx"
#include "Cart4KSC.hxx"
#include "CartAR.hxx"
#include "CartBF.hxx"
#include "CartBFSC.hxx"
#include "CartBUS.hxx"
#include "CartCDF.hxx"
#include "CartCM.hxx"
#include "CartCTY.hxx"
#include "CartCV.hxx"
#include "CartDF.hxx"
#include "CartDFSC.hxx"
#include "CartDPC.hxx"
#include "CartDPCPlus.hxx"
#include "CartE0.hxx"
#include "CartE7.hxx"
#include "CartE78K.hxx"
#include "CartEF.hxx"
#include "CartEFSC.hxx"
#include "CartF0.hxx"
#include "CartF4.hxx"
#include "CartF4SC.hxx"
#include "CartF6.hxx"
#include "CartF6SC.hxx"
#include "CartF8.hxx"
#include "CartF8SC.hxx"
#include "CartFA.hxx"
#include "CartFA2.hxx"
#include "CartFC.hxx"
#include "CartFE.hxx"
#include "CartMDM.hxx"
#include "CartSB.hxx"
#include "CartTVBoy.hxx"
#include "CartUA.hxx"
#include "CartWD.hxx"
#include "CartX07.hxx"
#include "MD5.hxx"
#include "Props.hxx"
#include "Logger.hxx"
#include "OSystem.hxx"

#include "CartDetector.hxx"
#include "CartCreator.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Cartridge> CartCreator::create(const FilesystemNode& file,
    const ByteBuffer& image, size_t size, string& md5,
    const string& propertiesType, Settings& settings)
{
  unique_ptr<Cartridge> cartridge;
  Bankswitch::Type type = Bankswitch::nameToType(propertiesType),
         detectedType = type;
  string id;

  // Collect some info about the ROM
  ostringstream buf;

  // First inspect the file extension itself
  // If a valid type is found, it will override the one passed into this method
  Bankswitch::Type typeByName = Bankswitch::typeFromExtension(file);
  if(typeByName != Bankswitch::Type::_AUTO)
    type = detectedType = typeByName;

  // See if we should try to auto-detect the cartridge type
  // If we ask for extended info, always do an autodetect
  if(type == Bankswitch::Type::_AUTO || settings.getBool("rominfo"))
  {
    detectedType = CartDetector::autodetectType(image, size);
    if(type != Bankswitch::Type::_AUTO && type != detectedType)
      cerr << "Auto-detection not consistent: "
           << Bankswitch::typeToName(type) << ", "
           << Bankswitch::typeToName(detectedType) << endl;

    type = detectedType;
    buf << Bankswitch::typeToName(type) << "*";
  }
  else
    buf << Bankswitch::typeToName(type);

  // Check for multicart first; if found, get the correct part of the image
  switch(type)
  {
    case Bankswitch::Type::_2IN1:
      // Make sure we have a valid sized image
      if(size == 2*2_KB || size == 2*4_KB || size == 2*8_KB || size == 2*16_KB)
      {
        cartridge =
          createFromMultiCart(image, size, 2, md5, detectedType, id, settings);
        buf << id;
      }
      else
        throw runtime_error("Invalid cart size for type '" +
                            Bankswitch::typeToName(type) + "'");
      break;

    case Bankswitch::Type::_4IN1:
      // Make sure we have a valid sized image
      if(size == 4*2_KB || size == 4*4_KB || size == 4*8_KB)
      {
        cartridge =
          createFromMultiCart(image, size, 4, md5, detectedType, id, settings);
        buf << id;
      }
      else
        throw runtime_error("Invalid cart size for type '" +
                            Bankswitch::typeToName(type) + "'");
      break;

    case Bankswitch::Type::_8IN1:
      // Make sure we have a valid sized image
      if(size == 8*2_KB || size == 8*4_KB || size == 8*8_KB)
      {
        cartridge =
          createFromMultiCart(image, size, 8, md5, detectedType, id, settings);
        buf << id;
      }
      else
        throw runtime_error("Invalid cart size for type '" +
                            Bankswitch::typeToName(type) + "'");
      break;

    case Bankswitch::Type::_16IN1:
      // Make sure we have a valid sized image
      if(size == 16*2_KB || size == 16*4_KB || size == 16*8_KB)
      {
        cartridge =
          createFromMultiCart(image, size, 16, md5, detectedType, id, settings);
        buf << id;
      }
      else
        throw runtime_error("Invalid cart size for type '" +
                            Bankswitch::typeToName(type) + "'");
      break;

    case Bankswitch::Type::_32IN1:
      // Make sure we have a valid sized image
      if(size == 32*2_KB || size == 32*4_KB)
      {
        cartridge =
          createFromMultiCart(image, size, 32, md5, detectedType, id, settings);
        buf << id;
      }
      else
        throw runtime_error("Invalid cart size for type '" +
                            Bankswitch::typeToName(type) + "'");
      break;

    case Bankswitch::Type::_64IN1:
      // Make sure we have a valid sized image
      if(size == 64*2_KB || size == 64*4_KB)
      {
        cartridge =
          createFromMultiCart(image, size, 64, md5, detectedType, id, settings);
        buf << id;
      }
      else
        throw runtime_error("Invalid cart size for type '" +
                            Bankswitch::typeToName(type) + "'");
      break;

    case Bankswitch::Type::_128IN1:
      // Make sure we have a valid sized image
      if(size == 128*2_KB || size == 128*4_KB)
      {
        cartridge =
          createFromMultiCart(image, size, 128, md5, detectedType, id, settings);
        buf << id;
      }
      else
        throw runtime_error("Invalid cart size for type '" +
                            Bankswitch::typeToName(type) + "'");
      break;

    default:
      cartridge = createFromImage(image, size, detectedType, md5, settings);
      break;
  }

  if(size < 1_KB)
    buf << " (" << size << "B) ";
  else
    buf << " (" << (size/1_KB) << "K) ";

  cartridge->setAbout(buf.str(), Bankswitch::typeToName(type), id);

  return cartridge;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Cartridge>
CartCreator::createFromMultiCart(const ByteBuffer& image, size_t& size,
    uInt32 numroms, string& md5, Bankswitch::Type type, string& id, Settings& settings)
{
  // Get a piece of the larger image
  uInt32 i = settings.getInt("romloadcount");

  // Move to the next game
  if(!settings.getBool("romloadprev"))
    i = (i + 1) % numroms;
  else
    i = (i - 1) % numroms;
  settings.setValue("romloadcount", i);

  size /= numroms;
  ByteBuffer slice = make_unique<uInt8[]>(size);
  std::copy_n(image.get()+i*size, size, slice.get());

  // We need a new md5 and name
  md5 = MD5::hash(slice, uInt32(size)); // FIXME
  ostringstream buf;
  buf << " [G" << (i+1) << "]";
  id = buf.str();

  if(size <= 2_KB)       type = Bankswitch::Type::_2K;
  else if(size == 4_KB)  type = Bankswitch::Type::_4K;
  else if(size == 8_KB)  type = Bankswitch::Type::_F8;
  else  /* default */    type = Bankswitch::Type::_4K;

  return createFromImage(slice, size, type, md5, settings);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Cartridge>
CartCreator::createFromImage(const ByteBuffer& image, size_t size, Bankswitch::Type type,
                             const string& md5, Settings& settings)
{
  // We should know the cart's type by now so let's create it
  switch(type)
  {
    case Bankswitch::Type::_0840:
      return make_unique<Cartridge0840>(image, size, md5, settings);
    case Bankswitch::Type::_2K:
      return make_unique<Cartridge2K>(image, size, md5, settings);
    case Bankswitch::Type::_3E:
      return make_unique<Cartridge3E>(image, size, md5, settings);
    case Bankswitch::Type::_3EX:
      return make_unique<Cartridge3EX>(image, size, md5, settings);
    case Bankswitch::Type::_3EP:
      return make_unique<Cartridge3EPlus>(image, size, md5, settings);
    case Bankswitch::Type::_3F:
      return make_unique<Cartridge3F>(image, size, md5, settings);
    case Bankswitch::Type::_4A50:
      return make_unique<Cartridge4A50>(image, size, md5, settings);
    case Bankswitch::Type::_4K:
      return make_unique<Cartridge4K>(image, size, md5, settings);
    case Bankswitch::Type::_4KSC:
      return make_unique<Cartridge4KSC>(image, size, md5, settings);
    case Bankswitch::Type::_AR:
      return make_unique<CartridgeAR>(image, size, md5, settings);
    case Bankswitch::Type::_BF:
      return make_unique<CartridgeBF>(image, size, md5, settings);
    case Bankswitch::Type::_BFSC:
      return make_unique<CartridgeBFSC>(image, size, md5, settings);
    case Bankswitch::Type::_BUS:
      return make_unique<CartridgeBUS>(image, size, md5, settings);
    case Bankswitch::Type::_CDF:
      return make_unique<CartridgeCDF>(image, size, md5, settings);
    case Bankswitch::Type::_CM:
      return make_unique<CartridgeCM>(image, size, md5, settings);
    case Bankswitch::Type::_CTY:
      return make_unique<CartridgeCTY>(image, size, md5, settings);
    case Bankswitch::Type::_CV:
      return make_unique<CartridgeCV>(image, size, md5, settings);
    case Bankswitch::Type::_DF:
      return make_unique<CartridgeDF>(image, size, md5, settings);
    case Bankswitch::Type::_DFSC:
      return make_unique<CartridgeDFSC>(image, size, md5, settings);
    case Bankswitch::Type::_DPC:
      return make_unique<CartridgeDPC>(image, size, md5, settings);
    case Bankswitch::Type::_DPCP:
      return make_unique<CartridgeDPCPlus>(image, size, md5, settings);
    case Bankswitch::Type::_E0:
      return make_unique<CartridgeE0>(image, size, md5, settings);
    case Bankswitch::Type::_E7:
      return make_unique<CartridgeE7>(image, size, md5, settings);
    case Bankswitch::Type::_E78K:
      return make_unique<CartridgeE78K>(image, size, md5, settings);
    case Bankswitch::Type::_EF:
      return make_unique<CartridgeEF>(image, size, md5, settings);
    case Bankswitch::Type::_EFSC:
      return make_unique<CartridgeEFSC>(image, size, md5, settings);
    case Bankswitch::Type::_F0:
      return make_unique<CartridgeF0>(image, size, md5, settings);
    case Bankswitch::Type::_F4:
      return make_unique<CartridgeF4>(image, size, md5, settings);
    case Bankswitch::Type::_F4SC:
      return make_unique<CartridgeF4SC>(image, size, md5, settings);
    case Bankswitch::Type::_F6:
      return make_unique<CartridgeF6>(image, size, md5, settings);
    case Bankswitch::Type::_F6SC:
      return make_unique<CartridgeF6SC>(image, size, md5, settings);
    case Bankswitch::Type::_F8:
      return make_unique<CartridgeF8>(image, size, md5, settings);
    case Bankswitch::Type::_F8SC:
      return make_unique<CartridgeF8SC>(image, size, md5, settings);
    case Bankswitch::Type::_FA:
      return make_unique<CartridgeFA>(image, size, md5, settings);
    case Bankswitch::Type::_FA2:
      return make_unique<CartridgeFA2>(image, size, md5, settings);
    case Bankswitch::Type::_FC:
      return make_unique<CartridgeFC>(image, size, md5, settings);
    case Bankswitch::Type::_FE:
      return make_unique<CartridgeFE>(image, size, md5, settings);
    case Bankswitch::Type::_MDM:
      return make_unique<CartridgeMDM>(image, size, md5, settings);
    case Bankswitch::Type::_UA:
      return make_unique<CartridgeUA>(image, size, md5, settings);
    case Bankswitch::Type::_UASW:
      return make_unique<CartridgeUA>(image, size, md5, settings, true);
    case Bankswitch::Type::_SB:
      return make_unique<CartridgeSB>(image, size, md5, settings);
    case Bankswitch::Type::_TVBOY:
      return make_unique<CartridgeTVBoy>(image, size, md5, settings);
    case Bankswitch::Type::_WD:
    case Bankswitch::Type::_WDSW:
      return make_unique<CartridgeWD>(image, size, md5, settings);
    case Bankswitch::Type::_X07:
      return make_unique<CartridgeX07>(image, size, md5, settings);
    default:
      return nullptr;  // The remaining types have already been handled
  }
}
