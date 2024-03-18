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
#include "Monitor.h"
#include "VICII.h"

namespace vc64 {

i64
Monitor::getOption(Option option) const
{
    switch (option) {

        case OPT_MON_PALETTE:               return config.palette;
        case OPT_MON_BRIGHTNESS:            return config.brightness;
        case OPT_MON_CONTRAST:              return config.contrast;
        case OPT_MON_SATURATION:            return config.saturation;
        case OPT_MON_HCENTER:               return config.hCenter;
        case OPT_MON_VCENTER:               return config.vCenter;
        case OPT_MON_HZOOM:                 return config.hZoom;
        case OPT_MON_VZOOM:                 return config.vZoom;
        case OPT_MON_UPSCALER:              return config.upscaler;
        case OPT_MON_BLUR:                  return config.blur;
        case OPT_MON_BLUR_RADIUS:           return config.blurRadius;
        case OPT_MON_BLOOM:                 return config.bloom;
        case OPT_MON_BLOOM_RADIUS:          return config.bloomRadius;
        case OPT_MON_BLOOM_BRIGHTNESS:      return config.brightness;
        case OPT_MON_BLOOM_WEIGHT:          return config.bloomWeight;
        case OPT_MON_DOTMASK:               return config.dotmask;
        case OPT_MON_DOTMASK_BRIGHTNESS:    return config.dotMaskBrightness;
        case OPT_MON_SCANLINES:             return config.scanlines;
        case OPT_MON_SCANLINE_BRIGHTNESS:   return config.scanlineBrightness;
        case OPT_MON_SCANLINE_WEIGHT:       return config.scanlineWeight;
        case OPT_MON_DISALIGNMENT:          return config.disalignment;
        case OPT_MON_DISALIGNMENT_H:        return config.disalignmentH;
        case OPT_MON_DISALIGNMENT_V:        return config.disalignmentV;

        default:
            fatalError;
    }
}

void
Monitor::setOption(Option option, i64 value)
{
    switch (option) {

        case OPT_MON_PALETTE:

            if (!PaletteEnum::isValid(value)) {
                throw VC64Error(ERROR_OPT_INVARG, PaletteEnum::keyList());
            }

            config.palette = Palette(value);
            vic.updatePalette();
            return;

        case OPT_MON_BRIGHTNESS:

            if (config.brightness < 0 || config.brightness > 100) {
                throw VC64Error(ERROR_OPT_INVARG, "Expected 0...100");
            }

            config.brightness = isize(value);
            vic.updatePalette();
            return;

        case OPT_MON_CONTRAST:

            if (config.contrast < 0 || config.contrast > 100) {
                throw VC64Error(ERROR_OPT_INVARG, "Expected 0...100");
            }

            config.contrast = isize(value);
            vic.updatePalette();
            return;

        case OPT_MON_SATURATION:

            if (config.saturation < 0 || config.saturation > 100) {
                throw VC64Error(ERROR_OPT_INVARG, "Expected 0...100");
            }

            config.saturation = isize(value);
            vic.updatePalette();
            return;

        case OPT_MON_HCENTER:

            config.hCenter = isize(value);
            return;

        case OPT_MON_VCENTER:

            config.vCenter = isize(value);
            return;

        case OPT_MON_HZOOM:

            config.hZoom = isize(value);
            return;

        case OPT_MON_VZOOM:

            config.vZoom = isize(value);
            return;

        case OPT_MON_UPSCALER:

            if (!UpscalerEnum::isValid(value)) {
                throw VC64Error(ERROR_OPT_INVARG, UpscalerEnum::keyList());
            }

            config.upscaler = Upscaler(value);
            return;

        case OPT_MON_BLUR:

            config.blur = isize(value);
            return;

        case OPT_MON_BLUR_RADIUS:

            config.blurRadius = isize(value);
            return;

        case OPT_MON_BLOOM:

            config.bloom = isize(value);
            return;

        case OPT_MON_BLOOM_RADIUS:

            config.blurRadius = isize(value);
            return;

        case OPT_MON_BLOOM_BRIGHTNESS:

            config.brightness = isize(value);
            return;

        case OPT_MON_BLOOM_WEIGHT:

            config.bloomWeight = isize(value);
            return;

        case OPT_MON_DOTMASK:

            if (!DotmaskEnum::isValid(value)) {
                throw VC64Error(ERROR_OPT_INVARG, DotmaskEnum::keyList());
            }

            config.dotmask = Dotmask(value);
            return;

        case OPT_MON_DOTMASK_BRIGHTNESS:

            config.dotMaskBrightness = isize(value);
            return;

        case OPT_MON_SCANLINES:

            if (!ScanlinesEnum::isValid(value)) {
                throw VC64Error(ERROR_OPT_INVARG, ScanlinesEnum::keyList());
            }

            config.scanlines = Scanlines(value);
            return;

        case OPT_MON_SCANLINE_BRIGHTNESS:

            config.scanlineBrightness = isize(value);
            return;

        case OPT_MON_SCANLINE_WEIGHT:

            config.scanlineWeight = isize(value);
            return;

        case OPT_MON_DISALIGNMENT: 

            config.disalignment = isize(value);
            return;

        case OPT_MON_DISALIGNMENT_H:

            config.disalignmentH = isize(value);
            return;

        case OPT_MON_DISALIGNMENT_V:

            config.disalignmentV = isize(value);
            return;

        default:
            fatalError;
    }
}

void
Monitor::_dump(Category category, std::ostream& os) const
{
    using namespace util;

    if (category == Category::Config) {

        dumpConfig(os);
    }
}

/* This implementation is mainly based on the following articles by pepto:
 * http://www.pepto.de/projects/colorvic/
 * http://unusedino.de/ec64/technical/misc/vic656x/colors/
 */

// TODO: Turn into Lamda expression
static double gammaCorrect(double value, double source, double target)
{
    // Reverse gamma correction of source
    double factor = pow(255.0, 1.0 - source);
    value = std::clamp(factor * pow(value, source), 0.0, 255.0);

    // Correct gamma for target
    factor = pow(255.0, 1.0 - (1.0 / target));
    value = std::clamp(factor * pow(value, 1 / target), 0.0, 255.0);

    return round(value);
}

u32
Monitor::getColor(isize nr, Palette palette)
{
    double y, u, v;

    // LUMA levels (varies between VICII models)
#define LUMA_VICE(x,y,z) ((double)(x - y) * 256)/((double)(z - y))
#define LUMA_COLORES(x) (x * 7.96875)

    double luma_vice_6569_r1[16] = {

        // Taken from VICE 3.2
        LUMA_VICE( 630,630,1850), LUMA_VICE(1850,630,1850),
        LUMA_VICE( 900,630,1850), LUMA_VICE(1560,630,1850),
        LUMA_VICE(1260,630,1850), LUMA_VICE(1260,630,1850),
        LUMA_VICE( 900,630,1850), LUMA_VICE(1560,630,1850),
        LUMA_VICE(1260,630,1850), LUMA_VICE( 900,630,1850),
        LUMA_VICE(1260,630,1850), LUMA_VICE( 900,630,1850),
        LUMA_VICE(1260,630,1850), LUMA_VICE(1560,630,1850),
        LUMA_VICE(1260,630,1850), LUMA_VICE(1560,630,1850)
    };

    double luma_vice_6569_r3[16] = {

        // Taken from VICE 3.2
        LUMA_VICE( 700,700,1850), LUMA_VICE(1850,700,1850),
        LUMA_VICE(1090,700,1850), LUMA_VICE(1480,700,1850),
        LUMA_VICE(1180,700,1850), LUMA_VICE(1340,700,1850),
        LUMA_VICE(1020,700,1850), LUMA_VICE(1620,700,1850),
        LUMA_VICE(1180,700,1850), LUMA_VICE(1020,700,1850),
        LUMA_VICE(1340,700,1850), LUMA_VICE(1090,700,1850),
        LUMA_VICE(1300,700,1850), LUMA_VICE(1620,700,1850),
        LUMA_VICE(1300,700,1850), LUMA_VICE(1480,700,1850),
    };

    double luma_vice_6567[16] = {

        // taken from VICE 3.2
        LUMA_VICE( 590,590,1825), LUMA_VICE(1825,590,1825),
        LUMA_VICE( 950,590,1825), LUMA_VICE(1380,590,1825),
        LUMA_VICE(1030,590,1825), LUMA_VICE(1210,590,1825),
        LUMA_VICE( 860,590,1825), LUMA_VICE(1560,590,1825),
        LUMA_VICE(1030,590,1825), LUMA_VICE( 860,590,1825),
        LUMA_VICE(1210,590,1825), LUMA_VICE( 950,590,1825),
        LUMA_VICE(1160,590,1825), LUMA_VICE(1560,590,1825),
        LUMA_VICE(1160,590,1825), LUMA_VICE(1380,590,1825)
    };

    double luma_vice_6567_r65a[16] = {

        // Taken from VICE 3.2
        LUMA_VICE( 560,560,1825), LUMA_VICE(1825,560,1825),
        LUMA_VICE( 840,560,1825), LUMA_VICE(1500,560,1825),
        LUMA_VICE(1180,560,1825), LUMA_VICE(1180,560,1825),
        LUMA_VICE( 840,560,1825), LUMA_VICE(1500,560,1825),
        LUMA_VICE(1180,560,1825), LUMA_VICE( 840,560,1825),
        LUMA_VICE(1180,560,1825), LUMA_VICE( 840,560,1825),
        LUMA_VICE(1180,560,1825), LUMA_VICE(1500,560,1825),
        LUMA_VICE(1180,560,1825), LUMA_VICE(1500,560,1825),
    };

    double luma_pepto[16] = {

        // Taken from Pepto's Colodore palette
        LUMA_COLORES(0),  LUMA_COLORES(32),
        LUMA_COLORES(10), LUMA_COLORES(20),
        LUMA_COLORES(12), LUMA_COLORES(16),
        LUMA_COLORES(8),  LUMA_COLORES(24),
        LUMA_COLORES(12), LUMA_COLORES(8),
        LUMA_COLORES(16), LUMA_COLORES(10),
        LUMA_COLORES(15), LUMA_COLORES(24),
        LUMA_COLORES(15), LUMA_COLORES(20)
    };

    double *luma;
    switch(vic.getConfig().revision) {

        case VICII_PAL_6569_R1:

            luma = luma_vice_6569_r1;
            break;

        case VICII_PAL_6569_R3:

            luma = luma_vice_6569_r3;
            break;

        case VICII_NTSC_6567:

            luma = luma_vice_6567;
            break;

        case VICII_NTSC_6567_R56A:

            luma = luma_vice_6567_r65a;
            break;

        case VICII_PAL_8565:
        case VICII_NTSC_8562:

            luma = luma_pepto;
            break;

        default:
            fatalError;
    }

    // Angles in the color plane
#define ANGLE_PEPTO(x) (x * 22.5 * M_PI / 180.0)
#define ANGLE_COLORES(x) ((x * 22.5 + 11.5) * M_PI / 180.0)

    // Pepto's first approach
    // http://unusedino.de/ec64/technical/misc/vic656x/colors/

    /*
     double angle[16] = {
     NAN,            NAN,
     ANGLE_PEPTO(5), ANGLE_PEPTO(13),
     ANGLE_PEPTO(2), ANGLE_PEPTO(10),
     ANGLE_PEPTO(0), ANGLE_PEPTO(8),
     ANGLE_PEPTO(6), ANGLE_PEPTO(7),
     ANGLE_PEPTO(5), NAN,
     NAN,            ANGLE_PEPTO(10),
     ANGLE_PEPTO(0), NAN
     };
     */

    // Pepto's second approach
    // http://www.pepto.de/projects/colorvic/

    double angle[16] = {
        NAN,               NAN,
        ANGLE_COLORES(4),  ANGLE_COLORES(12),
        ANGLE_COLORES(2),  ANGLE_COLORES(10),
        ANGLE_COLORES(15), ANGLE_COLORES(7),
        ANGLE_COLORES(5),  ANGLE_COLORES(6),
        ANGLE_COLORES(4),  NAN,
        NAN,               ANGLE_COLORES(10),
        ANGLE_COLORES(15), NAN
    };

    //
    // Compute YUV values (adapted from Pepto)
    //

    // Normalize
    double brightness = config.brightness - 50.0;
    double contrast = config.contrast / 100.0 + 0.2;
    double saturation = config.saturation / 1.25;

    // Compute Y, U, and V
    double ang = angle[nr];
    y = luma[nr];
    u = std::isnan(ang) ? 0 : cos(ang) * saturation;
    v = std::isnan(ang) ? 0 : sin(ang) * saturation;

    // Apply brightness and contrast
    y *= contrast;
    u *= contrast;
    v *= contrast;
    y += brightness;

    // Translate to monochrome if applicable
    switch(palette) {

        case PALETTE_BLACK_WHITE:
            u = 0.0;
            v = 0.0;
            break;

        case PALETTE_PAPER_WHITE:
            u = -128.0 + 120.0;
            v = -128.0 + 133.0;
            break;

        case PALETTE_GREEN:
            u = -128.0 + 29.0;
            v = -128.0 + 64.0;
            break;

        case PALETTE_AMBER:
            u = -128.0 + 24.0;
            v = -128.0 + 178.0;
            break;

        case PALETTE_SEPIA:
            u = -128.0 + 97.0;
            v = -128.0 + 154.0;
            break;

        default:
            assert(palette == PALETTE_COLOR);
    }

    // Convert YUV value to RGB
    double r = y             + 1.140 * v;
    double g = y - 0.396 * u - 0.581 * v;
    double b = y + 2.029 * u;
    r = std::clamp(r, 0.0, 255.0);
    g = std::clamp(g, 0.0, 255.0);
    b = std::clamp(b, 0.0, 255.0);

    // Apply Gamma correction for PAL models
    if (vic.pal()) {

        r = gammaCorrect(r, 2.8, 2.2);
        g = gammaCorrect(g, 2.8, 2.2);
        b = gammaCorrect(b, 2.8, 2.2);
    }

    return LO_LO_HI_HI((u8)r, (u8)g, (u8)b, 0xFF);
}

}
