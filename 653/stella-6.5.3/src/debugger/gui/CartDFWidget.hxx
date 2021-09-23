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

#ifndef CARTRIDGEDF_WIDGET_HXX
#define CARTRIDGEDF_WIDGET_HXX

class CartridgeDF;

#include "CartEnhancedWidget.hxx"

class CartridgeDFWidget : public CartridgeEnhancedWidget
{
  public:
    CartridgeDFWidget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      int x, int y, int w, int h,
                      CartridgeDF& cart);
    ~CartridgeDFWidget() override = default;

  private:
    string manufacturer() override { return "CPUWIZ"; }

    string description() override;

  private:
    // Following constructors and assignment operators not supported
    CartridgeDFWidget() = delete;
    CartridgeDFWidget(const CartridgeDFWidget&) = delete;
    CartridgeDFWidget(CartridgeDFWidget&&) = delete;
    CartridgeDFWidget& operator=(const CartridgeDFWidget&) = delete;
    CartridgeDFWidget& operator=(CartridgeDFWidget&&) = delete;
};

#endif
