/*****************************************************************************
 *   Copyright 2009 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
 *   Copyright 2013 - 2013 Yichao Yu <yyc1992@gmail.com>                     *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU Lesser General Public License as          *
 *   published by the Free Software Foundation; either version 2.1 of the    *
 *   License, or (at your option) version 3, or any later version accepted   *
 *   by the membership of KDE e.V. (or its successor approved by the         *
 *   membership of KDE e.V.), which shall act as a proxy defined in          *
 *   Section 6 of version 3 of the license.                                  *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 *   Lesser General Public License for more details.                         *
 *                                                                           *
 *   You should have received a copy of the GNU Lesser General Public        *
 *   License along with this library. If not,                                *
 *   see <http://www.gnu.org/licenses/>.                                     *
 *****************************************************************************/

#ifndef QTC_UTILS_SHADE_H
#define QTC_UTILS_SHADE_H

#include "utils.h"
#include "options.h"

#define QTC_NUM_STD_SHADES (6)
/* 3d effect - i.e. buttons, etc */
static const double qtc_intern_shades[2][11][QTC_NUM_STD_SHADES] = {
    {
        /* HSV & HSL */
        {1.05, 1.04, 0.90, 0.800, 0.830, 0.82},
        {1.06, 1.04, 0.90, 0.790, 0.831, 0.78},
        {1.07, 1.04, 0.90, 0.785, 0.832, 0.75},
        {1.08, 1.05, 0.90, 0.782, 0.833, 0.72},
        {1.09, 1.05, 0.90, 0.782, 0.834, 0.70},
        {1.10, 1.06, 0.90, 0.782, 0.836, 0.68},
        {1.12, 1.06, 0.90, 0.782, 0.838, 0.63},
        {1.16, 1.07, 0.90, 0.782, 0.840, 0.62}, /* default */
        {1.18, 1.07, 0.90, 0.783, 0.842, 0.60},
        {1.20, 1.08, 0.90, 0.784, 0.844, 0.58},
        {1.22, 1.08, 0.90, 0.786, 0.848, 0.55}
    }, { /* SIMPLE */
        {1.07, 1.03, 0.91, 0.780, 0.834, 0.75},
        {1.08, 1.03, 0.91, 0.781, 0.835, 0.74},
        {1.09, 1.03, 0.91, 0.782, 0.836, 0.73},
        {1.10, 1.04, 0.91, 0.783, 0.837, 0.72},
        {1.11, 1.04, 0.91, 0.784, 0.838, 0.71},
        {1.12, 1.05, 0.91, 0.785, 0.840, 0.70},
        {1.13, 1.05, 0.91, 0.786, 0.842, 0.69},
        {1.14, 1.06, 0.91, 0.787, 0.844, 0.68}, /* default */
        {1.16, 1.06, 0.91, 0.788, 0.846, 0.66},
        {1.18, 1.07, 0.91, 0.789, 0.848, 0.64},
        {1.20, 1.07, 0.91, 0.790, 0.850, 0.62}
    }
};

#define QTC_STD_BORDER (5)
#define QTC_DISABLED_BORDER QTC_STD_BORDER /* (3) */

QTC_ALWAYS_INLINE static inline double
qtcShadeGetIntern(int c, int s, bool darker, Shading shading)
{
    if (c > 10 || c < 0 || s >= QTC_NUM_STD_SHADES || s < 0) {
        return 1.0;
    }
    double shade =
        qtc_intern_shades[Shading::Simple == shading ? 1 : 0][c][s];
    if (darker && (QTC_STD_BORDER == s || QTC_DISABLED_BORDER == s)) {
        return shade - 0.1;
    }
    return shade;
}

#endif
