/*****************************************************************************
 *   Copyright 2010 Craig Drummond <craig.p.drummond@gmail.com>              *
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

#include "qtcurvetogglebutton.h"
#include <QAbstractButton>
#include <QStyle>
#include <QStyleOption>
#include <QBitmap>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QX11Info>
#include <klocalizedstring.h>
#include "qtcurveclient.h"
#include <common/common.h>

namespace KWinQtCurve {

static int
point2Pixel(double point)
{
    return (int)(((point * QX11Info::appDpiY()) / 72.0) + 0.5);
}

QtCurveToggleButton::QtCurveToggleButton(bool menubar, QtCurveClient *parent)
                   : KCommonDecorationButton(AboveButton, parent),
                     m_client(parent),
                     isMenuBar(menubar),
                     m_hover(false)
{
//     setAttribute(Qt::WA_PaintOnScreen, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAutoFillBackground(false);
//     setFocusPolicy(Qt::NoFocus);
//     setAttribute(Qt::WA_OpaquePaintEvent, false);
//     setAttribute(Qt::WA_Hover, true);
    setCursor(Qt::ArrowCursor);
    setToolTip(menubar ? i18n("Toggle Menubar") : i18n("Toggle Statusbar"));
    reset(DecorationReset);
}

void QtCurveToggleButton::reset(unsigned long changed)
{
    if (changed&DecorationReset || changed&ManualReset || changed&SizeChange || changed&StateChange)
        this->update();
}

void QtCurveToggleButton::enterEvent(QEvent *e)
{
    m_hover = true;
    KCommonDecorationButton::enterEvent(e);
    update();
    // Hacky NVIDIA fix - sometimes mouseover state gets 'stuck' - but only for some windows!!!
    QTimer::singleShot(50, this, SLOT(update()));
}

void QtCurveToggleButton::leaveEvent(QEvent *e)
{
    m_hover = false;
    KCommonDecorationButton::leaveEvent(e);
    update();
    // Hacky NVIDIA fix - sometimes mouseover state gets 'stuck' - but only for some windows!!!
    QTimer::singleShot(50, this, SLOT(update()));
}

void QtCurveToggleButton::paintEvent(QPaintEvent *ev)
{
    QPainter p(this);
    p.setClipRect(rect().intersected(ev->rect()));
    drawButton(&p);
}

void QtCurveToggleButton::drawButton(QPainter *painter)
{
    QRect  r(0, 0, width(), height());
    bool   active(m_client->isActive()),
           sunken(isDown());
    QColor col(KDecoration::options()->color(KDecoration::ColorFont, active/* || faded*/));

    col.setAlphaF(m_hover ? 0.99 : 0.15);
    painter->setRenderHint(QPainter::Antialiasing, true);
    //painter->setPen(QPen(col, (isChecked() ? 2.0 : 1.0)));
    painter->setPen(col);
    r.adjust(1, 1, -1, -1);

    QFont font(Handler()->titleFont());
    int   maxPixelSize=r.height()-2,
          fontPixelSize=font.pixelSize();
    bool  drawBorder=true;

    if(maxPixelSize<9)
    {
        maxPixelSize=r.height()+2;
        drawBorder=false;
        r.adjust(-1, -1, 1, 1);
    }

    if(-1==fontPixelSize)
        fontPixelSize=point2Pixel(font.pointSizeF());

    if(fontPixelSize>maxPixelSize)
        font.setPixelSize(maxPixelSize-2);
    painter->setFont(font);

    QRectF       ellipse(r.x()+0.5, r.y()+0.5, r.width(), r.height());
    QColor       bgnd(KDecoration::options()->color(KDecoration::ColorTitleBar, active));
    bool         round=Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_TitleBarButtons, 0L, 0L)&TITLEBAR_BUTTON_ROUND;
    double       squareRad=round || Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_Round, 0L, 0L)<ROUND_FULL ? 0.0 : 2.0;
    QPainterPath path;

    bgnd.setAlphaF(m_hover ? 0.9 : 0.4);


    if(round)
        path.addEllipse(ellipse);
    else
        path.addRoundedRect(ellipse, squareRad+0.5, squareRad+0.5);

    painter->fillPath(path, bgnd);
    if(sunken)
    {
        bgnd=col;
        bgnd.setAlphaF(0.2);
        painter->fillPath(path, bgnd);
    }
    if(drawBorder)
    {
        if(round)
            painter->drawEllipse(ellipse);
        else
            painter->drawRoundedRect(ellipse, squareRad, squareRad);
    }

    if(sunken)
        r.adjust(1, 1, 1, 1);

    painter->setPen(col);
    painter->drawText(r, Qt::AlignVCenter|Qt::AlignHCenter, isMenuBar ? i18n("M") : i18n("S"));
}

}
