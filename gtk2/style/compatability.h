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

#ifndef __QTC_COMPATABILITY_H__
#define __QTC_COMPATABILITY_H__

#include <gtk/gtk.h>

#if GTK_CHECK_VERSION(2, 90, 0)

#define qtcRangeHasStepperA(W)               \
    (opts.scrollbarType != SCROLLBAR_NONE && \
     opts.scrollbarType != SCROLLBAR_PLATINUM)
#define qtcRangeHasStepperB(W) (opts.scrollbarType == SCROLLBAR_NEXT)
#define qtcRangeHasStepperC(W)              \
    (opts.scrollbarType == SCROLLBAR_KDE || \
     opts.scrollbarType == SCROLLBAR_PLATINUM)
#define qtcRangeHasStepperD(W)               \
    (opts.scrollbarType != SCROLLBAR_NONE && \
     opts.scrollbarType != SCROLLBAR_NEXT)

#else

#define qtcRangeHasStepperA(W) (GTK_RANGE(W)->has_stepper_a)
#define qtcRangeHasStepperB(W) (GTK_RANGE(W)->has_stepper_b)
#define qtcRangeHasStepperC(W) (GTK_RANGE(W)->has_stepper_c)
#define qtcRangeHasStepperD(W) (GTK_RANGE(W)->has_stepper_d)

#endif

#endif
