/*****************************************************************************
 *   Copyright 2007 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
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
/*
  based on the window decoration "Plastik":
  Copyright (C) 2003-2005 Sandro Giessl <sandro@giessl.com>

  based on the window decoration "Web":
  Copyright (C) 2001 Rik Hemsley (rikkus) <rik@kde.org>
 */

#include <qtcurve-utils/x11wrap.h>
#include <qtcurve-utils/x11qtc.h>
#include <qtcurve-utils/log.h>

#define DRAW_INTO_PIXMAPS
#include <KDE/KLocale>
#include <QBitmap>
#include <QDateTime>
#include <QFontMetrics>
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QPixmap>
#include <QStyleOptionTitleBar>
#include <QStyle>
#include <QPainterPath>
#include <QLinearGradient>
#include <KDE/KColorUtils>
#include <KDE/KWindowInfo>
#include <KDE/KIconEffect>
#include <KDE/KWindowSystem>
#include <qdesktopwidget.h>
#include "qtcurveclient.h"
#include "qtcurvebutton.h"
#include "qtcurvetogglebutton.h"
#include "qtcurvesizegrip.h"
#include "tileset.h"
#include <common/common.h>

#include <style/qtcurve.h>

namespace QtCurve {
namespace KWin {

static const int constTitlePad = 4;

int getMenubarSizeProperty(WId wId)
{
    return qtcX11GetShortProp(wId, qtc_x11_qtc_menubar_size);
}

int getStatusbarSizeProperty(WId wId)
{
    return qtcX11GetShortProp(wId, qtc_x11_qtc_statusbar);
}

int getOpacityProperty(WId wId)
{
    int o = qtcX11GetShortProp(wId, qtc_x11_qtc_opacity);
    return o <= 0 || o >= 100 ? 100 : o;
}

void getBgndSettings(WId wId, EAppearance &app, QColor &col)
{
    auto reply = qtcX11GetProperty(0, wId, qtc_x11_qtc_bgnd,
                                   XCB_ATOM_CARDINAL, 0, 1);
    if (!reply) {
        return;
    }
    if (qtcX11GetPropertyValueLength(reply) > 0) {
        uint32_t val = *(int32_t*)qtcX11GetPropertyValue(reply);
        app = (EAppearance)(val & 0xFF);
        col.setRgb((val & 0xFF000000) >> 24, (val & 0x00FF0000) >> 16,
                   (val & 0x0000FF00) >> 8);
    }
    free(reply);
}

static QPainterPath createPath(const QRectF &r, double radiusTop, double radiusBot)
{
    QPainterPath path;
    double       diameterTop(radiusTop*2),
                 diameterBot(radiusBot*2);

    if (radiusBot>1.0)
        path.moveTo(r.x()+r.width(), r.y()+r.height()-radiusBot);
    else
        path.moveTo(r.x()+r.width(), r.y()+r.height());

    if (radiusTop>1.0)
        path.arcTo(r.x()+r.width()-diameterTop, r.y(), diameterTop, diameterTop, 0, 90);
    else
        path.lineTo(r.x()+r.width(), r.y());

    if (radiusTop>1.0)
        path.arcTo(r.x(), r.y(), diameterTop, diameterTop, 90, 90);
    else
        path.lineTo(r.x(), r.y());

    if (radiusBot>1.0)
        path.arcTo(r.x(), r.y()+r.height()-diameterBot, diameterBot, diameterBot, 180, 90);
    else
        path.lineTo(r.x(), r.y()+r.height());

    if (radiusBot>1.0)
        path.arcTo(r.x()+r.width()-diameterBot, r.y()+r.height()-diameterBot, diameterBot, diameterBot, 270, 90);
    else
        path.lineTo(r.x()+r.width(), r.y()+r.height());

    return path;
}

static void drawSunkenBevel(QPainter *p, const QRect &r, const QColor &bgnd, bool circular, int round)
{
    double          radius=circular
                            ? r.height()/2.0
                            : round>ROUND_FULL
                                ? 5.0
                                : round>ROUND_SLIGHT
                                    ? 3.0
                                    : 2.0;
    QPainterPath    path(createPath(QRectF(r), radius, radius));
    QLinearGradient g(r.topLeft(), r.bottomLeft());
    QColor          black(Qt::black),
                    white(Qt::white);

    black.setAlphaF(SUNKEN_BEVEL_DARK_ALPHA(bgnd));
    white.setAlphaF(SUNKEN_BEVEL_LIGHT_ALPHA(bgnd));
    g.setColorAt(0, black);
    g.setColorAt(1, white);
    p->fillPath(path, QBrush(g));
}

static QColor blendColors(const QColor &foreground, const QColor &background, double alpha)
{
    return KColorUtils::mix(background, foreground, alpha);
}

static void drawFadedLine(QPainter *painter, const QRect &r, const QColor &col, bool horiz=false, bool fadeStart=true, bool fadeEnd=true)
{
    bool            aa(painter->testRenderHint(QPainter::Antialiasing));
    QPointF         start(r.x()+(aa ? 0.5 : 0.0), r.y()+(aa ? 0.5 : 0.0)),
                    end(r.x()+(horiz ? r.width()-1 : 0)+(aa ? 0.5 : 0.0),
                        r.y()+(horiz ? 0 : r.height()-1)+(aa ? 0.5 : 0.0));
    QLinearGradient grad(start, end);
    QColor          c(col),
                    blank(Qt::white);

    c.setAlphaF(horiz ? 0.6 : 0.3);
    blank.setAlphaF(0.0);
    grad.setColorAt(0, fadeStart ? blank : c);
    grad.setColorAt(FADE_SIZE, c);
    grad.setColorAt(1.0-FADE_SIZE, c);
    grad.setColorAt(1, fadeEnd ? blank : c);
    painter->setPen(QPen(QBrush(grad), 1));
    painter->drawLine(start, end);
}

static void fillBackground(EAppearance app, QPainter &painter, const QColor &col, const QRect &fillRect, const QRect &widgetRect, const QPainterPath path)
{
    if (!qtcIsFlatBgnd(app) || !widgetRect.isEmpty()) {
        Style::BgndOption opt;
        opt.state|=QtC_StateKWin;
        opt.rect=fillRect;
        opt.widgetRect=widgetRect;
        opt.palette.setColor(QPalette::Window, col);
        opt.app=app;
        opt.path=path;
        Handler()->wStyle()->drawPrimitive(QtC_PE_DrawBackground, &opt, &painter, NULL);
    }
    else if(path.isEmpty())
        painter.fillRect(fillRect, col);
    else
        painter.fillPath(path, col);
}

// static inline bool isModified(const QString &title)
// {
//     return title.indexOf(i18n(" [modified] ")) > 3 ||
//            (title.length()>3 && QChar('*')==title[0] && QChar(' ')==title[1]);
// }

QtCurveClient::QtCurveClient(KDecorationBridge *bridge, QtCurveHandler *factory)
    : KCommonDecorationUnstable(bridge, factory),
      m_resizeGrip(0L),
      m_titleFont(QFont()),
      m_menuBarSize(-1),
      m_toggleMenuBarButton(0L),
      m_toggleStatusBarButton(0L)
      // m_hover(false)
{
    Handler()->addClient(this);
}

QtCurveClient::~QtCurveClient()
{
    Handler()->removeClient(this);
    deleteSizeGrip();
}

QString QtCurveClient::visibleName() const
{
    return i18n("QtCurve");
}

bool QtCurveClient::decorationBehaviour(DecorationBehaviour behaviour) const
{
    switch (behaviour)
    {
        case DB_MenuClose:
            return true;
        case DB_WindowMask:
            return false;
        default:
            return KCommonDecoration::decorationBehaviour(behaviour);
    }
}

int QtCurveClient::layoutMetric(LayoutMetric lm, bool respectWindowState,
                                const KCommonDecorationButton *btn) const
{
    switch (lm)
    {
        case LM_BorderLeft:
        case LM_BorderRight:
        case LM_BorderBottom:
            return respectWindowState && isMaximized() ? 0 : Handler()->borderSize(LM_BorderBottom==lm);
        case LM_TitleEdgeTop:
            return respectWindowState && isMaximized() ? 0 : Handler()->borderEdgeSize();
        case LM_TitleEdgeBottom:
            return respectWindowState && isMaximized() && Handler()->borderlessMax() ? 0 :  Handler()->borderEdgeSize();
        case LM_TitleEdgeLeft:
        case LM_TitleEdgeRight:
            return respectWindowState && isMaximized() ? 0 : Handler()->borderEdgeSize();
        case LM_TitleBorderLeft:
        case LM_TitleBorderRight:
            return 5;
        case LM_ButtonWidth:
        case LM_ButtonHeight:
        case LM_TitleHeight:
            return respectWindowState && isMaximized() && Handler()->borderlessMax()
                        ? 0
                        : respectWindowState && isToolWindow()
                            ? Handler()->titleHeightTool()
                            : Handler()->titleHeight();
        case LM_ButtonSpacing:
            return 0;
        case LM_ButtonMarginTop:
            return 0;
        case LM_OuterPaddingLeft:
        case LM_OuterPaddingRight:
        case LM_OuterPaddingTop:
        case LM_OuterPaddingBottom:
            if(Handler()->customShadows())
                return Handler()->shadowCache().shadowSize();
        default:
            return KCommonDecoration::layoutMetric(lm, respectWindowState, btn);
    }
}

KCommonDecorationButton *QtCurveClient::createButton(ButtonType type)
{
    switch (type) {
    case MenuButton:
    case OnAllDesktopsButton:
    case HelpButton:
    case MinButton:
    case MaxButton:
    case CloseButton:
    case AboveButton:
    case BelowButton:
    case ShadeButton:
#if KDE_IS_VERSION(4, 9, 85)
    case AppMenuButton:
#endif
        return new QtCurveButton(type, this);
    default:
        return 0;
    }
}

void QtCurveClient::init()
{
    m_titleFont=isToolWindow() ? Handler()->titleFontTool() : Handler()->titleFont();

    KCommonDecoration::init();
    widget()->setAutoFillBackground(false);
    widget()->setAttribute(Qt::WA_OpaquePaintEvent, true);
    widget()->setAttribute(Qt::WA_NoSystemBackground);

    if(Handler()->showResizeGrip())
        createSizeGrip();
    if (isPreview())
        m_caption =  isActive() ? i18n("Active Window") : i18n("Inactive Window");
    else
        captionChange();
}

void QtCurveClient::maximizeChange()
{
    reset(SettingBorder);
    if(m_resizeGrip)
        m_resizeGrip->setVisible(!(isShade() || isMaximized()));
    KCommonDecoration::maximizeChange();
}

void QtCurveClient::shadeChange()
{
    if(m_resizeGrip)
        m_resizeGrip->setVisible(!(isShade() || isMaximized()));
    KCommonDecoration::shadeChange();
}

void QtCurveClient::activeChange()
{
    if(m_resizeGrip && !(isShade() || isMaximized()))
    {
        m_resizeGrip->activeChange();
        m_resizeGrip->update();
    }

    informAppOfActiveChange();
    KCommonDecoration::activeChange();
}

void QtCurveClient::captionChange()
{
    m_caption=caption();
    widget()->update();
}

void QtCurveClient::paintEvent(QPaintEvent *e)
{
    bool compositing = compositingActive();
    QPainter painter(widget());
    QRect r(widget()->rect());
    QStyleOptionTitleBar opt;
    int windowBorder(Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_WindowBorder, 0L, 0L));
    bool active(isActive());
    bool colorTitleOnly(windowBorder&WINDOW_BORDER_COLOR_TITLEBAR_ONLY);
    bool roundBottom(Handler()->roundBottom());
    bool preview(isPreview());
    bool blend(!preview && Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_BlendMenuAndTitleBar, NULL, NULL));
    bool menuColor(windowBorder&WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR);
    bool separator(active && windowBorder&WINDOW_BORDER_SEPARATOR);
    bool maximized(isMaximized());
    QtCurveConfig::Shade outerBorder(Handler()->outerBorder()),
                         innerBorder(Handler()->innerBorder());
    const int            border(Handler()->borderEdgeSize()),
                         titleHeight(layoutMetric(LM_TitleHeight)),
                         titleEdgeTop(layoutMetric(LM_TitleEdgeTop)),
                         titleEdgeBottom(layoutMetric(LM_TitleEdgeBottom)),
                         titleEdgeLeft(layoutMetric(LM_TitleEdgeLeft)),
                         titleEdgeRight(layoutMetric(LM_TitleEdgeRight)),
                         titleBarHeight(titleHeight+titleEdgeTop+titleEdgeBottom+(isMaximized() ? border : 0)),
                         round=Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_Round, NULL, NULL),
                         buttonFlags=Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_TitleBarButtons, NULL, NULL);
    int                  rectX, rectY, rectX2, rectY2, shadowSize(0),
                         kwinOpacity(compositing ? Handler()->opacity(active) : 100),
                         opacity(kwinOpacity);
    EAppearance          bgndAppearance=APPEARANCE_FLAT;
    QColor               windowCol(widget()->palette().color(QPalette::Window));

    getBgndSettings(windowId(), bgndAppearance, windowCol);

    QColor               col(KDecoration::options()->color(KDecoration::ColorTitleBar, active)),
                         fillCol(colorTitleOnly ? windowCol : col);
                         // APPEARANCE_RAISED is used to signal flat background, but have background image!
                         // In which case we still have a custom background to draw.
    bool customBgnd = bgndAppearance != APPEARANCE_FLAT;
    bool customShadows = Handler()->customShadows();

    painter.setClipRegion(e->region());

    if(!preview && compositing && 100==opacity)
        opacity=getOpacityProperty(windowId());

    if (customShadows) {
        shadowSize = Handler()->shadowCache().shadowSize();

        if (compositing) {
            TileSet *tileSet=Handler()->shadowCache().tileSet(this, roundBottom);
            if (opacity < 100) {
                painter.save();
                painter.setClipRegion(QRegion(r).subtract(getMask(round, r.adjusted(shadowSize, shadowSize, -shadowSize, -shadowSize))), Qt::IntersectClip);
            }

            if(!isMaximized())
                tileSet->render(r.adjusted(5, 5, -5, -5), &painter, TileSet::Ring);
            else if(isShade())
                tileSet->render(r.adjusted(0, 5, 0, -5), &painter, TileSet::Bottom);
            if(opacity<100)
                painter.restore();
        }
        r.adjust(shadowSize, shadowSize, -shadowSize, -shadowSize);
    }

    int   side(layoutMetric(LM_BorderLeft)),
          bot(layoutMetric(LM_BorderBottom));
    QRect widgetRect(widget()->rect().adjusted(shadowSize+side, shadowSize+titleBarHeight, -(shadowSize+side), -(shadowSize+bot)));

    r.getCoords(&rectX, &rectY, &rectX2, &rectY2);

    if(!preview && (blend||menuColor) && -1==m_menuBarSize)
    {
        QString wc(windowClass());
        if(wc==QLatin1String("W Navigator Firefox browser") ||
           wc==QLatin1String("W Navigator Firefox view-source") ||
           wc==QLatin1String("W Mail Thunderbird 3pane") ||
           wc==QLatin1String("W Mail Thunderbird addressbook") ||
           wc==QLatin1String("W Mail Thunderbird messageWindow") ||
           wc==QLatin1String("D Calendar Thunderbird EventDialog") ||
           wc==QLatin1String("W Msgcompose Thunderbird Msgcompose") ||
           wc==QLatin1String("D Msgcompose Thunderbird Msgcompose"))
            m_menuBarSize=QFontMetrics(QApplication::font()).height()+8;

        else if(
#if 0 // Currently LibreOffice does not seem to pain menubar backgrounds for KDE - so disable check here...
                wc.startsWith(QLatin1String("W VCLSalFrame libreoffice-")) ||
                wc.startsWith(QLatin1String("W VCLSalFrame.DocumentWindow libreoffice-")) ||
                //wc==QLatin1String("W soffice.bin Soffice.bin") ||
#endif
                wc.startsWith(QLatin1String("W VCLSalFrame.DocumentWindow OpenOffice.org")) ||
                wc.startsWith(QLatin1String("W VCLSalFrame OpenOffice.org")) ||
                wc==QLatin1String("W soffice.bin Soffice.bin"))
            m_menuBarSize=QFontMetrics(QApplication::font()).height()+7;
        else
        {
            int val=getMenubarSizeProperty(windowId());
            if(val>-1)
                m_menuBarSize=val;
        }
    }

    if(menuColor && m_menuBarSize>0 &&
       (active || !Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_ShadeMenubarOnlyWhenActive, NULL, NULL)))
        col=QColor(QRgb(Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_MenubarColor, NULL, NULL)));

    if(opacity<100)
    {
        double alpha=opacity/100.0;
        col.setAlphaF(alpha);
        windowCol.setAlphaF(alpha);
        fillCol.setAlphaF(alpha);
    }

    opt.init(widget());
    opt.state=QStyle::State_Horizontal|QStyle::State_Enabled|QStyle::State_Raised|
             (active ? QStyle::State_Active : QStyle::State_None)|QtC_StateKWin;

    int   fillTopAdjust(maximized ? Handler()->borderEdgeSize() : 0),
          fillBotAdjust(maximized ? Handler()->borderSize(true) : 0),
          fillSideAdjust(maximized ? Handler()->borderSize(false) : 0);
    QRect fillRect(r.adjusted(-fillSideAdjust, -fillTopAdjust, fillSideAdjust, fillBotAdjust));
    bool  clipRegion((outerBorder || round<=ROUND_SLIGHT) && !maximized);

    painter.setRenderHint(QPainter::Antialiasing, true);
    if(clipRegion)
    {
        painter.save();
        painter.setClipRegion(getMask(round, r));
    }
    else if(maximized)
        painter.setClipRect(r, Qt::IntersectClip);

    //
    // Fill titlebar and border backgrounds...
    //
    QPainterPath fillPath(maximized || round<=ROUND_SLIGHT
                                ? QPainterPath()
                                : createPath(QRectF(fillRect),
                                             APPEARANCE_NONE==Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_TitleBarApp,
                                                                                               &opt, NULL) ? 6.0 : 8.0,
                                             roundBottom ? 6.0 : 1.0));


    bool fillTitlebar(windowBorder&WINDOW_BORDER_FILL_TITLEBAR),
         opaqueBorder(opacity<100 && Handler()->opaqueBorder());

    if(opaqueBorder)
    {
        QColor fc(fillCol);
        fc.setAlphaF(1.0);
        if(!clipRegion)
            painter.save();
        painter.setClipRect(r.adjusted(0, titleBarHeight, 0, 0), Qt::IntersectClip);
        fillBackground(bgndAppearance, painter, fc, fillRect, widgetRect, fillPath);
        if(fillTitlebar)
        {
            painter.save();
            painter.setClipRect(QRect(r.x(), r.y(), r.width(), titleBarHeight), Qt::IntersectClip);
            fillBackground(bgndAppearance, painter, fillCol, fillRect, widgetRect, fillPath);
            painter.restore();
        }
    }
    else
    {
        if(!fillTitlebar)
        {
            if(!clipRegion)
                painter.save();
            if(!compositing)
            {
                // Not compositing, so need to fill with background colour...
                if(fillPath.isEmpty())
                    painter.fillRect(QRect(r.x(), r.y(), r.width(), titleBarHeight), windowCol);
                else
                    painter.fillPath(fillPath, windowCol);
            }

            painter.setClipRect(r.adjusted(0, titleBarHeight, 0, 0), Qt::IntersectClip);
        }
        fillBackground(bgndAppearance, painter, fillCol, fillRect, widgetRect, fillPath);
    }

    if(clipRegion || opaqueBorder || !fillTitlebar)
        painter.restore();
    //
    // Titlebar and border backgrounds filled.
    //

    if(maximized)
        r.adjust(-3, -border, 3, 0);
    opt.palette.setColor(QPalette::Button, col);
    opt.palette.setColor(QPalette::Window, windowCol);
    opt.rect=QRect(r.x(), r.y()+6, r.width(), r.height()-6);


    if(!roundBottom)
        opt.state|=QtC_StateKWinNotFull;

    if(compositing && !preview)
        opt.state|=QtC_StateKWinCompositing;

    if (outerBorder) {
        if (outerBorder == QtCurveConfig::SHADE_SHADOW && customShadows) {
            opt.version = TBAR_BORDER_VERSION_HACK + 2;
            opt.palette.setColor(QPalette::Shadow, blendColors(Handler()->shadowCache().color(active), windowCol, active ? 0.75 : 0.4));
        } else {
            opt.version = (outerBorder == QtCurveConfig::SHADE_LIGHT ? 0 : 1) +
                TBAR_BORDER_VERSION_HACK;
        }
        painter.save();
        painter.setClipRect(r.adjusted(0, titleBarHeight, 0, 0), Qt::IntersectClip);
#ifdef DRAW_INTO_PIXMAPS
        if(!compositing && !preview)
        {
            // For some reason, on Jaunty drawing directly is *hideously* slow on intel graphics card!
            QPixmap pix(32, 32);
            pix.fill(Qt::transparent);
            QPainter p2(&pix);
            p2.setRenderHint(QPainter::Antialiasing, true);
            opt.rect=QRect(0, 0, pix.width(), pix.height());
            Handler()->wStyle()->drawPrimitive(QStyle::PE_FrameWindow, &opt, &p2, widget());
            p2.end();
            painter.drawTiledPixmap(r.x(), r.y()+10, 2, r.height()-18, pix.copy(0, 8, 2, 16));
            painter.drawTiledPixmap(r.x()+r.width()-2, r.y()+8, 2, r.height()-16, pix.copy(pix.width()-2, 8, 2, 16));
            painter.drawTiledPixmap(r.x()+8, r.y()+r.height()-2, r.width()-16, 2, pix.copy(8, pix.height()-2, 16, 2));
            painter.drawPixmap(r.x(), r.y()+r.height()-8, pix.copy(0, 24, 8, 8));
            painter.drawPixmap(r.x()+r.width()-8, r.y()+r.height()-8, pix.copy(24, 24, 8, 8));
        }
        else
#endif
            Handler()->wStyle()->drawPrimitive(QStyle::PE_FrameWindow, &opt, &painter, widget());
        painter.restore();
    }
    else
        opt.state|=QtC_StateKWinNoBorder;

    if(kwinOpacity<100 && (blend||menuColor))
    {
        // Turn off opacity for titlebar...
        QColor c(col);
        c.setAlphaF(1.0);
        opt.palette.setColor(QPalette::Button, c);
        c=windowCol;
        c.setAlphaF(1.0);
        opt.palette.setColor(QPalette::Window, c);
    }

    opt.rect=QRect(r.x(), r.y(), r.width(), titleBarHeight);
    opt.titleBarState=(active ? QStyle::State_Active : QStyle::State_None)|QtC_StateKWin;

    if(!preview && !isShade() && blend && -1!=m_menuBarSize)
        opt.rect.adjust(0, 0, 0, m_menuBarSize);

    // Remove QtC_StateKWinNotFull settings - as its the same value as QtC_StateKWinFillBgnd, and it is not
    // used in CC_TitleBar...
    if(!roundBottom)
        opt.state&=~QtC_StateKWinNotFull;

#ifdef DRAW_INTO_PIXMAPS
    if(!compositing && !preview && !customBgnd)
    {
        QPixmap  tPix(32, titleBarHeight+(!blend || m_menuBarSize<0 ? 0 : m_menuBarSize));
        QPainter tPainter(&tPix);
        tPainter.setRenderHint(QPainter::Antialiasing, true);
        if(!customBgnd)
            opt.state|=QtC_StateKWinFillBgnd;
        opt.rect=QRect(0, 0, tPix.width(), tPix.height());
        Handler()->wStyle()->drawComplexControl(QStyle::CC_TitleBar, &opt, &tPainter, widget());
        tPainter.end();
        painter.drawTiledPixmap(r.x()+12, r.y(), r.width()-24, tPix.height(), tPix.copy(8, 0, 16, tPix.height()));
        painter.drawPixmap(r.x(), r.y(), tPix.copy(0, 0, 16, tPix.height()));
        painter.drawPixmap(r.x()+r.width()-16, r.y(), tPix.copy(tPix.width()-16, 0, 16, tPix.height()));
    }
    else
#endif
        Handler()->wStyle()->drawComplexControl(QStyle::CC_TitleBar, &opt, &painter, widget());

    if(outerBorder && innerBorder)
    {
        QStyleOptionFrame frameOpt;

        frameOpt.palette=opt.palette;
        if (innerBorder == QtCurveConfig::SHADE_SHADOW && customShadows) {
            frameOpt.version = 2 + TBAR_BORDER_VERSION_HACK;
            frameOpt.palette.setColor(QPalette::Shadow, blendColors(Handler()->shadowCache().color(active), windowCol, active ? 0.75 : 0.4));
        } else {
            frameOpt.version = (innerBorder == QtCurveConfig::SHADE_LIGHT ? 0 : 1)+TBAR_BORDER_VERSION_HACK;
        }
        frameOpt.rect=widgetRect.adjusted(-1, -1, 1, 1);
        frameOpt.state=(active ? QStyle::State_Active : QStyle::State_None)|QtC_StateKWin;
        frameOpt.lineWidth=frameOpt.midLineWidth=1;
        Handler()->wStyle()->drawPrimitive(QStyle::PE_Frame, &frameOpt, &painter, widget());
    }

    if(buttonFlags&TITLEBAR_BUTTON_SUNKEN_BACKGROUND)
    {
        int  hOffset=2,
             vOffset=hOffset+(outerBorder ? 1 :0),
             posAdjust=maximized || outerBorder ? 2 : 0,
             edgePad=Handler()->edgePad();
        bool menuIcon=TITLEBAR_ICON_MENU_BUTTON==Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_TitleBarIcon, 0L, 0L),
             menuOnlyLeft=menuIcon && onlyMenuIcon(true),
             menuOnlyRight=menuIcon && !menuOnlyLeft && onlyMenuIcon(false);

        if(!menuOnlyLeft && buttonsLeftWidth()>(titleBarHeight-2*hOffset))
            drawSunkenBevel(&painter, QRect(r.left()+hOffset+posAdjust+edgePad+(compositing ? 0 : -1), r.top()+vOffset+edgePad,
                                            buttonsLeftWidth()-hOffset+(compositing ? 0 : 2),
                                            titleBarHeight-2*(vOffset+edgePad)), col, buttonFlags&TITLEBAR_BUTTON_ROUND, round);
        if(!menuOnlyRight && buttonsRightWidth()>(titleBarHeight-2*hOffset))
            drawSunkenBevel(&painter, QRect(r.right()-(buttonsRightWidth()+posAdjust+edgePad), r.top()+vOffset+edgePad,
                                            buttonsRightWidth(), titleBarHeight-2*(vOffset+edgePad)), col, buttonFlags&TITLEBAR_BUTTON_ROUND, round);
    }

    bool showIcon=TITLEBAR_ICON_NEXT_TO_TITLE==Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_TitleBarIcon,  0L, 0L);
    int  iconSize=showIcon ? Handler()->wStyle()->pixelMetric(QStyle::PM_SmallIconSize) : 0;

    m_captionRect=captionRect(); // also update m_captionRect!
    paintTitle(&painter, m_captionRect, QRect(rectX+titleEdgeLeft, m_captionRect.y(),
                                              rectX2-(titleEdgeRight+rectX+titleEdgeLeft),
                                              m_captionRect.height()),
               m_caption, showIcon ? icon().pixmap(iconSize) : QPixmap(), shadowSize);

    bool hideToggleButtons(true);
    int  toggleButtons(Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_ToggleButtons, NULL, NULL));

    if(toggleButtons)
    {
        if(!m_toggleMenuBarButton && toggleButtons&0x01 && (Handler()->wasLastMenu(windowId()) || getMenubarSizeProperty(windowId())>-1))
            m_toggleMenuBarButton=createToggleButton(true);
        if(!m_toggleStatusBarButton && toggleButtons&0x02 && (Handler()->wasLastStatus(windowId()) || getStatusbarSizeProperty(windowId())>-1))
            m_toggleStatusBarButton=createToggleButton(false);

        // if (m_hover)
        {
            if (active && (m_toggleMenuBarButton||m_toggleStatusBarButton)) {
                if( (buttonsLeftWidth()+buttonsRightWidth()+constTitlePad+
                    (m_toggleMenuBarButton ? m_toggleMenuBarButton->width() : 0) +
                    (m_toggleStatusBarButton ? m_toggleStatusBarButton->width() : 0)) < r.width())
                {
                    int  align(Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_TitleAlignment, 0L, 0L));
                    bool onLeft(align&Qt::AlignRight);

                    if(align&Qt::AlignHCenter)
                    {
                        QString left=options()->customButtonPositions() ? options()->titleButtonsLeft() : defaultButtonsLeft(),
                                right=options()->customButtonPositions() ? options()->titleButtonsRight() : defaultButtonsRight();
                        onLeft=left.length()<right.length();
                    }

                    int     offset=2,
                            posAdjust=maximized ? 2 : 0;
                    QRect   cr(onLeft
                                ? r.left()+buttonsLeftWidth()+posAdjust+constTitlePad+2
                                : r.right()-(buttonsRightWidth()+posAdjust+constTitlePad+2+
                                            (m_toggleMenuBarButton ? m_toggleMenuBarButton->width() : 0)+
                                            (m_toggleStatusBarButton ? m_toggleStatusBarButton->width() : 0)),
                            r.top()+offset,
                            (m_toggleMenuBarButton ? m_toggleMenuBarButton->width() : 0)+
                            (m_toggleStatusBarButton ? m_toggleStatusBarButton->width() : 0),
                            titleBarHeight-2*offset);

                    if(m_toggleMenuBarButton)
                    {
                        m_toggleMenuBarButton->move(cr.x(), r.y()+3+(outerBorder ? 2 : 0));
                        m_toggleMenuBarButton->show();
                    }
                    if(m_toggleStatusBarButton)
                    {
                        m_toggleStatusBarButton->move(cr.x()+(m_toggleMenuBarButton ? m_toggleMenuBarButton->width()+2 : 0),
                                                       r.y()+3+(outerBorder ? 2 : 0));
                        m_toggleStatusBarButton->show();
                    }
                    hideToggleButtons=false;
                }
            }
        }
    }
    if(hideToggleButtons)
    {
        if(m_toggleMenuBarButton)
            m_toggleMenuBarButton->hide();
        if(m_toggleStatusBarButton)
            m_toggleStatusBarButton->hide();
    }

    if(separator)
    {
        QColor        color(KDecoration::options()->color(KDecoration::ColorFont, isActive()));
        Qt::Alignment align((Qt::Alignment)Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_TitleAlignment, 0L, 0L));

        r.adjust(16, titleBarHeight-1, -16, 0);
        color.setAlphaF(0.5);
        drawFadedLine(&painter, r, color, true, align&(Qt::AlignHCenter|Qt::AlignRight), align&(Qt::AlignHCenter|Qt::AlignLeft));
    }

    painter.end();
}

void
QtCurveClient::paintTitle(QPainter *painter, const QRect &capRect,
                          const QRect &alignFullRect, const QString &cap,
                          const QPixmap &pix, int shadowSize, bool isTab,
                          bool isActiveTab)
{
    int iconX = capRect.x();
    bool showIcon = !pix.isNull() && capRect.width()>pix.width();

    if(!cap.isEmpty())
    {
        painter->setFont(m_titleFont);

        QFontMetrics  fm(painter->fontMetrics());
        QString       str(fm.elidedText(cap, Qt::ElideRight,
                            capRect.width()-(showIcon ? pix.width()+constTitlePad : 0), QPalette::WindowText));
        Qt::Alignment hAlign((Qt::Alignment)Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_TitleAlignment, 0L, 0L)),
                      alignment(Qt::AlignVCenter|hAlign);
        bool          alignFull(!isTab && Qt::AlignHCenter==hAlign),
                      reverse=Qt::RightToLeft==QApplication::layoutDirection(),
                      iconRight((!reverse && alignment&Qt::AlignRight) || (reverse && alignment&Qt::AlignLeft));
        QRect         textRect(alignFull ? alignFullRect : capRect);
        int           textWidth=alignFull || (showIcon && alignment&Qt::AlignHCenter)
                                    ? fm.boundingRect(str).width()+(showIcon ? pix.width()+constTitlePad : 0) : 0;
        EEffect       effect((EEffect)(Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_TitleBarEffect)));

        if(alignFull)
        {
            int halfWidth=(textWidth+(showIcon ? pix.width()+constTitlePad : 0))/2;

            if(capRect.left()>(textRect.x()+(textRect.width()/2)-halfWidth))
            {
                alignment=Qt::AlignVCenter|Qt::AlignLeft;
                textRect=capRect;
                hAlign=Qt::AlignLeft;
            }
            else if (capRect.right()<(textRect.x()+(textRect.width()/2)+halfWidth))
            {
                alignment=Qt::AlignVCenter|Qt::AlignRight;
                textRect=capRect;
                hAlign=Qt::AlignRight;
            }
        }

        if(showIcon)
        {
            if(alignment&Qt::AlignHCenter)
            {
                if(reverse)
                {
                    iconX=((textRect.width()-textWidth)/2.0)+0.5+textWidth+pix.width();
                    textRect.setX(textRect.x()-(pix.width()+constTitlePad));
                }
                else
                {
//                     iconX=(((textRect.width()-textWidth)/2.0)+0.5)+
//                             (shadowSize ? (Qt::AlignHCenter==hAlign ? shadowSize : capRect.x()) : 0)+
//                             (isTab ? capRect.x() : 0);

                    int adjustment=textRect==capRect ? capRect.x() : 0;

                    iconX=(((textRect.width()-textWidth)/2.0)+0.5)+
                            (shadowSize
                                ? (Qt::AlignHCenter==hAlign ? shadowSize : capRect.x())
                                : (isTab ? 0 : adjustment))+
                            (isTab ? capRect.x() : 0);
                    textRect.setX(iconX+pix.width()+constTitlePad);
                    alignment=Qt::AlignVCenter|Qt::AlignLeft;
                }
            }
            else if((!reverse && alignment&Qt::AlignLeft) || (reverse && alignment&Qt::AlignRight))
            {
                iconX=textRect.x();
                textRect.setX(textRect.x()+(pix.width()+constTitlePad));
            }
            else if((!reverse && alignment&Qt::AlignRight) || (reverse && alignment&Qt::AlignLeft))
            {
                if(iconRight)
                {
                    iconX=textRect.x()+textRect.width()-pix.width();
                    textRect.setWidth(textRect.width()-(pix.width()+constTitlePad));
                }
                else
                {
                    iconX=textRect.x()+textRect.width()-(textWidth+constTitlePad);
                    if(iconX<textRect.x())
                        iconX=textRect.x();
                }
            }
        }

//         painter->setClipRect(capRect.adjusted(-2, 0, 2, 0));

        QColor color(KDecoration::options()->color(KDecoration::ColorFont, isActive())),
               bgnd(KDecoration::options()->color(KDecoration::ColorTitleBar, isActive()));

        if(isTab && !isActiveTab)
            textRect.adjust(0, 1, 0, 1);

        if((!isTab || isActiveTab) && EFFECT_NONE!=effect)
        {
//             QColor shadow(WINDOW_SHADOW_COLOR(effect));
//             shadow.setAlphaF(WINDOW_TEXT_SHADOW_ALPHA(effect));
//             painter->setPen(shadow);
            painter->setPen(blendColors(WINDOW_SHADOW_COLOR(effect), bgnd, WINDOW_TEXT_SHADOW_ALPHA(effect)));
            painter->drawText(EFFECT_SHADOW==effect
                                ? textRect.adjusted(1, 1, 1, 1)
                                : textRect.adjusted(0, 1, 0, 1),
                              alignment, str);

            if (!isActive() && DARK_WINDOW_TEXT(color))
            {
                //color.setAlpha((color.alpha() * 180) >> 8);
                color=blendColors(color, bgnd, ((255 * 180) >> 8)/256.0);
            }
        }

//         if(isTab && !isActiveTab)
//             color.setAlphaF(0.45);
//         painter->setPen(color);
        painter->setPen(isTab && !isActiveTab ? blendColors(color, bgnd, 0.45) : color);
        painter->drawText(textRect, alignment, str);
//         painter->setClipping(false);
    }

    if(showIcon && iconX>=0)
    {
//         if(isModified(cap))
//         {
//             QPixmap mod=pix;
//             KIconEffect::semiTransparent(mod);
//             painter->drawPixmap(iconX, capRect.y()+((capRect.height()-pix.height())/2)+1+(isTab && !isActiveTab ? 1 : 0), mod);
//         }
//         else
            painter->drawPixmap(iconX, capRect.y()+((capRect.height()-pix.height())/2)+1+(isTab && !isActiveTab ? 1 : 0), pix);
    }
}

void QtCurveClient::updateWindowShape()
{
    if (isMaximized()) {
        clearMask();
    } else {
        QRect r(Handler()->customShadows() ?
                widget()->rect()
                .adjusted(layoutMetric(LM_OuterPaddingLeft),
                          layoutMetric(LM_OuterPaddingTop),
                          -layoutMetric(LM_OuterPaddingRight),
                          compositingActive() ? 0 :
                          -layoutMetric(LM_OuterPaddingBottom)) :
                widget()->rect());

        setMask(getMask(Handler()->wStyle()->pixelMetric(
                            (QStyle::PixelMetric)QtC_Round, NULL, NULL), r));
    }
}

QRegion QtCurveClient::getMask(int round, const QRect &r) const
{
    int x, y, w, h;

    r.getRect(&x, &y, &w, &h);

    switch(round)
    {
        case ROUND_NONE:
            return  QRegion(x, y, w, h);
        case ROUND_SLIGHT:
        {
            QRegion mask(x+1, y, w-2, h);
            mask += QRegion(x, y+1, 1, h-2);
            mask += QRegion(x+w-1, y+1, 1, h-2);

            return mask;
        }
        default: // ROUND_FULL
        {
            bool    roundBottom=!isShade() && Handler()->roundBottom();

// #if KDE_IS_VERSION(4, 3, 0)
//             if(!isPreview() && COMPOSITING_ENABLED)
//             {
//                 QRegion mask(x+4, y, w-8, h);
//
//                 if(roundBottom)
//                 {
//                     mask += QRegion(x, y+4, 1, h-8);
//                     mask += QRegion(x+1, y+2, 1, h-4);
//                     mask += QRegion(x+2, y+1, 1, h-2);
//                     mask += QRegion(x+3, y+1, 1, h-2);
//                     mask += QRegion(x+w-1, y+4, 1, h-8);
//                     mask += QRegion(x+w-2, y+2, 1, h-4);
//                     mask += QRegion(x+w-3, y+1, 1, h-2);
//                     mask += QRegion(x+w-4, y+1, 1, h-2);
//                 }
//                 else
//                 {
//                     mask += QRegion(x, y+4, 1, h-4);
//                     mask += QRegion(x+1, y+2, 1, h-1);
//                     mask += QRegion(x+2, y+1, 1, h);
//                     mask += QRegion(x+3, y+1, 1, h);
//                     mask += QRegion(x+w-1, y+4, 1, h-4);
//                     mask += QRegion(x+w-2, y+2, 1, h-1);
//                     mask += QRegion(x+w-3, y+1, 1, h-0);
//                     mask += QRegion(x+w-4, y+1, 1, h-0);
//                 }
//                 return mask;
//             }
//             else
// #endif
            {
                QRegion mask(x+5, y, w-10, h);

                if(roundBottom)
                {
                    mask += QRegion(x, y+5, 1, h-10);
                    mask += QRegion(x+1, y+3, 1, h-6);
                    mask += QRegion(x+2, y+2, 1, h-4);
                    mask += QRegion(x+3, y+1, 2, h-2);
                    mask += QRegion(x+w-1, y+5, 1, h-10);
                    mask += QRegion(x+w-2, y+3, 1, h-6);
                    mask += QRegion(x+w-3, y+2, 1, h-4);
                    mask += QRegion(x+w-5, y+1, 2, h-2);
                }
                else
                {
                    mask += QRegion(x, y+5, 1, h-5);
                    mask += QRegion(x+1, y+3, 1, h-3);
                    mask += QRegion(x+2, y+2, 1, h-2);
                    mask += QRegion(x+3, y+1, 2, h-1);
                    mask += QRegion(x+w-1, y+5, 1, h-5);
                    mask += QRegion(x+w-2, y+3, 1, h-3);
                    mask += QRegion(x+w-3, y+2, 1, h-2);
                    mask += QRegion(x+w-5, y+1, 2, h-1);
                }
                return mask;
            }
        }
    }

    return QRegion();
}

bool QtCurveClient::onlyMenuIcon(bool left) const
{
    return QLatin1String("M")==
                (left
                    ? (options()->customButtonPositions() ? options()->titleButtonsLeft() : defaultButtonsLeft())
                    : (options()->customButtonPositions() ? options()->titleButtonsRight() : defaultButtonsRight()));
}

QRect QtCurveClient::captionRect() const
{
    QRect     r(widget()->rect());
    const int titleHeight(layoutMetric(LM_TitleHeight)),
              titleEdgeTop(layoutMetric(LM_TitleEdgeTop)),
              titleEdgeLeft(layoutMetric(LM_TitleEdgeLeft)),
              marginLeft(layoutMetric(LM_TitleBorderLeft)),
              marginRight(layoutMetric(LM_TitleBorderRight)),
              titleLeft(r.left() + titleEdgeLeft + buttonsLeftWidth() + marginLeft),
              titleWidth(r.width() -
                           titleEdgeLeft - layoutMetric(LM_TitleEdgeRight) -
                           buttonsLeftWidth() - buttonsRightWidth() -
                           marginLeft - marginRight);
    if (Handler()->customShadows()) {
        int shadowSize = Handler()->shadowCache().shadowSize();
        return QRect(titleLeft + shadowSize,
                     r.top() + titleEdgeTop + shadowSize,
                     titleWidth - shadowSize * 2, titleHeight);
    }
    return QRect(titleLeft, r.top() + titleEdgeTop, titleWidth, titleHeight);
}

void QtCurveClient::updateCaption()
{
    QRect oldCaptionRect(m_captionRect);

    m_captionRect=QtCurveClient::captionRect();

    if (oldCaptionRect.isValid() && m_captionRect.isValid())
        widget()->update(oldCaptionRect|m_captionRect);
    else
        widget()->update();
}

bool QtCurveClient::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::StyleChange) {
        Handler()->setStyle();
    }

    // if (widget() == o) {
    //     if (m_toggleMenuBarButton || m_toggleStatusBarButton)
    //         switch (e->type()) {
    //         case QEvent::Enter:
    //             m_hover = true;
    //             widget()->update();
    //             break;
    //         case QEvent::Leave:
    //             m_hover = false;
    //             widget()->update();
    //         default:
    //             break;
    //         }
    //     return true;
    // }

    return KCommonDecoration::eventFilter(o, e);
}

void QtCurveClient::reset(unsigned long changed)
{
    if (changed & (SettingColors | SettingFont | SettingBorder)) {
        // Reset button backgrounds...
        for(int i=0; i<constNumButtonStates; ++i)
           m_buttonBackground[i].pix=QPixmap();
    }

    if (changed&SettingBorder)
    {
        if (maximizeMode()==MaximizeFull)
        {
            if (!options()->moveResizeMaximizedWindows() && m_resizeGrip)
                m_resizeGrip->hide();
        }
        else if (m_resizeGrip)
                m_resizeGrip->show();
    }

    if (changed&SettingColors)
    {
        // repaint the whole thing
        widget()->update();
        updateButtons();
    }
    else if (changed&SettingFont)
    {
        // font has changed -- update title height and font
        m_titleFont=isToolWindow() ? Handler()->titleFontTool() : Handler()->titleFont();

        updateLayout();
        widget()->update();
    }

    if(Handler()->showResizeGrip())
        createSizeGrip();
    else
        deleteSizeGrip();

    KCommonDecoration::reset(changed);
}

void QtCurveClient::createSizeGrip()
{
    if(!m_resizeGrip && ((isResizable() && 0!=windowId()) || isPreview()))
    {
        m_resizeGrip=new QtCurveSizeGrip(this);
        m_resizeGrip->setVisible(!(isMaximized() || isShade()));
    }
}

void QtCurveClient::deleteSizeGrip()
{
    if(m_resizeGrip)
    {
        delete m_resizeGrip;
        m_resizeGrip=0L;
    }
}

void QtCurveClient::informAppOfBorderSizeChanges()
{
    union {
        char _buff[32];
        xcb_client_message_event_t ev;
    } buff;
    xcb_client_message_event_t *xev = &buff.ev;
    xev->response_type = XCB_CLIENT_MESSAGE;
    xev->format = 32;
    xev->window = windowId();
    xev->type = qtc_x11_qtc_titlebar_size;
    xev->data.data32[0] = 0;
    qtcX11SendEvent(false, windowId(), XCB_EVENT_MASK_NO_EVENT, xev);
    qtcX11Flush();
}

void QtCurveClient::informAppOfActiveChange()
{
    if (Handler()->wStyle()->pixelMetric(
            (QStyle::PixelMetric)QtC_ShadeMenubarOnlyWhenActive, NULL, NULL)) {
        union {
            char _buff[32];
            xcb_client_message_event_t ev;
        } buff;
        xcb_client_message_event_t *xev = &buff.ev;
        xev->response_type = XCB_CLIENT_MESSAGE;
        xev->format = 32;
        xev->window = windowId();
        xev->type = qtc_x11_qtc_active_window;
        xev->data.data32[0] = isActive() ? 1 : 0;
        qtcX11SendEvent(false, windowId(), XCB_EVENT_MASK_NO_EVENT, xev);
        qtcX11Flush();
    }
}

void QtCurveClient::sendToggleToApp(bool menubar)
{
    // if (Handler()->wStyle()->pixelMetric(
    //         (QStyle::PixelMetric)QtC_ShadeMenubarOnlyWhenActive, NULL, NULL))
    {
        union {
            char _buff[32];
            xcb_client_message_event_t ev;
        } buff;
        xcb_client_message_event_t *xev = &buff.ev;
        xev->response_type = XCB_CLIENT_MESSAGE;
        xev->format = 32;
        xev->window = windowId();
        xev->type = (menubar ? qtc_x11_qtc_toggle_menubar :
                     qtc_x11_qtc_toggle_statusbar);
        xev->data.data32[0] = 0;
        qtcX11SendEvent(false, windowId(), XCB_EVENT_MASK_NO_EVENT, xev);
        qtcX11Flush();
        if (menubar) {
            Handler()->emitToggleMenuBar(windowId());
        } else {
            Handler()->emitToggleStatusBar(windowId());
        }
    }
}

const QString & QtCurveClient::windowClass()
{
    if (m_windowClass.isEmpty()) {
        KWindowInfo info(windowId(), NET::WMWindowType,
                         NET::WM2WindowClass | NET::WM2WindowRole);

        switch (info.windowType((int)NET::AllTypesMask)) {
            case NET::Dialog:
                m_windowClass="D "+info.windowClassName()+' '+info.windowClassClass()+' '+info.windowRole();
                break;
            case NET::Normal:
                m_windowClass="W "+info.windowClassName()+' '+info.windowClassClass()+' '+info.windowRole();
                break;
            default:
                m_windowClass="<>";
                break;
        }
    }

    return m_windowClass;
}

void QtCurveClient::menuBarSize(int size)
{
    m_menuBarSize=size;
    if(Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_ToggleButtons, NULL, NULL) &0x01)
    {
        if(!m_toggleMenuBarButton)
            m_toggleMenuBarButton=createToggleButton(true);
        //if(m_toggleMenuBarButton)
        //    m_toggleMenuBarButton->setChecked(m_menuBarSize>0);
    }
    KCommonDecoration::activeChange();
}

void QtCurveClient::statusBarState(bool state)
{
    Q_UNUSED(state)
    if(Handler()->wStyle()->pixelMetric((QStyle::PixelMetric)QtC_ToggleButtons, NULL, NULL) &0x02)
    {
        if(!m_toggleStatusBarButton)
            m_toggleStatusBarButton=createToggleButton(false);
        //if(m_toggleStatusBarButton)
        //    m_toggleStatusBarButton->setChecked(state);
    }
    KCommonDecoration::activeChange();
}

void QtCurveClient::toggleMenuBar()
{
    sendToggleToApp(true);
}

void QtCurveClient::toggleStatusBar()
{
    sendToggleToApp(false);
}

QtCurveToggleButton*
QtCurveClient::createToggleButton(bool menubar)
{
    QtCurveToggleButton *button = new QtCurveToggleButton(menubar, this);
    int size = layoutMetric(LM_TitleHeight) - 6;

    button->setFixedSize(size, size);
    // button->setCheckable(true);
    // button->setChecked(false);
    connect(button, SIGNAL(clicked()),
            menubar ? SLOT(toggleMenuBar()) : SLOT(toggleStatusBar()));
    // widget()->setAttribute(Qt::WA_Hover, true);
    // widget()->installEventFilter(this);
    return button;
}

}
}
