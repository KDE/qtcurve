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

#include "pixcache.h"
#include "check_on-png.h"
#include "check_x_on-png.h"
#include "blank16x16-png.h"
#include "qt_settings.h"

#include <qtcurve-utils/gtkutils.h>

#include <unordered_map>

namespace QtCurve {

struct PixKey {
    GdkColor col;
    double shade;
};

struct PixHash {
    size_t
    operator()(const PixKey &key) const
    {
        const GdkColor &col = key.col;
        return (std::hash<int>()(col.red) ^
                (std::hash<int>()(col.green) << 1) ^
                (std::hash<int>()(col.blue) << 2) ^
                (std::hash<double>()(key.shade) << 3));
    }
};

struct PixEqual {
    bool
    operator()(const PixKey &lhs, const PixKey &rhs) const
    {
        return memcmp(&lhs, &rhs, sizeof(PixKey)) == 0;
    }
};

static std::unordered_map<PixKey, GObjPtr<GdkPixbuf>,
                          PixHash, PixEqual> pixbufMap;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
// Replacement isn't available until the version it is deprecated
// (gdk-pixbuf-2.0 2.32)
static GObjPtr<GdkPixbuf> blankPixbuf =
    gdk_pixbuf_new_from_inline(-1, blank16x16, true, nullptr);
#pragma GCC diagnostic pop

static GdkPixbuf*
pixbufCacheValueNew(const PixKey &key)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    // Replacement isn't available until the version it is deprecated
    // (gdk-pixbuf-2.0 2.32)
    GdkPixbuf *res = gdk_pixbuf_new_from_inline(-1, opts.xCheck ? check_x_on :
                                                check_on, true, nullptr);
#pragma GCC diagnostic pop
    qtcAdjustPix(gdk_pixbuf_get_pixels(res), gdk_pixbuf_get_n_channels(res),
                 gdk_pixbuf_get_width(res), gdk_pixbuf_get_height(res),
                 gdk_pixbuf_get_rowstride(res),
                 key.col.red >> 8, key.col.green >> 8,
                 key.col.blue >> 8, key.shade, QTC_PIXEL_GDK);
    return res;
}

GdkPixbuf*
getPixbuf(GdkColor *widgetColor, EPixmap p, double shade)
{
    if (p != PIX_CHECK) {
        return blankPixbuf.get();
    }
    const PixKey key = {*widgetColor, shade};
    auto &pixbuf = pixbufMap[key];
    if (pixbuf.get() == nullptr) {
        pixbuf = pixbufCacheValueNew(key);
    }
    return pixbuf.get();
}

}
