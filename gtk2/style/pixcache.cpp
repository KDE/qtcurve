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

namespace QtCurve {

typedef struct {
    GdkColor col;
    double shade;
} QtcPixKey;

static GHashTable *_pixbufTable = NULL;
static GdkPixbuf *_blankPixbuf = NULL;

static GdkPixbuf*
getBlankPixbuf()
{
    if (!_blankPixbuf) {
        _blankPixbuf = gdk_pixbuf_new_from_inline(-1, blank16x16, true, NULL);
    }
    return _blankPixbuf;
}

static unsigned
pixbufCacheHashKey(const void *k)
{
    const QtcPixKey *key = (const QtcPixKey*)k;
    int hash = (((key->col.red >> 8) << 16) + ((key->col.green >> 8) << 8) +
                (key->col.blue >> 8));
    return g_int_hash(&hash);
}

static gboolean
pixbufCacheKeyEqual(const void *k1, const void *k2)
{
    return memcmp(k1, k2, sizeof(QtcPixKey)) == 0;
}

static inline GHashTable*
getPixbufTable()
{
    if (!_pixbufTable) {
        _pixbufTable = g_hash_table_new_full(pixbufCacheHashKey,
                                             pixbufCacheKeyEqual,
                                             g_free, g_object_unref);
    }
    return _pixbufTable;
}

__attribute__((constructor)) static void
_qtcPixcacheInit()
{
    getPixbufTable();
    getBlankPixbuf();
}

__attribute__((destructor)) static void
_qtcPixcacheDone()
{
    if (_pixbufTable) {
        g_hash_table_destroy(_pixbufTable);
        _pixbufTable = NULL;
    }
    if (_blankPixbuf) {
        g_object_unref(_blankPixbuf);
        _blankPixbuf = NULL;
    }
}

static GdkPixbuf*
pixbufCacheValueNew(const QtcPixKey *key)
{
    GdkPixbuf *res = gdk_pixbuf_new_from_inline(-1, opts.xCheck ? check_x_on :
                                                check_on, true, NULL);
    qtcAdjustPix(gdk_pixbuf_get_pixels(res), gdk_pixbuf_get_n_channels(res),
                 gdk_pixbuf_get_width(res), gdk_pixbuf_get_height(res),
                 gdk_pixbuf_get_rowstride(res),
                 key->col.red >> 8, key->col.green >> 8,
                 key->col.blue >> 8, key->shade, QTC_PIXEL_GDK);
    return res;
}

GdkPixbuf*
getPixbuf(GdkColor *widgetColor, EPixmap p, double shade)
{
    if (p != PIX_CHECK) {
        return getBlankPixbuf();
    }
    const QtcPixKey key = {
        .col = *widgetColor,
        .shade = shade
    };
    GHashTable *table = getPixbufTable();
    GdkPixbuf *pixbuf = (GdkPixbuf*)g_hash_table_lookup(table, &key);
    if (pixbuf) {
        return pixbuf;
    }
    // TODO: Thread safe?
    pixbuf = pixbufCacheValueNew(&key);
    g_hash_table_insert(table, g_memdup(&key, sizeof(key)), pixbuf);
    return pixbuf;
}

}
