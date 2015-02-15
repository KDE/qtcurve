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

/*
  based on the window decoration "Plastik":
  Copyright (C) 2003-2005 Sandro Giessl <sandro@giessl.com>

  based on the window decoration "Web":
  Copyright (C) 2001 Rik Hemsley (rikkus) <rik@kde.org>
 */

#include <QBitmap>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QStyleFactory>
#include <QStyle>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include "qtcurvehandler.h"
#include "qtcurveclient.h"
#include "qtcurvebutton.h"
#include "qtcurvedbus.h"
#include <KConfig>
#include <KConfigGroup>
#include <KColorUtils>
#include <KColorScheme>
#include <KGlobalSettings>
#include <KSaveFile>
#include <KWindowSystem>
#include <unistd.h>
#include <sys/types.h>
#include <kde_file.h>
#include <common/common.h>

#include <QX11Info>
#include <qtcurve-utils/x11base.h>
#include <qtcurve-utils/dirs.h>

static time_t getTimeStamp(const QString &item)
{
    KDE_struct_stat info;

    return !item.isEmpty() && 0==KDE_lstat(QFile::encodeName(item), &info) ? info.st_mtime : 0;
}

namespace QtCurve {

static const QString&
xdgConfigFolder()
{
    static QString xdgDir = QString::fromLocal8Bit(getXDGConfigHome());
    return xdgDir;
}

namespace KWin {

// make the handler accessible to other classes...
static QtCurveHandler *handler = 0;
QtCurveHandler*
Handler()
{
    return handler;
}

QtCurveHandler::QtCurveHandler() :
    m_lastMenuXid(0),
    m_lastStatusXid(0),
    m_style(nullptr),
    m_dBus(nullptr)
{
    qtcX11InitXlib(QX11Info::display());
    handler = this;
    setStyle();
    reset(0);

    m_dBus = new QtCurveDBus(this);
    QDBusConnection::sessionBus().registerObject("/QtCurve", this);
}

QtCurveHandler::~QtCurveHandler()
{
    handler = 0;
    delete m_style;
}

void QtCurveHandler::setStyle()
{
    // Need to use our own style instance, as want to update this when
    // settings change...
    if (!m_style) {
        KConfig kglobals("kdeglobals", KConfig::CascadeConfig);
        KConfigGroup general(&kglobals, "General");
        QString styleName = general.readEntry("widgetStyle", QString()).toLower();

        m_style = QStyleFactory::create(styleName.isEmpty() ||
                                         (styleName != "qtcurve"
#ifdef QTC_QT4_STYLE_SUPPORT
                                          && !styleName.startsWith(THEME_PREFIX)
#endif
                                             ) ? QString("QtCurve") : styleName);
        // Looks wrong with style support
        m_timeStamp = getTimeStamp(xdgConfigFolder() + "/qtcurve/stylerc");
    }
}

bool QtCurveHandler::reset(unsigned long changed)
{
    bool styleChanged = false;
    if (qtcAbs(m_timeStamp - getTimeStamp(xdgConfigFolder() +
                                          "/qtcurve/stylerc")) > 2) {
        delete m_style;
        m_style = 0L;
        setStyle();
        styleChanged = true;
    }

    // we assume the active font to be the same as the inactive font since the
    // control center doesn't offer different settings anyways.
    m_titleFont = KDecoration::options()->font(true, false); // not small
    m_titleFontTool = KDecoration::options()->font(true, true); // small

    m_hoverCols[0]=KColorScheme(QPalette::Inactive).decoration(KColorScheme::HoverColor).color();
    m_hoverCols[1]=KColorScheme(QPalette::Active).decoration(KColorScheme::HoverColor).color();

    // read in the configuration
    bool configChanged = readConfig(changed & SettingCompositing);

    setBorderSize();

    for (int t = 0;t < 2;++t) {
        for (int i = 0;i < NumButtonIcons;i++) {
            m_bitmaps[t][i] = QPixmap();
        }
    }

    // Do we need to "hit the wooden hammer" ?
    bool needHardReset = true;
    // TODO: besides the Color and Font settings I can maybe handle more changes
    //       without a hard reset. I will do this later...
    if (!styleChanged && (changed & ~(SettingColors | SettingFont | SettingButtons)) == 0)
       needHardReset = false;

    if (needHardReset || configChanged) {
        return true;
    } else {
        resetDecorations(changed);
        return false;
    }
}

void QtCurveHandler::setBorderSize()
{
    switch (m_config.borderSize()) {
    case QtCurveConfig::BORDER_NONE:
    case QtCurveConfig::BORDER_NO_SIDES:
        m_borderSize = 1;
        break;
    case QtCurveConfig::BORDER_TINY:
        m_borderSize = 2;
        break;
    case QtCurveConfig::BORDER_LARGE:
        m_borderSize = 8;
        break;
    case QtCurveConfig::BORDER_VERY_LARGE:
        m_borderSize = 12;
        break;
    case QtCurveConfig::BORDER_HUGE:
        m_borderSize = 18;
        break;
    case QtCurveConfig::BORDER_VERY_HUGE:
        m_borderSize = 27;
        break;
    case QtCurveConfig::BORDER_OVERSIZED:
        m_borderSize = 40;
        break;
    case QtCurveConfig::BORDER_NORMAL:
    default:
        m_borderSize = 4;
    }

    if (!outerBorder() && (m_borderSize == 1 || m_borderSize > 4)) {
        m_borderSize--;
    } else if (outerBorder() && innerBorder() &&
               m_config.borderSize() <= QtCurveConfig::BORDER_NORMAL) {
        m_borderSize += 2;
    }
}

KDecoration*
QtCurveHandler::createDecoration(KDecorationBridge *bridge)
{
    return (new QtCurveClient(bridge, this))->decoration();
}

bool QtCurveHandler::supports(Ability ability) const
{
    switch (ability) {
    // announce
    case AbilityAnnounceButtons:
    case AbilityAnnounceColors:
    // buttons
    case AbilityButtonMenu:
    case AbilityButtonOnAllDesktops:
    case AbilityButtonSpacer:
    case AbilityButtonHelp:
    case AbilityButtonMinimize:
    case AbilityButtonMaximize:
    case AbilityButtonClose:
    case AbilityButtonAboveOthers:
    case AbilityButtonBelowOthers:
    case AbilityButtonShade:
    // TODO
    // case AbilityButtonResize:
#if KDE_IS_VERSION(4, 9, 85)
    case AbilityButtonApplicationMenu:
#endif
    // colors
    case AbilityColorTitleBack:
    case AbilityColorTitleFore:
    case AbilityColorFrame:
        return true;
    case AbilityUsesAlphaChannel:
        return true; // !Handler()->outerBorder(); ???
    case AbilityProvidesShadow:
        return customShadows();
    case AbilityUsesBlurBehind:
        return opacity(true)<100 || opacity(false)<100 || wStyle()->pixelMetric((QStyle::PixelMetric)QtC_CustomBgnd, 0L, 0L);
        // TODO's
    default:
        return false;
    }
}

bool QtCurveHandler::readConfig(bool compositingToggled)
{
    QtCurveConfig      oldConfig=m_config;
    KConfig            configFile("kwinqtcurverc");
    const KConfigGroup config(&configFile, "General");
    QFontMetrics       fm(m_titleFont);  // active font = inactive font
    int                oldSize=m_titleHeight,
                       oldToolSize=m_titleHeightTool;
    bool               changedBorder=false;

    // The title should stretch with bigger font sizes!
    m_titleHeight = qMax(16, fm.height() + 4); // 4 px for the shadow etc.
    // have an even title/button size so the button icons are fully centered...
    if (m_titleHeight%2 == 0)
        m_titleHeight++;

    fm = QFontMetrics(m_titleFontTool);  // active font = inactive font
    // The title should stretch with bigger font sizes!
    m_titleHeightTool = qMax(13, fm.height()); // don't care about the shadow etc.
    // have an even title/button size so the button icons are fully centered...
    if (m_titleHeightTool%2 == 0)
        m_titleHeightTool++;

    m_config.load(&configFile);

    static bool borderHack = false;
    if (borderHack) {
    m_config.setOuterBorder(KWindowSystem::compositingActive() ?
                            QtCurveConfig::SHADE_NONE :
                            (m_config.customShadows() ?
                             QtCurveConfig::SHADE_SHADOW :
                             QtCurveConfig::SHADE_DARK));
        changedBorder = true;
        borderHack = false;
    } else if (compositingToggled && !m_config.outerBorder() &&
               (m_config.borderSize() < QtCurveConfig::BORDER_TINY ||
                (wStyle()->pixelMetric((QStyle::PixelMetric)QtC_WindowBorder,
                                       0L, 0L) &
                 WINDOW_BORDER_COLOR_TITLEBAR_ONLY))) {
        QDBusConnection::sessionBus().send(
            QDBusMessage::createSignal("/KWin", "org.kde.KWin",
                                       "reloadConfig"));
        borderHack = true;
    }

    m_titleHeight += 2 * titleBarPad();

    QFile in(xdgConfigFolder() + "/qtcurve/" BORDER_SIZE_FILE);
    int prevSize(-1), prevToolSize(-1), prevSide(-1), prevBot(-1);

    if (in.open(QIODevice::ReadOnly)) {
        QTextStream stream(&in);
        prevSize=in.readLine().toInt();
        prevToolSize=in.readLine().toInt();
        prevBot=in.readLine().toInt();
        prevSide=in.readLine().toInt();
        in.close();
    }

    setBorderSize();

    int borderEdge=borderEdgeSize()*2;
    bool borderSizesChanged=prevSize!=(m_titleHeight+borderEdge) || prevToolSize!=(m_titleHeightTool+borderEdge) ||
                            prevBot!=borderSize(true) || prevSide!=borderSize(false);
    if(borderSizesChanged)
    {
        KSaveFile sizeFile(xdgConfigFolder() + "/qtcurve/" BORDER_SIZE_FILE);

        if (sizeFile.open())
        {
            QTextStream stream(&sizeFile);
            stream << m_titleHeight+borderEdge << endl
                   << m_titleHeightTool+borderEdge << endl
                   << borderSize(true) << endl
                   << borderSize(false) << endl;
            stream.flush();
            sizeFile.finalize();
            sizeFile.close();
        }
    }
    bool shadowChanged(false);

    if (customShadows()) {
        ShadowConfig actShadow(QPalette::Active);
        ShadowConfig inactShadow(QPalette::Inactive);

        actShadow.load(&configFile);
        inactShadow.load(&configFile);

        shadowChanged = (m_shadowCache.shadowConfigChanged(actShadow) ||
                         m_shadowCache.shadowConfigChanged(inactShadow));

        m_shadowCache.setShadowConfig(actShadow);
        m_shadowCache.setShadowConfig(inactShadow);

        if(shadowChanged || oldConfig.roundBottom()!=roundBottom())
            m_shadowCache.reset();
    }

    if(m_dBus && (borderSizesChanged || changedBorder))
    {
        m_dBus->emitBorderSizes(); // KDE4 apps...
        borderSizeChanged(); // Gtk2 apps...
    }

    return (changedBorder || oldSize != m_titleHeight ||
            oldToolSize != m_titleHeightTool || shadowChanged ||
            m_config != oldConfig);
}

const QBitmap & QtCurveHandler::buttonBitmap(ButtonIcon type, const QSize &size, bool toolWindow)
{
    int typeIndex(type),
        reduceW(size.width()>14 ? static_cast<int>((2.0*(size.width()/3.5))+0.5) : 6),
        reduceH(size.height()>14 ? static_cast<int>((2.0*(size.height()/3.5))+0.5) : 6),
        w(size.width() - reduceW),
        h(size.height() - reduceH);

    if (m_bitmaps[toolWindow][typeIndex].size()!=QSize(w,h))
        m_bitmaps[toolWindow][typeIndex] = IconEngine::icon(type /*icon*/, qMin(w, h), wStyle());
    return m_bitmaps[toolWindow][typeIndex];
}

int QtCurveHandler::borderSize(bool bot) const
{
    if(bot)
    {
        if(QtCurveConfig::BORDER_NO_SIDES==m_config.borderSize())
            return m_borderSize+5;
        else if(QtCurveConfig::BORDER_TINY==m_config.borderSize() && m_config.roundBottom() && m_config.outerBorder())
            return m_borderSize+1;
    }
    return m_borderSize;
}

void QtCurveHandler::borderSizeChanged()
{
    foreach (QtCurveClient *client, m_clients) {
        client->informAppOfBorderSizeChanges();
    }
}

void QtCurveHandler::menuBarSize(unsigned int xid, int size)
{
    foreach (QtCurveClient *client, m_clients) {
        if (client->windowId() == xid) {
            client->menuBarSize(size);
            break;
        }
    }
    m_lastMenuXid = xid;
}

void QtCurveHandler::statusBarState(unsigned int xid, bool state)
{
    foreach (QtCurveClient *client, m_clients) {
        if (client->windowId() == xid) {
            client->statusBarState(state);
            break;
        }
    }
    m_lastStatusXid = xid;
}

void QtCurveHandler::emitToggleMenuBar(int xid)
{
    m_dBus->emitMbToggle(xid);
}

void QtCurveHandler::emitToggleStatusBar(int xid)
{
    m_dBus->emitSbToggle(xid);
}

int
QtCurveHandler::borderEdgeSize() const
{
    auto edgePad = m_config.edgePad();
    if (!outerBorder()) {
        return edgePad + 1;
    } else if (m_config.borderSize() <= QtCurveConfig::BORDER_NO_SIDES ||
               wStyle()->pixelMetric((QStyle::PixelMetric)QtC_Round,
                                     nullptr, nullptr) >= ROUND_FULL) {
        return edgePad + 3;
    } else if (wStyle()->pixelMetric((QStyle::PixelMetric)QtC_WindowBorder,
                                     nullptr, nullptr) &
               WINDOW_BORDER_ADD_LIGHT_BORDER) {
        return edgePad + 2;
    } else {
        return edgePad + 1;
    }
}

void QtCurveHandler::removeClient(QtCurveClient *c)
{
    if(c->windowId()==m_lastMenuXid)
        m_lastMenuXid=0;
    if(c->windowId()==m_lastStatusXid)
        m_lastStatusXid=0;
    m_clients.removeAll(c);
}

}
}

KWIN_DECORATION(QtCurve::KWin::QtCurveHandler)

#include "qtcurvedbus.moc"
#include "qtcurvehandler.moc"
