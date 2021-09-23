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

#ifndef BOOSTERGRIP_HXX
#define BOOSTERGRIP_HXX

#include "Joystick.hxx"

/**
  The standard Atari 2600 joystick controller fitted with the
  CBS Booster grip.  The Booster grip has two more fire buttons
  on it (a booster and a trigger).

  @author  Bradford W. Mott
*/
class BoosterGrip : public Joystick
{
  public:
    /**
      Create a new booster grip joystick plugged into the specified jack

      @param jack   The jack the controller is plugged into
      @param event  The event object to use for events
      @param system The system using this controller
    */
    BoosterGrip(Jack jack, const Event& event, const System& system);
    ~BoosterGrip() override = default;

  public:
    /**
      Returns the name of this controller.
    */
    string name() const override { return "BoosterGrip"; }

  private:
    /**
      Update the button pin states.
    */
    void updateButtons() override;

  private:
    // Pre-compute the events we care about based on given port
    // This will eliminate test for left or right port in update()
    Event::Type myBoosterEvent, myTriggerEvent;

  private:
    // Following constructors and assignment operators not supported
    BoosterGrip() = delete;
    BoosterGrip(const BoosterGrip&) = delete;
    BoosterGrip(BoosterGrip&&) = delete;
    BoosterGrip& operator=(const BoosterGrip&) = delete;
    BoosterGrip& operator=(BoosterGrip&&) = delete;
};

#endif
