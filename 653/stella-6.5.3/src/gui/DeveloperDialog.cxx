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
#include "OSystem.hxx"
#include "Joystick.hxx"
#include "Paddles.hxx"
#include "PointingDevice.hxx"
#include "SaveKey.hxx"
#include "AtariVox.hxx"
#include "Settings.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "RadioButtonWidget.hxx"
#include "ColorWidget.hxx"
#include "TabWidget.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "Console.hxx"
#include "TIA.hxx"
#include "OSystem.hxx"
#include "EventHandler.hxx"
#include "StateManager.hxx"
#include "RewindManager.hxx"
#include "M6502.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
  #include "DebuggerDialog.hxx"
#endif
#include "DeveloperDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DeveloperDialog::DeveloperDialog(OSystem& osystem, DialogContainer& parent,
                                 const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Developer settings")
{
  const int lineHeight   = Dialog::lineHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();
  int xpos, ypos;

  // Set real dimensions
  setSize(53 * fontWidth + HBORDER * 2,
          _th + VGAP * 3 + lineHeight + 13 * (lineHeight + VGAP) + buttonHeight + VBORDER * 3,
          max_w, max_h);

  // The tab widget
  xpos = 2; ypos = VGAP;
  myTab = new TabWidget(this, font, xpos, ypos + _th,
                        _w - 2 * xpos,
                        _h - _th - VGAP - buttonHeight - VBORDER * 2);
  addTabWidget(myTab);

  addEmulationTab(font);
  addTiaTab(font);
  addVideoTab(font);
  addTimeMachineTab(font);
  addDebuggerTab(font);

  WidgetArray wid;
  addDefaultsOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);

  // Activate the first tab
  myTab->setActiveTab(0);

  setHelpAnchor("Debugger");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addEmulationTab(const GUI::Font& font)
{
  const int lineHeight = Dialog::lineHeight(),
            fontWidth  = Dialog::fontWidth(),
            VBORDER    = Dialog::vBorder(),
            HBORDER    = Dialog::hBorder(),
            VGAP       = Dialog::vGap(),
            INDENT     = Dialog::indent();
  int ypos = VBORDER;
  WidgetArray wid;
  VariantList items;
  int tabID = myTab->addTab(" Emulation ", TabWidget::AUTO_WIDTH);

  // settings set
  mySettingsGroupEmulation = new RadioButtonGroup();
  RadioButtonWidget* r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1,
                                               "Player settings", mySettingsGroupEmulation, kPlrSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP;
  r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1,
                            "Developer settings", mySettingsGroupEmulation, kDevSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 1;

  myFrameStatsWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                          "Console info overlay");
  wid.push_back(myFrameStatsWidget);

  myDetectedInfoWidget = new CheckboxWidget(myTab, font,
                                            myFrameStatsWidget->getRight() + fontWidth * 2.5, ypos + 1,
                                            "Detected settings info");
  myDetectedInfoWidget->setToolTip("Display detected controllers, bankswitching\n"
                                   "and TV types at ROM start.");
  wid.push_back(myDetectedInfoWidget);
  ypos += lineHeight + VGAP;

  // 2600/7800 mode
  items.clear();
  VarList::push_back(items, "Atari 2600", "2600");
  VarList::push_back(items, "Atari 7800", "7800");
  int lwidth = font.getStringWidth("Console ");
  int pwidth = font.getStringWidth("Atari 2600");

  myConsoleWidget = new PopUpWidget(myTab, font, HBORDER + INDENT * 1, ypos, pwidth, lineHeight, items,
                                    "Console ", lwidth, kConsole);
  myConsoleWidget->setToolTip("Emulate Color/B&W/Pause key and zero\n"
                              "page RAM initialization differenly.");
  wid.push_back(myConsoleWidget);
  ypos += lineHeight + VGAP;

  // Randomize items
  myLoadingROMLabel = new StaticTextWidget(myTab, font, HBORDER + INDENT*1, ypos + 1, "When loading a ROM:");
  wid.push_back(myLoadingROMLabel);
  ypos += lineHeight + VGAP;

  myRandomBankWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1,
                                          "Random startup bank");
  myRandomBankWidget->setToolTip("Randomize the startup bank for\n"
                                 "most classic bankswitching types.");
  wid.push_back(myRandomBankWidget);
  ypos += lineHeight + VGAP;

  // Randomize RAM
  myRandomizeRAMWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1,
                                            "Randomize zero-page and extended RAM", kRandRAMID);
  wid.push_back(myRandomizeRAMWidget);
  ypos += lineHeight + VGAP;

  // Randomize CPU
  myRandomizeCPULabel = new StaticTextWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1, "Randomize CPU ");
  wid.push_back(myRandomizeCPULabel);

  const std::array<string, 5> cpuregsLabels = {"SP", "A", "X", "Y", "PS"};
  int xpos = myRandomizeCPULabel->getRight() + fontWidth * 1.25;
  for(int i = 0; i < 5; ++i)
  {
    myRandomizeCPUWidget[i] = new CheckboxWidget(myTab, font, xpos, ypos + 1,
                                           cpuregsLabels[i], kRandCPUID);
    wid.push_back(myRandomizeCPUWidget[i]);
    xpos += CheckboxWidget::boxSize(font) + font.getStringWidth("XX") + fontWidth * 2.5;
  }
  ypos += lineHeight + VGAP;

  // How to handle undriven TIA pins
  myUndrivenPinsWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                            "Drive unused TIA pins randomly on a read/peek");
  myUndrivenPinsWidget->setToolTip("Read TIA pins random instead of last databus values.\n"
                                   "Helps detecting missing '#' for immediate loads.");
  wid.push_back(myUndrivenPinsWidget);
  ypos += lineHeight + VGAP;

#ifdef DEBUGGER_SUPPORT
  myRWPortBreakWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                           "Break on reads from write ports");
  myRWPortBreakWidget->setToolTip("Cause reads from write ports to interrupt\n"
                                  "emulation and enter debugger.");
  wid.push_back(myRWPortBreakWidget);
  ypos += lineHeight + VGAP;

  myWRPortBreakWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                           "Break on writes to read ports");
  myWRPortBreakWidget->setToolTip("Cause writes to read ports to interrupt\n"
                                  "emulation and enter debugger.");
  wid.push_back(myWRPortBreakWidget);
  ypos += lineHeight + VGAP;
#endif

  // Thumb ARM emulation exception
  myThumbExceptionWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                              "Fatal ARM emulation error throws exception");
  myThumbExceptionWidget->setToolTip("Cause Thumb ARM emulation to throw exceptions\n"
                                     "on fatal errors and enter the debugger.");
  wid.push_back(myThumbExceptionWidget);
  ypos += lineHeight + VGAP;

  // AtariVox/SaveKey EEPROM access
  myEEPROMAccessWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                            "Display AtariVox/SaveKey EEPROM R/W access");
  myEEPROMAccessWidget->setToolTip("Cause message display when AtariVox/\n"
                                   "SaveKey EEPROM is read or written.");
  wid.push_back(myEEPROMAccessWidget);

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);

  myTab->parentWidget(tabID)->setHelpAnchor("DeveloperEmulator");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addTiaTab(const GUI::Font& font)
{
  const int lineHeight = Dialog::lineHeight(),
            VBORDER    = Dialog::vBorder(),
            HBORDER    = Dialog::hBorder(),
            VGAP       = Dialog::vGap(),
            INDENT     = Dialog::indent();
  int ypos = VBORDER;
  int pwidth = font.getStringWidth("Faulty Cosmic Ark stars");
  WidgetArray wid;
  VariantList items;
  int tabID = myTab->addTab("  TIA  ", TabWidget::AUTO_WIDTH);

  wid.clear();

  // settings set
  mySettingsGroupTia = new RadioButtonGroup();
  RadioButtonWidget* r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1,
                                               "Player settings", mySettingsGroupTia, kPlrSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP;
  r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1,
                            "Developer settings", mySettingsGroupTia, kDevSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 1;

  items.clear();
  VarList::push_back(items, "Standard", "standard");
  VarList::push_back(items, "Faulty Kool-Aid Man", "koolaidman");
  VarList::push_back(items, "Faulty Cosmic Ark stars", "cosmicark");
  VarList::push_back(items, "Glitched Pesco", "pesco");
  VarList::push_back(items, "Glitched Quick Step!", "quickstep");
  VarList::push_back(items, "Glitched Indy 500 menu", "indy500");
  VarList::push_back(items, "Glitched He-Man title", "heman");
  VarList::push_back(items, "Custom", "custom");
  myTIATypeWidget = new PopUpWidget(myTab, font, HBORDER + INDENT, ypos - 1,
                                    pwidth, lineHeight, items, "Chip type ", 0, kTIAType);
  myTIATypeWidget->setToolTip("Select which TIA chip type to emulate.\n"
                              "Some types cause defined glitches.");
  wid.push_back(myTIATypeWidget);
  ypos += lineHeight + VGAP * 1;

  myInvPhaseLabel = new StaticTextWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1,
                                         "Inverted HMOVE clock phase for");
  myInvPhaseLabel->setToolTip("Objects react different to too\n"
                              "early HM" + ELLIPSIS + " after HMOVE changes.");
  wid.push_back(myInvPhaseLabel);
  ypos += lineHeight + VGAP * 1;

  myPlInvPhaseWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 3, ypos + 1,
                                          "Players");
  wid.push_back(myPlInvPhaseWidget);

  myMsInvPhaseWidget = new CheckboxWidget(myTab, font, myPlInvPhaseWidget->getRight() + 20, ypos + 1,
                                          "Missiles");
  wid.push_back(myMsInvPhaseWidget);

  myBlInvPhaseWidget = new CheckboxWidget(myTab, font, myMsInvPhaseWidget->getRight() + 20, ypos + 1,
                                          "Ball");
  wid.push_back(myBlInvPhaseWidget);
  ypos += lineHeight + VGAP * 1;

  myPlayfieldLabel = new StaticTextWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1,
                                         "Delayed playfield");
  myPlayfieldLabel->setToolTip("Playfield reacts one color clock slower to updates.");
  wid.push_back(myPlayfieldLabel);
  ypos += lineHeight + VGAP * 1;

  myPFBitsWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 3, ypos + 1, "Bits");
  wid.push_back(myPFBitsWidget);
  //ypos += lineHeight + VGAP * 1;

  myPFColorWidget = new CheckboxWidget(myTab, font, myPFBitsWidget->getRight() + 20, ypos + 1, "Color");
  wid.push_back(myPFColorWidget);
  ypos += lineHeight + VGAP * 1;

  myBackgroundLabel = new StaticTextWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1,
                                          "Delayed background");
  myBackgroundLabel->setToolTip("Background color reacts one color clock slower to updates.");
  wid.push_back(myBackgroundLabel);
  ypos += lineHeight + VGAP * 1;

  myBKColorWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 3, ypos + 1, "Color");
  wid.push_back(myBKColorWidget);
  ypos += lineHeight + VGAP * 1;

  ostringstream ss;
  ss << "Delayed VDEL" << ELLIPSIS << " swap for";
  mySwapLabel = new StaticTextWidget(myTab, font, HBORDER + INDENT * 2, ypos + 1, ss.str());
  mySwapLabel->setToolTip("VDELed objects react one color clock slower to updates.");
  wid.push_back(mySwapLabel);
  ypos += lineHeight + VGAP * 1;

  myPlSwapWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 3, ypos + 1, "Players");
  wid.push_back(myPlSwapWidget);

  myBlSwapWidget = new CheckboxWidget(myTab, font, myPlSwapWidget->getRight() + 20, ypos + 1, "Ball");
  wid.push_back(myBlSwapWidget);

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);

  myTab->parentWidget(tabID)->setHelpAnchor("DeveloperTIA");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addVideoTab(const GUI::Font& font)
{
  const int lineHeight = Dialog::lineHeight(),
            fontHeight = Dialog::fontHeight(),
            fontWidth  = Dialog::fontWidth(),
            VBORDER    = Dialog::vBorder(),
            HBORDER    = Dialog::hBorder(),
            VGAP       = Dialog::vGap(),
            INDENT     = Dialog::indent();
  int ypos = VBORDER;
  int lwidth = font.getStringWidth("Intensity ");
  int pwidth = fontWidth * 6;
  WidgetArray wid;
  VariantList items;
  int tabID = myTab->addTab(" Video ", TabWidget::AUTO_WIDTH);

  wid.clear();

  // settings set
  mySettingsGroupVideo = new RadioButtonGroup();
  RadioButtonWidget* r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1,
                                               "Player settings", mySettingsGroupVideo, kPlrSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP;
  r = new RadioButtonWidget(myTab, font, HBORDER, ypos + 1,
                            "Developer settings", mySettingsGroupVideo, kDevSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP * 1;

  // TV jitter effect
  myTVJitterWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                        "Jitter/roll effect", kTVJitter);
  myTVJitterWidget->setToolTip("Enable to emulate TV loss of sync.");
  wid.push_back(myTVJitterWidget);
  myTVJitterRecWidget = new SliderWidget(myTab, font,
                                         myTVJitterWidget->getRight() + fontWidth * 3, ypos - 1,
                                         "Recovery ", 0, kTVJitterChanged);
  myTVJitterRecWidget->setMinValue(1); myTVJitterRecWidget->setMaxValue(20);
  myTVJitterRecWidget->setTickmarkIntervals(5);
  myTVJitterRecWidget->setToolTip("Define speed of sync recovery.");
  wid.push_back(myTVJitterRecWidget);
  myTVJitterRecLabelWidget = new StaticTextWidget(myTab, font,
                                                  myTVJitterRecWidget->getRight() + 4,
                                                  myTVJitterRecWidget->getTop() + 2,
                                                  5 * fontWidth, fontHeight);
  ypos += lineHeight + VGAP;

  myColorLossWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                         "PAL color-loss");
  myColorLossWidget->setToolTip("PAL games with odd scanline count\n"
                                "will be displayed without color.");
  wid.push_back(myColorLossWidget);
  ypos += lineHeight + VGAP;

  // debug colors
  myDebugColorsWidget = new CheckboxWidget(myTab, font, HBORDER + INDENT * 1, ypos + 1,
                                           "Debug colors (*)");
  wid.push_back(myDebugColorsWidget);
  ypos += lineHeight + VGAP + 2;

  items.clear();
  VarList::push_back(items, "Red", "r");
  VarList::push_back(items, "Orange", "o");
  VarList::push_back(items, "Yellow", "y");
  VarList::push_back(items, "Green", "g");
  VarList::push_back(items, "Purple", "p");
  VarList::push_back(items, "Blue", "b");

  static constexpr std::array<int, DEBUG_COLORS> dbg_cmds = {
    kP0ColourChangedCmd,  kM0ColourChangedCmd,  kP1ColourChangedCmd,
    kM1ColourChangedCmd,  kPFColourChangedCmd,  kBLColourChangedCmd
  };

  auto createDebugColourWidgets = [&](int idx, const string& desc)
  {
    int x = HBORDER + INDENT * 1;
    myDbgColour[idx] = new PopUpWidget(myTab, font, x, ypos - 1,
                                       pwidth, lineHeight, items, desc, lwidth, dbg_cmds[idx]);
    wid.push_back(myDbgColour[idx]);
    x += myDbgColour[idx]->getWidth() + fontWidth * 1.25;
    myDbgColourSwatch[idx] = new ColorWidget(myTab, font, x, ypos - 1,
                                             uInt32(2 * lineHeight), lineHeight);
    ypos += lineHeight + VGAP * 1;
  };

  createDebugColourWidgets(0, "Player 0  ");
  createDebugColourWidgets(1, "Missile 0 ");
  createDebugColourWidgets(2, "Player 1  ");
  createDebugColourWidgets(3, "Missile 1 ");
  createDebugColourWidgets(4, "Playfield ");
  createDebugColourWidgets(5, "Ball      ");

  // Add message concerning usage
  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  ypos = myTab->getHeight() - fontHeight - infofont.getFontHeight() - VGAP - VBORDER;
  lwidth = infofont.getStringWidth("(*) Colors identical for player and developer settings");
  new StaticTextWidget(myTab, infofont, HBORDER, ypos,
                       std::min(lwidth, _w - HBORDER * 2), infofont.getFontHeight(),
                       "(*) Colors identical for player and developer settings");

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);

  myTab->parentWidget(tabID)->setHelpAnchor("DeveloperVideo");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addTimeMachineTab(const GUI::Font& font)
{
  const std::array<string, NUM_INTERVALS> INTERVALS = {
    " 1 frame",
    " 3 frames",
    "10 frames",
    "30 frames",
    " 1 second",
    " 3 seconds",
    "10 seconds"
  };
  const std::array<string, NUM_INTERVALS> INT_SETTINGS = {
    "1f",
    "3f",
    "10f",
    "30f",
    "1s",
    "3s",
    "10s"
  };
  const std::array<string, NUM_HORIZONS> HORIZONS = {
    " 3 seconds",
    "10 seconds",
    "30 seconds",
    " 1 minute",
    " 3 minutes",
    "10 minutes",
    "30 minutes",
    "60 minutes"
  };
  const std::array<string, NUM_HORIZONS> HOR_SETTINGS = {
    "3s",
    "10s",
    "30s",
    "1m",
    "3m",
    "10m",
    "30m",
    "60m"
  };
  const int lineHeight = Dialog::lineHeight(),
            fontHeight = Dialog::fontHeight(),
            fontWidth  = Dialog::fontWidth(),
            VBORDER    = Dialog::vBorder(),
            HBORDER    = Dialog::hBorder(),
            VGAP       = Dialog::vGap(),
            INDENT     = Dialog::indent();
  int xpos = HBORDER,
      ypos = VBORDER,
      lwidth = fontWidth * 11;
  WidgetArray wid;
  VariantList items;
  int tabID = myTab->addTab(" Time Machine ", TabWidget::AUTO_WIDTH);

  // settings set
  mySettingsGroupTM = new RadioButtonGroup();
  RadioButtonWidget* r = new RadioButtonWidget(myTab, font, xpos, ypos + 1,
                                               "Player settings", mySettingsGroupTM, kPlrSettings);
  wid.push_back(r);
  ypos += lineHeight + VGAP;
  r = new RadioButtonWidget(myTab, font, xpos, ypos + 1,
                            "Developer settings", mySettingsGroupTM, kDevSettings);
  wid.push_back(r);
  xpos += INDENT;
  ypos += lineHeight + VGAP * 1;

  myTimeMachineWidget = new CheckboxWidget(myTab, font, xpos, ypos + 1,
                                           "Time Machine", kTimeMachine);
  wid.push_back(myTimeMachineWidget);
  xpos += CheckboxWidget::prefixSize(font);
  ypos += lineHeight + VGAP;

  int swidth = fontWidth * 12 + 5; // width of PopUpWidgets below
  myStateSizeWidget = new SliderWidget(myTab, font, xpos,  ypos - 1, swidth, lineHeight,
                                       "Buffer size (*)   ", 0, kSizeChanged, lwidth, " states");
  myStateSizeWidget->setMinValue(20);
#ifdef RETRON77
  myStateSizeWidget->setMaxValue(100);
#else
  myStateSizeWidget->setMaxValue(1000);
#endif
  myStateSizeWidget->setStepValue(20);
  myStateSizeWidget->setTickmarkIntervals(5);
  myStateSizeWidget->setToolTip("Define the total Time Machine buffer size.");
  wid.push_back(myStateSizeWidget);
  ypos += lineHeight + VGAP;

  myUncompressedWidget = new SliderWidget(myTab, font, xpos, ypos - 1, swidth, lineHeight,
                                          "Uncompressed size ", 0, kUncompressedChanged, lwidth, " states");
  myUncompressedWidget->setMinValue(0);
#ifdef RETRON77
  myUncompressedWidget->setMaxValue(100);
#else
  myUncompressedWidget->setMaxValue(1000);
#endif
  myUncompressedWidget->setStepValue(20);
  myUncompressedWidget->setTickmarkIntervals(5);
  myUncompressedWidget->setToolTip("Define the number of completely kept states.\n"
                                   "States beyond this number will be slowly removed\n"
                                   "to fit the requested horizon into the buffer.");
  wid.push_back(myUncompressedWidget);
  ypos += lineHeight + VGAP;

  items.clear();
  for(int i = 0; i < NUM_INTERVALS; ++i)
    VarList::push_back(items, INTERVALS[i], INT_SETTINGS[i]);
  int pwidth = font.getStringWidth("10 seconds");
  myStateIntervalWidget = new PopUpWidget(myTab, font, xpos, ypos, pwidth,
                                          lineHeight, items, "Interval          ", 0, kIntervalChanged);
  myStateIntervalWidget->setToolTip("Define the interval between each saved state.");
  wid.push_back(myStateIntervalWidget);
  ypos += lineHeight + VGAP;

  items.clear();
  for(int i = 0; i < NUM_HORIZONS; ++i)
    VarList::push_back(items, HORIZONS[i], HOR_SETTINGS[i]);
  myStateHorizonWidget = new PopUpWidget(myTab, font, xpos, ypos, pwidth,
                                         lineHeight, items, "Horizon         ~ ", 0, kHorizonChanged);
  myStateHorizonWidget->setToolTip("Define how far the Time Machine\n"
                                   "will allow moving back in time.");
  wid.push_back(myStateHorizonWidget);

  // Add message concerning usage
  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  ypos = myTab->getHeight() - fontHeight - infofont.getFontHeight() - VGAP - VBORDER;
  lwidth = infofont.getStringWidth("(*) Any size change clears the buffer");
  new StaticTextWidget(myTab, infofont, HBORDER, ypos,
                       std::min(lwidth, _w - HBORDER * 2), infofont.getFontHeight(),
                       "(*) Any size change clears the buffer");

  addToFocusList(wid, myTab, tabID);

  myTab->parentWidget(tabID)->setHelpAnchor("DeveloperTimeMachine");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::addDebuggerTab(const GUI::Font& font)
{
  int tabID = myTab->addTab(" Debugger ", TabWidget::AUTO_WIDTH);
  WidgetArray wid;

#ifdef DEBUGGER_SUPPORT
  const int lineHeight = Dialog::lineHeight(),
            fontHeight = Dialog::fontHeight(),
            fontWidth  = Dialog::fontWidth(),
            VBORDER    = Dialog::vBorder(),
            HBORDER    = Dialog::hBorder(),
            VGAP       = Dialog::vGap();
  VariantList items;
  int xpos, ypos, pwidth;
  const Common::Size& ds = instance().frameBuffer().desktopSize(BufferType::Debugger);

  xpos = HBORDER;
  ypos = VBORDER;

  // font size
  items.clear();
  VarList::push_back(items, "Small", "small");
  VarList::push_back(items, "Medium", "medium");
  VarList::push_back(items, "Large", "large");
  pwidth = font.getStringWidth("Medium");
  myDebuggerFontSize =
    new PopUpWidget(myTab, font, HBORDER, ypos + 1, pwidth, lineHeight, items,
                    "Font size (*)  ", 0, kDFontSizeChanged);
  wid.push_back(myDebuggerFontSize);
  ypos += lineHeight + VGAP;

  // Font style (bold label vs. text, etc)
  items.clear();
  VarList::push_back(items, "All normal font", "0");
  VarList::push_back(items, "Bold labels only", "1");
  VarList::push_back(items, "Bold non-labels only", "2");
  VarList::push_back(items, "All bold font", "3");
  pwidth = font.getStringWidth("Bold non-labels only");
  myDebuggerFontStyle =
    new PopUpWidget(myTab, font, HBORDER, ypos + 1, pwidth, lineHeight, items,
                    "Font style (*) ", 0);
  wid.push_back(myDebuggerFontStyle);

  ypos += lineHeight + VGAP * 4;

  // Debugger width and height
  myDebuggerWidthSlider = new SliderWidget(myTab, font, xpos, ypos-1, "Debugger width (*)  ",
                                           0, 0, 6 * fontWidth, "px");
  myDebuggerWidthSlider->setMinValue(DebuggerDialog::kSmallFontMinW);
  myDebuggerWidthSlider->setMaxValue(ds.w);
  myDebuggerWidthSlider->setStepValue(10);
  // one tickmark every ~100 pixel
  myDebuggerWidthSlider->setTickmarkIntervals((ds.w - DebuggerDialog::kSmallFontMinW + 50) / 100);
  wid.push_back(myDebuggerWidthSlider);
  ypos += lineHeight + VGAP;

  myDebuggerHeightSlider = new SliderWidget(myTab, font, xpos, ypos-1, "Debugger height (*) ",
                                            0, 0, 6 * fontWidth, "px");
  myDebuggerHeightSlider->setMinValue(DebuggerDialog::kSmallFontMinH);
  myDebuggerHeightSlider->setMaxValue(ds.h);
  myDebuggerHeightSlider->setStepValue(10);
  // one tickmark every ~100 pixel
  myDebuggerHeightSlider->setTickmarkIntervals((ds.h - DebuggerDialog::kSmallFontMinH + 50) / 100);
  wid.push_back(myDebuggerHeightSlider);
  ypos += lineHeight + VGAP * 4;

  myGhostReadsTrapWidget = new CheckboxWidget(myTab, font, HBORDER, ypos + 1,
                                             "Trap on 'ghost' reads");
  myGhostReadsTrapWidget->setToolTip("Traps will consider CPU 'ghost' reads too.");
  wid.push_back(myGhostReadsTrapWidget);

  // Add message concerning usage
  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  ypos = myTab->getHeight() - fontHeight - infofont.getFontHeight() - VGAP - VBORDER;
  new StaticTextWidget(myTab, infofont, HBORDER, ypos, "(*) Change requires a ROM reload");

#if defined(DEBUGGER_SUPPORT) && defined(WINDOWED_SUPPORT)
  // Debugger is only realistically available in windowed modes 800x600 or greater
  // (and when it's actually been compiled into the app)
  if(ds.w < 800 || ds.h < 600)  // TODO - maybe this logic can disappear?
  {
    myDebuggerWidthSlider->clearFlags(Widget::FLAG_ENABLED);
    myDebuggerHeightSlider->clearFlags(Widget::FLAG_ENABLED);
  }
#endif
#else
  new StaticTextWidget(myTab, font, 0, 20, _w - 20, font.getFontHeight(),
                       "Debugger support not included", TextAlign::Center);
#endif

  addToFocusList(wid, myTab, tabID);

  myTab->parentWidget(tabID)->setHelpAnchor("DeveloperDebugger");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::loadSettings(SettingsSet set)
{
  bool devSettings = set == SettingsSet::developer;
  const string& prefix = devSettings ? "dev." : "plr.";

  myFrameStats[set] = instance().settings().getBool(prefix + "stats");
  myDetectedInfo[set] = instance().settings().getBool(prefix + "detectedinfo");
  myConsole[set] = instance().settings().getString(prefix + "console") == "7800" ? 1 : 0;
  // Randomization
  myRandomBank[set] = instance().settings().getBool(prefix + "bankrandom");
  myRandomizeRAM[set] = instance().settings().getBool(prefix + "ramrandom");
  myRandomizeCPU[set] = instance().settings().getString(prefix + "cpurandom");
  // Undriven TIA pins
  myUndrivenPins[set] = devSettings ? instance().settings().getBool("dev.tiadriven") : false;
#ifdef DEBUGGER_SUPPORT
  // Read from write ports break
  myRWPortBreak[set] = devSettings ? instance().settings().getBool("dev.rwportbreak") : false;
  // Write to read ports break
  myWRPortBreak[set] = devSettings ? instance().settings().getBool("dev.wrportbreak") : false;
#endif
  // Thumb ARM emulation exception
  myThumbException[set] = devSettings ? instance().settings().getBool("dev.thumb.trapfatal") : false;
  // AtariVox/SaveKey EEPROM access
  myEEPROMAccess[set] = instance().settings().getBool(prefix + "eepromaccess");

  // TIA tab
  myTIAType[set] = devSettings ? instance().settings().getString("dev.tia.type") : "standard";
  myPlInvPhase[set] = devSettings ? instance().settings().getBool("dev.tia.plinvphase") : false;
  myMsInvPhase[set] = devSettings ? instance().settings().getBool("dev.tia.msinvphase") : false;
  myBlInvPhase[set] = devSettings ? instance().settings().getBool("dev.tia.blinvphase") : false;
  myPFBits[set] = devSettings ? instance().settings().getBool("dev.tia.delaypfbits") : false;
  myPFColor[set] = devSettings ? instance().settings().getBool("dev.tia.delaypfcolor") : false;
  myBKColor[set] = devSettings ? instance().settings().getBool("dev.tia.delaybkcolor") : false;
  myPlSwap[set] = devSettings ? instance().settings().getBool("dev.tia.delayplswap") : false;
  myBlSwap[set] = devSettings ? instance().settings().getBool("dev.tia.delayblswap") : false;

  // Debug colors
  myDebugColors[set] = instance().settings().getBool(prefix + "debugcolors");
  // PAL color-loss effect
  myColorLoss[set] = instance().settings().getBool(prefix + "colorloss");
  // Jitter
  myTVJitter[set] = instance().settings().getBool(prefix + "tv.jitter");
  myTVJitterRec[set] = instance().settings().getInt(prefix + "tv.jitter_recovery");

  // States
  myTimeMachine[set] = instance().settings().getBool(prefix + "timemachine");
  myStateSize[set] = instance().settings().getInt(prefix + "tm.size");
  myUncompressed[set] = instance().settings().getInt(prefix + "tm.uncompressed");
  myStateInterval[set] = instance().settings().getString(prefix + "tm.interval");
  myStateHorizon[set] = instance().settings().getString(prefix + "tm.horizon");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::saveSettings(SettingsSet set)
{
  bool devSettings = set == SettingsSet::developer;
  const string& prefix = devSettings ? "dev." : "plr.";

  instance().settings().setValue(prefix + "stats", myFrameStats[set]);
  instance().settings().setValue(prefix + "detectedinfo", myDetectedInfo[set]);
  instance().settings().setValue(prefix + "console", myConsole[set] == 1 ? "7800" : "2600");
  if(instance().hasConsole())
    instance().eventHandler().set7800Mode();

  // Randomization
  instance().settings().setValue(prefix + "bankrandom", myRandomBank[set]);
  instance().settings().setValue(prefix + "ramrandom", myRandomizeRAM[set]);
  instance().settings().setValue(prefix + "cpurandom", myRandomizeCPU[set]);

  if(devSettings)
  {
    // Undriven TIA pins
    instance().settings().setValue("dev.tiadriven", myUndrivenPins[set]);
  #ifdef DEBUGGER_SUPPORT
    // Read from write ports break
    instance().settings().setValue("dev.rwportbreak", myRWPortBreak[set]);
    // Write to read ports break
    instance().settings().setValue("dev.wrportbreak", myWRPortBreak[set]);
  #endif
    // Thumb ARM emulation exception
    instance().settings().setValue("dev.thumb.trapfatal", myThumbException[set]);
  }

  // AtariVox/SaveKey EEPROM access
  instance().settings().setValue(prefix + "eepromaccess", myEEPROMAccess[set]);

  // TIA tab
  if (devSettings)
  {
    instance().settings().setValue("dev.tia.type", myTIAType[set]);
    if (BSPF::equalsIgnoreCase("custom", myTIAType[set]))
    {
      instance().settings().setValue("dev.tia.plinvphase", myPlInvPhase[set]);
      instance().settings().setValue("dev.tia.msinvphase", myMsInvPhase[set]);
      instance().settings().setValue("dev.tia.blinvphase", myBlInvPhase[set]);
      instance().settings().setValue("dev.tia.delaypfbits", myPFBits[set]);
      instance().settings().setValue("dev.tia.delaypfcolor", myPFColor[set]);
      instance().settings().setValue("dev.tia.delaybkcolor", myBKColor[set]);
      instance().settings().setValue("dev.tia.delayplswap", myPlSwap[set]);
      instance().settings().setValue("dev.tia.delayblswap", myBlSwap[set]);
    }
  }

  // Debug colors
  instance().settings().setValue(prefix + "debugcolors", myDebugColors[set]);
  // PAL color loss
  instance().settings().setValue(prefix + "colorloss", myColorLoss[set]);
  // Jitter
  instance().settings().setValue(prefix + "tv.jitter", myTVJitter[set]);
  instance().settings().setValue(prefix + "tv.jitter_recovery", myTVJitterRec[set]);

  // States
  instance().settings().setValue(prefix + "timemachine", myTimeMachine[set]);
  instance().settings().setValue(prefix + "tm.size", myStateSize[set]);
  instance().settings().setValue(prefix + "tm.uncompressed", myUncompressed[set]);
  instance().settings().setValue(prefix + "tm.interval", myStateInterval[set]);
  instance().settings().setValue(prefix + "tm.horizon", myStateHorizon[set]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::getWidgetStates(SettingsSet set)
{
  myFrameStats[set] = myFrameStatsWidget->getState();
  myDetectedInfo[set] = myDetectedInfoWidget->getState();
  myConsole[set] = myConsoleWidget->getSelected() == 1;
  // Randomization
  myRandomBank[set] = myRandomBankWidget->getState();
  myRandomizeRAM[set] = myRandomizeRAMWidget->getState();
  string cpurandom;
  const std::array<string, 5> cpuregs = {"S", "A", "X", "Y", "P"};

  for(int i = 0; i < 5; ++i)
    if(myRandomizeCPUWidget[i]->getState())
      cpurandom += cpuregs[i];
  myRandomizeCPU[set] = cpurandom;
  // Undriven TIA pins
  myUndrivenPins[set] = myUndrivenPinsWidget->getState();
#ifdef DEBUGGER_SUPPORT
  // Read from write ports break
  myRWPortBreak[set] = myRWPortBreakWidget->getState();
  myWRPortBreak[set] = myWRPortBreakWidget->getState();
#endif
  // Thumb ARM emulation exception
  myThumbException[set] = myThumbExceptionWidget->getState();
  // AtariVox/SaveKey EEPROM access
  myEEPROMAccess[set] = myEEPROMAccessWidget->getState();

  // TIA tab
  myTIAType[set] = myTIATypeWidget->getSelectedTag().toString();
  myPlInvPhase[set] = myPlInvPhaseWidget->getState();
  myMsInvPhase[set] = myMsInvPhaseWidget->getState();
  myBlInvPhase[set] = myBlInvPhaseWidget->getState();
  myPFBits[set] = myPFBitsWidget->getState();
  myPFColor[set] = myPFColorWidget->getState();
  myBKColor[set] = myBKColorWidget->getState();
  myPlSwap[set] = myPlSwapWidget->getState();
  myBlSwap[set] = myBlSwapWidget->getState();

  // Debug colors
  myDebugColors[set] = myDebugColorsWidget->getState();
  // PAL color-loss effect
  myColorLoss[set] = myColorLossWidget->getState();
  // Jitter
  myTVJitter[set] = myTVJitterWidget->getState();
  myTVJitterRec[set] = myTVJitterRecWidget->getValue();

  // States
  myTimeMachine[set] = myTimeMachineWidget->getState();
  myStateSize[set] = myStateSizeWidget->getValue();
  myUncompressed[set] = myUncompressedWidget->getValue();
  myStateInterval[set] = myStateIntervalWidget->getSelectedTag().toString();
  myStateHorizon[set] = myStateHorizonWidget->getSelectedTag().toString();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::setWidgetStates(SettingsSet set)
{
  myFrameStatsWidget->setState(myFrameStats[set]);
  myDetectedInfoWidget->setState(myDetectedInfo[set]);
  myConsoleWidget->setSelectedIndex(myConsole[set]);
  // Randomization
  myRandomBankWidget->setState(myRandomBank[set]);
  myRandomizeRAMWidget->setState(myRandomizeRAM[set]);

  const string& cpurandom = myRandomizeCPU[set];
  const std::array<string, 5> cpuregs = {"S", "A", "X", "Y", "P"};

  for(int i = 0; i < 5; ++i)
    myRandomizeCPUWidget[i]->setState(BSPF::containsIgnoreCase(cpurandom, cpuregs[i]));
  // Undriven TIA pins
  myUndrivenPinsWidget->setState(myUndrivenPins[set]);
#ifdef DEBUGGER_SUPPORT
  // Read from write ports break
  myRWPortBreakWidget->setState(myRWPortBreak[set]);
  myWRPortBreakWidget->setState(myWRPortBreak[set]);
#endif
  // Thumb ARM emulation exception
  myThumbExceptionWidget->setState(myThumbException[set]);
  // AtariVox/SaveKey EEPROM access
  myEEPROMAccessWidget->setState(myEEPROMAccess[set]);
  handleConsole();

  // TIA tab
  myTIATypeWidget->setSelected(myTIAType[set], "standard");
  handleTia();

  // Debug colors
  myDebugColorsWidget->setState(myDebugColors[set]);
  // PAL color-loss effect
  myColorLossWidget->setState(myColorLoss[set]);
  // Jitter
  myTVJitterWidget->setState(myTVJitter[set]);
  myTVJitterRecWidget->setValue(myTVJitterRec[set]);

  handleTVJitterChange(myTVJitterWidget->getState());

  // States
  myTimeMachineWidget->setState(myTimeMachine[set]);
  myStateSizeWidget->setValue(myStateSize[set]);
  myUncompressedWidget->setValue(myUncompressed[set]);
  myStateIntervalWidget->setSelected(myStateInterval[set]);
  myStateHorizonWidget->setSelected(myStateHorizon[set]);

  handleTimeMachine();
  handleSize();
  handleUncompressed();
  handleInterval();
  handleHorizon();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::loadConfig()
{
  bool devSettings = instance().settings().getBool("dev.settings");
  handleSettings(devSettings);
  mySettings = devSettings;
  mySettingsGroupEmulation->setSelected(devSettings ? 1 : 0);
  mySettingsGroupTia->setSelected(devSettings ? 1 : 0);
  mySettingsGroupVideo->setSelected(devSettings ? 1 : 0);
  mySettingsGroupTM->setSelected(devSettings ? 1 : 0);

  // load both setting sets...
  loadSettings(SettingsSet::player);
  loadSettings(SettingsSet::developer);
  // ...and select the current one
  setWidgetStates(SettingsSet(mySettingsGroupEmulation->getSelected()));

  // Debug colours
  handleDebugColours(instance().settings().getString("tia.dbgcolors"));

#ifdef DEBUGGER_SUPPORT
  uInt32 w, h;

  // Debugger size
  const Common::Size& ds = instance().settings().getSize("dbg.res");
  w = ds.w; h = ds.h;

  myDebuggerWidthSlider->setValue(w);
  myDebuggerHeightSlider->setValue(h);

  // Debugger font size
  string size = instance().settings().getString("dbg.fontsize");
  myDebuggerFontSize->setSelected(size, "medium");

  // Debugger font style
  int style = instance().settings().getInt("dbg.fontstyle");
  myDebuggerFontStyle->setSelected(style, "0");

  // Ghost reads trap
  myGhostReadsTrapWidget->setState(instance().settings().getBool("dbg.ghostreadstrap"));

  handleFontSize();
#endif

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::saveConfig()
{
  instance().settings().setValue("dev.settings", mySettingsGroupEmulation->getSelected() == SettingsSet::developer);
  // copy current widget status into set...
  getWidgetStates(SettingsSet(mySettingsGroupEmulation->getSelected()));
  // ...and save both sets
  saveSettings(SettingsSet::player);
  saveSettings(SettingsSet::developer);

  // activate the current settings
  instance().frameBuffer().showFrameStats(myFrameStatsWidget->getState());
  // jitter
  if(instance().hasConsole())
  {
    instance().console().tia().toggleJitter(myTVJitterWidget->getState() ? 1 : 0);
    instance().console().tia().setJitterRecoveryFactor(myTVJitterRecWidget->getValue());
  }

  // TIA tab
  if(instance().hasConsole())
  {
    instance().console().tia().setPlInvertedPhaseClock(myPlInvPhaseWidget->getState());
    instance().console().tia().setMsInvertedPhaseClock(myMsInvPhaseWidget->getState());
    instance().console().tia().setBlInvertedPhaseClock(myBlInvPhaseWidget->getState());
    instance().console().tia().setPFBitsDelay(myPFBitsWidget->getState());
    instance().console().tia().setPFColorDelay(myPFColorWidget->getState());
    instance().console().tia().setBKColorDelay(myBKColorWidget->getState());
    instance().console().tia().setPlSwapDelay(myPlSwapWidget->getState());
    instance().console().tia().setBlSwapDelay(myBlSwapWidget->getState());
  }

  handleEnableDebugColors();
  // PAL color loss
  if(instance().hasConsole())
    instance().console().enableColorLoss(myColorLossWidget->getState());

  // Debug colours
  string dbgcolors;
  for(int i = 0; i < DEBUG_COLORS; ++i)
    dbgcolors += myDbgColour[i]->getSelectedTag().toString();
  if(instance().hasConsole() &&
     instance().console().tia().setFixedColorPalette(dbgcolors))
    instance().settings().setValue("tia.dbgcolors", dbgcolors);

  // update RewindManager
  instance().state().rewindManager().setup();
  instance().state().setRewindMode(myTimeMachineWidget->getState() ?
                                   StateManager::Mode::TimeMachine : StateManager::Mode::Off);

#ifdef DEBUGGER_SUPPORT
  // Debugger font style
  instance().settings().setValue("dbg.fontstyle",
                                 myDebuggerFontStyle->getSelectedTag().toString());
  // Debugger size
  instance().settings().setValue("dbg.res",
                                 Common::Size(myDebuggerWidthSlider->getValue(),
                                 myDebuggerHeightSlider->getValue()));
  // Debugger font size
  instance().settings().setValue("dbg.fontsize", myDebuggerFontSize->getSelectedTag().toString());

  // Ghost reads trap
  instance().settings().setValue("dbg.ghostreadstrap", myGhostReadsTrapWidget->getState());
  if(instance().hasConsole())
    instance().console().system().m6502().setGhostReadsTrap(myGhostReadsTrapWidget->getState());

  // Read from write ports and write to read ports breaks
  if (instance().hasConsole())
  {
    instance().console().system().m6502().setReadFromWritePortBreak(myRWPortBreakWidget->getState());
    instance().console().system().m6502().setWriteToReadPortBreak(myWRPortBreakWidget->getState());
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::setDefaults()
{
  bool devSettings = mySettings;
  SettingsSet set = SettingsSet(mySettingsGroupEmulation->getSelected());

  switch(myTab->getActiveTab())
  {
    case 0: // Emulation
      myFrameStats[set] = devSettings ? true : false;
      myDetectedInfo[set] = devSettings ? true : false;
      myConsole[set] = 0;
      // Randomization
      myRandomBank[set] = devSettings ? true : false;
      myRandomizeRAM[set] = true;
      myRandomizeCPU[set] = devSettings ? "SAXYP" : "AXYP";
      // Undriven TIA pins
      myUndrivenPins[set] = devSettings ? true : false;
    #ifdef DEBUGGER_SUPPORT
      // Reads from write ports
      myRWPortBreak[set] = devSettings ? true : false;
      myWRPortBreak[set] = devSettings ? true : false;
    #endif
      // Thumb ARM emulation exception
      myThumbException[set] = devSettings ? true : false;
      // AtariVox/SaveKey EEPROM access
      myEEPROMAccess[set] = devSettings ? true : false;

      setWidgetStates(set);
      break;

    case 1: // TIA
      myTIAType[set] = "standard";
      // reset "custom" mode
      myPlInvPhase[set] = devSettings ? true : false;
      myMsInvPhase[set] = devSettings ? true : false;
      myBlInvPhase[set] = devSettings ? true : false;
      myPFBits[set] = devSettings ? true : false;
      myPFColor[set] = devSettings ? true : false;
      myBKColor[set] = devSettings ? true : false;
      myPlSwap[set] = devSettings ? true : false;
      myBlSwap[set] = devSettings ? true : false;

      setWidgetStates(set);
      break;

    case 2: // Video
      // Jitter
      myTVJitter[set] = true;
      myTVJitterRec[set] = devSettings ? 2 : 10;
      // PAL color-loss effect
      myColorLoss[set] = devSettings ? true : false;
      // Debug colors
      myDebugColors[set] = false;
      handleDebugColours("roygpb");

      setWidgetStates(set);
      break;

    case 3: // States
      myTimeMachine[set] = true;
      myStateSize[set] = devSettings ? 1000 : 200;
      myUncompressed[set] = devSettings ? 600 : 60;
      myStateInterval[set] = devSettings ? "1f" : "30f";
      myStateHorizon[set] = devSettings ? "30s" : "10m";

      setWidgetStates(set);
      break;

    case 4: // Debugger options
    {
#ifdef DEBUGGER_SUPPORT
      const Common::Size& size = instance().frameBuffer().desktopSize(BufferType::Debugger);

      uInt32 w = std::min(size.w, uInt32(DebuggerDialog::kMediumFontMinW));
      uInt32 h = std::min(size.h, uInt32(DebuggerDialog::kMediumFontMinH));
      myDebuggerWidthSlider->setValue(w);
      myDebuggerHeightSlider->setValue(h);
      myDebuggerFontSize->setSelected("medium");
      myDebuggerFontStyle->setSelected("0");

      myGhostReadsTrapWidget->setState(true);

      handleFontSize();
#endif
      break;
    }

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kPlrSettings:
      handleSettings(false);
      break;

    case kDevSettings:
      handleSettings(true);
      break;

    case kTIAType:
      handleTia();
      break;

    case kConsole:
      handleConsole();
        break;

    case kTVJitter:
      handleTVJitterChange(myTVJitterWidget->getState());
      break;

    case kTVJitterChanged:
      myTVJitterRecLabelWidget->setValue(myTVJitterRecWidget->getValue());
      break;

    case kPPinCmd:
      instance().console().tia().driveUnusedPinsRandom(myUndrivenPinsWidget->getState());
      break;

    case kTimeMachine:
      handleTimeMachine();
      break;

    case kSizeChanged:
      handleSize();
      break;

    case kUncompressedChanged:
      handleUncompressed();
      break;

    case kIntervalChanged:
      handleInterval();
      break;

    case kHorizonChanged:
      handleHorizon();
      break;

    case kP0ColourChangedCmd:
      handleDebugColours(0, myDbgColour[0]->getSelected());
      break;

    case kM0ColourChangedCmd:
      handleDebugColours(1, myDbgColour[1]->getSelected());
      break;

    case kP1ColourChangedCmd:
      handleDebugColours(2, myDbgColour[2]->getSelected());
      break;

    case kM1ColourChangedCmd:
      handleDebugColours(3, myDbgColour[3]->getSelected());
      break;

    case kPFColourChangedCmd:
      handleDebugColours(4, myDbgColour[4]->getSelected());
      break;

    case kBLColourChangedCmd:
      handleDebugColours(5, myDbgColour[5]->getSelected());
      break;

#ifdef DEBUGGER_SUPPORT
    case kDFontSizeChanged:
      handleFontSize();
      break;
#endif

    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kCloseCmd:
      // Revert changes made to event mapping
      close();
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleSettings(bool devSettings)
{
  myUndrivenPinsWidget->setEnabled(devSettings);
#ifdef DEBUGGER_SUPPORT
  myRWPortBreakWidget->setEnabled(devSettings);
  myWRPortBreakWidget->setEnabled(devSettings);
#endif
  myThumbExceptionWidget->setEnabled(devSettings);

  if (mySettings != devSettings)
  {
    mySettings = devSettings; // block redundant events first!
    SettingsSet set = devSettings ? SettingsSet::developer : SettingsSet::player;
    mySettingsGroupEmulation->setSelected(set);
    mySettingsGroupTia->setSelected(set);
    mySettingsGroupVideo->setSelected(set);
    mySettingsGroupTM->setSelected(set);
    getWidgetStates(devSettings ? SettingsSet::player : SettingsSet::developer);
    setWidgetStates(set);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleTVJitterChange(bool enable)
{
  myTVJitterRecWidget->setEnabled(enable);
  myTVJitterRecLabelWidget->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleEnableDebugColors()
{
  if(instance().hasConsole())
  {
    bool fixed = instance().console().tia().usingFixedColors();
    if(fixed != myDebugColorsWidget->getState())
      instance().console().tia().toggleFixedColors();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleConsole()
{
  bool is7800 = myConsoleWidget->getSelected() == 1;

  myRandomizeRAMWidget->setEnabled(!is7800);
  if(is7800)
    myRandomizeRAMWidget->setState(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleTia()
{
  bool enable = BSPF::equalsIgnoreCase("custom", myTIATypeWidget->getSelectedTag().toString());

  myTIATypeWidget->setEnabled(mySettings);
  myInvPhaseLabel->setEnabled(enable);
  myPlInvPhaseWidget->setEnabled(enable);
  myMsInvPhaseWidget->setEnabled(enable);
  myBlInvPhaseWidget->setEnabled(enable);
  myPlayfieldLabel->setEnabled(enable);
  myBackgroundLabel->setEnabled(enable);
  myPFBitsWidget->setEnabled(enable);
  myPFColorWidget->setEnabled(enable);
  myBKColorWidget->setEnabled(enable);
  mySwapLabel->setEnabled(enable);
  myPlSwapWidget->setEnabled(enable);
  myBlSwapWidget->setEnabled(enable);

  if(BSPF::equalsIgnoreCase("custom", myTIATypeWidget->getSelectedTag().toString()))
  {
    myPlInvPhaseWidget->setState(myPlInvPhase[SettingsSet::developer]);
    myMsInvPhaseWidget->setState(myMsInvPhase[SettingsSet::developer]);
    myBlInvPhaseWidget->setState(myBlInvPhase[SettingsSet::developer]);
    myPFBitsWidget->setState(myPFBits[SettingsSet::developer]);
    myPFColorWidget->setState(myPFColor[SettingsSet::developer]);
    myBKColorWidget->setState(myBKColor[SettingsSet::developer]);
    myPlSwapWidget->setState(myPlSwap[SettingsSet::developer]);
    myBlSwapWidget->setState(myBlSwap[SettingsSet::developer]);
  }
  else
  {
    myPlInvPhaseWidget->setState(BSPF::equalsIgnoreCase("koolaidman", myTIATypeWidget->getSelectedTag().toString()));
    myMsInvPhaseWidget->setState(BSPF::equalsIgnoreCase("cosmicark", myTIATypeWidget->getSelectedTag().toString()));
    myBlInvPhaseWidget->setState(false);
    myPFBitsWidget->setState(BSPF::equalsIgnoreCase("pesco", myTIATypeWidget->getSelectedTag().toString()));
    myPFColorWidget->setState(BSPF::equalsIgnoreCase("quickstep", myTIATypeWidget->getSelectedTag().toString()));
    myBKColorWidget->setState(BSPF::equalsIgnoreCase("indy500", myTIATypeWidget->getSelectedTag().toString()));
    myPlSwapWidget->setState(BSPF::equalsIgnoreCase("heman", myTIATypeWidget->getSelectedTag().toString()));
    myBlSwapWidget->setState(false);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleTimeMachine()
{
  bool enable = myTimeMachineWidget->getState();

  myStateSizeWidget->setEnabled(enable);
  myUncompressedWidget->setEnabled(enable);
  myStateIntervalWidget->setEnabled(enable);

  uInt32 size = myStateSizeWidget->getValue();
  uInt32 uncompressed = myUncompressedWidget->getValue();

  myStateHorizonWidget->setEnabled(enable && size > uncompressed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleSize()
{
  uInt32 size = myStateSizeWidget->getValue();
  uInt32 uncompressed = myUncompressedWidget->getValue();
  Int32 interval = myStateIntervalWidget->getSelected();
  Int32 horizon = myStateHorizonWidget->getSelected();
  bool found = false;
  Int32 i;

  // handle illegal values
  if(interval == -1)
    interval = 0;
  if(horizon == -1)
    horizon = 0;

  // adapt horizon and interval
  do
  {
    for(i = horizon; i < NUM_HORIZONS; ++i)
    {
      if(uInt64(size) * instance().state().rewindManager().INTERVAL_CYCLES[interval]
         <= instance().state().rewindManager().HORIZON_CYCLES[i])
      {
        found = true;
        break;
      }
    }
    if(!found)
      --interval;
  } while(!found);

  if(size < uncompressed)
    myUncompressedWidget->setValue(size);
  myStateIntervalWidget->setSelectedIndex(interval);
  myStateHorizonWidget->setSelectedIndex(i);
  myStateHorizonWidget->setEnabled(myTimeMachineWidget->getState() && size > uncompressed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleUncompressed()
{
  uInt32 size = myStateSizeWidget->getValue();
  uInt32 uncompressed = myUncompressedWidget->getValue();

  if(size < uncompressed)
    myStateSizeWidget->setValue(uncompressed);
  myStateHorizonWidget->setEnabled(myTimeMachineWidget->getState() && size > uncompressed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleInterval()
{
  uInt32 size = myStateSizeWidget->getValue();
  uInt32 uncompressed = myUncompressedWidget->getValue();
  Int32 interval = myStateIntervalWidget->getSelected();
  Int32 horizon = myStateHorizonWidget->getSelected();
  bool found = false;
  Int32 i;

  // handle illegal values
  if(interval == -1)
    interval = 0;
  if(horizon == -1)
    horizon = 0;

  // adapt horizon and size
  do
  {
    for(i = horizon; i < NUM_HORIZONS; ++i)
    {
      if(uInt64(size) * instance().state().rewindManager().INTERVAL_CYCLES[interval]
         <= instance().state().rewindManager().HORIZON_CYCLES[i])
      {
        found = true;
        break;
      }
    }
    if(!found)
      size -= myStateSizeWidget->getStepValue();
  } while(!found);

  myStateHorizonWidget->setSelectedIndex(i);
  myStateSizeWidget->setValue(size);
  if(size < uncompressed)
    myUncompressedWidget->setValue(size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleHorizon()
{
  uInt32 size = myStateSizeWidget->getValue();
  uInt32 uncompressed = myUncompressedWidget->getValue();
  Int32 interval = myStateIntervalWidget->getSelected();
  Int32 horizon = myStateHorizonWidget->getSelected();
  bool found = false;
  Int32 i;

  // handle illegal values
  if(interval == -1)
    interval = 0;
  if(horizon == -1)
    horizon = 0;

  // adapt interval and size
  do
  {
    for(i = interval; i >= 0; --i)
    {
      if(uInt64(size) * instance().state().rewindManager().INTERVAL_CYCLES[i]
         <= instance().state().rewindManager().HORIZON_CYCLES[horizon])
      {
        found = true;
        break;
      }
    }
    if(!found)
      size -= myStateSizeWidget->getStepValue();
  } while(!found);

  myStateIntervalWidget->setSelectedIndex(i);
  myStateSizeWidget->setValue(size);
  if(size < uncompressed)
    myUncompressedWidget->setValue(size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleDebugColours(int idx, int color)
{
  if(idx < 0 || idx >= DEBUG_COLORS)
    return;

  if(!instance().hasConsole())
  {
    myDbgColour[idx]->clearFlags(Widget::FLAG_ENABLED);
    myDbgColourSwatch[idx]->clearFlags(Widget::FLAG_ENABLED);
    return;
  }

  static constexpr BSPF::array2D<ColorId, 3, DEBUG_COLORS> dbg_color = {{
    {
      TIA::FixedColor::NTSC_RED,
      TIA::FixedColor::NTSC_ORANGE,
      TIA::FixedColor::NTSC_YELLOW,
      TIA::FixedColor::NTSC_GREEN,
      TIA::FixedColor::NTSC_PURPLE,
      TIA::FixedColor::NTSC_BLUE
    },
    {
      TIA::FixedColor::PAL_RED,
      TIA::FixedColor::PAL_ORANGE,
      TIA::FixedColor::PAL_YELLOW,
      TIA::FixedColor::PAL_GREEN,
      TIA::FixedColor::PAL_PURPLE,
      TIA::FixedColor::PAL_BLUE
    },
    {
      TIA::FixedColor::SECAM_RED,
      TIA::FixedColor::SECAM_ORANGE,
      TIA::FixedColor::SECAM_YELLOW,
      TIA::FixedColor::SECAM_GREEN,
      TIA::FixedColor::SECAM_PURPLE,
      TIA::FixedColor::SECAM_BLUE
    }
  }};

  int timing = instance().console().timing() == ConsoleTiming::ntsc ? 0
    : instance().console().timing() == ConsoleTiming::pal ? 1 : 2;

  myDbgColourSwatch[idx]->setColor(dbg_color[timing][color]);
  myDbgColour[idx]->setSelectedIndex(color);

  // make sure the selected debug colors are all different
  std::array<bool, DEBUG_COLORS> usedCol;

  // identify used colors
  for(int i = 0; i < DEBUG_COLORS; ++i)
  {
    usedCol[i] = false;
    for(int j = 0; j < DEBUG_COLORS; ++j)
    {
      if(myDbgColourSwatch[j]->getColor() == dbg_color[timing][i])
      {
        usedCol[i] = true;
        break;
      }
    }
  }
  // check if currently changed color was used somewhere else
  for(int i = 0; i < DEBUG_COLORS; ++i)
  {
    if (i != idx && myDbgColourSwatch[i]->getColor() == dbg_color[timing][color])
    {
      // if already used, change the other color to an unused one
      for(int j = 0; j < DEBUG_COLORS; ++j)
      {
        if(!usedCol[j])
        {
          myDbgColourSwatch[i]->setColor(dbg_color[timing][j]);
          myDbgColour[i]->setSelectedIndex(j);
          break;
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleDebugColours(const string& colors)
{
  for(int i = 0; i < DEBUG_COLORS; ++i)
  {
    switch(colors[i])
    {
      case 'r': handleDebugColours(i, 0); break;
      case 'o': handleDebugColours(i, 1); break;
      case 'y': handleDebugColours(i, 2); break;
      case 'g': handleDebugColours(i, 3); break;
      case 'p': handleDebugColours(i, 4); break;
      case 'b': handleDebugColours(i, 5); break;
      default:                            break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DeveloperDialog::handleFontSize()
{
#ifdef DEBUGGER_SUPPORT
  uInt32 minW, minH;
  int fontSize = myDebuggerFontSize->getSelected();

  if(fontSize == 0)
  {
    minW = DebuggerDialog::kSmallFontMinW;
    minH = DebuggerDialog::kSmallFontMinH;
  }
  else if(fontSize == 1)
  {
    minW = DebuggerDialog::kMediumFontMinW;
    minH = DebuggerDialog::kMediumFontMinH;
  }
  else // large
  {
    minW = DebuggerDialog::kLargeFontMinW;
    minH = DebuggerDialog::kLargeFontMinH;
  }
  const Common::Size& size = instance().frameBuffer().desktopSize(BufferType::Debugger);
  minW = std::min(size.w, minW);
  minH = std::min(size.h, minH);

  myDebuggerWidthSlider->setMinValue(minW);
  if(minW > uInt32(myDebuggerWidthSlider->getValue()))
    myDebuggerWidthSlider->setValue(minW);

  myDebuggerHeightSlider->setMinValue(minH);
  if(minH > uInt32(myDebuggerHeightSlider->getValue()))
    myDebuggerHeightSlider->setValue(minH);
#endif
}
