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

#include "CartDPCPlus.hxx"
#include "DataGridWidget.hxx"
#include "PopUpWidget.hxx"
#include "EditTextWidget.hxx"
#include "CartDPCPlusWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDPCPlusWidget::CartridgeDPCPlusWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeDPCPlus& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart{cart}
{
  size_t size = cart.mySize;

  ostringstream info;
  info << "Extended DPC cartridge, six 4K banks, 4K display bank, 1K frequency table, "
       << "8K DPC RAM\n"
       << "DPC registers accessible @ $F000 - $F07F\n"
       << "  $F000 - $F03F (R), $F040 - $F07F (W)\n"
       << "Banks accessible at hotspots $FFF6 to $FFFB\n"
       << "Startup bank = " << cart.startBank() << "\n"
       << "Ver = " << cart.myDriverMD5;

#if 0
  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC, spot = 0xFF6; i < 6; ++i, offset += 0x1000)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << i << " @ $" << HEX4 << (start + 0x80) << " - "
         << "$" << (start + 0xFFF) << " (hotspot = $" << (spot+i) << ")\n";
  }
#endif

  int xpos = 2,
      ypos = addBaseInformation(size, "Activision (Pitfall II)", info.str()) +
              myLineHeight;

  VariantList items;
  VarList::push_back(items, "0 ($FFF6)");
  VarList::push_back(items, "1 ($FFF7)");
  VarList::push_back(items, "2 ($FFF8)");
  VarList::push_back(items, "3 ($FFF9)");
  VarList::push_back(items, "4 ($FFFA)");
  VarList::push_back(items, "5 ($FFFB)");
  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("0 ($FFFx)"),
                    myLineHeight, items, "Set bank     ",
                    0, kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);

  // Top registers
  int lwidth = _font.getStringWidth("Counter Registers ");
  xpos = 2;  ypos += myLineHeight + 8;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Top Registers ", TextAlign::Left);
  xpos += lwidth;

  myTops = new DataGridWidget(boss, _nfont, xpos, ypos-2, 8, 1, 2, 8, Common::Base::Fmt::_16);
  myTops->setTarget(this);
  myTops->setEditable(false);

  // Bottom registers
  xpos = 2;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Bottom Registers ", TextAlign::Left);
  xpos += lwidth;

  myBottoms = new DataGridWidget(boss, _nfont, xpos, ypos-2, 8, 1, 2, 8, Common::Base::Fmt::_16);
  myBottoms->setTarget(this);
  myBottoms->setEditable(false);

  // Counter registers
  xpos = 2;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Counter Registers ", TextAlign::Left);
  xpos += lwidth;

  myCounters = new DataGridWidget(boss, _nfont, xpos, ypos-2, 8, 1, 4, 16, Common::Base::Fmt::_16_4);
  myCounters->setTarget(this);
  myCounters->setEditable(false);

  // Fractional counter registers
  xpos = 2;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Frac Counters ", TextAlign::Left);
  xpos += lwidth;

  myFracCounters = new DataGridWidget(boss, _nfont, xpos, ypos-2, 4, 2, 8, 32, Common::Base::Fmt::_16_8);
  myFracCounters->setTarget(this);
  myFracCounters->setEditable(false);

  // Fractional increment registers
  xpos = 2;  ypos += myFracCounters->getHeight() + 8;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Frac Increments ", TextAlign::Left);
  xpos += lwidth;

  myFracIncrements = new DataGridWidget(boss, _nfont, xpos, ypos-2, 8, 1, 2, 8, Common::Base::Fmt::_16);
  myFracIncrements->setTarget(this);
  myFracIncrements->setEditable(false);

  // Special function parameters
  xpos = 2;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Function Params ", TextAlign::Left);
  xpos += lwidth;

  myParameter = new DataGridWidget(boss, _nfont, xpos, ypos-2, 8, 1, 2, 8, Common::Base::Fmt::_16);
  myParameter->setTarget(this);
  myParameter->setEditable(false);

  // Music counters
  xpos = 2;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Music Counters ", TextAlign::Left);
  xpos += lwidth;

  myMusicCounters = new DataGridWidget(boss, _nfont, xpos, ypos-2, 3, 1, 8, 32, Common::Base::Fmt::_16_8);
  myMusicCounters->setTarget(this);
  myMusicCounters->setEditable(false);

  // Music frequencies
  xpos = 2;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Music Frequencies ", TextAlign::Left);
  xpos += lwidth;

  myMusicFrequencies = new DataGridWidget(boss, _nfont, xpos, ypos-2, 3, 1, 8, 32, Common::Base::Fmt::_16_8);
  myMusicFrequencies->setTarget(this);
  myMusicFrequencies->setEditable(false);

  // Music waveforms
  xpos = 2;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Music Waveforms ", TextAlign::Left);
  xpos += lwidth;

  myMusicWaveforms = new DataGridWidget(boss, _nfont, xpos, ypos-2, 3, 1, 4, 16, Common::Base::Fmt::_16_4);
  myMusicWaveforms->setTarget(this);
  myMusicWaveforms->setEditable(false);

  // Current random number
  lwidth = _font.getStringWidth("Current random number ");
  xpos = 2;  ypos += myLineHeight + 4;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Current random number ", TextAlign::Left);
  xpos += lwidth;

  myRandom = new DataGridWidget(boss, _nfont, xpos, ypos-2, 1, 1, 8, 32, Common::Base::Fmt::_16_8);
  myRandom->setTarget(this);
  myRandom->setEditable(false);

  // Fast fetch and immediate mode LDA flags
  xpos += myRandom->getWidth() + 30;
  myFastFetch = new CheckboxWidget(boss, _font, xpos, ypos, "Fast Fetcher enabled");
  myFastFetch->setTarget(this);
  myFastFetch->setEditable(false);
  ypos += myLineHeight + 4;
  myIMLDA = new CheckboxWidget(boss, _font, xpos, ypos, "Immediate mode LDA");
  myIMLDA->setTarget(this);
  myIMLDA->setEditable(false);

  xpos = 2;  ypos += myLineHeight + 4 * 1;
  new StaticTextWidget(boss, _font, xpos, ypos + 1, "Last ARM run stats:");
  xpos = 2 + _font.getMaxCharWidth() * 2; ypos += myLineHeight + 4;
  StaticTextWidget* s = new StaticTextWidget(boss, _font, xpos, ypos + 1, "Mem. cycles ");
  myThumbMemCycles = new EditTextWidget(boss, _font, s->getRight(), ypos - 1,
                                        EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myThumbMemCycles->setEditable(false);
  myThumbMemCycles->setToolTip("Number of memory cycles of last ARM run.");

  s = new StaticTextWidget(boss, _font, myThumbMemCycles->getRight() + _fontWidth * 2, ypos + 1, "Fetches ");
  myThumbFetches = new EditTextWidget(boss, _font, s->getRight(), ypos - 1,
                                      EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myThumbFetches->setEditable(false);
  myThumbFetches->setToolTip("Number of fetches/instructions of last ARM run.");

  ypos += myLineHeight + 4;
  s = new StaticTextWidget(boss, _font, xpos, ypos + 1, "Reads ");
  myThumbReads = new EditTextWidget(boss, _font, myThumbMemCycles->getLeft(), ypos - 1,
                                    EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myThumbReads->setEditable(false);
  myThumbReads->setToolTip("Number of reads of last ARM run.");

  s = new StaticTextWidget(boss, _font, myThumbReads->getRight() + _fontWidth * 2, ypos + 1, "Writes ");
  myThumbWrites = new EditTextWidget(boss, _font, myThumbFetches->getLeft(), ypos - 1,
                                     EditTextWidget::calcWidth(_font, 6), myLineHeight, "");
  myThumbWrites->setEditable(false);
  myThumbWrites->setToolTip("Number of write of last ARM run.");

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCPlusWidget::saveOldState()
{
  myOldState.tops.clear();
  myOldState.bottoms.clear();
  myOldState.counters.clear();
  myOldState.fraccounters.clear();
  myOldState.fracinc.clear();
  myOldState.param.clear();
  myOldState.mcounters.clear();
  myOldState.mfreqs.clear();
  myOldState.mwaves.clear();
  myOldState.internalram.clear();

  for(uInt32 i = 0; i < 8; ++i)
  {
    myOldState.tops.push_back(myCart.myTops[i]);
    myOldState.bottoms.push_back(myCart.myBottoms[i]);
    myOldState.counters.push_back(myCart.myCounters[i]);
    myOldState.fraccounters.push_back(myCart.myFractionalCounters[i]);
    myOldState.fracinc.push_back(myCart.myFractionalIncrements[i]);
    myOldState.param.push_back(myCart.myParameter[i]);
  }
  for(uInt32 i = 0; i < 3; ++i)
  {
    myOldState.mcounters.push_back(myCart.myMusicCounters[i]);
    myOldState.mfreqs.push_back(myCart.myMusicFrequencies[i]);
    myOldState.mwaves.push_back(myCart.myMusicWaveforms[i]);
  }

  myOldState.random = myCart.myRandomNumber;

  for(uInt32 i = 0; i < internalRamSize(); ++i)
    myOldState.internalram.push_back(myCart.myDisplayImage[i]);

  myOldState.bank = myCart.getBank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCPlusWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.getBank(), myCart.getBank() != myOldState.bank);

  // Get registers, using change tracking
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myTops[i]);
    changed.push_back(myCart.myTops[i] != myOldState.tops[i]);
  }
  myTops->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myBottoms[i]);
    changed.push_back(myCart.myBottoms[i] != myOldState.bottoms[i]);
  }
  myBottoms->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myCounters[i]);
    changed.push_back(myCart.myCounters[i] != myOldState.counters[i]);
  }
  myCounters->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myFractionalCounters[i]);
    changed.push_back(myCart.myFractionalCounters[i] != uInt32(myOldState.fraccounters[i]));
  }
  myFracCounters->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myFractionalIncrements[i]);
    changed.push_back(myCart.myFractionalIncrements[i] != myOldState.fracinc[i]);
  }
  myFracIncrements->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 8; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myParameter[i]);
    changed.push_back(myCart.myParameter[i] != myOldState.param[i]);
  }
  myParameter->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 3; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myMusicCounters[i]);
    changed.push_back(myCart.myMusicCounters[i] != uInt32(myOldState.mcounters[i]));
  }
  myMusicCounters->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 3; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myMusicFrequencies[i]);
    changed.push_back(myCart.myMusicFrequencies[i] != uInt32(myOldState.mfreqs[i]));
  }
  myMusicFrequencies->setList(alist, vlist, changed);

  alist.clear();  vlist.clear();  changed.clear();
  for(int i = 0; i < 3; ++i)
  {
    alist.push_back(0);  vlist.push_back(myCart.myMusicWaveforms[i]);
    changed.push_back(myCart.myMusicWaveforms[i] != myOldState.mwaves[i]);
  }
  myMusicWaveforms->setList(alist, vlist, changed);

  myRandom->setList(0, myCart.myRandomNumber,
      myCart.myRandomNumber != myOldState.random);

  myFastFetch->setState(myCart.myFastFetch);
  myIMLDA->setState(myCart.myLDAimmediate);

  myThumbMemCycles->setText(Common::Base::toString(myCart.stats().fetches
                            + myCart.stats().reads + myCart.stats().writes,
                            Common::Base::Fmt::_10_6));
  myThumbFetches->setText(Common::Base::toString(myCart.stats().fetches,
                          Common::Base::Fmt::_10_6));
  myThumbReads->setText(Common::Base::toString(myCart.stats().reads,
                        Common::Base::Fmt::_10_6));
  myThumbWrites->setText(Common::Base::toString(myCart.stats().writes,
                         Common::Base::Fmt::_10_6));

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCPlusWidget::handleCommand(CommandSender* sender,
                                           int cmd, int data, int id)
{
  if(cmd == kBankChanged)
  {
    myCart.unlockBank();
    myCart.bank(myBank->getSelected());
    myCart.lockBank();
    invalidate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeDPCPlusWidget::bankState()
{
  ostringstream& buf = buffer();

  static constexpr std::array<const char*, 6> spot = {
    "$FFF6", "$FFF7", "$FFF8", "$FFF9", "$FFFA", "$FFFB"
  };
  buf << "Bank = " << std::dec << myCart.getBank()
      << ", hotspot = " << spot[myCart.getBank()];

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeDPCPlusWidget::internalRamSize()
{
  return 5*1024;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeDPCPlusWidget::internalRamRPort(int start)
{
  return 0x0000 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeDPCPlusWidget::internalRamDescription()
{
  ostringstream desc;
  desc << "$0000 - $0FFF - 4K display data\n"
       << "                indirectly accessible to 6507\n"
       << "                via DPC+'s Data Fetcher registers\n"
       << "$1000 - $13FF - 1K frequency table,\n"
       << "                C variables and C stack\n"
       << "                not accessible to 6507";

  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeDPCPlusWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; i++)
    myRamOld.push_back(myOldState.internalram[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeDPCPlusWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; i++)
    myRamCurrent.push_back(myCart.myDisplayImage[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDPCPlusWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myDisplayImage[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeDPCPlusWidget::internalRamGetValue(int addr)
{
  return myCart.myDisplayImage[addr];
}
