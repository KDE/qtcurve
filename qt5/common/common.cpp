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

#include <stdarg.h>
#include "common.h"

// DO NOT unconditionally include QT/GTK headers here

void
qtcSetupGradient(Gradient *grad, EGradientBorder border, int numStops, ...)
{
    va_list ap;
    int i;

    grad->border = border;
#ifndef QTC_UTILS_QT
    grad->numStops = numStops;
    grad->stops = qtcNew(GradientStop, numStops);
#endif
    va_start(ap, numStops);
    for (i = 0;i < numStops;++i) {
        double pos = va_arg(ap, double);
        double val = va_arg(ap, double);
#ifdef QTC_UTILS_QT
        grad->stops.insert(GradientStop(pos, val));
#else
        grad->stops[i].pos = pos;
        grad->stops[i].val = val;
        grad->stops[i].alpha = 1.0;
#endif
    }
    va_end(ap);
}

const Gradient*
qtcGetGradient(EAppearance app, const Options *opts)
{
    if (IS_CUSTOM(app)) {
#ifdef QTC_UTILS_QT
        auto grad = opts->customGradient.find(app);

        if (grad != opts->customGradient.end()) {
            return &grad->second;
        }
#else
        Gradient *grad = opts->customGradient[app - APPEARANCE_CUSTOM1];

        if (grad) {
            return grad;
        }
#endif
        app = APPEARANCE_RAISED;
    }

    static Gradient stdGradients[NUM_STD_APP];
    static bool init = false;

    if (!init) {
        qtcSetupGradient(&stdGradients[APPEARANCE_FLAT - APPEARANCE_FLAT],
                         GB_3D, 2, 0.0, 1.0, 1.0, 1.0);
        qtcSetupGradient(&stdGradients[APPEARANCE_RAISED - APPEARANCE_FLAT],
                         GB_3D_FULL, 2, 0.0, 1.0, 1.0, 1.0);
        qtcSetupGradient(&stdGradients[APPEARANCE_DULL_GLASS -
                                       APPEARANCE_FLAT], GB_LIGHT, 4, 0.0,
                         1.05, 0.499, 0.984, 0.5, 0.928,
                         1.0, 1.0);
        qtcSetupGradient(&stdGradients[APPEARANCE_SHINY_GLASS -
                                       APPEARANCE_FLAT], GB_LIGHT, 4, 0.0,
                         1.2, 0.499, 0.984, 0.5, 0.9,
                         1.0, 1.06);
        qtcSetupGradient(&stdGradients[APPEARANCE_AGUA - APPEARANCE_FLAT],
                         GB_SHINE, 2, 0.0, 0.6, 1.0, 1.1);
        qtcSetupGradient(&stdGradients[APPEARANCE_SOFT_GRADIENT -
                                       APPEARANCE_FLAT], GB_3D, 2, 0.0,
                         1.04, 1.0, 0.98);
        qtcSetupGradient(
            &stdGradients[APPEARANCE_GRADIENT - APPEARANCE_FLAT], GB_3D, 2,
            0.0, 1.1, 1.0, 0.94);
        qtcSetupGradient(&stdGradients[APPEARANCE_HARSH_GRADIENT -
                                       APPEARANCE_FLAT], GB_3D, 2, 0.0, 1.3,
                         1.0, 0.925);
        qtcSetupGradient(
            &stdGradients[APPEARANCE_INVERTED - APPEARANCE_FLAT], GB_3D, 2,
            0.0, 0.93, 1.0, 1.04);
        qtcSetupGradient(&stdGradients[APPEARANCE_DARK_INVERTED -
                                       APPEARANCE_FLAT], GB_NONE, 3, 0.0,
                         0.8, 0.7, 0.95, 1.0, 1.0);
        qtcSetupGradient(&stdGradients[APPEARANCE_SPLIT_GRADIENT -
                                       APPEARANCE_FLAT], GB_3D, 4, 0.0,
                         1.06, 0.499, 1.004, 0.5, 0.986,
                         1.0, 0.92);
        qtcSetupGradient(
            &stdGradients[APPEARANCE_BEVELLED - APPEARANCE_FLAT], GB_3D, 4,
            0.0, 1.05, 0.1, 1.02, 0.9, 0.985, 1.0, 0.94);
        qtcSetupGradient(&stdGradients[APPEARANCE_LV_BEVELLED -
                                       APPEARANCE_FLAT], GB_3D, 3, 0.0,
                         1.00, 0.85, 1.0, 1.0, 0.90);
        qtcSetupGradient(
            &stdGradients[APPEARANCE_AGUA_MOD - APPEARANCE_FLAT], GB_NONE,
            3, 0.0, 1.5, 0.49, 0.85, 1.0, 1.3);
        qtcSetupGradient(
            &stdGradients[APPEARANCE_LV_AGUA - APPEARANCE_FLAT], GB_NONE, 4,
            0.0, 0.98, 0.35, 0.95, 0.4, 0.93, 1.0, 1.15);
        init = true;
    }

    return &stdGradients[app - APPEARANCE_FLAT];
}

EAppearance
#ifdef QTC_UTILS_QT
qtcWidgetApp(EWidget w, const Options *opts, bool active)
#else
qtcWidgetApp(EWidget w, const Options *opts)
#endif
{
    switch (w) {
    case WIDGET_SB_BGND:
        return opts->sbarBgndAppearance;
    case WIDGET_LISTVIEW_HEADER:
        return opts->lvAppearance;
    case WIDGET_SB_BUTTON:
    case WIDGET_SLIDER:
    case WIDGET_SB_SLIDER:
        return opts->sliderAppearance;
    case WIDGET_FILLED_SLIDER_TROUGH:
        return opts->sliderFill;
    case WIDGET_TAB_TOP:
    case WIDGET_TAB_BOT:
        return opts->tabAppearance;
    case WIDGET_MENU_ITEM:
        return opts->menuitemAppearance;
    case WIDGET_PROGRESSBAR:
#ifndef QTC_UTILS_QT
    case WIDGET_ENTRY_PROGRESSBAR:
#endif
        return opts->progressAppearance;
    case WIDGET_PBAR_TROUGH:
        return opts->progressGrooveAppearance;
    case WIDGET_SELECTION:
        return opts->selectionAppearance;
#ifdef QTC_UTILS_QT
    case WIDGET_DOCK_WIDGET_TITLE:
        return opts->dwtAppearance;
    case WIDGET_MDI_WINDOW:
    case WIDGET_MDI_WINDOW_TITLE:
        return (active ? opts->titlebarAppearance :
                opts->inactiveTitlebarAppearance);
    case WIDGET_MDI_WINDOW_BUTTON:
        return opts->titlebarButtonAppearance;
    case WIDGET_DIAL:
        return qtcIsFlat(opts->appearance) ? APPEARANCE_RAISED :
        APPEARANCE_SOFT_GRADIENT;
#endif
    case WIDGET_TROUGH:
    case WIDGET_SLIDER_TROUGH:
        return opts->grooveAppearance;
#ifndef QTC_UTILS_QT
    case WIDGET_SPIN_UP:
    case WIDGET_SPIN_DOWN:
#endif
    case WIDGET_SPIN:
        return MODIFY_AGUA(opts->appearance);
    case WIDGET_TOOLBAR_BUTTON:
        return (APPEARANCE_NONE == opts->tbarBtnAppearance ?
                opts->appearance : opts->tbarBtnAppearance);
    default:
        break;
    }
    return opts->appearance;
}

#define CAN_EXTRA_ROUND(MOD)                                            \
    (QtCurve::isExtraRoundWidget(widget) &&                             \
     (IS_SLIDER(widget) || WIDGET_TROUGH == widget ||                   \
      (((w > (MIN_ROUND_EXTRA_SIZE(widget) + MOD)) ||                   \
        (WIDGET_NO_ETCH_BTN == widget || WIDGET_MENU_BUTTON == widget)) && \
       (h > (MIN_ROUND_EXTRA_SIZE(widget) + MOD)))))
#define CAN_FULL_ROUND(MOD)                                             \
    (w > (MIN_ROUND_FULL_SIZE + MOD) && h > (MIN_ROUND_FULL_SIZE + MOD))

// **NOTE** MUST KEEP IN SYNC WITH getRadius/RADIUS_ETCH !!!
ERound
qtcGetWidgetRound(const Options *opts, int w, int h, EWidget widget)
{
    ERound r = opts->round;

    if ((QtCurve::oneOf(widget, WIDGET_PBAR_TROUGH, WIDGET_PROGRESSBAR) &&
         (opts->square & SQUARE_PROGRESS)) ||
        (widget == WIDGET_ENTRY && (opts->square & SQUARE_ENTRY)) ||
        (widget == WIDGET_SCROLLVIEW && (opts->square & SQUARE_SCROLLVIEW))) {
        return ROUND_NONE;
    }

    if (QtCurve::oneOf(widget, WIDGET_CHECKBOX, WIDGET_FOCUS) &&
        r != ROUND_NONE) {
        r = ROUND_SLIGHT;
    }

#ifdef QTC_UTILS_QT
    if ((widget == WIDGET_MDI_WINDOW_BUTTON &&
         (opts->titlebarButtons & TITLEBAR_BUTTON_ROUND)) ||
        QtCurve::oneOf(widget, WIDGET_RADIO_BUTTON, WIDGET_DIAL)) {
        return ROUND_MAX;
    }
#endif
#ifndef QTC_UTILS_QT
    if (widget == WIDGET_RADIO_BUTTON) {
        return ROUND_MAX;
    }
#endif
    if (QtCurve::oneOf(opts->sliderStyle, SLIDER_ROUND, SLIDER_ROUND_ROTATED,
                       SLIDER_CIRCULAR) && widget == WIDGET_SLIDER) {
        return ROUND_MAX;
    }
    switch (r) {
    case ROUND_MAX:
        if (IS_SLIDER(widget) || widget == WIDGET_TROUGH ||
            (w > (MIN_ROUND_MAX_WIDTH + 2) && h > (MIN_ROUND_MAX_HEIGHT + 2) &&
             QtCurve::isMaxRoundWidget(widget))) {
            return ROUND_MAX;
        }
        QTC_FALLTHROUGH();
    case ROUND_EXTRA:
        if (CAN_EXTRA_ROUND(2)) {
            return ROUND_EXTRA;
        }
        QTC_FALLTHROUGH();
    case ROUND_FULL:
        if (CAN_FULL_ROUND(2)) {
            return ROUND_FULL;
        }
        QTC_FALLTHROUGH();
    case ROUND_SLIGHT:
        return ROUND_SLIGHT;
    case ROUND_NONE:
        return ROUND_NONE;
    }
    return ROUND_NONE;
}

double
qtcGetRadius(const Options *opts, int w, int h, EWidget widget, ERadius rad)
{
    ERound r = opts->round;

    if (QtCurve::oneOf(widget, WIDGET_CHECKBOX, WIDGET_FOCUS) &&
        ROUND_NONE != r) {
        r = ROUND_SLIGHT;
    }

    if ((QtCurve::oneOf(widget, WIDGET_PBAR_TROUGH, WIDGET_PROGRESSBAR) &&
         (opts->square & SQUARE_PROGRESS)) ||
        (widget == WIDGET_ENTRY && (opts->square & SQUARE_ENTRY)) ||
        (widget == WIDGET_SCROLLVIEW && (opts->square & SQUARE_SCROLLVIEW))) {
        return 0.0;
    }

#ifdef QTC_UTILS_QT
    if ((widget == WIDGET_MDI_WINDOW_BUTTON &&
         (opts->titlebarButtons & TITLEBAR_BUTTON_ROUND)) ||
        QtCurve::oneOf(widget, WIDGET_RADIO_BUTTON, WIDGET_DIAL)) {
        return (w > h ? h : w) / 2.0;
    }
#endif
#ifndef QTC_UTILS_QT
    if (widget == WIDGET_RADIO_BUTTON) {
        return (w > h ? h : w) / 2.0;
    }
#endif

    if (QtCurve::oneOf(opts->sliderStyle, SLIDER_ROUND, SLIDER_ROUND_ROTATED,
                       SLIDER_CIRCULAR) && widget == WIDGET_SLIDER) {
        return (w > h ? h : w) / 2.0;
    }

    if (rad == RADIUS_EXTERNAL && !opts->fillProgress &&
        (widget == WIDGET_PROGRESSBAR
#ifndef QTC_UTILS_QT
         || widget == WIDGET_ENTRY_PROGRESSBAR
#endif
            )) {
        rad = RADIUS_INTERNAL;
    }

    switch (rad) {
    case RADIUS_SELECTION:
        switch (r) {
        case ROUND_MAX:
        case ROUND_EXTRA:
            if (/* (WIDGET_RUBBER_BAND==widget && w>14 && h>14) || */
                (w > 48 && h > 48)) {
                return 6.0;
            }
            QTC_FALLTHROUGH();
        case ROUND_FULL:
            /* if( /\*(WIDGET_RUBBER_BAND==widget && w>11 && h>11) || *\/ */
            /*     (w>48 && h>48)) */
            /*     return 3.0; */
            if (w > MIN_ROUND_FULL_SIZE && h > MIN_ROUND_FULL_SIZE) {
                return 3.0;
            }
            QTC_FALLTHROUGH();
        case ROUND_SLIGHT:
            return 2.0;
        case ROUND_NONE:
            return 0;
        }
        QTC_FALLTHROUGH();
    case RADIUS_INTERNAL:
        switch (r) {
        case ROUND_MAX:
            if (IS_SLIDER(widget) || WIDGET_TROUGH == widget) {
                double r = ((w > h ? h : w) -
                            (WIDGET_SLIDER == widget ? 1 : 0)) / 2.0;
                return r > MAX_RADIUS_INTERNAL ? MAX_RADIUS_INTERNAL : r;
            }
            if (w > (MIN_ROUND_MAX_WIDTH - 2) &&
                h > (MIN_ROUND_MAX_HEIGHT - 2) &&
                QtCurve::isMaxRoundWidget(widget)) {
                double r = ((w > h ? h : w) - 2.0) / 2.0;
                return r > 9.5 ? 9.5 : r;
            }
            QTC_FALLTHROUGH();
        case ROUND_EXTRA:
            if (CAN_EXTRA_ROUND(-2)) {
                return EXTRA_INNER_RADIUS;
            }
            QTC_FALLTHROUGH();
        case ROUND_FULL:
            if (CAN_FULL_ROUND(-2)) {
                return FULL_INNER_RADIUS;
            }
            QTC_FALLTHROUGH();
        case ROUND_SLIGHT:
            return SLIGHT_INNER_RADIUS;
        case ROUND_NONE:
            return 0;
        }
        QTC_FALLTHROUGH();
    case RADIUS_EXTERNAL:
        switch (r) {
        case ROUND_MAX:
            if (IS_SLIDER(widget) || WIDGET_TROUGH == widget) {
                double r = ((w > h ? h : w) -
                            (WIDGET_SLIDER == widget ? 1 : 0)) / 2.0;
                return r > MAX_RADIUS_EXTERNAL ? MAX_RADIUS_EXTERNAL : r;
            }
            if (w > MIN_ROUND_MAX_WIDTH && h > MIN_ROUND_MAX_HEIGHT &&
                QtCurve::isMaxRoundWidget(widget)) {
                double r = ((w > h ? h : w) - 2.0) / 2.0;
                return r > 10.5 ? 10.5 : r;
            }
            QTC_FALLTHROUGH();
        case ROUND_EXTRA:
            if (CAN_EXTRA_ROUND(0)) {
                return EXTRA_OUTER_RADIUS;
            }
            QTC_FALLTHROUGH();
        case ROUND_FULL:
            if (CAN_FULL_ROUND(0)) {
                return FULL_OUTER_RADIUS;
            }
            QTC_FALLTHROUGH();
        case ROUND_SLIGHT:
            return SLIGHT_OUTER_RADIUS;
        case ROUND_NONE:
            return 0;
        }
        QTC_FALLTHROUGH();
    case RADIUS_ETCH:
        // **NOTE** MUST KEEP IN SYNC WITH getWidgetRound !!!
        switch (r) {
        case ROUND_MAX:
            if (IS_SLIDER(widget) || WIDGET_TROUGH == widget) {
                double r = ((w > h ? h : w) -
                            (WIDGET_SLIDER == widget ? 1 : 0)) / 2.0;
                return r > MAX_RADIUS_EXTERNAL ? MAX_RADIUS_EXTERNAL : r;
            }
            if (w > (MIN_ROUND_MAX_WIDTH + 2) &&
                h > (MIN_ROUND_MAX_HEIGHT + 2) &&
                QtCurve::isMaxRoundWidget(widget)) {
                double r = ((w > h ? h : w) - 2.0) / 2.0;
                return r > 11.5 ? 11.5 : r;
            }
            QTC_FALLTHROUGH();
        case ROUND_EXTRA:
            if (CAN_FULL_ROUND(2)) {
                return EXTRA_ETCH_RADIUS;
            }
            QTC_FALLTHROUGH();
        case ROUND_FULL:
            if (w > (MIN_ROUND_FULL_SIZE + 2) &&
                h > (MIN_ROUND_FULL_SIZE + 2)) {
                return FULL_ETCH_RADIUS;
            }
            QTC_FALLTHROUGH();
        case ROUND_SLIGHT:
            return SLIGHT_ETCH_RADIUS;
        case ROUND_NONE:
            return 0;
        }
        QTC_FALLTHROUGH();
    default:
        // (empty case required for the fall-through declaration above)
        break;
    }
    return 0;
}
