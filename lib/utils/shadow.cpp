/*****************************************************************************
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

#include "shadow_p.h"
#include "log.h"

static void
qtcCreateShadowGradient(float *buff, size_t size)
{
    const float r = size / 6.5;
    for (size_t i = 0;i < size;i++) {
        buff[i] = qtcMax(0, expf(-(i / r)) - 0.0015);
    }
}

static void
qtcFillShadowPixel(uint8_t *pixel, const QtcColor *c1,
                   const QtcColor *c2, double bias, QtcPixelByteOrder order)
{
    uint8_t alpha = qtcBound(0, 0xff * bias, 0xff);
    if (alpha == 0) {
        memset(pixel, 0, 4);
        return;
    }
    QtcColor color;
    // c1 is the start color, c2 the end color
    _qtcColorMix(c2, c1, bias, &color);
    uint8_t red = qtcBound(0, 0xff * color.red, 0xff) * alpha / 0xff;
    uint8_t green = qtcBound(0, 0xff * color.green, 0xff) * alpha / 0xff;
    uint8_t blue = qtcBound(0, 0xff * color.blue, 0xff) * alpha / 0xff;
    switch (order) {
    case QTC_PIXEL_ARGB:
        pixel[0] = alpha;
        pixel[1] = red;
        pixel[2] = green;
        pixel[3] = blue;
        break;
    case QTC_PIXEL_BGRA:
        pixel[0] = blue;
        pixel[1] = green;
        pixel[2] = red;
        pixel[3] = alpha;
        break;
    default:
    case QTC_PIXEL_RGBA:
        pixel[0] = red;
        pixel[1] = green;
        pixel[2] = blue;
        pixel[3] = alpha;
        break;
    }
}

static inline float
_qtcDistance(int x, int y, int x0, int y0, bool square)
{
    int dx = x - x0;
    int dy = y - y0;
    if (dx == 0) {
        return qtcAbs(dy);
    }
    if (dy == 0) {
        return qtcAbs(dx);
    }
    return square ? qtcMax(qtcAbs(dx), qtcAbs(dy)) : sqrtf(dx * dx + dy * dy);
}

static inline float
_qtcGradientGetValue(float *gradient, size_t size, float distance)
{
    if (distance < 0 || distance > size - 1) {
        return 0;
    }
    int index = floorf(distance);
    if (qtcEqual(index, distance)) {
        return gradient[index];
    }
    return (gradient[index] * (index + 1 - distance) +
            gradient[index + 1] * (distance - index));
}

static QtcImage*
qtcShadowSubImage(size_t size, float *gradient, int vertical_align,
                  int horizontal_align, const QtcColor *c1, const QtcColor *c2,
                  bool square, QtcPixelByteOrder order)
{
    int height = vertical_align ? size : 1;
    int y0 = vertical_align == -1 ? height - 1 : 0;
    int width = horizontal_align ? size : 1;
    int x0 = horizontal_align == -1 ? width - 1 : 0;
    QtcImage *res = qtcImageNew(width, height, 32);
    for (int x = 0;x < width;x++) {
        for (int y = 0;y < height;y++) {
            qtcFillShadowPixel(
                res->data + (x + y * width) * 4, c1, c2,
                _qtcGradientGetValue(
                    gradient, size, _qtcDistance(x, y, x0, y0, square)), order);
        }
    }
    return res;
}

void
qtcShadowCreate(size_t size, const QtcColor *c1, const QtcColor *c2,
                size_t radius, bool square, QtcPixelByteOrder order,
                QtcImage **images)
{
    size_t full_size = size + radius;
    QTC_DEF_LOCAL_BUFF(float, gradient, 128, full_size);
    for (size_t i = 0;i < radius;i++) {
        gradient.p[i] = 0;
    }
    qtcCreateShadowGradient(gradient.p + radius, size);
    int aligns[8][2] = {
        {0, -1},
        {1, -1},
        {1, 0},
        {1, 1},
        {0, 1},
        {-1, 1},
        {-1, 0},
        {-1, -1},
    };
    for (int i = 0;i < 8;i++) {
        images[i] = qtcShadowSubImage(full_size, gradient.p, aligns[i][1],
                                      aligns[i][0], c1, c2, square, order);
    }
    QTC_FREE_LOCAL_BUFF(gradient);
}
