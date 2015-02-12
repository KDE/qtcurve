/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
 *   Copyright 2013 - 2014 Yichao Yu <yyc1992@gmail.com>                     *
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

#ifndef QTC_UTILS_OPTIONS_H
#define QTC_UTILS_OPTIONS_H

#include "number.h"

enum class Shading {
    Simple,
    HSL,
    HSV,
    HCY,
};

enum EScrollbar {
    SCROLLBAR_KDE,
    SCROLLBAR_WINDOWS,
    SCROLLBAR_PLATINUM,
    SCROLLBAR_NEXT,
    SCROLLBAR_NONE
};

enum EAppearance {
    APPEARANCE_CUSTOM1,
    APPEARANCE_CUSTOM2,
    APPEARANCE_CUSTOM3,
    APPEARANCE_CUSTOM4,
    APPEARANCE_CUSTOM5,
    APPEARANCE_CUSTOM6,
    APPEARANCE_CUSTOM7,
    APPEARANCE_CUSTOM8,
    APPEARANCE_CUSTOM9,
    APPEARANCE_CUSTOM10,
    APPEARANCE_CUSTOM11,
    APPEARANCE_CUSTOM12,
    APPEARANCE_CUSTOM13,
    APPEARANCE_CUSTOM14,
    APPEARANCE_CUSTOM15,
    APPEARANCE_CUSTOM16,
    APPEARANCE_CUSTOM17,
    APPEARANCE_CUSTOM18,
    APPEARANCE_CUSTOM19,
    APPEARANCE_CUSTOM20,
    APPEARANCE_CUSTOM21,
    APPEARANCE_CUSTOM22,
    APPEARANCE_CUSTOM23,

    NUM_CUSTOM_GRAD,

    APPEARANCE_FLAT = NUM_CUSTOM_GRAD,
    APPEARANCE_RAISED,
    APPEARANCE_DULL_GLASS,
    APPEARANCE_SHINY_GLASS,
    APPEARANCE_AGUA,
    APPEARANCE_SOFT_GRADIENT,
    APPEARANCE_GRADIENT,
    APPEARANCE_HARSH_GRADIENT,
    APPEARANCE_INVERTED,
    APPEARANCE_DARK_INVERTED,
    APPEARANCE_SPLIT_GRADIENT,
    APPEARANCE_BEVELLED,
    APPEARANCE_FADE, /* Only for poupmenu items! */
    APPEARANCE_STRIPED = APPEARANCE_FADE, /* Only for windows  and menus */
    APPEARANCE_NONE = APPEARANCE_FADE, /* Only for titlebars */
    APPEARANCE_FILE, /* Only for windows  and menus */
    APPEARANCE_LV_BEVELLED, /* To be used only with qtcGetGradient */
    APPEARANCE_AGUA_MOD,
    APPEARANCE_LV_AGUA,
    NUM_STD_APP = (APPEARANCE_LV_AGUA - NUM_CUSTOM_GRAD) + 1
};

enum EFrame {
    FRAME_NONE,
    FRAME_PLAIN,
    FRAME_LINE,
    FRAME_SHADED,
    FRAME_FADED
};

enum EGradientBorder {
    GB_NONE,
    GB_LIGHT,
    GB_3D,
    GB_3D_FULL,
    GB_SHINE
};

enum ECornerBits {
    CORNER_TL = 1 << 0,
    CORNER_TR = 1 << 1,
    CORNER_BR = 1 << 2,
    CORNER_BL = 1 << 3,

    ROUNDED_NONE = 0,
    ROUNDED_TOP = CORNER_TL | CORNER_TR,
    ROUNDED_BOTTOM = CORNER_BL | CORNER_BR,
    ROUNDED_LEFT = CORNER_TL | CORNER_BL,
    ROUNDED_RIGHT = CORNER_TR | CORNER_BR,
    ROUNDED_TOPRIGHT = CORNER_TR,
    ROUNDED_BOTTOMRIGHT = CORNER_BR,
    ROUNDED_TOPLEFT = CORNER_TL,
    ROUNDED_BOTTOMLEFT = CORNER_BL,
    ROUNDED_ALL = CORNER_TL | CORNER_TR | CORNER_BR | CORNER_BL,
};

QTC_ALWAYS_INLINE static inline bool
qtcUseBorder(EGradientBorder border)
{
    return QtCurve::noneOf(border, GB_SHINE, GB_NONE);
}

QTC_ALWAYS_INLINE static inline bool
qtcNoFrame(EFrame frame)
{
    return QtCurve::oneOf(frame, FRAME_NONE, FRAME_LINE);
}

QTC_ALWAYS_INLINE static inline bool
qtcIsFlatBgnd(EAppearance appear)
{
    return QtCurve::oneOf(appear, APPEARANCE_FLAT, APPEARANCE_RAISED);
}

QTC_ALWAYS_INLINE static inline bool
qtcIsFlat(EAppearance appear)
{
    return qtcIsFlatBgnd(appear) || appear == APPEARANCE_FADE;
}

/**
 * Number of scrollbar buttons.
 **/
QTC_ALWAYS_INLINE static inline int
qtcScrollbarButtonNum(EScrollbar type)
{
    switch (type) {
    default:
    case SCROLLBAR_KDE:
        return 3;
    case SCROLLBAR_WINDOWS:
    case SCROLLBAR_PLATINUM:
    case SCROLLBAR_NEXT:
        return 2;
    case SCROLLBAR_NONE:
        return 0;
    }
}

/**
 * Number of scrollbar button we leave space for.
 * Some applications (e.g. choqok) use the minimum size of scrollbar to
 * determine the height of input area. Setting a lower limit on this makes
 * sure that the calculated minimum size is not too small.
 * See https://github.com/QtCurve/qtcurve-qt4/issues/7
 * and https://bugs.kde.org/show_bug.cgi?id=317690
 **/
QTC_ALWAYS_INLINE static inline int
qtcScrollbarButtonNumSize(EScrollbar type)
{
    return qtcMax(qtcScrollbarButtonNum(type), 2);
}

namespace QtCurve {
namespace Config {

template<typename T> T loadValue(const char *str, T def);

#define _QTC_CONFIG_DEF_LOAD_VALUE(type, str, def)              \
    type loadValue<type>(const char *str, type def)

#define QTC_CONFIG_DEF_LOAD_VALUE(type)                 \
    template _QTC_CONFIG_DEF_LOAD_VALUE(type, str, def)

#ifndef __QTC_UTILS_OPTIONS_INTERNAL__
extern QTC_CONFIG_DEF_LOAD_VALUE(Shading);
extern QTC_CONFIG_DEF_LOAD_VALUE(EScrollbar);
extern QTC_CONFIG_DEF_LOAD_VALUE(EFrame);
#endif

}

}


#endif
