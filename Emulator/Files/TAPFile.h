// -----------------------------------------------------------------------------
// This file is part of VirtualC64
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v2
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef _TAPFILE_H
#define _TAPFILE_H

#include "AnyFile.h"

class TAPFile : public AnyFile {
    
private:
    
    // Header signature
    static const u8 magicBytes[];
    
    // File pointer (offset into the data array)
    int fp;
        
public:
    
    //
    // Class methods
    //
    
    static bool isTAPBuffer(const u8 *buffer, size_t length);
    static bool isTAPFile(const char *filename);
    
    
    //
    // Initializing
    //
    
    TAPFile();
    const char *getDescription() override { return "TAPFile"; }

    static TAPFile *makeWithBuffer(const u8 *buffer, size_t length);
    static TAPFile *makeWithFile(const char *filename);
    
    
    //
    // Methods from AnyFile
    //
    
    void dealloc() override;
    FileType type() override { return FileType_TAP; }
    const char *getName() override;
    bool matchingBuffer(const u8 *buf, size_t len) override;
    bool matchingFile(const char *path) override;
    
    // bool matchingFile(const char *filename) override { return isTAPFile(filename); }
    bool oldReadFromBuffer(const u8 *buffer, size_t length) override;
    
    //
    // Retrieving tape information
    //
    
    // Returns the TAP version (0 = original layout, 1 = updated layout)
    TAPVersion version() { return (TAPVersion)data[0x000C]; }
    
    // Returns the beginning of the data area
    u8 *getData() { return data + 0x14; }
    
    // Returns the size of the data area in bytes
    size_t getDataSize() { return size - 0x14; }
};

#endif
