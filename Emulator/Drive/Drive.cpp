// -----------------------------------------------------------------------------
// This file is part of VirtualC64
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Drive.h"
#include "C64.h"
#include "IO.h"

Drive::Drive(DriveID id, C64 &ref) : C64Component(ref), deviceNr(id)
{
    assert(deviceNr == DRIVE8 || deviceNr == DRIVE9);
	
    subComponents = std::vector <HardwareComponent *> {
        
        &mem,
        &cpu,
        &via1,
        &via2,
        &disk
    };
}

const char *
Drive::getDescription() const
{
    assert(deviceNr == DRIVE8 || deviceNr == DRIVE9);
    return deviceNr == DRIVE8 ? "Drive8" : "Drive9";
}

void
Drive::_initialize()
{
    resetConfig();
    
    insertionStatus = DISK_FULLY_EJECTED;
    disk.clearDisk();
}

void
Drive::_reset()
{
    RESET_SNAPSHOT_ITEMS

    cpu.reg.pc = 0xEAA0;
    halftrack = 41;
}

void
Drive::resetConfig()
{
    setConfigItem(OPT_DRIVE_CONNECT, deviceNr, deviceNr == DRIVE8);
    setConfigItem(OPT_DRIVE_POWER_SWITCH, deviceNr, true);
    setConfigItem(OPT_DRIVE_TYPE, deviceNr, DRIVE_VC1541II);
    setConfigItem(OPT_DRIVE_PAN, deviceNr, 0);
    setConfigItem(OPT_POWER_VOLUME, deviceNr, 50);
    setConfigItem(OPT_STEP_VOLUME, deviceNr, 50);
    setConfigItem(OPT_INSERT_VOLUME, deviceNr, 50);
    setConfigItem(OPT_EJECT_VOLUME, deviceNr, 50);
}

i64
Drive::getConfigItem(Option option) const
{
    switch (option) {
            
        case OPT_DRIVE_TYPE:          return (i64)config.type;
        case OPT_DRIVE_CONNECT:       return (i64)config.connected;
        case OPT_DRIVE_POWER_SWITCH:  return (i64)config.switchedOn;
        case OPT_DRIVE_PAN:           return (i64)config.pan;
        case OPT_POWER_VOLUME:        return (i64)config.powerVolume;
        case OPT_STEP_VOLUME:         return (i64)config.stepVolume;
        case OPT_INSERT_VOLUME:       return (i64)config.insertVolume;
        case OPT_EJECT_VOLUME:        return (i64)config.ejectVolume;
            
        default:
            assert(false);
            return 0;
    }
}

bool
Drive::setConfigItem(Option option, i64 value)
{
    switch (option) {
            
        case OPT_VIC_REVISION:
        {
            u64 duration = 10000000000 / VICII::getFrequency((VICIIRevision)value);
            
            if (durationOfOneCpuCycle == duration) {
                return false;
            }
            
            durationOfOneCpuCycle = duration;
            return true;
        }
        case OPT_POWER_VOLUME:
        case OPT_STEP_VOLUME:
        case OPT_INSERT_VOLUME:
        case OPT_EJECT_VOLUME:
        case OPT_DRIVE_PAN:
        {
            bool result1 = setConfigItem(option, DRIVE8, value);
            bool result2 = setConfigItem(option, DRIVE9, value);
            return result1 || result2;
        }
        default:
            return false;
    }
}

bool
Drive::setConfigItem(Option option, long id, i64 value)
{
    if (id != deviceNr) return false;
    
    switch (option) {
            
        case OPT_DRIVE_TYPE:
        {
            if (!DriveTypeEnum::isValid(value)) {
                throw ConfigArgError(DriveTypeEnum::keyList());
            }
            if (config.type == value) return false;
            
            config.type = (DriveType)value;
            return true;
        }
        case OPT_DRIVE_CONNECT:
        {
            if (config.connected == value) {
                return false;
            }
            if (value && !c64.hasRom(ROM_TYPE_VC1541)) {
                warn("Can't connect drive (ROM missing).\n");
                return false;
            }
            
            suspend();
            config.connected = value;
            bool wasActive = active;
            active = config.connected && config.switchedOn;
            reset();
            resume();
            messageQueue.put(value ? MSG_DRIVE_CONNECT : MSG_DRIVE_DISCONNECT, deviceNr);
            if (wasActive != active)
                messageQueue.put(active ? MSG_DRIVE_ACTIVE : MSG_DRIVE_INACTIVE, deviceNr);
            return true;
        }
        case OPT_DRIVE_POWER_SWITCH:
        {
            if (config.switchedOn == value) {
                return false;
            }
            
            suspend();
            config.switchedOn = value;
            bool wasActive = active;
            active = config.connected && config.switchedOn;
            reset();
            resume();
            messageQueue.put(value ? MSG_DRIVE_POWER_ON : MSG_DRIVE_POWER_OFF, deviceNr);
            if (wasActive != active) {
                messageQueue.put(active ? MSG_DRIVE_ACTIVE : MSG_DRIVE_INACTIVE,
                                 config.pan << 24 | config.powerVolume << 16 | deviceNr);
            }
            return true;
        }
        case OPT_DRIVE_PAN:
        {
            if (config.pan == value) {
                return false;
            }
            config.pan = value;
            return true;
        }
        case OPT_POWER_VOLUME:
        {
            value = std::clamp(value, 0LL, 100LL);

            if (config.powerVolume == value) {
                return false;
            }
            config.powerVolume = value;
            return true;
        }
        case OPT_STEP_VOLUME:
        {
            value = std::clamp(value, 0LL, 100LL);

            if (config.stepVolume == value) {
                return false;
            }
            config.stepVolume = value;
            return true;
        }
        case OPT_EJECT_VOLUME:
        {
            value = std::clamp(value, 0LL, 100LL);
            
            if (config.ejectVolume == value) {
                return false;
            }
            config.ejectVolume = value;
            printf("New eject volume: %d\n", config.ejectVolume);
            return true;
        }
        case OPT_INSERT_VOLUME:
        {
            value = std::clamp(value, 0LL, 100LL);
            
            if (config.insertVolume == value) {
                return false;
            }
            config.insertVolume = value;
            return true;
        }
        default:
            return false;
    }
}

void
Drive::_dump(dump::Category category, std::ostream& os) const
{
    using namespace util;
    
    if (category & dump::Config) {
    
        os << tab("Drive type");
        os << DriveTypeEnum::key(config.type) << std::endl;
        os << tab("Connected");
        os << bol(config.connected) << std::endl;
        os << tab("Power switch");
        os << bol(config.switchedOn, "on", "off") << std::endl;
        os << tab("Pan");
        os << config.pan << std::endl;
        os << tab("Power volume");
        os << dec(config.powerVolume) << std::endl;
        os << tab("Step volume");
        os << dec(config.stepVolume) << std::endl;
        os << tab("Insert volume");
        os << dec(config.insertVolume) << std::endl;
        os << tab("Eject volume");
        os << dec(config.ejectVolume) << std::endl;
    }
    
    if (category & dump::State) {
         
        os << tab("Has disk");
        os << bol(hasDisk()) << std::endl;
        os << tab("Bit ready timer");
        os << dec(bitReadyTimer) << std::endl;
        os << tab("Head position");
        os << dec(halftrack) << "::" << dec(offset) << std::endl;
        os << tab("SYNC");
        os << bol(sync) << std::endl;
        os << tab("Read mode");
        os << bol(readMode()) << std::endl;
    }
}

void
Drive::_run()
{
    // Make sure the emulator has been configured properly
    assert(durationOfOneCpuCycle > 0);
}

void
Drive::execute(u64 duration)
{
    elapsedTime += duration;
    while (nextClock < (i64)elapsedTime || nextCarry < (i64)elapsedTime) {

        if (nextClock <= nextCarry) {
            
            // Execute CPU and VIAs
            u64 cycle = ++cpu.cycle;
            cpu.executeOneCycle();
            if (cycle >= via1.wakeUpCycle) via1.execute(); else via1.idleCounter++;
            if (cycle >= via2.wakeUpCycle) via2.execute(); else via2.idleCounter++;
            updateByteReady();
            if (iec.isDirtyDriveSide) iec.updateIecLinesDriveSide();

            nextClock += 10000;

        } else {
            
            // Execute read/write logic
            if (spinning) executeUF4();
            nextCarry += delayBetweenTwoCarryPulses[zone];
        }
    }
    assert(nextClock >= (i64)elapsedTime && nextCarry >= (i64)elapsedTime);
}

void
Drive::executeUF4()
{
    // Increase counter
    counterUF4++;
    carryCounter++;
    
    // We assume that a new bit comes in every fourth cycle.
    // Later, we can decouple timing here to emulate asynchronicity.
    if (carryCounter % 4 == 0) {
        
        // When a bit comes in and ...
        //   ... it's value equals 0, nothing happens.
        //   ... it's value equals 1, counter UF4 is reset.
        if (readMode() && readBitFromHead()) {
            counterUF4 = 0;
        }
        rotateDisk();
    }

    // Update SYNC signal
    sync = (readShiftreg & 0x3FF) != 0x3FF || writeMode();
    if (!sync) byteReadyCounter = 0;
    
    // The lower two bits of counter UF4 are used to clock the logic board:
    //
    //                        (6) Load the write shift register
    //                         |      if the byte ready counter equals 7.
    //                         v
    //         ---- ----           ---- ----
    // QBQA:  | 00   01 | 10   11 | 00   01 | 10   11 |
    //                   ---- ----           ---- ----
    //                   ^          ^    ^    ^    ^
    //                   |          |    |    |    |
    //                   |          |    |   (2) Byte ready is always 1 here.
    //                   |         (1)  (1) Byte ready may be 0 here.
    //                   |
    //                  (3) Execute UE3 (the byte ready counter)
    //                  (4) Execute write shift register
    //                  (5) Execute read shift register
    //
    
    switch (counterUF4 & 0x03) {
            
        case 0x00:
        case 0x01:
            
            // Computation of the Byte Ready and the Load signal
            //
            //           74LS191                             ---
            //           -------               VIA2::CA2 ---|   |
            //  SYNC --o| Load  |                UF4::QB --o| & |o-- Byte Ready
            //    QB ---| Clk   |                        ---|   |
            //          |    QD |   ---                  |   ---
            //          |    QC |--|   |    ---          |   ---
            //          |    QB |--| & |o--| 1 |o-----------|   |
            //          |    QA |--|   |    ---   UF4::QB --| & |o-- load UD3
            //           -------    ---           UF4::QA --|   |
            //             UE3                               ---
            
            // (1) Update value on Byte Ready line
            updateByteReady();
            break;
            
        case 0x02:
            
            // (2)
            raiseByteReady();
            
            // (3) Execute byte ready counter
            byteReadyCounter = sync ? (byteReadyCounter + 1) % 8 : 0;
            
            // (4) Execute the write shift register
            if (writeMode() && !getLightBarrier()) {
                writeBitToHead(writeShiftreg & 0x80);
                disk.setModified(true);
            }
            writeShiftreg <<= 1;
            
            // (5) Execute read shift register
            readShiftreg <<= 1;
            readShiftreg |= ((counterUF4 & 0x0C) == 0);
            break;
            
        case 0x03:
            
            // (6)
            if (byteReadyCounter == 7) {
                writeShiftreg = via2.getPA();
            }
            break;
    }
}

void
Drive::updateByteReady()
{
    //
    //           74LS191                             ---
    //           -------               VIA2::CA2 ---|   |
    //  SYNC --o| Load  |                UF4::QB --o| & |o-- Byte Ready
    //    QB ---| Clk   |                        ---|   |
    //          |    QD |   ---                  |   ---
    //          |    QC |--|   |    ---          |
    //          |    QB |--| & |o--| 1 |o---------
    //          |    QA |--|   |    ---
    //           -------    ---
    //             UE3
    
    bool ca2 = via2.getCA2();
    bool qb = counterUF4 & 0x02;
    bool ue3 = (byteReadyCounter == 7);
    bool newByteReady = !(ca2 && !qb && ue3);
    
    if (byteReady != newByteReady) {
        byteReady = newByteReady;
        via2.CA1action(byteReady);
    }
}

void
Drive::raiseByteReady()
{
    if (!byteReady) {
        byteReady = true;
        via2.CA1action(true);
    }
}

void
Drive::setZone(u8 value)
{
    assert(value < 4);
    
    if (value != zone) {
        trace(DRV_DEBUG, "Switching from disk zone %d to disk zone %d\n", zone, value);
        zone = value;
    }
}

void
Drive::setRedLED(bool b)
{
    if (!redLED && b) {
        redLED = true;
        c64.putMessage(MSG_DRIVE_LED_ON, deviceNr);
    } else if (redLED && !b) {
        redLED = false;
        c64.putMessage(MSG_DRIVE_LED_OFF, deviceNr);
    }
}

void
Drive::setRotating(bool b)
{
    if (spinning == b) return;
    
    spinning = b;
    c64.putMessage(b ? MSG_DRIVE_MOTOR_ON : MSG_DRIVE_MOTOR_OFF, deviceNr);
    iec.updateTransferStatus();
}

void
Drive::moveHeadUp()
{
    if (halftrack < 84) {

        float position = (float)offset / (float)disk.lengthOfHalftrack(halftrack);
        halftrack++;
        offset = (HeadPos)(position * disk.lengthOfHalftrack(halftrack));
        
        trace(DRV_DEBUG, "Moving head up to halftrack %d (track %2.1f) (offset %d)\n",
              halftrack, (halftrack + 1) / 2.0, offset);
        trace(DRV_DEBUG, "Halftrack %d has %d bits.\n", halftrack, disk.lengthOfHalftrack(halftrack));
    }
   
    assert(disk.isValidHeadPos(halftrack, offset));
    
    c64.putMessage(MSG_DRIVE_STEP,
                   config.pan << 24 | config.stepVolume << 16 | halftrack << 8 | deviceNr);
}

void
Drive::moveHeadDown()
{
    if (halftrack > 1) {
        float position = (float)offset / (float)disk.lengthOfHalftrack(halftrack);
        halftrack--;
        offset = (HeadPos)(position * disk.lengthOfHalftrack(halftrack));
        
        trace(DRV_DEBUG, "Moving head down to halftrack %d (track %2.1f)\n",
              halftrack, (halftrack + 1) / 2.0);
        trace(DRV_DEBUG, "Halftrack %d has %d bits.\n", halftrack, disk.lengthOfHalftrack(halftrack));
    }
    
    assert(disk.isValidHeadPos(halftrack, offset));
    
    c64.putMessage(MSG_DRIVE_STEP,
                   config.pan << 24 | config.stepVolume << 16 | halftrack << 8 | deviceNr);
}

void
Drive::setModifiedDisk(bool value)
{
    disk.setModified(value);
    c64.putMessage(value ? MSG_DISK_UNSAVED : MSG_DISK_SAVED, deviceNr);
}

void
Drive::insertDisk(const string &path)
{
    ErrorCode ec;
    AnyCollection *file = nullptr;
    
    if (!file) file = AnyFile::make <PRGFile> (path, &ec);
        
    if (file) {
        insertDisk(*file);
    }
}

void
Drive::insertDisk(Disk *otherDisk)
{
    debug(DSKCHG_DEBUG, "insertDisk(otherDisk %p)\n", otherDisk);

    suspend();
    
    if (!diskToInsert) {
        
        // Initiate the disk change procedure
        diskToInsert = otherDisk;
        diskChangeCounter = 1;
    }
    
    resume();
}

void
Drive::insertNewDisk(DOSType fsType)
{
    PETName<16> name = PETName<16>("NEW DISK");
    insertNewDisk(fsType, name);
}

void
Drive::insertNewDisk(DOSType fsType, PETName<16> name)
{
    Disk *newDisk = Disk::make(c64, fsType, name);
    insertDisk(newDisk);
}

void
Drive::insertFileSystem(FSDevice *device)
{
    debug(DSKCHG_DEBUG, "insertFileSystem(%p)\n", device);
    insertDisk(Disk::makeWithFileSystem(c64, *device));
}

void
Drive::insertG64(G64File *g64)
{
    debug(DSKCHG_DEBUG, "insertG64(%p)\n", g64);
    insertDisk(Disk::makeWithG64(c64, g64));
}

void
Drive::insertDisk(AnyCollection &collection)
{
    debug(DSKCHG_DEBUG, "insertDisk(collection)\n");
    insertDisk(Disk::makeWithCollection(c64, collection));
}

void 
Drive::ejectDisk()
{
    debug(DSKCHG_DEBUG, "ejectDisk()\n");

    suspend();
    
    if (insertionStatus == DISK_FULLY_INSERTED && !diskToInsert) {
        
        // Initiate the disk change procedure
        diskChangeCounter = 1;
    }
    
    resume();
}

void
Drive::vsyncHandler()
{
    // Only proceed if a disk change state transition is to be performed
    if (--diskChangeCounter) return;
    
    switch (insertionStatus) {
            
        case DISK_FULLY_INSERTED:
        {
            trace(DSKCHG_DEBUG, "FULLY_INSERTED -> PARTIALLY_EJECTED\n");

            // Pull the disk half out (blocks the light barrier)
            insertionStatus = DISK_PARTIALLY_EJECTED;
            
            // Make sure the drive can no longer read from this disk
            disk.clearDisk();
            
            // Schedule the next transition
            diskChangeCounter = 17;
            return;
        }
        case DISK_PARTIALLY_EJECTED:
        {
            trace(DSKCHG_DEBUG, "PARTIALLY_EJECTED -> FULLY_EJECTED\n");

            // Take the disk out (unblocks the light barrier)
            insertionStatus = DISK_FULLY_EJECTED;
            
            // Inform listeners
            c64.putMessage(MSG_DISK_EJECT,
                           config.pan << 24 | config.ejectVolume << 16 | halftrack << 8 | deviceNr);
            
            // Schedule the next transition
            diskChangeCounter = 17;
            return;
        }
        case DISK_FULLY_EJECTED:
        {
            trace(DSKCHG_DEBUG, "FULLY_EJECTED -> PARTIALLY_INSERTED\n");

            // Only proceed if a new disk is waiting for insertion
            if (!diskToInsert) return;
            
            // Push the new disk half in (blocks the light barrier)
            insertionStatus = DISK_PARTIALLY_INSERTED;
            
            // Schedule the next transition
            diskChangeCounter = 17;
            return;
        }
        case DISK_PARTIALLY_INSERTED:
        {
            trace(DSKCHG_DEBUG, "PARTIALLY_INSERTED -> FULLY_INSERTED\n");

            // Fully insert the disk (unblocks the light barrier)
            insertionStatus = DISK_FULLY_INSERTED;

            // Copy the disk contents
            usize size = diskToInsert->size();
            u8 *buffer = new u8[size];
            diskToInsert->save(buffer);
            disk.load(buffer);
            delete[] buffer;
            diskToInsert = nullptr;

            // Inform listeners
            c64.putMessage(MSG_DISK_INSERT,
                           config.pan << 24 | config.insertVolume << 16 | halftrack << 8 | deviceNr);
            return;
        }
        default:
            assert(false);
    }
}
