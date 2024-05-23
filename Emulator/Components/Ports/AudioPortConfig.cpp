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

#include "config.h"
#include "AudioPort.h"
#include "Emulator.h"

namespace vc64 {

i64
AudioPort::getOption(Option option) const
{
    switch (option) {

        case OPT_AUD_VOL0:      return config.vol[0];
        case OPT_AUD_VOL1:      return config.vol[1];
        case OPT_AUD_VOL2:      return config.vol[2];
        case OPT_AUD_VOL3:      return config.vol[3];
        case OPT_AUD_PAN0:      return config.pan[0];
        case OPT_AUD_PAN1:      return config.pan[1];
        case OPT_AUD_PAN2:      return config.pan[2];
        case OPT_AUD_PAN3:      return config.pan[3];
        case OPT_AUD_VOL_L:     return config.volL;
        case OPT_AUD_VOL_R:     return config.volR;

        default:
            fatalError;
    }
}

void
AudioPort::setOption(Option option, i64 value)
{
    // bool wasMuted = isMuted();
    isize channel = 0;

    switch (option) {

        case OPT_AUD_VOL3: channel++;
        case OPT_AUD_VOL2: channel++;
        case OPT_AUD_VOL1: channel++;
        case OPT_AUD_VOL0:
        {
            config.vol[channel] = std::clamp(value, 0LL, 100LL);
            vol[channel] = powf(config.vol[channel] / 100.0f, 1.4f) * 0.000025f;
            if (emscripten) vol[channel] *= 0.15f;

            /*
            if (wasMuted != sidBridge.isMuted()) {
                msgQueue.put(MSG_MUTE, sidBridge.isMuted());
            }
            */
            return;
        }

        case OPT_AUD_PAN3: channel++;
        case OPT_AUD_PAN2: channel++;
        case OPT_AUD_PAN1: channel++;
        case OPT_AUD_PAN0:
        {
            config.pan[channel] = value;
            pan[channel] = float(0.5 * (sin(config.pan[channel] * M_PI / 200.0) + 1));
            return;
        }

        case OPT_AUD_VOL_L:

            config.volL = std::clamp(value, 0LL, 100LL);
            volL.maximum = powf((float)config.volL / 50, 1.4f);

            /*
            if (wasMuted != isMuted()) {
                msgQueue.put(MSG_MUTE, isMuted());
            }
            */
            return;

        case OPT_AUD_VOL_R:

            config.volR = std::clamp(value, 0LL, 100LL);
            volR.maximum = powf((float)config.volR / 50, 1.4f);

            /*
            if (wasMuted != isMuted()) {
                msgQueue.put(MSG_MUTE, isMuted());
            }
            */
            return;

        default:
            fatalError;
    }
}

}
