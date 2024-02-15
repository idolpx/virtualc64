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

class GameKiller : public Cartridge {
    
    virtual CartridgeTraits traits() const override {

        return CartridgeTraits {

            .type       = CRT_GAME_KILLER,
            .title      = "Game Killer",

            .button1    = "Freeze"
        };
    }

public:
    
    GameKiller(C64 &ref) : Cartridge(ref) { };
    
private:
    
    //
    // Accessing cartridge memory
    //
    
    void resetCartConfig() override;
    
    u8 peek(u16 addr) override;
    u8 peekIO1(u16 addr) override { return 0; }
    u8 spypeekIO1(u16 addr) const override { return 0; }
    u8 peekIO2(u16 addr) override { return 0; }
    u8 spypeekIO2(u16 addr) const override { return 0; }
    void pokeIO1(u16 addr, u8 value) override;
    void pokeIO2(u16 addr, u8 value) override;
    
    
    //
    // Operating buttons
    //
    
    isize numButtons() const override { return 1; }
    const char *getButtonTitle(isize nr) const override;
    void pressButton(isize nr) override;
    void releaseButton(isize nr) override;

    
    //
    // Handling delegation calls
    //

    void updatePeekPokeLookupTables() override;
    void nmiWillTrigger() override;
};

