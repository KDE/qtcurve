/*****************************************************************************
 *   Copyright 2007 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
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

#include "qtcurve_p.h"
#include "utils.h"

#include <QToolBar>
#include <QToolButton>
#include <QAbstractItemView>
#include <QDialog>
#include <QSplitter>
#include <QMdiSubWindow>
#include <QMainWindow>
#include <QComboBox>
#include <QTreeView>
#include <QGroupBox>
#include <QListView>
#include <QCheckBox>
#include <QRadioButton>
#include <QTextEdit>
#include <QDial>
#include <QLabel>
#include <QStackedLayout>
#include <QMenuBar>
#include <QMouseEvent>
#include <QScrollBar>
#include <QWizard>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QHeaderView>
#include <QLineEdit>
#include <QSpinBox>
#include <QDir>
#include <QSettings>
#include <QPixmapCache>
#include <QTextStream>

#include "shadowhelper.h"
#include <qtcurve-utils/x11qtc.h>
#include <qtcurve-utils/qtutils.h>
#include <sys/time.h>

namespace QtCurve {

bool
blendOOMenuHighlight(const QPalette &pal, const QColor &highlight)
{
    QColor text(pal.text().color());
    QColor hl(pal.highlightedText().color());

    return ((text.red() < 50) && (text.green() < 50) && (text.blue() < 50) &&
            (hl.red() > 127) && (hl.green() > 127) && (hl.blue() > 127) &&
            TOO_DARK(highlight));
}

bool isNoEtchWidget(const QWidget *widget)
{
    if (theThemedApp == APP_KRUNNER) {
        return true;
    }
    if (theThemedApp == APP_PLASMA) {
        const QWidget *top = widget->window();

        return !top || (!qobject_cast<const QDialog*>(top) &&
                        !qobject_cast<const QMainWindow*>(top));
    }

    if (widget && widget->inherits("QWebView")) {
        return true;
    }
    // KHTML:  widget -> QWidget -> QWidget -> KHTMLView
    const QObject *w = (widget && widget->parent() &&
                        widget->parent()->parent() ?
                        widget->parent()->parent()->parent() : nullptr);

    return ((w && isA(w, "KHTMLView")) ||
            (widget && isInQAbstractItemView(widget->parentWidget())));
}

void
setOpacityProp(QWidget *w, unsigned short opacity)
{
    // DO NOT condition compile on QTC_ENABLE_X11.
    // There's no direct linkage on X11 and the following code will just do
    // nothing if X11 is not enabled (either at compile time or at run time).
    QTC_RET_IF_FAIL(qtcX11Enabled());
    if (WId wid = qtcGetWid(w->window())) {
        qtcX11SetOpacity(wid, opacity);
    }
}

void
setBgndProp(QWidget *w, EAppearance app, bool haveBgndImage)
{
    // DO NOT condition compile on QTC_ENABLE_X11.
    // There's no direct linkage on X11 and the following code will just do
    // nothing if X11 is not enabled (either at compile time or at run time).
    QTC_RET_IF_FAIL(qtcX11Enabled());
    if (WId wid = qtcGetWid(w->window())) {
        uint32_t prop = (((qtcIsFlatBgnd(app) ?
                           (haveBgndImage ? APPEARANCE_RAISED :
                            APPEARANCE_FLAT) : app) & 0xFF) |
                         (w->palette().window().color().rgb() &
                          0x00FFFFFF) << 8);
        qtcX11SetBgnd(wid, prop);
    }
}

void setSbProp(QWidget *w)
{
    // DO NOT condition compile on QTC_ENABLE_X11.
    // There's no direct linkage on X11 and the following code will just do
    // nothing if X11 is not enabled (either at compile time or at run time).
    QTC_RET_IF_FAIL(qtcX11Enabled());
    if (WId wid = qtcGetWid(w->window())) {
        static const char *constStatusBarProperty = "qtcStatusBar";
        QVariant prop(w->property(constStatusBarProperty));

        if (!prop.isValid() || !prop.toBool()) {
            w->setProperty(constStatusBarProperty, true);
            qtcX11SetStatusBar(wid);
        }
    }
}

QWidget*
scrollViewFrame(QWidget *widget)
{
    QWidget *w = widget;

    for (int i = 0; i < 10 && w;i++, w = w->parentWidget()) {
        if ((qobject_cast<QFrame*>(w) &&
             ((QFrame*)w)->frameWidth() > 0) ||
            qobject_cast<QTabWidget*>(w)) {
            return w;
        }
    }
    return nullptr;
}

QToolBar*
getToolBarChild(QWidget *w)
{
    for (QObject *child: w->children()) {
        if (child->isWidgetType()) {
            if (qobject_cast<QToolBar*>(child))
                return static_cast<QToolBar*>(child);
            QToolBar *tb = getToolBarChild((QWidget*)child);
            if (tb) {
                return tb;
            }
        }
    }

    return nullptr;
}

void
setStyleRecursive(QWidget *w, QStyle *s, int minSize)
{
    w->setStyle(s);
    if (qobject_cast<QToolButton*>(w))
        w->setMinimumSize(1, minSize);

    for (QObject *child: w->children()) {
        if (child->isWidgetType()) {
            setStyleRecursive((QWidget*)child, s, minSize);
        }
    }
}

//
// QtCurve's menu's have a 2 pixel border all around - but want the top,
// and left edges to active the nearest menu item. Therefore, when we get a
// mouse event in that region then adjsut its position...
bool
updateMenuBarEvent(QMouseEvent *event, QMenuBar *menu)
{
    struct HackEvent: public QMouseEvent {
        bool
        adjust()
        {
            // TODO
            // if (l.x() < 2 || l.y() < 2) {
            //     l = QPointF(l.x() < 2 ? l.x() + 2 : l.x(),
            //                 l.y() < 2 ? l.y() + 2 : l.y());
            //     s = QPointF(l.x() < 2 ? s.x() + 2 : s.x(),
            //                 l.y() < 2 ? s.y() + 2 : s.y());
            //     return true;
            // }
            return false;
        }
    };

    if (((HackEvent*)event)->adjust()) {
        static_cast<QObject*>(menu)->event(event);
        return true;
    }
    return false;
}

QRegion
windowMask(const QRect &r, bool full)
{
    int x, y, w, h;
    r.getRect(&x, &y, &w, &h);

    if (full) {
        QRegion region(x + 4, y + 0, w-4*2, h-0*2);
        region += QRegion(x + 0, y + 4, w-0*2, h-4*2);
        region += QRegion(x + 2, y + 1, w-2*2, h-1*2);
        region += QRegion(x + 1, y + 2, w-1*2, h-2*2);
        return region;
    } else {
        QRegion region(x+1, y+1, w-2, h-2);
        region += QRegion(x, y+2, w, h-4);
        region += QRegion(x+2, y, w-4, h);
        return region;
    }
}

const QWidget*
getWidget(const QPainter *p)
{
    if (p) {
        if (p->device()->devType() == QInternal::Widget) {
            return static_cast<const QWidget*>(p->device());
        }
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
        // No-op on 5.13+.
        else {
            QPaintDevice *dev = QPainter::redirected(p->device());
            if (dev && dev->devType() == QInternal::Widget) {
                return static_cast<const QWidget*>(dev);
            }
        }
#endif
    }
    return nullptr;
}

const QImage*
getImage(const QPainter *p)
{
    return (p && p->device() && QInternal::Image==p->device()->devType() ?
            static_cast<const QImage*>(p->device()) : nullptr);
}

const QAbstractButton*
getButton(const QWidget *w, const QPainter *p)
{
    const QWidget *widget = w ? w : getWidget(p);
    return widget ? qobject_cast<const QAbstractButton*>(widget) : nullptr;
}

void
drawDots(QPainter *p, const QRect &r, bool horiz, int nLines, int offset,
         const QColor *cols, int startOffset, int dark)
{
    int space((nLines*2)+(nLines-1)),
        x(horiz ? r.x() : r.x()+((r.width()-space)>>1)),
        y(horiz ? r.y()+((r.height()-space)>>1) : r.y()),
        i, j,
        numDots((horiz ? (r.width()-(2*offset))/3 : (r.height()-(2*offset))/3)+1);

    p->setRenderHint(QPainter::Antialiasing, true);
    if (horiz) {
        if(startOffset && y+startOffset>0)
            y+=startOffset;

        p->setPen(cols[dark]);
        for(i=0; i<space; i+=3)
            for(j=0; j<numDots; j++)
                drawAaPoint(p, x+offset+(3*j), y+i);

        p->setPen(cols[0]);
        for(i=1; i<space; i+=3)
            for(j=0; j<numDots; j++)
                drawAaPoint(p, x+offset+1+(3*j), y+i);
    } else {
        if(startOffset && x+startOffset>0)
            x+=startOffset;

        p->setPen(cols[dark]);
        for(i=0; i<space; i+=3)
            for(j=0; j<numDots; j++)
                drawAaPoint(p, x+i, y+offset+(3*j));

        p->setPen(cols[0]);
        for(i=1; i<space; i+=3)
            for(j=0; j<numDots; j++)
                drawAaPoint(p, x+i, y+offset+1+(3*j));
    }
    QPAINTER_RENDERHINT_AA_MAYBE_OFF(p);
}

bool
isInQAbstractItemView(const QObject *w)
{
    int level = 8;
    while (w && --level > 0) {
        if (qobject_cast<const QAbstractItemView*>(w))
            return true;
        if (qobject_cast<const QDialog*>(w)
            /* || qobject_cast<const QMainWindow *>(w)*/)
            return false;
        w = w->parent();
    }
    return false;
}

const QToolBar*
getToolBar(const QWidget *w)
{
    return (w ? qobject_cast<const QToolBar*>(w) ?
            static_cast<const QToolBar*>(w) :
            getToolBar(w->parentWidget()) : nullptr);
}

void
drawTbArrow(const QStyle *style, const QStyleOptionToolButton *toolbutton,
            const QRect &rect, QPainter *painter, const QWidget *widget)
{
    QStyle::PrimitiveElement pe;
    switch (toolbutton->arrowType) {
    case Qt::LeftArrow:
        pe = QStyle::PE_IndicatorArrowLeft;
        break;
    case Qt::RightArrow:
        pe = QStyle::PE_IndicatorArrowRight;
        break;
    case Qt::UpArrow:
        pe = QStyle::PE_IndicatorArrowUp;
        break;
    case Qt::DownArrow:
        pe = QStyle::PE_IndicatorArrowDown;
        break;
    default:
        return;
    }

    QStyleOption arrowOpt;
    arrowOpt.rect = rect;
    arrowOpt.palette = toolbutton->palette;
    arrowOpt.state = toolbutton->state;
    style->drawPrimitive(pe, &arrowOpt, painter, widget);
}

void
adjustToolbarButtons(const QWidget *widget, const QToolBar *toolbar,
                     int &leftAdjust, int &topAdjust, int &rightAdjust,
                     int &bottomAdjust, int &round)
{
    const int constAdjust=6;
    const int d = 1;
    QRect geo(widget->geometry());
    if (toolbar->orientation() == Qt::Horizontal) {
        bool haveLeft =
            qobject_cast<QToolButton*>(toolbar->childAt(geo.x() -
                                                        d, geo.y()));
        bool haveRight =
            qobject_cast<QToolButton*>(toolbar->childAt(geo.right() +
                                                        d, geo.y()));

        if (haveLeft && haveRight) {
            leftAdjust = -constAdjust;
            rightAdjust = constAdjust;
            round = ROUNDED_NONE;
        } else if (haveLeft) {
            leftAdjust = -constAdjust;
            round = ROUNDED_RIGHT;
        } else if (haveRight) {
            rightAdjust = constAdjust;
            round = ROUNDED_LEFT;
        }
    } else {
        bool haveTop =
            qobject_cast<QToolButton*>(toolbar->childAt(geo.x(), geo.y() - d));
        bool haveBot =
            qobject_cast<QToolButton*>(toolbar->childAt(geo.x(),
                                                        geo.bottom() + d));
        if (haveTop && haveBot) {
            topAdjust = -constAdjust;
            bottomAdjust = constAdjust;
            round = ROUNDED_NONE;
        } else if (haveTop) {
            topAdjust = -constAdjust;
            round = ROUNDED_BOTTOM;
        } else if (haveBot) {
            bottomAdjust = constAdjust;
            round = ROUNDED_TOP;
        }
    }
}

bool
isA(const QObject *w, const char *type)
{
    return (w && (0 == strcmp(w->metaObject()->className(), type) ||
                  (w->parent() &&
                   0 == strcmp(w->parent()->metaObject()->className(), type))));
}

}
