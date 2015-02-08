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

#include "color.h"

static void
qtcColorHCYFromColor(const QtcColor *color, QtcColorHCY *hcy)
{
    double r = qtcColorHCYGamma(color->red);
    double g = qtcColorHCYGamma(color->green);
    double b = qtcColorHCYGamma(color->blue);

    // luma component
    hcy->y = qtcColorHCYLumag(r, g, b);

    // hue component
    double p = qtcMax(qtcMax(r, g), b);
    double n = qtcMin(qtcMin(r, g), b);
    double d = 6.0 * (p - n);
    if (n == p) {
        hcy->h = 0.0;
    } else if (r == p) {
        hcy->h = ((g - b) / d);
    } else if (g == p) {
        hcy->h = ((b - r) / d) + (1.0 / 3.0);
    } else {
        hcy->h = ((r - g) / d) + (2.0 / 3.0);
    }

    // chroma component
    if (0.0 == hcy->y || 1.0 == hcy->y) {
        hcy->c = 0.0;
    } else {
        hcy->c = qtcMax((hcy->y - n) / hcy->y, (p - hcy->y) / (1 - hcy->y));
    }
}

static void
qtcColorHCYToColor(const QtcColorHCY *hcy, QtcColor *color)
{
    // start with sane component values
    double _h = qtcColorWrap(hcy->h, 1);
    double _c = qtcBound(0, hcy->c, 1);
    double _y = qtcBound(0, hcy->y, 1);

    // calculate some needed variables
    double _hs = _h * 6.0, th, tm;
    if (_hs < 1.0) {
        th = _hs;
        tm = _qtc_yc[0] + _qtc_yc[1] * th;
    } else if (_hs < 2.0) {
        th = 2.0 - _hs;
        tm = _qtc_yc[1] + _qtc_yc[0] * th;
    } else if (_hs < 3.0) {
        th = _hs - 2.0;
        tm = _qtc_yc[1] + _qtc_yc[2] * th;
    } else if (_hs < 4.0) {
        th = 4.0 - _hs;
        tm = _qtc_yc[2] + _qtc_yc[1] * th;
    } else if (_hs < 5.0) {
        th = _hs - 4.0;
        tm = _qtc_yc[2] + _qtc_yc[0] * th;
    } else {
        th = 6.0 - _hs;
        tm = _qtc_yc[0] + _qtc_yc[2] * th;
    }

    // calculate RGB channels in sorted order
    double tn, to, tp;
    if (tm >= _y) {
        tp = _y + _y * _c * (1.0 - tm) / tm;
        to = _y + _y * _c * (th - tm) / tm;
        tn = _y - (_y * _c);
    } else {
        tp = _y + (1.0 - _y) * _c;
        to = _y + (1.0 - _y) * _c * (th - tm) / (1.0 - tm);
        tn = _y - (1.0 - _y) * _c * tm / (1.0 - tm);
    }

    tp = qtcColorHCYIGamma(tp);
    to = qtcColorHCYIGamma(to);
    tn = qtcColorHCYIGamma(tn);
    // return RGB channels in appropriate order
    if (_hs < 1.0) {
        qtcColorFill(color, tp, to, tn);
    } else if (_hs < 2.0) {
        qtcColorFill(color, to, tp, tn);
    } else if (_hs < 3.0) {
        qtcColorFill(color, tn, tp, to);
    } else if (_hs < 4.0) {
        qtcColorFill(color, tn, to, tp);
    } else if (_hs < 5.0) {
        qtcColorFill(color, to, tn, tp);
    } else {
        qtcColorFill(color, tp, tn, to);
    }
}

static double
qtcColorContrastRatio(const QtcColor *c1, const QtcColor *c2)
{
    double y1 = qtcColorHCYLuma(c1);
    double y2 = qtcColorHCYLuma(c2);
    if (y1 > y2) {
        return (y1 + 0.05) / (y2 + 0.05);
    } else {
        return (y2 + 0.05) / (y1 + 0.05);
    }
}

QTC_EXPORT void
_qtcColorLighten(QtcColor *color, double ky, double kc)
{
    QtcColorHCY hcy;
    qtcColorHCYFromColor(color, &hcy);

    hcy.y = 1.0 - qtcBound(0, (1.0 - hcy.y) * (1.0 - ky), 1);
    hcy.c = 1.0 - qtcBound(0, (1.0 - hcy.c) * kc, 1);
    qtcColorHCYToColor(&hcy, color);
}

QTC_EXPORT void
_qtcColorDarken(QtcColor *color, double ky, double kc)
{
    QtcColorHCY hcy;
    qtcColorHCYFromColor(color, &hcy);

    hcy.y = qtcBound(0, hcy.y * (1.0 - ky), 1);
    hcy.c = qtcBound(0, hcy.c * kc, 1);
    qtcColorHCYToColor(&hcy, color);
}

QTC_EXPORT void
_qtcColorShade(QtcColor *color, double ky, double kc)
{
    QtcColorHCY hcy;
    qtcColorHCYFromColor(color, &hcy);

    hcy.y = qtcBound(0, hcy.y + ky, 1);
    hcy.c = qtcBound(0, hcy.c + kc, 1);
    return qtcColorHCYToColor(&hcy, color);
}

static void
qtcColorTintHelper(const QtcColor *base, const QtcColor *col,
                   double amount, QtcColor *out)
{
    QtcColor mixed;
    _qtcColorMix(base, col, pow(amount, 0.3), &mixed);
    QtcColorHCY hcy;
    qtcColorHCYFromColor(&mixed, &hcy);
    hcy.y = qtcColorMixF(qtcColorHCYLuma(base), hcy.y, amount);

    qtcColorHCYToColor(&hcy, out);
}

QTC_EXPORT void
_qtcColorTint(const QtcColor *base, const QtcColor *col,
              double amount, QtcColor *out)
{
    if (qtcUnlikely(amount <= 0.0 || isnan(amount))) {
        *out = *base;
        return;
    }
    if (qtcUnlikely(amount >= 1.0)) {
        *out = *col;
        return;
    }

    double ri = qtcColorContrastRatio(base, col);
    double rg = 1.0 + ((ri + 1.0) * amount * amount * amount);
    double u = 1.0, l = 0.0;
    int i;
    for (i = 12;i;i--) {
        double a = 0.5 * (l + u);
        qtcColorTintHelper(base, col, a, out);
        double ra = qtcColorContrastRatio(base, out);
        if (ra > rg) {
            u = a;
        } else {
            l = a;
        }
    }
}

QTC_EXPORT void
_qtcColorMix(const QtcColor *c1, const QtcColor *c2, double bias, QtcColor *out)
{
    if (qtcUnlikely(bias <= 0.0 || isnan(bias))) {
        *out = *c1;
        return;
    }
    if (bias >= 1.0) {
        *out = *c2;
        return;
    }

    qtcColorFill(out, qtcColorMixF(c1->red, c2->red, bias),
                 qtcColorMixF(c1->green, c2->green, bias),
                 qtcColorMixF(c1->blue, c2->blue, bias));
}

static inline void
rgbToHsl(double r, double g, double b, double *h, double *s, double *l)
{
    double min = qtcMin(qtcMin(r, g), b);
    double max = qtcMax(qtcMax(r, g), b);

    *l = 0.5 * (max + min);
    *s = 0.0;
    *h = 0.0;

    if (max != min) {
        double delta = max - min;

        if (*l <= 0.5) {
            *s = delta / (max + min);
        } else {
            *s = delta / (2.0 - max - min);
        }

        if (r == max) {
            *h = (g - b) / delta;
        } else if (g == max) {
            *h = 2.0 + (b - r) / delta;
        } else if (b == max) {
            *h = 4.0 + (r - g) / delta;
        }

        *h /= 6.0;
        if (*h < 0.0) {
            (*h) += 1.0;
        }
    }
}

static inline double
h2c(double h, double m1, double m2)
{
    h = qtcColorWrap(h, 6.0);

    if (h < 1.0) {
        return qtcColorMixF(m1, m2, h);
    }
    if (h < 3.0) {
        return m2;
    }
    if (h < 4.0) {
        return qtcColorMixF(m1, m2, 4.0 - h);
    }
    return m1;
}

static inline void
hslToRgb(double h, double s, double l, double *r, double *g, double *b)
{
    double m1, m2;

    // TODO h2rgb( h, r, g, b );
    h *= 6.0;

    if (l <= 0.5) {
        m2 = l * (1.0 + s);
    } else {
        m2 = l + s * (1.0 - l);
    }
    m1 = 2.0 * l - m2;

    *r = h2c(h + 2.0, m1, m2);
    *g = h2c(h, m1, m2);
    *b = h2c(h - 2.0, m1, m2);
}

QTC_EXPORT void
_qtcShade(const QtcColor *ca, QtcColor *cb, double k, Shading shading)
{
    // Should be handled already by the wrapper inline functions.
    if (qtcUnlikely(qtcEqual(k, 1.0))) {
        *cb = *ca;
        return;
    }
    switch (shading) {
    case Shading::Simple: {
        double v = k - 1;
        qtcColorFill(cb, qtcLimit(ca->red + v, 1.0),
                     qtcLimit(ca->green + v, 1.0),
                     qtcLimit(ca->blue + v, 1.0));
        break;
    }
    case Shading::HSL: {
        double r = ca->red;
        double g = ca->green;
        double b = ca->blue;
        double h, s, l;

        rgbToHsl(r, g, b, &h, &s, &l);
        l = qtcBound(0, l * k, 1);
        s = qtcBound(0, s * k, 1);
        hslToRgb(h, s, l, &r, &g, &b);
        qtcColorFill(cb, qtcLimit(r, 1.0), qtcLimit(g, 1.0),
                     qtcLimit(b, 1.0));
        break;
    }
    case Shading::HSV: {
        double r = ca->red;
        double g = ca->green;
        double b = ca->blue;
        double h, s, v;

        qtcRgbToHsv(r, g, b, &h, &s, &v);
        v *= k;
        if (v > 1.0) {
            s -= v - 1.0;
            if (s < 0) {
                s = 0;
            }
            v = 1.0;
        }
        qtcHsvToRgb(&r, &g, &b, h, s, v);
        qtcColorFill(cb, qtcLimit(r, 1.0), qtcLimit(g, 1.0),
                     qtcLimit(b, 1.0));
        break;
    }
    case Shading::HCY:
#define HCY_FACTOR 0.15
        *cb = *ca;
        if (k > 1) {
            _qtcColorLighten(cb, (k * (1 + HCY_FACTOR)) - 1.0, 1.0);
        } else {
            _qtcColorDarken(cb, 1.0 - (k * (1 - HCY_FACTOR)), 1.0);
        }
    }
}

QTC_EXPORT double
_qtcShineAlpha(const QtcColor *bgnd)
{
    double h = 0, s = 0, v = 0;
    qtcRgbToHsv(bgnd->red, bgnd->green, bgnd->blue, &h, &s, &v);
    return v * 0.8;
}

QTC_EXPORT double qtc_ring_alpha[3] = {0.125, 0.125, 0.5};

QTC_EXPORT void
_qtcCalcRingAlphas(const QtcColor *bgnd)
{
    double r = bgnd->red;
    double g = bgnd->green;
    double b = bgnd->blue;
    double h = 0, s = 0, v = 0;
    qtcRgbToHsv(r, g, b, &h, &s, &v);
    qtc_ring_alpha[0] = v * 0.26;
    qtc_ring_alpha[1] = v * 0.14;
    qtc_ring_alpha[2] = v * 0.55;
}

QTC_EXPORT void
qtcAdjustPix(unsigned char *data, int numChannels, int w, int h, int stride,
             int ro, int go, int bo, double shade,
             QtcPixelByteOrder byte_order)
{
    int width = w * numChannels;
    int offset = 0;
    int r = (int)(ro * shade + 0.5);
    int g = (int)(go * shade + 0.5);
    int b = (int)(bo * shade + 0.5);

    for (int row = 0;row < h;row++) {
        for (int column = 0;column < width;column += numChannels) {
            unsigned char source = data[offset + column + 1];
            int new_r = qtcBound(0, r - source, 255);
            int new_g = qtcBound(0, g - source, 255);
            int new_b = qtcBound(0, b - source, 255);
            switch (byte_order) {
            case QTC_PIXEL_ARGB:
                /* ARGB */
                data[offset + column + 1] = new_r;
                data[offset + column + 2] = new_g;
                data[offset + column + 3] = new_b;
                break;
            case QTC_PIXEL_BGRA:
                /* BGRA */
                data[offset + column] = new_b;
                data[offset + column + 1] = new_g;
                data[offset + column + 2] = new_r;
                break;
            default:
            case QTC_PIXEL_RGBA:
                /* GdkPixbuf is RGBA */
                data[offset + column] = new_r;
                data[offset + column + 1] = new_g;
                data[offset + column + 2] = new_b;
                break;
            }
        }
        offset += stride;
    }
}

QTC_ALWAYS_INLINE static inline int
_c2h(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else {
        return 0;
    }
}

QTC_ALWAYS_INLINE static inline int
_2c2h(const char *s)
{
    return _c2h(s[0]) * 16 + _c2h(s[1]);
}

QTC_EXPORT void
qtcColorFromStr(QtcColor *color, const char *str)
{
    color->red = 0;
    color->green = 0;
    color->blue = 0;
    QTC_RET_IF_FAIL(str);
    str += strspn(str, " \t\b\n\f\v");
    if (str[0] == '#') {
        str++;
        size_t len = strlen(str);
        if (len >= 6) {
            color->red = _2c2h(str) / 255.0;
            color->green = _2c2h(str + 2) / 255.0;
            color->blue = _2c2h(str + 4) / 255.0;
            return;
        } else if (len >= 3) {
            color->red = _c2h(str[0]) / 15.0;
            color->green = _c2h(str[1]) / 15.0;
            color->blue = _c2h(str[2]) / 15.0;
            return;
        }
    }
}

QTC_EXPORT void
qtcColorToStr(const QtcColor *color, char *str)
{
    sprintf(str, "#%02X%02X%02X", (unsigned)(qtcBound(0, color->red, 1) * 255),
            (unsigned)(qtcBound(0, color->green, 1) * 255),
            (unsigned)(qtcBound(0, color->blue, 1) * 255));
}
