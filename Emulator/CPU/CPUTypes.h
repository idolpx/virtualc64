// -----------------------------------------------------------------------------
// This file is part of VirtualC64
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Aliases.h"
#include "Reflection.h"
#include "PeddleTypes.h"

//
// Bit fields
//

// typedef u8 IntSource;

#define INTSRC_CIA  0b00000001
#define INTSRC_VIC  0b00000010
#define INTSRC_VIA1 0b00000100
#define INTSRC_VIA2 0b00001000
#define INTSRC_EXP  0b00010000
#define INTSRC_KBD  0b00100000


//
// Enumerations
//

#ifdef __cplusplus
struct CPURevisionEnum : util::Reflection<CPURevisionEnum, peddle::CPURevision> {
    
    static constexpr long minVal = 0;
    static constexpr long maxVal = peddle::MOS_6502;
    static bool isValid(auto value) { return value >= minVal && value <= maxVal; }
    
    static const char *prefix() { return nullptr; }
    static const char *key(peddle::CPURevision value)
    {
        switch (value) {
                
            case peddle::MOS_6510:  return "MOS_6510";
            case peddle::MOS_6502:  return "MOS_6502";
        }
        return "???";
    }
};
#endif

enum_long(BPTYPE)
{
    BPTYPE_NONE,
    BPTYPE_HARD,
    BPTYPE_SOFT
};
typedef BPTYPE BreakpointType;

#ifdef __cplusplus
struct BreakpointTypeEnum : util::Reflection<BreakpointTypeEnum, BreakpointType> {
    
	static constexpr long minVal = 0;
    static constexpr long maxVal = BPTYPE_SOFT;
    static bool isValid(auto value) { return value >= minVal && value <= maxVal; }
    
    static const char *prefix() { return "BPTYPE"; }
    static const char *key(BreakpointType value)
    {
        switch (value) {
                
            case BPTYPE_NONE:   return "NONE";
            case BPTYPE_HARD:   return "HARD";
            case BPTYPE_SOFT:   return "SOFT";
        }
        return "???";
    }
};
#endif

typedef struct
{
    u64 cycle;

    u16 pc0;
    u8 sp;
    u8 a;
    u8 x;
    u8 y;
    u8 sr;

    u8 irq;
    u8 nmi;
    bool rdy;
    bool jammed;
    
    u8 processorPort;
    u8 processorPortDir;
}
CPUInfo;
