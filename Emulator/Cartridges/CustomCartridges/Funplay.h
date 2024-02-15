// -----------------------------------------------------------------------------
// This file is part of VirtualC64
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// This FILE is dual-licensed. You are free to choose between:
//
//     - The GNU General Public License v3 (or any later version)
//     - The Mozilla Public License v2
//
// SPDX-License-Identifier: GPL-3.0-or-later OR MPL-2.0
// -----------------------------------------------------------------------------

#pragma once

#include "Cartridge.h"

class Funplay : public Cartridge {
    
    virtual CartridgeTraits traits() const override {

        return CartridgeTraits {

            .type       = CRT_FUNPLAY,
            .title      = "Fun Play",
        };
    }

public:

    Funplay(C64 &ref) : Cartridge(ref) { };
    
    
    //
    // Accessing cartridge memory
    //
    
public:
    
    void pokeIO1(u16 addr, u8 value) override;
};
