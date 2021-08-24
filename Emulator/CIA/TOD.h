// -----------------------------------------------------------------------------
// This file is part of VirtualC64
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "CIATypes.h"
#include "SubComponent.h"

inline u8 incBCD(u8 x)
{
    return ((x & 0xF) == 9) ? (x & 0xF0) + 0x10 : (x & 0xF0) + ((x + 1) & 0xF);
}

/* Time of day clock (TOD). Each CIA contains a time of day clock, counting
 * hours, minutes, seconds and tenths of a second. Furthermore, every TOD clock
 * features an alarm mechanism. When the alarm time is reached, an interrupt
 * is triggered.
 */
class TOD : public SubComponent {
    
    friend class CIA;
    
private:
    
    // Result of the latest inspection
    mutable TODInfo info = { };
    
    // Reference to the connected CIA
    class CIA &cia;
    
    // Time of day clock
	TimeOfDay tod;

    // Time of day clock latch
    TimeOfDay latch;

    // Alarm time
	TimeOfDay alarm;
	
	/* Indicates if the TOD registers are frozen. The CIA freezes the registers
     * when the hours-part is read and reactivates them, when the 1/10th part
     * is read. Although the values stay constant, the internal clock continues
     * to advance. Hence, if the hours-part is read first, the clock won't
     * change until all fragments have been read.
     */
	bool frozen;
	
	/* Indicates if the TOD clock is halted. The CIA chip stops the TOD clock
     * when the hours-part is written and restarts it, when the 1/10th part is
     * written. This ensures that the clock doesn't start until the time is
     * set completely.
     */
	bool stopped;
	
    /* Indicates if tod time matches the alarm time. This value is read in
     * checkIrq() for edge detection.
     */
    bool matching;

    // Cycle where the tenth of a second counter needs to be incremented
    Cycle nextTodTrigger;

        
    //
    // Initializing
    //
    
public:
    
	TOD(C64 &ref, CIA &cia);
    const char *getDescription() const override;

private:
    
    void _reset(bool hard) override;

    
    //
    // Analyzing
    //

public:
    
    // Returns the result of the most recent call to inspect()
    TODInfo getInfo() const { return C64Component::getInfo(info); }

private:
    
    void _inspect() const override;
    void _dump(dump::Category category, std::ostream& os) const override;
    
    
    //
    // Serializing
    //
    
private:
    
    template <class T>
    void applyToPersistentItems(T& worker)
    {
    }
    
    template <class T>
    void applyToResetItems(T& worker, bool hard = true)
    {
        worker
        
        << tod.value
        << latch.value
        << alarm.value
        << frozen
        << stopped
        << matching
        << nextTodTrigger;
    }
    
    isize _size() override { COMPUTE_SNAPSHOT_SIZE }
    isize _load(const u8 *buffer) override { LOAD_SNAPSHOT_ITEMS }
    isize _save(u8 *buffer) override { SAVE_SNAPSHOT_ITEMS }
    
    
    //
    // Accessing
    //
        
    // Returns the hours digits of the time of day clock
    u8 getTodHours() const { return (frozen ? latch.hour : tod.hour) & 0x9F; }
    
    // Returns the minutes digits of the time of day clock
    u8 getTodMinutes() const { return (frozen ? latch.min : tod.min) & 0x7F; }
    
    // Returns the seconds digits of the time of day clock
    u8 getTodSeconds() const { return (frozen ? latch.sec : tod.sec) & 0x7F; }
    
    // Returns the tenth-of-a-second digits of the time of day clock
    u8 getTodTenth() const { return (frozen ? latch.tenth : tod.tenth) & 0x0F; }
    
    // Returns the hours digits of the alarm time
    u8 getAlarmHours() const { return alarm.hour & 0x9F; }
    
    // Returns the minutes digits of the alarm time
    u8 getAlarmMinutes() const { return alarm.min & 0x7F; }
    
    // Returns the seconds digits of the alarm time
    u8 getAlarmSeconds() const { return alarm.sec & 0x7F; }
    
    // Returns the tenth-of-a-second digits of the alarm time
    u8 getAlarmTenth() const { return alarm.tenth & 0x0F; }
    
    // Sets the hours digits of the time of day clock
    void setTodHours(u8 value) { tod.hour = value & 0x9F; checkIrq(); }
    
    // Sets the minutes digits of the time of day clock
    void setTodMinutes(u8 value) { tod.min = value & 0x7F; checkIrq(); }
    
    // Sets the seconds digits of the time of day clock
    void setTodSeconds(u8 value) { tod.sec = value & 0x7F; checkIrq(); }
    
    // Sets the tenth-of-a-second digits of the time of day clock
    void setTodTenth(u8 value) { tod.tenth = value & 0x0F; checkIrq(); }
    
    // Sets the hours digits of the alarm time
    void setAlarmHours(u8 value) { alarm.hour = value & 0x9F; checkIrq(); }
    
    // Sets the minutes digits of the alarm time
    void setAlarmMinutes(u8 value) { alarm.min = value & 0x7F; checkIrq(); }
    
    // Sets the seconds digits of the alarm time
    void setAlarmSeconds(u8 value) { alarm.sec = value & 0x7F; checkIrq(); }
    
    // Sets the tenth-of-a-second digits of the time of day clock
    void setAlarmTenth(u8 value) { alarm.tenth = value & 0x0F; checkIrq(); }
    
    
    //
    // Emulating
    //
    
public:
    
    // Increments the TOD clock if necessary (called after each scanline)
    void increment();

private:
    
    // Freezes the time of day clock
    void freeze() { if (!frozen) { latch.value = tod.value; frozen = true; } }
    
    // Unfreezes the time of day clock
    void defreeze() { frozen = false; }
    
    // Stops the time of day clock
    void stop() { stopped = true; }

    // Starts the time of day clock
    void cont();

    // Updates variable 'matching'. A positive edge triggers an interrupt.
    void checkIrq();
};
