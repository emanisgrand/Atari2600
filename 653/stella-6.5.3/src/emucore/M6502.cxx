//============================================================================
//
// MM     MM  6666  555555  0000   2222
// MMMM MMMM 66  66 55     00  00 22  22
// MM MMM MM 66     55     00  00     22
// MM  M  MM 66666  55555  00  00  22222  --  "A 6502 Microprocessor Emulator"
// MM     MM 66  66     55 00  00 22
// MM     MM 66  66 55  55 00  00 22
// MM     MM  6666   5555   0000  222222
//
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
  #include "Expression.hxx"
  #include "Device.hxx"
  #include "Base.hxx"

  // Flags for access types
  #define DISASM_CODE  Device::CODE
  #define DISASM_DATA  Device::DATA
  #define DISASM_WRITE Device::WRITE
  #define DISASM_NONE  Device::NONE
#else
  // Flags for access types
  #define DISASM_CODE  0
  #define DISASM_DATA  0
  #define DISASM_WRITE 0
  #define DISASM_NONE  0
#endif
#include "Settings.hxx"
#include "Vec.hxx"

#include "Cart.hxx"
#include "TIA.hxx"
#include "M6532.hxx"
#include "System.hxx"
#include "M6502.hxx"
#include "DispatchResult.hxx"
#include "exception/EmulationWarning.hxx"
#include "exception/FatalEmulationError.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6502::M6502(const Settings& settings)
  : mySettings{settings}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::install(System& system)
{
  // Remember which system I'm installed in
  mySystem = &system;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::reset()
{
  // Clear the execution status flags
  myExecutionStatus = 0;

  // Set registers to random or default values
  bool devSettings = mySettings.getBool("dev.settings");
  const string& cpurandom = mySettings.getString(devSettings ? "dev.cpurandom" : "plr.cpurandom");
  SP = BSPF::containsIgnoreCase(cpurandom, "S") ?
          mySystem->randGenerator().next() : 0xfd;
  A  = BSPF::containsIgnoreCase(cpurandom, "A") ?
          mySystem->randGenerator().next() : 0x00;
  X  = BSPF::containsIgnoreCase(cpurandom, "X") ?
          mySystem->randGenerator().next() : 0x00;
  Y  = BSPF::containsIgnoreCase(cpurandom, "Y") ?
          mySystem->randGenerator().next() : 0x00;
  PS(BSPF::containsIgnoreCase(cpurandom, "P") ?
          mySystem->randGenerator().next() : 0x20);

  icycles = 0;

  // Load PC from the reset vector
  PC = uInt16(mySystem->peek(0xfffc)) | (uInt16(mySystem->peek(0xfffd)) << 8);

  myLastAddress = myLastPeekAddress = myLastPokeAddress = myLastPeekBaseAddress = myLastPokeBaseAddress;
  myLastSrcAddressS = myLastSrcAddressA =
    myLastSrcAddressX = myLastSrcAddressY = -1;
  myDataAddressForPoke = 0;
  myFlags = DISASM_NONE;

  myHaltRequested = false;
  myGhostReadsTrap = mySettings.getBool("dbg.ghostreadstrap");
  myReadFromWritePortBreak = devSettings ? mySettings.getBool("dev.rwportbreak") : false;
  myWriteToReadPortBreak = devSettings ? mySettings.getBool("dev.wrportbreak") : false;

  myLastBreakCycle = ULLONG_MAX;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt8 M6502::peek(uInt16 address, Device::AccessFlags flags)
{
  handleHalt();

  ////////////////////////////////////////////////
  // TODO - move this logic directly into CartAR
  if(address != myLastAddress)
  {
    ++myNumberOfDistinctAccesses;
    myLastAddress = address;
  }
  ////////////////////////////////////////////////
  mySystem->incrementCycles(SYSTEM_CYCLES_PER_CPU);
  icycles += SYSTEM_CYCLES_PER_CPU;
  myFlags = flags;
  uInt8 result = mySystem->peek(address, flags);
  myLastPeekAddress = address;

#ifdef DEBUGGER_SUPPORT
  if(myReadTraps.isInitialized() && myReadTraps.isSet(address)
     && (myGhostReadsTrap || flags != DISASM_NONE))
  {
    myLastPeekBaseAddress = myDebugger->getBaseAddress(myLastPeekAddress, true); // mirror handling
    int cond = evalCondTraps();
    if(cond > -1)
    {
      myJustHitReadTrapFlag = true;
      stringstream msg;
      msg << "RTrap" << (flags == DISASM_NONE ? "G[" : "[") << Common::Base::HEX2 << cond << "]"
        << (myTrapCondNames[cond].empty() ? ": " : "If: {" + myTrapCondNames[cond] + "} ");
      myHitTrapInfo.message = msg.str();
      myHitTrapInfo.address = address;
    }
  }
#endif  // DEBUGGER_SUPPORT

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void M6502::poke(uInt16 address, uInt8 value, Device::AccessFlags flags)
{
  ////////////////////////////////////////////////
  // TODO - move this logic directly into CartAR
  if(address != myLastAddress)
  {
    ++myNumberOfDistinctAccesses;
    myLastAddress = address;
  }
  ////////////////////////////////////////////////
  mySystem->incrementCycles(SYSTEM_CYCLES_PER_CPU);
  icycles += SYSTEM_CYCLES_PER_CPU;
  mySystem->poke(address, value, flags);
  myLastPokeAddress = address;

#ifdef DEBUGGER_SUPPORT
  if(myWriteTraps.isInitialized() && myWriteTraps.isSet(address))
  {
    myLastPokeBaseAddress = myDebugger->getBaseAddress(myLastPokeAddress, false); // mirror handling
    int cond = evalCondTraps();
    if(cond > -1)
    {
      myJustHitWriteTrapFlag = true;
      stringstream msg;
      msg << "WTrap[" << Common::Base::HEX2 << cond << "]" << (myTrapCondNames[cond].empty() ? ": " : "If: {" + myTrapCondNames[cond] + "} ");
      myHitTrapInfo.message = msg.str();
      myHitTrapInfo.address = address;
    }
  }
#endif  // DEBUGGER_SUPPORT
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::requestHalt()
{
  if (!myOnHaltCallback) throw runtime_error("onHaltCallback not configured");
  myHaltRequested = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void M6502::handleHalt()
{
  if (myHaltRequested) {
    myOnHaltCallback();
    myHaltRequested = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::execute(uInt64 number, DispatchResult& result)
{
  _execute(number, result);

#ifdef DEBUGGER_SUPPORT
  // Debugger hack: this ensures that stepping a "STA WSYNC" will actually end at the
  // beginning of the next line (otherwise, the next instruction would be stepped in order for
  // the halt to take effect). This is safe because as we know that the next cycle will be a read
  // cycle anyway.
  handleHalt();
#endif

  // Make sure that the hardware state matches the current system clock. This is necessary
  // to maintain a consistent state for the debugger after stepping and to make sure
  // that audio samples are generated for the whole timeslice.
  mySystem->tia().updateEmulation();
  mySystem->m6532().updateEmulation();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool M6502::execute(uInt64 number)
{
  DispatchResult result;

  execute(number, result);

  return result.isSuccess();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void M6502::_execute(uInt64 cycles, DispatchResult& result)
{
  myExecutionStatus = 0;

#ifdef DEBUGGER_SUPPORT
  TIA& tia = mySystem->tia();
  M6532& riot = mySystem->m6532();
#endif

  uInt64 previousCycles = mySystem->cycles();
  uInt64 currentCycles = 0;

  // Loop until execution is stopped or a fatal error occurs
  for(;;)
  {
    while (!myExecutionStatus && currentCycles < cycles * SYSTEM_CYCLES_PER_CPU)
    {
  #ifdef DEBUGGER_SUPPORT
      // Don't break if we haven't actually executed anything yet
      if (myLastBreakCycle != mySystem->cycles()) {
        if(myJustHitReadTrapFlag || myJustHitWriteTrapFlag)
        {
          bool read = myJustHitReadTrapFlag;
          myJustHitReadTrapFlag = myJustHitWriteTrapFlag = false;

          myLastBreakCycle = mySystem->cycles();
          result.setDebugger(currentCycles, myHitTrapInfo.message,
                             read ? "Read trap" : "Write trap",
                             myHitTrapInfo.address, read);
          return;
        }

        if(myBreakPoints.isInitialized())
        {
          uInt8 bank = mySystem->cart().getBank(PC);

          if(myBreakPoints.check(PC, bank))
          {
            myLastBreakCycle = mySystem->cycles();
            // disable a one-shot breakpoint
            if(myBreakPoints.get(PC, bank) & BreakpointMap::ONE_SHOT)
            {
              myBreakPoints.erase(PC, bank);
            }
            else
            {
              ostringstream msg;

              msg << "BP: $" << Common::Base::HEX4 << PC << ", bank #" << std::dec << int(bank);
              result.setDebugger(currentCycles, msg.str(), "Breakpoint");
            }
            return;
          }
        }

        int cond = evalCondBreaks();
        if(cond > -1)
        {
          ostringstream msg;

          msg << "CBP[" << Common::Base::HEX2 << cond << "]: " << myCondBreakNames[cond];

          myLastBreakCycle = mySystem->cycles();
          result.setDebugger(currentCycles, msg.str(), "Conditional breakpoint");
          return;
        }
      }

      int cond = evalCondSaveStates();
      if(cond > -1)
      {
        ostringstream msg;
        msg << "conditional savestate [" << Common::Base::HEX2 << cond << "]";
        myDebugger->addState(msg.str());
      }

      mySystem->cart().clearAllRAMAccesses();
  #endif  // DEBUGGER_SUPPORT

      // Reset the data poke address pointer
      myDataAddressForPoke = 0;

      try {
        uInt16 operandAddress = 0, intermediateAddress = 0;
        uInt8 operand = 0;

        icycles = 0;
    #ifdef DEBUGGER_SUPPORT
        uInt16 oldPC = PC;
    #endif

        // Fetch instruction at the program counter
        IR = peek(PC++, DISASM_CODE);  // This address represents a code section

        // Call code to execute the instruction
        switch(IR)
        {
          // 6502 instruction emulation is generated by an M4 macro file
          #include "M6502.ins"

          default:
            FatalEmulationError::raise("invalid instruction");
        }

    #ifdef DEBUGGER_SUPPORT
        if(myReadFromWritePortBreak)
        {
          uInt16 rwpAddr = mySystem->cart().getIllegalRAMReadAccess();
          if(rwpAddr)
          {
            ostringstream msg;
            msg << "RWP[@ $" << Common::Base::HEX4 << rwpAddr << "]: ";
            result.setDebugger(currentCycles, msg.str(), "Read from write port", oldPC);
            return;
          }
        }

        if (myWriteToReadPortBreak)
        {
          uInt16 wrpAddr = mySystem->cart().getIllegalRAMWriteAccess();
          if (wrpAddr)
          {
            ostringstream msg;
            msg << "WRP[@ $" << Common::Base::HEX4 << wrpAddr << "]: ";
            result.setDebugger(currentCycles, msg.str(), "Write to read port", oldPC);
            return;
          }
        }
    #endif  // DEBUGGER_SUPPORT
      } catch (const FatalEmulationError& e) {
        myExecutionStatus |= FatalErrorBit;
        result.setMessage(e.what());
      } catch (const EmulationWarning& e) {
        result.setDebugger(currentCycles, e.what(), "Emulation exception", PC);
        return;
      }

      currentCycles = (mySystem->cycles() - previousCycles);

  #ifdef DEBUGGER_SUPPORT
      if(myStepStateByInstruction)
      {
        // Check out M6502::execute for an explanation.
        handleHalt();

        tia.updateEmulation();
        riot.updateEmulation();
      }
  #endif
    }

    // See if we need to handle an interrupt
    if((myExecutionStatus & MaskableInterruptBit) ||
        (myExecutionStatus & NonmaskableInterruptBit))
    {
      // Yes, so handle the interrupt
      interruptHandler();
    }

    // See if a fatal error has occurred
    if(myExecutionStatus & FatalErrorBit)
    {
      // Yes, so answer that something when wrong. The message has already been set when
      // the exception was handled.
      result.setFatal(currentCycles);
      return;
    }

    // See if execution has been stopped
    if(myExecutionStatus & StopExecutionBit)
    {
      // Yes, so answer that everything finished fine
      result.setOk(currentCycles);
      return;
    }

    if (currentCycles >= cycles * SYSTEM_CYCLES_PER_CPU) {
      result.setOk(currentCycles);
      return;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::interruptHandler()
{
  // Handle the interrupt
  if((myExecutionStatus & MaskableInterruptBit) && !I)
  {
    mySystem->incrementCycles(7 * SYSTEM_CYCLES_PER_CPU);
    mySystem->poke(0x0100 + SP--, (PC - 1) >> 8);
    mySystem->poke(0x0100 + SP--, (PC - 1) & 0x00ff);
    mySystem->poke(0x0100 + SP--, PS() & (~0x10));
    D = false;
    I = true;
    PC = uInt16(mySystem->peek(0xFFFE)) | (uInt16(mySystem->peek(0xFFFF)) << 8);
  }
  else if(myExecutionStatus & NonmaskableInterruptBit)
  {
    mySystem->incrementCycles(7 * SYSTEM_CYCLES_PER_CPU);
    mySystem->poke(0x0100 + SP--, (PC - 1) >> 8);
    mySystem->poke(0x0100 + SP--, (PC - 1) & 0x00ff);
    mySystem->poke(0x0100 + SP--, PS() & (~0x10));
    D = false;
    PC = uInt16(mySystem->peek(0xFFFA)) | (uInt16(mySystem->peek(0xFFFB)) << 8);
  }

  // Clear the interrupt bits in myExecutionStatus
  myExecutionStatus &= ~(MaskableInterruptBit | NonmaskableInterruptBit);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool M6502::save(Serializer& out) const
{
  try
  {
    out.putByte(A);    // Accumulator
    out.putByte(X);    // X index register
    out.putByte(Y);    // Y index register
    out.putByte(SP);   // Stack Pointer
    out.putByte(IR);   // Instruction register
    out.putShort(PC);  // Program Counter

    out.putBool(N);    // N flag for processor status register
    out.putBool(V);    // V flag for processor status register
    out.putBool(B);    // B flag for processor status register
    out.putBool(D);    // D flag for processor status register
    out.putBool(I);    // I flag for processor status register
    out.putBool(notZ); // Z flag complement for processor status register
    out.putBool(C);    // C flag for processor status register

    out.putByte(myExecutionStatus);

    // Indicates the number of distinct memory accesses
    out.putInt(myNumberOfDistinctAccesses);
    // Indicates the last address(es) which was accessed
    out.putShort(myLastAddress);
    out.putShort(myLastPeekAddress);
    out.putShort(myLastPokeAddress);
    out.putShort(myDataAddressForPoke);
    out.putInt(myLastSrcAddressS);
    out.putInt(myLastSrcAddressA);
    out.putInt(myLastSrcAddressX);
    out.putInt(myLastSrcAddressY);
    out.putByte(myFlags);

    out.putBool(myHaltRequested);
    out.putLong(myLastBreakCycle);
  }
  catch(...)
  {
    cerr << "ERROR: M6502::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool M6502::load(Serializer& in)
{
  try
  {
    A = in.getByte();    // Accumulator
    X = in.getByte();    // X index register
    Y = in.getByte();    // Y index register
    SP = in.getByte();   // Stack Pointer
    IR = in.getByte();   // Instruction register
    PC = in.getShort();  // Program Counter

    N = in.getBool();    // N flag for processor status register
    V = in.getBool();    // V flag for processor status register
    B = in.getBool();    // B flag for processor status register
    D = in.getBool();    // D flag for processor status register
    I = in.getBool();    // I flag for processor status register
    notZ = in.getBool(); // Z flag complement for processor status register
    C = in.getBool();    // C flag for processor status register

    myExecutionStatus = in.getByte();

    // Indicates the number of distinct memory accesses
    myNumberOfDistinctAccesses = in.getInt();
    // Indicates the last address(es) which was accessed
    myLastAddress = in.getShort();
    myLastPeekAddress = in.getShort();
    myLastPokeAddress = in.getShort();
    myDataAddressForPoke = in.getShort();
    myLastSrcAddressS = in.getInt();
    myLastSrcAddressA = in.getInt();
    myLastSrcAddressX = in.getInt();
    myLastSrcAddressY = in.getInt();
    myFlags = in.getByte();

    myHaltRequested = in.getBool();
    myLastBreakCycle = in.getLong();

  #ifdef DEBUGGER_SUPPORT
    updateStepStateByInstruction();
  #endif
  }
  catch(...)
  {
    cerr << "ERROR: M6502::load" << endl;
    return false;
  }

  return true;
}

#ifdef DEBUGGER_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::attach(Debugger& debugger)
{
  // Remember the debugger for this microprocessor
  myDebugger = &debugger;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 M6502::addCondBreak(Expression* e, const string& name, bool oneShot)
{
  myCondBreaks.emplace_back(e);
  myCondBreakNames.push_back(name);

  updateStepStateByInstruction();

  return uInt32(myCondBreaks.size() - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool M6502::delCondBreak(uInt32 idx)
{
  if(idx < myCondBreaks.size())
  {
    Vec::removeAt(myCondBreaks, idx);
    Vec::removeAt(myCondBreakNames, idx);

    updateStepStateByInstruction();

    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::clearCondBreaks()
{
  myCondBreaks.clear();
  myCondBreakNames.clear();

  updateStepStateByInstruction();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const StringList& M6502::getCondBreakNames() const
{
  return myCondBreakNames;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 M6502::addCondSaveState(Expression* e, const string& name)
{
  myCondSaveStates.emplace_back(e);
  myCondSaveStateNames.push_back(name);

  updateStepStateByInstruction();

  return uInt32(myCondSaveStates.size() - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool M6502::delCondSaveState(uInt32 idx)
{
  if(idx < myCondSaveStates.size())
  {
    Vec::removeAt(myCondSaveStates, idx);
    Vec::removeAt(myCondSaveStateNames, idx);

    updateStepStateByInstruction();

    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::clearCondSaveStates()
{
  myCondSaveStates.clear();
  myCondSaveStateNames.clear();

  updateStepStateByInstruction();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const StringList& M6502::getCondSaveStateNames() const
{
  return myCondSaveStateNames;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 M6502::addCondTrap(Expression* e, const string& name)
{
  myTrapConds.emplace_back(e);
  myTrapCondNames.push_back(name);

  updateStepStateByInstruction();

  return uInt32(myTrapConds.size() - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool M6502::delCondTrap(uInt32 brk)
{
  if(brk < myTrapConds.size())
  {
    Vec::removeAt(myTrapConds, brk);
    Vec::removeAt(myTrapCondNames, brk);

    updateStepStateByInstruction();

    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::clearCondTraps()
{
  myTrapConds.clear();
  myTrapCondNames.clear();

  updateStepStateByInstruction();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const StringList& M6502::getCondTrapNames() const
{
  return myTrapCondNames;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::updateStepStateByInstruction()
{
  myStepStateByInstruction = myCondBreaks.size() || myCondSaveStates.size() ||
                             myTrapConds.size();
}
#endif  // DEBUGGER_SUPPORT
