/*****************************************************************************
 *   Copyright 2007 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
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
/*
  based on the window decoration "Plastik":
  Copyright (C) 2003-2005 Sandro Giessl <sandro@giessl.com>

  based on the window decoration "Web":
  Copyright (C) 2001 Rik Hemsley (rikkus) <rik@kde.org>
*/

#include "qtcurvebutton.h"
#include <QAbstractButton>
#include <QStyle>
#include <QStyleOption>
#include <QBitmap>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include "qtcurveclient.h"
#include <common/common.h>

namespace QtCurve {
namespace KWin {

QtCurveButton::QtCurveButton(ButtonType type, QtCurveClient *parent)
    : KCommonDecorationButton(type, parent),
      m_client(parent),
      m_iconType(NumButtonIcons),
      m_hover(false)
{
    // setAttribute(Qt::WA_PaintOnScreen, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAutoFillBackground(false);
    // setFocusPolicy(Qt::NoFocus);
    // setAttribute(Qt::WA_OpaquePaintEvent, false);
    // setAttribute(Qt::WA_Hover, true);
    setCursor(Qt::ArrowCursor);
    reset(DecorationReset);
}

void QtCurveButton::reset(unsigned long changed)
{
    if (changed & DecorationReset || changed & ManualReset ||
        changed & SizeChange || changed & StateChange) {
        switch (type()) {
#if KDE_IS_VERSION(4, 9, 85)
        case AppMenuButton:
            m_iconType = MenuIcon;
            break;
#endif
            // TODO ItemMenuButton?
        case ItemCloseButton:
            m_iconType = CloseTabIcon;
            break;
        case CloseButton:
            m_iconType = CloseIcon;
            break;
        case HelpButton:
            m_iconType = HelpIcon;
            break;
        case MinButton:
            m_iconType = MinIcon;
            break;
        case MaxButton:
            m_iconType = isChecked() ? MaxRestoreIcon : MaxIcon;
            break;
        case OnAllDesktopsButton:
            m_iconType = isChecked() ? NotOnAllDesktopsIcon : OnAllDesktopsIcon;
            break;
        case ShadeButton:
            m_iconType = isChecked() ? UnShadeIcon : ShadeIcon;
            break;
        case AboveButton:
            m_iconType = isChecked() ? NoKeepAboveIcon : KeepAboveIcon;
            break;
        case BelowButton:
            m_iconType = isChecked() ? NoKeepBelowIcon : KeepBelowIcon;
            break;
        case MenuButton:
            m_iconType = MenuIcon;
            break;
        default:
            m_iconType = NumButtonIcons; // empty...
            break;
        }

        this->update();
    }
}

void QtCurveButton::enterEvent(QEvent *e)
{
    m_hover = true;
    KCommonDecorationButton::enterEvent(e);
    update();
    // Hacky NVIDIA fix - sometimes mouseover state gets 'stuck' -
    // but only for some windows!!!
    QTimer::singleShot(50, this, SLOT(update()));
}

void QtCurveButton::leaveEvent(QEvent *e)
{
    m_hover = false;
    KCommonDecorationButton::leaveEvent(e);
    update();
    // Hacky NVIDIA fix - sometimes mouseover state gets 'stuck' -
    // but only for some windows!!!
    QTimer::singleShot(50, this, SLOT(update()));
}

void QtCurveButton::paintEvent(QPaintEvent *ev)
{
    if (m_client->compositingActive()) {
        QPainter p(this);
        p.setClipRect(rect().intersected(ev->rect()));
        drawButton(&p);
    } else {
        QPixmap pixmap(size());
        {
            QPainter p(&pixmap);
            p.setRenderHints(QPainter::Antialiasing);
            parentWidget()->render(&p, QPoint(), geometry(),
                                   QWidget::DrawWindowBackground);
            drawButton(&p);
        }
        QPainter p(this);
        p.setClipRect(ev->rect());
        p.drawPixmap(QPoint(), pixmap);
    }
}

void QtCurveButton::drawButton(QPainter *painter)
{
    int flags = Handler()->wStyle()->pixelMetric(
        (QStyle::PixelMetric)QtC_TitleBarButtons, 0L, 0L);
    bool active(m_client->isActive());

    if (!active && !m_hover && flags & TITLEBAR_BUTTOM_HIDE_ON_INACTIVE_WINDOW)
        return;

    QRect r(0, 0, width(), height());
    int versionHack = 0;
    bool sunken(isDown());
    bool drawFrame(!(flags & TITLEBAR_BUTTON_NO_FRAME) &&
                   (m_hover || sunken ||
                    !(flags & TITLEBAR_BUTTON_HOVER_FRAME)));
    bool drewFrame(false);
    bool iconForMenu(TITLEBAR_ICON_MENU_BUTTON ==
                     Handler()->wStyle()->pixelMetric(
                         (QStyle::PixelMetric)QtC_TitleBarIcon, 0L, 0L));
    QColor buttonColor(KDecoration::options()->color(KDecoration::ColorTitleBar,
                                                     active));
    QPixmap buffer(width(), height());
    buffer.fill(Qt::transparent);
    QPainter bP(&buffer);
    bool isTabClose(ItemCloseButton == type());

    // isItemMenu?
    if (isTabClose && drawFrame && !(sunken || m_hover))
        drawFrame = false;
    // if(CloseButton == type())
    //     buttonColor = midColor(QColor(180,64,32), buttonColor);

    switch (type()) {
    case HelpButton:
        versionHack = TBAR_VERSION_HACK + TITLEBAR_HELP;
        break;
    case MaxButton:
        versionHack = TBAR_VERSION_HACK + TITLEBAR_MAX;
        break;
    case MinButton:
        versionHack = TBAR_VERSION_HACK + TITLEBAR_MIN;
        break;
    case ItemCloseButton:
    case CloseButton:
        versionHack = TBAR_VERSION_HACK + TITLEBAR_CLOSE;
        break;
#if KDE_IS_VERSION(4, 9, 85)
    case AppMenuButton:
#endif
    case MenuButton:
        versionHack = TBAR_VERSION_HACK + TITLEBAR_MENU;
        break;
    case OnAllDesktopsButton:
        versionHack = TBAR_VERSION_HACK + TITLEBAR_ALL_DESKTOPS;
        break;
    case AboveButton:
        versionHack = TBAR_VERSION_HACK + TITLEBAR_KEEP_ABOVE;
        break;
    case BelowButton:
        versionHack = TBAR_VERSION_HACK + TITLEBAR_KEEP_BELOW;
        break;
    case ShadeButton:
        versionHack = TBAR_VERSION_HACK + TITLEBAR_SHADE;
    default:
        break;
    }

    if (drawFrame &&
        (/*!(flags&TITLEBAR_BUTTON_ROUND) || */
            MenuButton != type() || !iconForMenu)) {
        QStyleOption opt;
        int offset = flags & TITLEBAR_BUTTON_ROUND &&
            !m_client->isToolWindow() ? 1 : 0;

        if (flags & TITLEBAR_BUTTON_SUNKEN_BACKGROUND)
            // && flags & TITLEBAR_BUTTON_ROUND)
            offset++;
        opt.init(this);
        opt.rect = QRect(offset, offset, width() - (2 * offset),
                         height() - (2 * offset));
        opt.state |= (isDown() ? QStyle::State_Sunken : QStyle::State_Raised) |
            (active ? QStyle::State_Active : QStyle::State_None) |
            (m_hover ? QStyle::State_MouseOver : QStyle::State_None) |
            QStyle::State_Horizontal | QtC_StateKWin;
        opt.state &= ~QStyle::State_HasFocus;
        if (!isEnabled()) {
            opt.palette.setColor(QPalette::Button, buttonColor);
        } else {
            if (!(flags & TITLEBAR_BUTTON_STD_COLOR) ||
                (flags & TITLEBAR_BUTTON_COLOR_MOUSE_OVER && !m_hover &&
                 !(flags&TITLEBAR_BUTTON_COLOR)))
                opt.palette.setColor(QPalette::Button, buttonColor);
            if (flags & TITLEBAR_BUTTON_COLOR &&
                !(flags & TITLEBAR_BUTTON_COLOR_SYMBOL))
                opt.version = versionHack;
        }
        Handler()->wStyle()->drawPrimitive(QStyle::PE_PanelButtonCommand,
                                           &opt, &bP, 0L);
        drewFrame = true;
    }

    if (MenuButton == type() && iconForMenu) {
        QPixmap menuIcon(m_client->icon().pixmap(
                             style()->pixelMetric(QStyle::PM_SmallIconSize)));
        if (width() < menuIcon.width() || height() < menuIcon.height())
            menuIcon = menuIcon.scaled(width(), height());

        int dX(((width() - menuIcon.width()) / 2.0) + 0.5),
                      dY(((height() - menuIcon.height()) / 2.0) + 0.5);

        if (sunken) {
            dY++;
            dX++;
        }
        bP.drawPixmap(dX, dY, menuIcon);
    } else if (isEnabled() && (!(flags & TITLEBAR_BUTTON_HOVER_SYMBOL_FULL) ||
                               sunken || m_hover)) {
        const QBitmap &icon =
            Handler()->buttonBitmap(m_iconType, size(),
                                    decoration()->isToolWindow());
        bool customCol(false);
        bool faded(!m_hover && flags & TITLEBAR_BUTTON_HOVER_SYMBOL);
        QColor col(KDecoration::options()->color(KDecoration::ColorFont, active/* || faded*/));
        int dX(r.x()+(r.width() - icon.width())/2);
        int dY(r.y()+(r.height() - icon.height())/2);
        EEffect effect((EEffect)(style()->pixelMetric((QStyle::PixelMetric)QtC_TitleBarEffect)));

        if(EFFECT_ETCH==effect && drewFrame)
            effect=EFFECT_SHADOW;

        if(m_hover || !(flags&TITLEBAR_BUTTON_HOVER_SYMBOL))
        {
            bool userDefined=flags&TITLEBAR_BUTTON_ICON_COLOR;

            if(userDefined || (flags&TITLEBAR_BUTTON_COLOR && flags&TITLEBAR_BUTTON_COLOR_SYMBOL))
            {
                QStyleOption opt;

                opt.init(this);
                if(userDefined)
                {
                    versionHack+=NUM_TITLEBAR_BUTTONS;
                    if(!active)
                        versionHack+=NUM_TITLEBAR_BUTTONS;
                }
                opt.version=versionHack;
                col=QColor(QRgb(Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_TitleBarIconColor, &opt, 0L)));
                customCol=true;
            }
        }

        if (sunken) {
            dY++;
            dX++;
        } else if (!faded && EFFECT_NONE != effect) {
            QColor shadow(WINDOW_SHADOW_COLOR(effect));

            shadow.setAlphaF(WINDOW_TEXT_SHADOW_ALPHA(effect));
            bP.setPen(shadow);
            bP.drawPixmap(EFFECT_SHADOW==effect ? dX+1 : dX, dY+1, icon);
        }

        if (m_hover && !sunken && !(flags&TITLEBAR_BUTTON_COLOR) && !customCol) {
            if (CloseButton == type() || isTabClose) {
                col = CLOSE_COLOR;
            } else if(flags&TITLEBAR_BUTTON_USE_HOVER_COLOR) {
                col = Handler()->hoverCol(active);
            }
        }

        if (faded) {
            col.setAlphaF(HOVER_BUTTON_ALPHA(col));
        } else {
            // If dont set an alpha level here, then (at least on intel)
            // the background colour is used!
            col.setAlpha(254);
        }

        bP.setPen(col);
        bP.drawPixmap(dX, dY, icon);
    }

    bP.end();
    painter->drawPixmap(0, 0, buffer);
}

QBitmap IconEngine::icon(ButtonIcon icon, int size, QStyle *style)
{
    if (size%2 == 0)
        --size;

    QBitmap bitmap(size, size);
    bitmap.fill(Qt::color0);
    QPainter p(&bitmap);

    p.setPen(Qt::color1);

    QRect r = bitmap.rect();

    // line widths
    int lwTitleBar = 1;
    if (r.width() > 16)
        lwTitleBar = 4;
    else if (r.width() > 4)
        lwTitleBar = 2;

    switch (icon) {
    case CloseTabIcon:
        r.adjust(1, 1, -1, -1);
    case CloseIcon: {
        int lineWidth = 1;
        if (r.width() > 16) {
            lineWidth = 3;
        } else if (r.width() > 4) {
            lineWidth = 2;
        }

        drawObject(p, DiagonalLine, r.x(), r.y(), r.width(), lineWidth);
        drawObject(p, CrossDiagonalLine, r.x(), r.bottom(),
                   r.width(), lineWidth);
        break;
    }
    case MaxIcon:
        if (Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_TitleBarButtons, 0L, 0L) & TITLEBAR_BUTTOM_ARROW_MIN_MAX) {
            QStyleOption opt;
            opt.rect = r;
            opt.state = QStyle::State_Enabled | QtC_StateKWin;
            style->drawPrimitive(QStyle::PE_IndicatorArrowUp, &opt, &p, 0L);
        } else {
            int lineWidth2 = 1; // frame
            if (r.width() > 16) {
                lineWidth2 = 2;
            } else if (r.width() > 4) {
                lineWidth2 = 1;
            }
            drawObject(p, HorizontalLine, r.x(), r.top(),
                       r.width(), lwTitleBar);
            drawObject(p, HorizontalLine, r.x(), r.bottom() - (lineWidth2 - 1),
                       r.width(), lineWidth2);
            drawObject(p, VerticalLine, r.x(), r.top(), r.height(), lineWidth2);
            drawObject(p, VerticalLine, r.right() - (lineWidth2 - 1),
                       r.top(), r.height(), lineWidth2);
        }
        break;
    case MaxRestoreIcon:
        if (Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_TitleBarButtons, 0L, 0L) & TITLEBAR_BUTTOM_ARROW_MIN_MAX) {
            p.drawLine(r.x() + 1, r.y(), r.x() + r.width() - 2, r.y());
            p.drawLine(r.x() + 1, r.y() + r.height() - 1,
                       r.x() + r.width() - 2, r.y() + r.height() - 1);
            p.drawLine(r.x(), r.y() + 1, r.x(), r.y() + r.height() - 2);
            p.drawLine(r.x() + r.width() - 1, r.y() + 1,
                       r.x() + r.width() - 1, r.y() + r.height() - 2);
            p.drawRect(r.x() + 1, r.y() + 1, r.width() - 3, r.height() - 3);
        } else {
            int lineWidth2 = 1; // frame
            if (r.width() > 16) {
                lineWidth2 = 2;
            } else if (r.width() > 4) {
                lineWidth2 = 1;
            }
            int margin1, margin2;
            margin1 = margin2 = lineWidth2 * 2 + 1;
            if (r.width() < 8)
                margin1 = 1;
            int margin1h = margin1 - 1;
            int margin2h = margin2 - 1;

            // background window
            drawObject(p, HorizontalLine, r.x() + margin1h, r.top(),
                       r.width() - margin1, lwTitleBar);
            drawObject(p, HorizontalLine, r.right() - margin2h,
                       r.bottom() - (lineWidth2 - 1) - margin1, margin2h,
                       lineWidth2);
            drawObject(p, VerticalLine, r.x() + margin1h, r.top(), margin2,
                       lineWidth2);
            drawObject(p, VerticalLine, r.right() - (lineWidth2 - 1),
                       r.top(), r.height() - margin1, lineWidth2);

            // foreground window
            drawObject(p, HorizontalLine, r.x(), r.top() + margin2,
                       r.width() - margin2h, lwTitleBar);
            drawObject(p, HorizontalLine, r.x(),
                       r.bottom() - (lineWidth2 - 1), r.width() - margin2h,
                       lineWidth2);
            drawObject(p, VerticalLine, r.x(), r.top() + margin2, r.height(),
                       lineWidth2);
            drawObject(p, VerticalLine, r.right() - (lineWidth2 - 1) - margin2h,
                       r.top() + margin2, r.height(), lineWidth2);
        }
        break;
    case MinIcon:
        if (Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_TitleBarButtons, 0L, 0L)&TITLEBAR_BUTTOM_ARROW_MIN_MAX) {
            QStyleOption opt;

            opt.rect = r;
            opt.state = QStyle::State_Enabled | QtC_StateKWin;
            style->drawPrimitive(QStyle::PE_IndicatorArrowDown, &opt, &p, 0L);
        } else {
            drawObject(p, HorizontalLine, r.x(), r.bottom() - (lwTitleBar - 1),
                       r.width(), lwTitleBar);
        }
        break;
    case HelpIcon: {
        int center = r.x()+r.width()/2; // -1;
        int side = r.width()/4;

        // paint a question mark... code is quite messy,
        // to be cleaned up later...! :o
        if (r.width() > 16) {
            int lineWidth = 3;

            // top bar
            drawObject(p, HorizontalLine, center - side + 3, r.y(),
                       2 * side - 3 - 1, lineWidth);
            // top bar rounding
            drawObject(p, CrossDiagonalLine, center - side - 1, r.y() + 5,
                       6, lineWidth);
            drawObject(p, DiagonalLine, center + side - 3, r.y(), 5, lineWidth);
            // right bar
            drawObject(p, VerticalLine, center + side + 2 - lineWidth,
                       r.y() + 3, r.height() - (2 * lineWidth + side + 2 + 1),
                       lineWidth);
            // bottom bar
            drawObject(p, CrossDiagonalLine, center, r.bottom() - 2 * lineWidth,
                       side + 2, lineWidth);
            drawObject(p, HorizontalLine, center,
                       r.bottom() - 3 * lineWidth + 2, lineWidth, lineWidth);
            // the dot
            drawObject(p, HorizontalLine, center, r.bottom() - (lineWidth - 1),
                       lineWidth, lineWidth);
        } else if (r.width() > 8) {
            int lineWidth = 2;

            // top bar
            drawObject(p, HorizontalLine, center - (side - 1), r.y(),
                       2 * side - 1, lineWidth);
            // top bar rounding
            if (r.width() > 9)
                drawObject(p, CrossDiagonalLine, center - side - 1, r.y() + 3, 3, lineWidth);
            else
                drawObject(p, CrossDiagonalLine, center - side - 1, r.y() + 2, 3, lineWidth);
            drawObject(p, DiagonalLine, center + side - 1, r.y(), 3, lineWidth);
            // right bar
            drawObject(p, VerticalLine, center + side + 2 - lineWidth, r.y() + 2, r.height() - (2*lineWidth + side + 1), lineWidth);
            // bottom bar
            drawObject(p, CrossDiagonalLine, center, r.bottom() - 2*lineWidth + 1, side + 2, lineWidth);
            // the dot
            drawObject(p, HorizontalLine, center, r.bottom() - (lineWidth - 1), lineWidth, lineWidth);
        }
        else
        {
            int lineWidth = 1;

            // top bar
            drawObject(p, HorizontalLine, center - (side - 1), r.y(), 2*side, lineWidth);
            // top bar rounding
            drawObject(p, CrossDiagonalLine, center - side - 1, r.y() + 1, 2, lineWidth);
            // right bar
            drawObject(p, VerticalLine, center + side + 1, r.y(), r.height() - (side + 2 + 1), lineWidth);
            // bottom bar
            drawObject(p, CrossDiagonalLine, center, r.bottom() - 2, side + 2, lineWidth);
            // the dot
            drawObject(p, HorizontalLine, center, r.bottom(), 1, 1);
        }

        break;
    }
    case NotOnAllDesktopsIcon:
    {
        r.adjust(1, 1, -1, -1);

        int lwMark = (r.width() - 1)/2;
        if (lwMark < 1)
            lwMark = 3;

        drawObject(p, HorizontalLine, r.x(), r.y(), lwMark, lwMark);
        drawObject(p, HorizontalLine, r.x() + (r.width() - lwMark), r.y(), lwMark, lwMark);
        drawObject(p, HorizontalLine, r.x() + (r.width() - lwMark), r.y() + (r.height() - lwMark), lwMark, lwMark);
        drawObject(p, HorizontalLine, r.x(), r.y() + (r.height() - lwMark), lwMark, lwMark);
        break;
    }
    case OnAllDesktopsIcon:
    {
        int lwMark = r.width() - lwTitleBar*2 - 2;
        if (lwMark < 1)
            lwMark = 3;

        drawObject(p, HorizontalLine, r.x() + (r.width() - lwMark)/2, r.y() + (r.height() - lwMark)/2, lwMark, lwMark);
        break;
    }
    case NoKeepAboveIcon:
    case KeepAboveIcon:
    {
        QStyleOption opt;

        opt.rect=r.adjusted(0, -(1 + lwTitleBar), 0, 0);
        opt.rect.adjust(2, 2, -2, -2);
        opt.state=QStyle::State_Enabled|QtC_StateKWin;

        opt.rect.adjust(0, -1, 0, -1);
        if(NoKeepAboveIcon==icon)
        {
            int w=r.width()/3;
            if(0==w%2)
                w++;
            drawObject(p, HorizontalLine, r.x() + (r.width() - w)/2, r.y() + 1, w, 2);
        }
        else
            style->drawPrimitive(QStyle::PE_IndicatorArrowUp, &opt, &p, 0L);
        opt.rect.adjust(0, 4, 0, 4);
        style->drawPrimitive(QStyle::PE_IndicatorArrowUp, &opt, &p, 0L);
        break;
    }
    case NoKeepBelowIcon:
    case KeepBelowIcon:
    {
        QStyleOption opt;

        opt.rect=r.adjusted(0, -(1 + lwTitleBar), 0, 0);
        opt.rect.adjust(2, 2, -2, -2);
        opt.state=QStyle::State_Enabled|QtC_StateKWin;
        opt.rect.adjust(0, -1, 0, -1);
        style->drawPrimitive(QStyle::PE_IndicatorArrowDown, &opt, &p, 0L);
        opt.rect.adjust(0, 4, 0, 4);
        if(NoKeepBelowIcon==icon)
        {
            int w=r.width()/3;
            if(0==w%2)
                w++;
            drawObject(p, HorizontalLine, r.x() + (r.width() - w)/2, r.y() + r.height() - 3, w, 2);
        }
        else
            style->drawPrimitive(QStyle::PE_IndicatorArrowDown, &opt, &p, 0L);
        break;
    }
    case UnShadeIcon:
    case ShadeIcon: {
        QStyleOption opt;

        //opt.init(btn);
        opt.rect=r.adjusted(0, -(1 + lwTitleBar), 0, 0);
        opt.rect.adjust(2, 2, -2, -2);
        if(UnShadeIcon==icon)
            opt.rect.adjust(0, -1, 0, -1);
        opt.state=QStyle::State_Enabled|QtC_StateKWin;

        //opt.palette.setColor(QPalette::Button, Qt::red);
        style->drawPrimitive(ShadeIcon==icon ? QStyle::PE_IndicatorArrowDown
                             : QStyle::PE_IndicatorArrowUp,
                             &opt, &p, 0L);
        drawObject(p, HorizontalLine, r.x() + 1, r.bottom() - (lwTitleBar - 1), r.width() - 2, lwTitleBar);
        break;
    }
    case MenuIcon: {
        int offset=(r.height() - 7)/2;
        for (int i = 0;i < 3;i++)
            drawObject(p, HorizontalLine, r.x() + 1, r.y() + offset + (i*3), r.width() - 2, 1);
        break;
    }
    default:
        break;
    }

    p.end();
    bitmap.setMask(bitmap);

    return bitmap;
}

void
IconEngine::drawObject(QPainter &p, Object object, int x, int y,
                       int length, int lineWidth)
{
    switch(object) {
    case DiagonalLine:
        if (lineWidth <= 1) {
            for (int i = 0;i < length;i++) {
                p.drawPoint(x + i, y + i);
            }
        } else if (lineWidth <= 2) {
            for (int i = 0;i < length;i++) {
                p.drawPoint(x + i, y + i);
            }
            for (int i = 0;i < length - 1;i++) {
                p.drawPoint(x + 1 + i, y + i);
                p.drawPoint(x + i, y + 1 + i);
            }
        } else {
            for (int i = 1;i < length - 1;i++) {
                p.drawPoint(x + i, y + i);
            }
            for (int i = 0;i < length - 1;i++) {
                p.drawPoint(x + 1 + i, y + i);
                p.drawPoint(x + i, y + 1 + i);
            }
            for (int i = 0;i < (length - 2);i++) {
                p.drawPoint(x + 2 + i, y + i);
                p.drawPoint(x + i, y + 2 + i);
            }
        }
        break;
    case CrossDiagonalLine:
        if (lineWidth <= 1) {
            for (int i = 0;i < length;i++) {
                p.drawPoint(x + i, y - i);
            }
        } else if (lineWidth <= 2) {
            for (int i = 0;i < length;i++) {
                p.drawPoint(x + i, y - i);
            }
            for (int i = 0;i < length - 1;i++) {
                p.drawPoint(x + 1 + i, y - i);
                p.drawPoint(x + i, y - 1 - i);
            }
        } else {
            for (int i = 1;i < length - 1;i++) {
                p.drawPoint(x + i, y - i);
            }
            for (int i = 0;i < length - 1;i++) {
                p.drawPoint(x + 1 + i, y - i);
                p.drawPoint(x + i, y - 1 - i);
            }
            for (int i = 0;i < length - 2;i++) {
                p.drawPoint(x + 2 + i, y - i);
                p.drawPoint(x + i, y - 2 - i);
            }
        }
        break;
    case HorizontalLine:
        for (int i = 0;i < lineWidth;i++) {
            p.drawLine(x, y + i, x + length - 1, y + i);
        }
        break;
    case VerticalLine:
        for (int i = 0;i < lineWidth;i++) {
            p.drawLine(x + i, y, x + i, y + length - 1);
        }
    default:
        break;
    }
}

}
}
