/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
 *   Copyright 2013 - 2015 Yichao Yu <yyc1992@gmail.com>                     *
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

#define __QTC_UTILS_OPTIONS_INTERNAL__

#include "options.h"
#include "map.h"

namespace QtCurve {
namespace Config {

template<>
QTC_EXPORT Shading
loadValue<Shading>(const char *str, Shading def)
{
    static const StrMap<Shading> map{
        {"simple", Shading::Simple},
        {"hsl", Shading::HSL},
        {"hsv", Shading::HSV},
        {"hcy", Shading::HCY},
    };
    return map.search(str, def);
}
template Shading loadValue<Shading>(const char *str, Shading def);

template<>
QTC_EXPORT EScrollbar
loadValue<EScrollbar>(const char *str, EScrollbar def)
{
    static const StrMap<EScrollbar> map{
        {"kde", SCROLLBAR_KDE},
        {"windows", SCROLLBAR_WINDOWS},
        {"platinum", SCROLLBAR_PLATINUM},
        {"next", SCROLLBAR_NEXT},
        {"none", SCROLLBAR_NONE},
    };
    return map.search(str, def);
}
template EScrollbar loadValue<EScrollbar>(const char *str, EScrollbar def);

}
}
