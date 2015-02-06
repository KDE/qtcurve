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

#ifndef __QTCURVE_H__
#define __QTCURVE_H__

#include <common/common.h>
#include "config.h"

QTC_BEGIN_DECLS

typedef struct _QtCurveStyleClass QtCurveStyleClass;

extern GType qtcurve_type_style;

#define QTCURVE_TYPE_STYLE              qtcurve_type_style
/*#define QTCURVE_STYLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), QTCURVE_TYPE_STYLE, QtCurveStyle))*/
#define QTCURVE_STYLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), QTCURVE_TYPE_STYLE, QtCurveStyleClass))
#define QTCURVE_IS_STYLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), QTCURVE_TYPE_STYLE))
#define QTCURVE_IS_STYLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), QTCURVE_TYPE_STYLE))
#define QTCURVE_STYLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), QTCURVE_TYPE_STYLE, QtCurveStyleClass))

typedef struct _QtCurveRcStyle QtCurveRcStyle;
typedef struct _QtCurveRcStyleClass QtCurveRcStyleClass;

extern GType qtcurve_type_rc_style;

#define QTCURVE_TYPE_RC_STYLE              qtcurve_type_rc_style
#define QTCURVE_RC_STYLE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), QTCURVE_TYPE_RC_STYLE, QtCurveRcStyle))
#define QTCURVE_RC_STYLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), QTCURVE_TYPE_RC_STYLE, QtCurveRcStyleClass))
#define QTCURVE_IS_RC_STYLE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), QTCURVE_TYPE_RC_STYLE))
#define QTCURVE_IS_RC_STYLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), QTCURVE_TYPE_RC_STYLE))
#define QTCURVE_RC_STYLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), QTCURVE_TYPE_RC_STYLE, QtCurveRcStyleClass))

struct _QtCurveRcStyle
{
    GtkRcStyle parent_instance;
};

struct _QtCurveRcStyleClass
{
    GtkRcStyleClass parent_class;
};

void qtcurve_rc_style_register_type (GTypeModule *module);

typedef struct {
    GtkStyle parent_instance;
    GdkColor *button_text[2];
    GdkColor *menutext[2];
} QtCurveStyle;

struct _QtCurveStyleClass {
    GtkStyleClass parent_class;
};

QTC_END_DECLS

#endif
