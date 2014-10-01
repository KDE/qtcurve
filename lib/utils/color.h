/*****************************************************************************
 *   Copyright 2009 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
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

#ifndef QTC_UTILS_COLOR_H
#define QTC_UTILS_COLOR_H

#include "utils.h"
#include "options.h"

#ifdef __cplusplus
#include <cmath>
// Using c99 function in c++ mode seems to cause trouble on some OSX versions.
#define _isnan(x) std::isnan(x)
#else
#define _isnan(x) isnan(x)
#endif

QTC_BEGIN_DECLS

typedef struct {
    double red;
    double green;
    double blue;
} QtcColor;

typedef struct {
    double h;
    double c;
    double y;
} QtcColorHCY;

extern double qtc_ring_alpha[3];

// use 709 for now
#define HCY_REC 709
#if HCY_REC == 601
static const double _qtc_yc[3] = {0.299, 0.587, 0.114};
#elif HCY_REC == 709
static const double _qtc_yc[3] = {0.2126, 0.7152, 0.0722};
#else // use Qt values
static const double _qtc_yc[3] = {0.34375, 0.5, 0.15625};
#endif
#undef HCY_REC

QTC_ALWAYS_INLINE static inline double
qtcColorWrap(double a, double d)
{
    double r = fmod(a, d);
    return (r < 0.0 ? d + r : (r > 0.0 ? r : 0.0));
}

QTC_ALWAYS_INLINE static inline double
qtcColorMixF(double a, double b, double bias)
{
    return a + (b - a) * bias;
}

QTC_ALWAYS_INLINE static inline double
qtcColorHCYGamma(double n)
{
    return pow(qtcBound(0, n, 1), 2.2);
}

QTC_ALWAYS_INLINE static inline double
qtcColorHCYIGamma(double n)
{
    return pow(qtcBound(0, n, 1), 1.0 / 2.2);
}

QTC_ALWAYS_INLINE static inline double
qtcColorHCYLumag(double r, double g, double b)
{
    return r * _qtc_yc[0] + g * _qtc_yc[1] + b * _qtc_yc[2];
}

QTC_ALWAYS_INLINE static inline void
qtcColorFill(QtcColor *color, double r, double g, double b)
{
    color->red = r;
    color->green = g;
    color->blue = b;
}

QTC_ALWAYS_INLINE static inline double
qtcColorHCYLuma(const QtcColor *color)
{
    return qtcColorHCYLumag(qtcColorHCYGamma(color->red),
                            qtcColorHCYGamma(color->green),
                            qtcColorHCYGamma(color->blue));
}

static inline void
qtcHsvToRgb(double *r, double *g, double *b, double h, double s, double v)
{
    if (0 == s) {
        *r = *g = *b = v;
    } else {
        int i;
        double f;
        double p;

        h /= 60; /* sector 0 to 5 */
        i = (int)floor(h);
        f = h - i; /* factorial part of h */
        p = v * (1 - s);
        switch (i) {
        case 0:
            *r = v;
            *g = v * (1 - s * (1 - f));
            *b = p;
            break;
        case 1:
            *r = v * (1 - s * f);
            *g = v;
            *b = p;
            break;
        case 2:
            *r = p;
            *g = v;
            *b = v * (1 - s * (1 - f));
            break;
        case 3:
            *r = p;
            *g = v * (1 - s * f);
            *b = v;
            break;
        case 4:
            *r = v * (1 - s * (1 - f));
            *g = p;
            *b = v;
            break;
        case 5:
        default:
            *r = v;
            *g = p;
            *b = v * (1 - s * f);
            break;
        }
    }
}

static inline void
qtcRgbToHsv(double r, double g, double b, double *h, double *s, double *v)
{
    double min = qtcMin(qtcMin(r, g), b);
    double max = qtcMax(qtcMax(r, g), b);
    double delta = max - min;

    *v = max;
    if (max != 0) {
        *s = delta / max;
    } else {
        *s = 0;
    }

    if (*s == 0.0) {
        *h = 0.0;
    } else {
        if (r == max) {
            *h = (g - b) / delta; /* between yellow & magenta */
        } else if (g == max) {
            *h = 2 + (b - r) / delta; /* between cyan & yellow */
        } else {
            *h = 4 + (r - g) / delta; /* between magenta & cyan */
        }
        *h *= 60; /* degrees */
        if (*h < 0) {
            *h += 360;
        }
    }
}

void _qtcColorLighten(QtcColor *color, double ky, double kc);
void _qtcColorDarken(QtcColor *color, double ky, double kc);
void _qtcColorShade(QtcColor *color, double ky, double kc);
void _qtcColorTint(const QtcColor *base, const QtcColor *col,
                   double amount, QtcColor *out);
void _qtcColorMix(const QtcColor *c1, const QtcColor *c2,
                  double bias, QtcColor *out);
void _qtcShade(const QtcColor *ca, QtcColor *cb, double k, EShading shading);
double _qtcShineAlpha(const QtcColor *bgnd);
void _qtcCalcRingAlphas(const QtcColor *bgnd);
void qtcColorFromStr(QtcColor *color, const char *str);
void qtcColorToStr(const QtcColor *color, char *str);

typedef enum {
    QTC_PIXEL_ARGB,
    QTC_PIXEL_BGRA,
    QTC_PIXEL_RGBA,
    QTC_PIXEL_GDK = QTC_PIXEL_RGBA,
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    QTC_PIXEL_XCB = QTC_PIXEL_ARGB,
    QTC_PIXEL_QT = QTC_PIXEL_ARGB,
#else
    QTC_PIXEL_XCB = QTC_PIXEL_BGRA,
    QTC_PIXEL_QT = QTC_PIXEL_BGRA,
#endif
} QtcPixelByteOrder;
void qtcAdjustPix(unsigned char *data, int numChannels, int w, int h,
                  int stride, int ro, int go, int bo, double shade,
                  QtcPixelByteOrder byte_order);

QTC_END_DECLS

#ifndef QTC_UTILS_INTERNAL

// Use __cplusplus to distinguish between gtk and qt for now.
#if defined QTC_UTILS_QT5 || defined QTC_UTILS_QT4
#include <QColor>

QTC_ALWAYS_INLINE static inline QColor
qtcColorLighten(const QColor *color, double ky, double kc)
{
    QtcColor qtc_color = {color->redF(), color->greenF(), color->blueF()};
    _qtcColorLighten(&qtc_color, ky, kc);
    return QColor::fromRgbF(qtc_color.red, qtc_color.green, qtc_color.blue);
}

QTC_ALWAYS_INLINE static inline QColor
qtcColorDarken(const QColor *color, double ky, double kc)
{
    QtcColor qtc_color = {color->redF(), color->greenF(), color->blueF()};
    _qtcColorDarken(&qtc_color, ky, kc);
    return QColor::fromRgbF(qtc_color.red, qtc_color.green, qtc_color.blue);
}

QTC_ALWAYS_INLINE static inline QColor
qtcColorShade(const QColor *color, double ky, double kc)
{
    QtcColor qtc_color = {color->redF(), color->greenF(), color->blueF()};
    _qtcColorShade(&qtc_color, ky, kc);
    return QColor::fromRgbF(qtc_color.red, qtc_color.green, qtc_color.blue);
}

QTC_ALWAYS_INLINE static inline QColor
qtcColorTint(const QColor *base, const QColor *col, double amount)
{
    if (amount <= 0.0) {
        return *base;
    } else if (amount >= 1.0) {
        return *col;
    } else if (std::isnan(amount)) {
        return *base;
    }
    const QtcColor qtc_base = {base->redF(), base->greenF(), base->blueF()};
    const QtcColor qtc_col = {col->redF(), col->greenF(), col->blueF()};
    QtcColor out;
    _qtcColorTint(&qtc_base, &qtc_col, amount, &out);
    return QColor::fromRgbF(out.red, out.green, out.blue);
}

QTC_ALWAYS_INLINE static inline QColor
qtcColorMix(const QColor *c1, const QColor *c2, double bias)
{
    if (bias <= 0.0) {
        return *c1;
    } else if (bias >= 1.0) {
        return *c2;
    } else if (std::isnan(bias)) {
        return *c1;
    }
    const QtcColor qtc_c1 = {c1->redF(), c1->greenF(), c1->blueF()};
    const QtcColor qtc_c2 = {c2->redF(), c2->greenF(), c2->blueF()};
    QtcColor out;
    _qtcColorMix(&qtc_c1, &qtc_c2, bias, &out);
    return QColor::fromRgbF(out.red, out.green, out.blue);
}

QTC_ALWAYS_INLINE static inline double
qtcColorLuma(const QColor *color)
{
    QtcColor qtc_color = {color->redF(), color->greenF(), color->blueF()};
    return qtcColorHCYLuma(&qtc_color);
}

QTC_ALWAYS_INLINE static inline void
qtcShade(const QColor *ca, QColor *cb, double k, EShading shading)
{
    if (qtcEqual(k, 1.0)) {
        *cb = *ca;
        return;
    }
    const QtcColor qtc_ca = {ca->redF(), ca->greenF(), ca->blueF()};
    QtcColor qtc_cb;
    _qtcShade(&qtc_ca, &qtc_cb, k, shading);
    cb->setRgbF(qtc_cb.red, qtc_cb.green, qtc_cb.blue, ca->alphaF());
}

QTC_ALWAYS_INLINE static inline double
qtcShineAlpha(const QColor *bgnd)
{
    const QtcColor qtc_bgnd = {bgnd->redF(), bgnd->greenF(), bgnd->blueF()};
    return _qtcShineAlpha(&qtc_bgnd);
}

QTC_ALWAYS_INLINE static inline void
qtcCalcRingAlphas(const QColor *bgnd)
{
    const QtcColor qtc_bgnd = {bgnd->redF(), bgnd->greenF(), bgnd->blueF()};
    _qtcCalcRingAlphas(&qtc_bgnd);
}
#endif

#if defined QTC_UTILS_GTK2 ||  defined QTC_UTILS_GTK3
#include <gdk/gdk.h>

QTC_ALWAYS_INLINE static inline GdkColor
_qtcColorToGdk(const QtcColor *qtc_color)
{
    GdkColor color;
    color.red = qtc_color->red * 65535;
    color.green = qtc_color->green * 65535;
    color.blue = qtc_color->blue * 65535;
    return color;
}

QTC_ALWAYS_INLINE static inline QtcColor
_qtc_color_from_gdk(const GdkColor *color)
{
    QtcColor qtc_color;
    qtc_color.red = color->red / 65535.0;
    qtc_color.green = color->green / 65535.0;
    qtc_color.blue = color->blue / 65535.0;
    return qtc_color;
}

QTC_ALWAYS_INLINE static inline GdkColor
qtcColorLighten(const GdkColor *color, double ky, double kc)
{
    QtcColor qtc_color = _qtc_color_from_gdk(color);
    _qtcColorLighten(&qtc_color, ky, kc);
    return _qtcColorToGdk(&qtc_color);
}

QTC_ALWAYS_INLINE static inline GdkColor
qtcColorDarken(const GdkColor *color, double ky, double kc)
{
    QtcColor qtc_color = _qtc_color_from_gdk(color);
    _qtcColorDarken(&qtc_color, ky, kc);
    return _qtcColorToGdk(&qtc_color);
}

QTC_ALWAYS_INLINE static inline GdkColor
qtcColorShade(const GdkColor *color, double ky, double kc)
{
    QtcColor qtc_color = _qtc_color_from_gdk(color);
    _qtcColorShade(&qtc_color, ky, kc);
    return _qtcColorToGdk(&qtc_color);
}

QTC_ALWAYS_INLINE static inline GdkColor
qtcColorTint(const GdkColor *base, const GdkColor *col, double amount)
{
    if (amount <= 0.0) {
        return *base;
    } else if (amount >= 1.0) {
        return *col;
    } else if (_isnan(amount)) {
        return *base;
    }
    QtcColor qtc_base = _qtc_color_from_gdk(base);
    QtcColor qtc_col = _qtc_color_from_gdk(col);
    QtcColor out;
    _qtcColorTint(&qtc_base, &qtc_col, amount, &out);
    return _qtcColorToGdk(&out);
}

QTC_ALWAYS_INLINE static inline GdkColor
qtcColorMix(const GdkColor *c1, const GdkColor *c2, double bias)
{
    if (bias <= 0.0) {
        return *c1;
    } else if (bias >= 1.0) {
        return *c2;
    } else if (_isnan(bias)) {
        return *c1;
    }
    QtcColor qtc_c1 = _qtc_color_from_gdk(c1);
    QtcColor qtc_c2 = _qtc_color_from_gdk(c2);
    QtcColor out;
    _qtcColorMix(&qtc_c1, &qtc_c2, bias, &out);
    return _qtcColorToGdk(&out);
}

QTC_ALWAYS_INLINE static inline double
qtcColorLuma(const GdkColor *color)
{
    QtcColor qtc_color = _qtc_color_from_gdk(color);
    return qtcColorHCYLuma(&qtc_color);
}

QTC_ALWAYS_INLINE static inline void
qtcShade(const GdkColor *ca, GdkColor *cb, double k, EShading shading)
{
    if (qtcEqual(k, 1.0)) {
        *cb = *ca;
        return;
    }
    QtcColor qtc_ca = _qtc_color_from_gdk(ca);
    QtcColor qtc_cb;
    _qtcShade(&qtc_ca, &qtc_cb, k, shading);
    *cb = _qtcColorToGdk(&qtc_cb);
}

QTC_ALWAYS_INLINE static inline double
qtcShineAlpha(const GdkColor *bgnd)
{
    QtcColor qtc_bgnd = _qtc_color_from_gdk(bgnd);
    return _qtcShineAlpha(&qtc_bgnd);
}

QTC_ALWAYS_INLINE static inline void
qtcCalcRingAlphas(const GdkColor *bgnd)
{
    QtcColor qtc_bgnd = _qtc_color_from_gdk(bgnd);
    _qtcCalcRingAlphas(&qtc_bgnd);
}

#endif

#endif

#endif
