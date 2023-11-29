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
#include "qtcurve_plugin.h"
#include "qtcurve_fonthelper.h"
#include <qtcurve-utils/qtprops.h>

#include <qglobal.h>
#include <QDBusConnection>
#include <QDBusInterface>
#include "windowmanager.h"
#include "blurhelper.h"
#include "shortcuthandler.h"
#include <common/config_file.h>
#include "check_on-png.h"
#include "check_x_on-png.h"

#ifndef QTC_QT6_ENABLE_KDE
#  include "dialog_error-png.h"
#  include "dialog_warning-png.h"
#  include "dialog_information-png.h"
#endif

#include <QFormLayout>
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
#include <QHeaderView>
#include <QLineEdit>
#include <QSpinBox>
#include <QDir>
#include <QSettings>
#include <QPixmapCache>
#include <QTextStream>
#include <QtDebug>

#include "shadowhelper.h"
#include <qtcurve-utils/x11qtc.h>
#include <sys/time.h>

#ifdef QTC_QT6_ENABLE_KDE
#include <KConfigCore/KSharedConfig>
#include <KWindowSystem/KWindowSystem>
#include <KConfigWidgets/KColorScheme>
#include <KConfigWidgets/KStandardAction>
#include <KXmlGui/KActionCollection>
#include <KXmlGui/KXmlGuiWindow>
#endif

#include <qtcurve-utils/color.h>

namespace QtCurve {

class Style::DBusHelper {
public:
    DBusHelper()
        : m_dBus(0)
        , m_dbusConnected(false)
    {}
    ~DBusHelper()
    {
        if (m_dBus) {
            m_dBus->disconnect();
            m_dBus->deleteLater();
            m_dBus = 0;
        }
    }

    std::once_flag m_aboutToQuitInit;
    QDBusInterface *m_dBus;
    bool m_dbusConnected;
};

static inline void setPainterPen(QPainter *p, const QColor &col, const qreal width=1.0)
{
    p->setPen(QPen(col, width));
}

static Style::Icon
pix2Icon(QStyle::StandardPixmap pix)
{
    switch (pix) {
    case QStyle::SP_TitleBarNormalButton:
        return Style::ICN_RESTORE;
    case QStyle::SP_TitleBarShadeButton:
        return Style::ICN_SHADE;
    case QStyle::SP_ToolBarHorizontalExtensionButton:
        return Style::ICN_RIGHT;
    case QStyle::SP_ToolBarVerticalExtensionButton:
        return Style::ICN_DOWN;
    case QStyle::SP_TitleBarUnshadeButton:
        return Style::ICN_UNSHADE;
    default:
    case QStyle::SP_DockWidgetCloseButton:
    case QStyle::SP_TitleBarCloseButton:
        return Style::ICN_CLOSE;
    }
}

static Style::Icon
subControlToIcon(QStyle::SubControl sc)
{
    switch (sc) {
    case QStyle::SC_TitleBarMinButton:
        return Style::ICN_MIN;
    case QStyle::SC_TitleBarMaxButton:
        return Style::ICN_MAX;
    case QStyle::SC_TitleBarCloseButton:
    default:
        return Style::ICN_CLOSE;
    case QStyle::SC_TitleBarNormalButton:
        return Style::ICN_RESTORE;
    case QStyle::SC_TitleBarShadeButton:
        return Style::ICN_SHADE;
    case QStyle::SC_TitleBarUnshadeButton:
        return Style::ICN_UNSHADE;
    case QStyle::SC_TitleBarSysMenu:
        return Style::ICN_MENU;
    }
}

QtcThemedApp theThemedApp = APP_OTHER;

// do not initialise the variable here by calling qApp->arguments(). Normally it will
// be safe when we're loaded as a style plugin in a Qt application (which happens during
// the Q*Application initialisation phase so the qApp instance exists). However, when
// we're loaded in a different context this can happen:
// `QCoreApplication::arguments: Please instantiate the QApplication object first`
// `Segmentation fault`
QString appName;

static QColor
checkColour(const QStyleOption *option, QPalette::ColorRole role)
{
    QColor col(option->palette.brush(role).color());

    if (col.alpha() == 255 && isBlack(col))
        return QApplication::palette().brush(role).color();
    return col;
}

static void
addStripes(QPainter *p, const QPainterPath &path,
           const QRect &rect, bool horizontal)
{
    QColor col(Qt::white);
    QLinearGradient patternGradient(rect.topLeft(),
                                    rect.topLeft() +
                                    (horizontal ? QPoint(STRIPE_WIDTH, 0) :
                                     QPoint(0, STRIPE_WIDTH)));

    col.setAlphaF(0.0);
    patternGradient.setColorAt(0.0, col);
    col.setAlphaF(0.15);
    patternGradient.setColorAt(1.0, col);
    patternGradient.setSpread(QGradient::ReflectSpread);
    if (path.isEmpty()) {
        p->fillRect(rect, patternGradient);
    } else {
        p->save();
        p->setRenderHint(QPainter::Antialiasing, true);
        p->fillPath(path, patternGradient);
        p->restore();
    }
}

#ifndef QTC_QT6_ENABLE_KDE
static void
setRgb(QColor *col, const QStringList &rgb)
{
    if (rgb.size() == 3) {
        *col = QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt());
    }
}

static QString
kdeHome()
{
    static QString kdeHomePath;
    if (kdeHomePath.isEmpty()) {
        kdeHomePath = QString::fromLocal8Bit(qgetenv("KDEHOME"));
        if (kdeHomePath.isEmpty()) {
            QDir homeDir(QDir::homePath());
            QString kdeConfDir(QLatin1String("/.kde"));
            if (homeDir.exists(QLatin1String(".kde4")))
                kdeConfDir = QLatin1String("/.kde4");
            kdeHomePath = QDir::homePath() + kdeConfDir;
        }
    }
    return kdeHomePath;
}
#endif

static bool
isHoriz(const QStyleOption *option, EWidget w, bool joinedTBar)
{
    return (option->state & QStyle::State_Horizontal ||
            (WIDGET_BUTTON(w) &&
             (!joinedTBar || noneOf(w, WIDGET_TOOLBAR_BUTTON,
                                    WIDGET_NO_ETCH_BTN,
                                    WIDGET_MENU_BUTTON))));
}

static bool
isOnToolbar(const QWidget *widget)
{
    const QWidget *wid = widget ? widget->parentWidget() : 0L;
    while (wid) {
        if (qobject_cast<const QToolBar*>(wid)) {
            return true;
        }
        wid = wid->parentWidget();
    }
    return false;
}

/*
  Cache key:
  widgettype 2
  app        5
  size      15
  horiz      1
  alpha      8
  blue       8
  green      8
  red        8
  type       1  (0 for widget, 1 for pixmap)
  ------------
  56
*/
enum ECacheType {
    CACHE_STD,
    CACHE_PBAR,
    CACHE_TAB_TOP,
    CACHE_TAB_BOT
};

static QtcKey
createKey(qulonglong size, const QColor &color, bool horiz, int app, EWidget w)
{
    ECacheType type=WIDGET_TAB_TOP==w
        ? CACHE_TAB_TOP
        : WIDGET_TAB_BOT==w
        ? CACHE_TAB_BOT
        : WIDGET_PROGRESSBAR==w
        ? CACHE_PBAR
        : CACHE_STD;

    return (color.rgba()<<1)+
        (((qulonglong)(horiz ? 1 : 0))<<33)+
        (((qulonglong)(size&0xFFFF))<<34)+
        (((qulonglong)(app&0x1F))<<50)+
        (((qulonglong)(type&0x03))<<55);
}

static QtcKey createKey(const QColor &color, EPixmap p)
{
    return 1 +
        ((color.rgb()&RGB_MASK)<<1)+
        (((qulonglong)(p&0x1F))<<33)+
        (((qulonglong)1)<<38);
}

#ifdef QTC_QT6_ENABLE_KDE
static void parseWindowLine(const QString &line, QList<int> &data)
{
    int len(line.length());

    for(int i = 0;i < len;++i) {
        switch(line[i].toLatin1()) {
        case 'M':
            data.append(QStyle::SC_TitleBarSysMenu);
            break;
        case '_':
            data.append(WINDOWTITLE_SPACER);
            break;
        case 'H':
            data.append(QStyle::SC_TitleBarContextHelpButton);
            break;
        case 'L':
            data.append(QStyle::SC_TitleBarShadeButton);
            break;
        case 'I':
            data.append(QStyle::SC_TitleBarMinButton);
            break;
        case 'A':
            data.append(QStyle::SC_TitleBarMaxButton);
            break;
        case 'X':
            data.append(QStyle::SC_TitleBarCloseButton);
        default:
            break;
        }
    }
}
#endif

Style::Style() :
    m_dBusHelper(new DBusHelper()),
    m_fntHelper(new FontHelper()),
    m_popupMenuCols(0L),
    m_sliderCols(0L),
    m_defBtnCols(0L),
    m_comboBtnCols(0L),
    m_checkRadioSelCols(0L),
    m_sortedLvColors(0L),
    m_ooMenuCols(0L),
    m_progressCols(0L),
    m_saveMenuBarStatus(false),
    m_usePixmapCache(true),
    m_inactiveChangeSelectionColor(false),
    m_isPreview(PREVIEW_FALSE),
    m_sidebarButtonsCols(0L),
    m_activeMdiColors(0L),
    m_mdiColors(0L),
    m_pixmapCache(150000),
    m_active(true),
    m_sbWidget(0L),
    m_clickedLabel(0L),
    m_progressBarAnimateTimer(0),
    m_animateStep(0),
    m_titlebarHeight(0),
    m_shadowHelper(new ShadowHelper(this)),
    m_sViewSBar(0L),
    m_windowManager(new WindowManager(this)),
    m_blurHelper(new BlurHelper(this)),
    m_shortcutHandler(new ShortcutHandler(this))
{
    const char *env = getenv(QTCURVE_PREVIEW_CONFIG);
#ifdef QTC_QT6_ENABLE_KDE
    m_configFile = KSharedConfig::openConfig();
    m_kdeGlobals = KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::NoGlobals);
#endif
    if (env && strcmp(env, QTCURVE_PREVIEW_CONFIG) == 0) {
        // To enable preview of QtCurve settings, the style config module will set QTCURVE_PREVIEW_CONFIG
        // and use CE_QtC_SetOptions to set options. If this is set, we do not use the QPixmapCache as it
        // will interfere with that of the kcm's widgets!
        m_isPreview=PREVIEW_MDI;
        m_usePixmapCache=false;
    } else if(env && strcmp(env, QTCURVE_PREVIEW_CONFIG_FULL) == 0) {
        // As above, but preview is in window - so can use opacity settings!
        m_isPreview=PREVIEW_WINDOW;
        m_usePixmapCache=false;
    } else {
        init(true);
    }
}

void Style::init(bool initial)
{
    if(!initial)
        freeColors();

    if (m_isPreview) {
        if (m_isPreview != PREVIEW_WINDOW) {
            opts.bgndOpacity = opts.dlgOpacity = opts.menuBgndOpacity = 100;
        }
    } else {
        qtcReadConfig(QString(), &opts);

        if (initial) {
            if (QCoreApplication::instance()) {
                // we can obtain the application name safely.
                appName = QFileInfo(QCoreApplication::instance()->arguments()[0]).fileName();
            }
#ifdef Q_OS_MACOS
            if (opts.nonnativeMenubarApps.contains("kde") || opts.nonnativeMenubarApps.contains(appName)) {
                QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);
                opts.currentNonnativeMenubarApps << appName;
            }
#endif
            connectDBus();
#ifdef QTC_QT6_ENABLE_KDE
            connect(KWindowSystem::self(), &KWindowSystem::compositingChanged, this, &Style::compositingToggled);
#endif
            // prepare the cleanup handler
            if (QCoreApplication::instance()) {
                std::call_once(m_dBusHelper->m_aboutToQuitInit, [this] {
                    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this] () {
                            // disconnect from the session DBus. We're no longer interested in the
                            // information it might send when the app we're serving is shutting down.
                            disconnectDBus();
                            // Stop listening to select signals. We shouldn't stop emitting signals
                            // (like QObject::destroyed) but we can reduce the likelihood that pending
                            // signals will be sent to us post-mortem.
#ifdef QTC_QT6_ENABLE_KDE
                            disconnect(KWindowSystem::self(), &KWindowSystem::compositingChanged,
                                       this, &Style::compositingToggled);
#endif
                        } );
                } );
            }
        }
    }

    opts.contrast=QSettings(QLatin1String("Trolltech")).value("/Qt/KDE/contrast", DEFAULT_CONTRAST).toInt();
    if(opts.contrast<0 || opts.contrast>10)
        opts.contrast=DEFAULT_CONTRAST;

    shadeColors(QApplication::palette().color(QPalette::Active, QPalette::Highlight), m_highlightCols);
    shadeColors(QApplication::palette().color(QPalette::Active, QPalette::Background), m_backgroundCols);
    shadeColors(QApplication::palette().color(QPalette::Active, QPalette::Button), m_buttonCols);

    // Set defaults for Hover and Focus, these will be changed when KDE4 palette is applied...
    shadeColors(QApplication::palette().color(QPalette::Active,
                                              QPalette::Highlight), m_focusCols);
    shadeColors(QApplication::palette().color(QPalette::Active, QPalette::Highlight), m_mouseOverCols);
// Dont setup KDE4 fonts/colours here - seems to mess things up when using proxy styles.
// See http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=638629
//#ifdef QTC_QT6_ENABLE_KDE
//    setupKde4();
//#endif

    m_windowManager->initialize(opts.windowDrag, opts.windowDragWhiteList.values(), opts.windowDragBlackList.values());

    switch(opts.shadeSliders)
    {
    default:
    case SHADE_DARKEN:
    case SHADE_NONE:
        break;
    case SHADE_SELECTED:
        m_sliderCols=m_highlightCols;
        break;
    case SHADE_BLEND_SELECTED:
    case SHADE_CUSTOM:
        if (!m_sliderCols) {
            m_sliderCols = new QColor[TOTAL_SHADES + 1];
        }
        shadeColors(opts.shadeSliders == SHADE_BLEND_SELECTED ?
                    midColor(m_highlightCols[ORIGINAL_SHADE],
                             m_buttonCols[ORIGINAL_SHADE]) :
                    opts.customSlidersColor, m_sliderCols);
    }

    switch(opts.defBtnIndicator)
    {
    case IND_GLOW:
    case IND_SELECTED:
        m_defBtnCols=m_highlightCols;
        break;
    case IND_TINT:
        m_defBtnCols=new QColor [TOTAL_SHADES+1];
        shadeColors(tint(m_buttonCols[ORIGINAL_SHADE],
                         m_highlightCols[ORIGINAL_SHADE], DEF_BNT_TINT),
                    m_defBtnCols);
        break;
    default:
        break;
    case IND_COLORED:
        if (opts.shadeSliders == SHADE_BLEND_SELECTED) {
            m_defBtnCols = m_sliderCols;
        } else {
            m_defBtnCols = new QColor[TOTAL_SHADES + 1];
            shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE],
                                 m_buttonCols[ORIGINAL_SHADE]), m_defBtnCols);
        }
    }

    switch(opts.comboBtn) {
    default:
    case SHADE_DARKEN:
    case SHADE_NONE:
        break;
    case SHADE_SELECTED:
        m_comboBtnCols=m_highlightCols;
        break;
    case SHADE_BLEND_SELECTED:
        if (opts.shadeSliders == SHADE_BLEND_SELECTED) {
            m_comboBtnCols = m_sliderCols;
            break;
        }
        QTC_FALLTHROUGH();
    case SHADE_CUSTOM:
        if (opts.shadeSliders == SHADE_CUSTOM &&
            opts.customSlidersColor == opts.customComboBtnColor) {
            m_comboBtnCols = m_sliderCols;
            break;
        }
        if (!m_comboBtnCols) {
            m_comboBtnCols = new QColor[TOTAL_SHADES + 1];
        }
        shadeColors(opts.comboBtn == SHADE_BLEND_SELECTED ?
                    midColor(m_highlightCols[ORIGINAL_SHADE],
                             m_buttonCols[ORIGINAL_SHADE]) :
                    opts.customComboBtnColor, m_comboBtnCols);
    }

    switch (opts.sortedLv) {
    case SHADE_DARKEN:
        if (!m_sortedLvColors) {
            m_sortedLvColors = new QColor[TOTAL_SHADES + 1];
        }
        shadeColors(shade(opts.lvButton ? m_buttonCols[ORIGINAL_SHADE] : m_backgroundCols[ORIGINAL_SHADE], LV_HEADER_DARK_FACTOR), m_sortedLvColors);
        break;
    default:
    case SHADE_NONE:
        break;
    case SHADE_SELECTED:
        m_sortedLvColors=m_highlightCols;
        break;
    case SHADE_BLEND_SELECTED:
        if (opts.shadeSliders == SHADE_BLEND_SELECTED) {
            m_sortedLvColors = m_sliderCols;
            break;
        } else if (opts.comboBtn == SHADE_BLEND_SELECTED) {
            m_sortedLvColors = m_comboBtnCols;
            break;
        }
        QTC_FALLTHROUGH();
    case SHADE_CUSTOM:
        if (opts.shadeSliders == SHADE_CUSTOM &&
            opts.customSlidersColor == opts.customSortedLvColor) {
            m_sortedLvColors = m_sliderCols;
            break;
        }
        if (opts.comboBtn == SHADE_CUSTOM &&
            opts.customComboBtnColor == opts.customSortedLvColor) {
            m_sortedLvColors = m_comboBtnCols;
            break;
        }
        if (!m_sortedLvColors) {
            m_sortedLvColors = new QColor[TOTAL_SHADES + 1];
        }
        shadeColors(opts.sortedLv == SHADE_BLEND_SELECTED ?
                    midColor(m_highlightCols[ORIGINAL_SHADE],
                             (opts.lvButton ? m_buttonCols[ORIGINAL_SHADE] :
                              m_backgroundCols[ORIGINAL_SHADE])) :
                    opts.customSortedLvColor, m_sortedLvColors);
    }

    switch (opts.crColor) {
    default:
    case SHADE_NONE:
        m_checkRadioSelCols = m_buttonCols;
        break;
    case SHADE_DARKEN:
        if (!m_checkRadioSelCols) {
            m_checkRadioSelCols = new QColor[TOTAL_SHADES + 1];
        }
        shadeColors(shade(m_buttonCols[ORIGINAL_SHADE], LV_HEADER_DARK_FACTOR),
                    m_checkRadioSelCols);
        break;
    case SHADE_SELECTED:
        m_checkRadioSelCols = m_highlightCols;
        break;
    case SHADE_CUSTOM:
        if(SHADE_CUSTOM==opts.shadeSliders && opts.customSlidersColor==opts.customCrBgndColor)
            m_checkRadioSelCols=m_sliderCols;
        else if(SHADE_CUSTOM==opts.comboBtn && opts.customComboBtnColor==opts.customCrBgndColor)
            m_checkRadioSelCols=m_comboBtnCols;
        else if(SHADE_CUSTOM==opts.sortedLv && opts.customSortedLvColor==opts.customCrBgndColor)
            m_checkRadioSelCols=m_sortedLvColors;
        else
        {
            if(!m_checkRadioSelCols)
                m_checkRadioSelCols=new QColor [TOTAL_SHADES+1];
            shadeColors(opts.customCrBgndColor, m_checkRadioSelCols);
        }
        break;
    case SHADE_BLEND_SELECTED:
        if(SHADE_BLEND_SELECTED==opts.shadeSliders)
            m_checkRadioSelCols=m_sliderCols;
        else if(SHADE_BLEND_SELECTED==opts.comboBtn)
            m_checkRadioSelCols=m_comboBtnCols;
        else if(SHADE_BLEND_SELECTED==opts.sortedLv)
            m_checkRadioSelCols=m_sortedLvColors;
        else
        {
            if(!m_checkRadioSelCols)
                m_checkRadioSelCols=new QColor [TOTAL_SHADES+1];
            shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE], m_buttonCols[ORIGINAL_SHADE]), m_checkRadioSelCols);
        }
    }

    switch(opts.progressColor)
    {
    case SHADE_NONE:
        m_progressCols=m_backgroundCols;
        break;
    default:
        // Not set!
        break;
    case SHADE_CUSTOM:
        if(SHADE_CUSTOM==opts.shadeSliders && opts.customSlidersColor==opts.customProgressColor)
            m_progressCols=m_sliderCols;
        else if(SHADE_CUSTOM==opts.comboBtn && opts.customComboBtnColor==opts.customProgressColor)
            m_progressCols=m_comboBtnCols;
        else if(SHADE_CUSTOM==opts.sortedLv && opts.customSortedLvColor==opts.customProgressColor)
            m_progressCols=m_sortedLvColors;
        else if(SHADE_CUSTOM==opts.crColor && opts.customCrBgndColor==opts.customProgressColor)
            m_progressCols=m_checkRadioSelCols;
        else
        {
            if(!m_progressCols)
                m_progressCols=new QColor [TOTAL_SHADES+1];
            shadeColors(opts.customProgressColor, m_progressCols);
        }
        break;
    case SHADE_BLEND_SELECTED:
        if(SHADE_BLEND_SELECTED==opts.shadeSliders)
            m_progressCols=m_sliderCols;
        else if(SHADE_BLEND_SELECTED==opts.comboBtn)
            m_progressCols=m_comboBtnCols;
        else if(SHADE_BLEND_SELECTED==opts.sortedLv)
            m_progressCols=m_sortedLvColors;
        else
        {
            if(!m_progressCols)
                m_progressCols=new QColor [TOTAL_SHADES+1];
            shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE], m_backgroundCols[ORIGINAL_SHADE]), m_progressCols);
        }
    }

    setMenuColors(QApplication::palette().color(QPalette::Active, QPalette::Background));

    switch(opts.shadeCheckRadio)
    {
    default:
        m_checkRadioCol=QApplication::palette().color(QPalette::Active, opts.crButton ? QPalette::ButtonText : QPalette::Text);
        break;
    case SHADE_BLEND_SELECTED:
    case SHADE_SELECTED:
        m_checkRadioCol=QApplication::palette().color(QPalette::Active, QPalette::Highlight);
        break;
    case SHADE_CUSTOM:
        m_checkRadioCol=opts.customCheckRadioColor;
    }

    if(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR && opts.titlebarButtonColors.size()>=NUM_TITLEBAR_BUTTONS)
        for(int i=0; i<NUM_TITLEBAR_BUTTONS; ++i)
        {
            QColor *cols=new QColor [TOTAL_SHADES+1];
            shadeColors(opts.titlebarButtonColors[(ETitleBarButtons)i], cols);
            m_titleBarButtonsCols[i]=cols;
        }
    else
        opts.titlebarButtons&=~TITLEBAR_BUTTON_COLOR;

    if (oneOf(opts.bgndImage.type, IMG_PLAIN_RINGS, IMG_BORDERED_RINGS,
              IMG_SQUARE_RINGS) ||
        oneOf(opts.menuBgndImage.type, IMG_PLAIN_RINGS, IMG_BORDERED_RINGS,
              IMG_SQUARE_RINGS)) {
        qtcCalcRingAlphas(&m_backgroundCols[ORIGINAL_SHADE]);
    }

    m_blurHelper->setEnabled(opts.bgndOpacity != 100 ||
                             opts.dlgOpacity != 100 ||
                             opts.menuBgndOpacity != 100);

    opts.fontTickWidth=-1;
    opts.menuTick=QString(QChar(0x2713));

#ifdef QTC_QT6_ENABLE_KDE
    // We need to set the decoration colours for the preview now...
    if (m_isPreview) {
        setDecorationColors();
    }
#endif
}

void Style::connectDBus()
{
    if (m_dBusHelper->m_dbusConnected)
        return;
    auto bus = QDBusConnection::sessionBus();
    if (bus.isConnected()) {
        m_dBusHelper->m_dbusConnected = true;
        bus.connect(QString(), "/KGlobalSettings", "org.kde.KGlobalSettings",
                    "notifyChange", this, SLOT(kdeGlobalSettingsChange(int, int)));
#ifndef QTC_QT6_ENABLE_KDE
        bus.connect("org.kde.kwin", "/KWin", "org.kde.KWin", "compositingToggled",
                    this, SLOT(compositingToggled()));
#endif

        if (!qApp || (appName != "kwin" && appName != "kwin_x11" && appName != "kwin_wayland")) {
            // don't connect to signals if we know we're sending them out ourselves
            bus.connect("org.kde.kwin", "/QtCurve", "org.kde.QtCurve",
                        "borderSizesChanged", this, SLOT(borderSizesChanged()));
            if (opts.menubarHiding & HIDE_KWIN)
                bus.connect("org.kde.kwin", "/QtCurve", "org.kde.QtCurve",
                            "toggleMenuBar",
                            this, SLOT(toggleMenuBar(unsigned int)));

            if (opts.statusbarHiding & HIDE_KWIN) {
                bus.connect("org.kde.kwin", "/QtCurve", "org.kde.QtCurve",
                            "toggleStatusBar",
                            this, SLOT(toggleStatusBar(unsigned int)));
            }
        }
    }
}

void Style::disconnectDBus()
{
    if (!m_dBusHelper->m_dbusConnected)
        return;
    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return;
    m_dBusHelper->m_dbusConnected = false;
    if (getenv("QTCURVE_DEBUG")) {
        qWarning() << Q_FUNC_INFO << this << "Disconnecting from" << bus.name() << "/" << bus.baseService();
        dumpObjectInfo();
    }
    bus.disconnect(QString(), "/KGlobalSettings", "org.kde.KGlobalSettings",
                   "notifyChange",
                   this, SLOT(kdeGlobalSettingsChange(int, int)));
#ifndef QTC_QT6_ENABLE_KDE
    bus.disconnect("org.kde.kwin", "/KWin", "org.kde.KWin", "compositingToggled",
                   this, SLOT(compositingToggled()));
#endif

    if (!qApp || (appName != "kwin" && appName != "kwin_x11" && appName != "kwin_wayland")) {
        bus.disconnect("org.kde.kwin", "/QtCurve", "org.kde.QtCurve",
                    "borderSizesChanged", this, SLOT(borderSizesChanged()));
        if (opts.menubarHiding & HIDE_KWIN)
            bus.disconnect("org.kde.kwin", "/QtCurve", "org.kde.QtCurve",
                           "toggleMenuBar",
                           this, SLOT(toggleMenuBar(unsigned int)));

        if (opts.statusbarHiding & HIDE_KWIN) {
            bus.disconnect("org.kde.kwin", "/QtCurve", "org.kde.QtCurve",
                           "toggleStatusBar",
                           this, SLOT(toggleStatusBar(unsigned int)));
        }
    }
}

Style::~Style()
{
    qtcInfo("Deleting style instance %p\n", this);
    disconnectDBus();
    if (m_plugin && m_plugin->m_styleInstances.contains(this)) {
        m_plugin->m_styleInstances.removeAll(this);
    }
    freeColors();
    delete m_fntHelper;
    delete m_dBusHelper;
}

void Style::freeColor(QSet<QColor *> &freedColors, QColor **cols)
{
    if(!freedColors.contains(*cols) &&
       *cols!=m_highlightCols &&
       *cols!=m_backgroundCols &&
       *cols!=m_menubarCols &&
       *cols!=m_focusCols &&
       *cols!=m_mouseOverCols &&
       *cols!=m_buttonCols &&
       *cols!=m_coloredButtonCols &&
       *cols!=m_coloredBackgroundCols &&
       *cols!=m_coloredHighlightCols) {
        freedColors.insert(*cols);
        delete [] *cols;
    }
    *cols=0L;
}

void Style::freeColors()
{
    if(0!=m_progressBarAnimateTimer) {
        killTimer(m_progressBarAnimateTimer);
        m_progressBarAnimateTimer = 0;
    }

    QSet<QColor*> freedColors;

    freeColor(freedColors, &m_sidebarButtonsCols);
    freeColor(freedColors, &m_popupMenuCols);
    freeColor(freedColors, &m_activeMdiColors);
    freeColor(freedColors, &m_mdiColors);
    freeColor(freedColors, &m_progressCols);
    freeColor(freedColors, &m_checkRadioSelCols);
    freeColor(freedColors, &m_sortedLvColors);
    freeColor(freedColors, &m_comboBtnCols);
    freeColor(freedColors, &m_defBtnCols);
    freeColor(freedColors, &m_sliderCols);

    if (opts.titlebarButtons & TITLEBAR_BUTTON_COLOR) {
        for (int i = 0;i < NUM_TITLEBAR_BUTTONS;i++) {
            delete []m_titleBarButtonsCols[i];
            m_titleBarButtonsCols[i] = 0L;
        }
    }
    if (m_ooMenuCols) {
        delete []m_ooMenuCols;
        m_ooMenuCols = 0L;
    }
}

#if 1
static QFontMetrics styledFontMetrics(const QStyleOption *option, const QWidget *widget)
{
    return option
        ? option->fontMetrics
        : widget
        ? widget->fontMetrics()
        : qApp->fontMetrics();
}

static int fontHeight(const QStyleOption *option, const QWidget *widget)
{
    return styledFontMetrics(option, widget).height();
}

// Taken from skulpture 0.2.3
void Style::polishFormLayout(QFormLayout *layout)
{
    int widgetSize=-1;

    if (layout->labelAlignment() & Qt::AlignVCenter)
        return;

    int addedHeight = -1;
    for (int row = 0; row < layout->rowCount(); ++row)
    {
        QLayoutItem *labelItem = layout->itemAt(row, QFormLayout::LabelRole);
        if (!labelItem)
            continue;

        QLayoutItem *fieldItem = layout->itemAt(row, QFormLayout::FieldRole);
        if (!fieldItem)
            continue;

        QWidget *label = labelItem->widget();
        if (!label)
            continue;

        int labelHeight;
        if (addedHeight < 0)
            addedHeight = 4 + 2 * widgetSize;
        if (qobject_cast<QLabel *>(label))
            labelHeight = label->sizeHint().height() + addedHeight;
        else if (qobject_cast<QCheckBox *>(label))
            labelHeight = label->sizeHint().height();
        else
            continue;

        int fieldHeight = fieldItem->sizeHint().height();
        /* for large fields, we don't center */
        if (fieldHeight <= 2 * fontHeight(0, label) + addedHeight)
        {
            if (fieldHeight > labelHeight)
                labelHeight = fieldHeight;
        }
//         else if (verticalTextShift(label->fontMetrics()) & 1)
//                 labelHeight += 1;
        if (qobject_cast<QCheckBox *>(label)) {
            label->setMinimumHeight(labelHeight);
        } else {
            label->setMinimumHeight((labelHeight * 4 + 6) / 7);
        }
    }
}

void Style::polishLayout(QLayout *layout)
{
    if (QFormLayout *formLayout = qobject_cast<QFormLayout *>(layout))
        polishFormLayout(formLayout);
    // recurse into layouts
    for (int i = 0; i < layout->count(); ++i)
        if (QLayout *l = layout->itemAt(i)->layout())
            polishLayout(l);
}
#endif

// Taken from oxygen!
void Style::polishScrollArea(QAbstractScrollArea *scrollArea, bool isKFilePlacesView) const
{
    if(!scrollArea)
        return;

    // HACK: add exception for KPIM transactionItemView, which is an overlay widget and must have filled background. This is a temporary workaround
    // until a more robust solution is found.
    if(scrollArea->inherits("KPIM::TransactionItemView"))
    {
        // also need to make the scrollarea background plain (using autofill background) so that optional vertical scrollbar background is not
        // transparent either.
        // TODO: possibly add an event filter to use the "normal" window background instead of something flat.
        scrollArea->setAutoFillBackground(true);
        return;
    }

    // check frame style and background role
    if(QFrame::NoFrame!=scrollArea->frameShape() || QPalette::Window!=scrollArea->backgroundRole())
        return;

    // get viewport and check background role
    QWidget *viewport(scrollArea->viewport());
    if(!(viewport && viewport->backgroundRole() == QPalette::Window) &&
       !isKFilePlacesView) {
        return;
    }

    // change viewport autoFill background.
    // do the same for children if the background role is QPalette::Window
    viewport->setAutoFillBackground(false);
    for (QWidget *child: viewport->findChildren<QWidget*>()) {
        if (child->parent() == viewport &&
            child->backgroundRole() == QPalette::Window) {
            child->setAutoFillBackground(false);
        }
    }
}

QIcon Style::standardIcon(StandardPixmap pix, const QStyleOption *option,
                          const QWidget *widget) const
{
    switch (pix) {
    // case SP_TitleBarMenuButton:
    // case SP_TitleBarMinButton:
    // case SP_TitleBarMaxButton:
    // case SP_TitleBarContextHelpButton:
    case SP_TitleBarNormalButton:
    case SP_TitleBarShadeButton:
    case SP_TitleBarUnshadeButton:
    case SP_DockWidgetCloseButton:
    case SP_TitleBarCloseButton: {
        QPixmap pm(13, 13);

        pm.fill(Qt::transparent);

        QPainter painter(&pm);

        drawIcon(&painter, Qt::color1, QRect(0, 0, pm.width(), pm.height()),
                 false, pix2Icon(pix), oneOf(pix, SP_TitleBarShadeButton,
                                             SP_TitleBarUnshadeButton));
        return QIcon(pm);
    }
    case SP_ToolBarHorizontalExtensionButton:
    case SP_ToolBarVerticalExtensionButton:
    {
        QPixmap pm(9, 9);

        pm.fill(Qt::transparent);

        QPainter painter(&pm);

        drawIcon(&painter, Qt::color1, QRect(0, 0, pm.width(), pm.height()), false, pix2Icon(pix), true);
        return QIcon(pm);
    }
#ifndef QTC_QT6_ENABLE_KDE
    case SP_MessageBoxQuestion:
    case SP_MessageBoxInformation: {
        static QIcon icn(QPixmap::fromImage(qtc_dialog_information));
        return icn;
    }
    case SP_MessageBoxWarning: {
        static QIcon icn(QPixmap::fromImage(qtc_dialog_warning));
        return icn;
    }
    case SP_MessageBoxCritical: {
        static QIcon icn(QPixmap::fromImage(qtc_dialog_error));
        return icn;
    }
/*
  case SP_DialogYesButton:
  case SP_DialogOkButton:
  {
  static QIcon icn(load(dialog_ok_png_len, dialog_ok_png_data));
  return icn;
  }
  case SP_DialogNoButton:
  case SP_DialogCancelButton:
  {
  static QIcon icn(load(dialog_cancel_png_len, dialog_cancel_png_data));
  return icn;
  }
  case SP_DialogHelpButton:
  {
  static QIcon icn(load(help_contents_png_len, help_contents_png_data));
  return icn;
  }
  case SP_DialogCloseButton:
  {
  static QIcon icn(load(dialog_close_png_len, dialog_close_png_data));
  return icn;
  }
  case SP_DialogApplyButton:
  {
  static QIcon icn(load(dialog_ok_apply_png_len, dialog_ok_apply_png_data));
  return icn;
  }
  case SP_DialogResetButton:
  {
  static QIcon icn(load(document_revert_png_len, document_revert_png_data));
  return icn;
  }
*/
#else
    case SP_MessageBoxInformation:
        return QIcon::fromTheme(QStringLiteral("dialog-information"));
    case SP_MessageBoxWarning:
        return QIcon::fromTheme(QStringLiteral("dialog-warning"));
    case SP_MessageBoxCritical:
        return QIcon::fromTheme(QStringLiteral("dialog-error"));
    case SP_MessageBoxQuestion:
        return QIcon::fromTheme(QStringLiteral("dialog-information"));
    case SP_DesktopIcon:
        return QIcon::fromTheme(QStringLiteral("user-desktop"));
    case SP_TrashIcon:
        return QIcon::fromTheme(QStringLiteral("user-trash"));
    case SP_ComputerIcon:
        return QIcon::fromTheme(QStringLiteral("computer"));
    case SP_DriveFDIcon:
        return QIcon::fromTheme(QStringLiteral("media-floppy"));
    case SP_DriveHDIcon:
        return QIcon::fromTheme(QStringLiteral("drive-harddisk"));
    case SP_DriveCDIcon:
    case SP_DriveDVDIcon:
        return QIcon::fromTheme(QStringLiteral("media-optical"));
    case SP_DriveNetIcon:
        return QIcon::fromTheme(QStringLiteral("network-server"));
    case SP_DirOpenIcon:
        return QIcon::fromTheme(QStringLiteral("document-open"));
    case SP_DirIcon:
    case SP_DirClosedIcon:
        return QIcon::fromTheme(QStringLiteral("folder"));
    // case SP_DirLinkIcon:
    case SP_FileIcon:
        return QIcon::fromTheme(QStringLiteral("application-x-zerosize"));
    // case SP_FileLinkIcon:
    case SP_FileDialogStart:
        return QIcon::fromTheme(QApplication::layoutDirection() == Qt::RightToLeft ?
                     QStringLiteral("go-edn") : QStringLiteral("go-first"));
    case SP_FileDialogEnd:
        return QIcon::fromTheme(QApplication::layoutDirection() == Qt::RightToLeft ?
                     QStringLiteral("go-first") : QStringLiteral("go-end"));
    case SP_FileDialogToParent:
        return QIcon::fromTheme(QStringLiteral("go-up"));
    case SP_FileDialogNewFolder:
        return QIcon::fromTheme(QStringLiteral("folder-new"));
    case SP_FileDialogDetailedView:
        return QIcon::fromTheme(QStringLiteral("view-list-details"));
    // case SP_FileDialogInfoView:
    //     return QIcon::fromTheme(QStringLiteral("dialog-ok"));
    // case SP_FileDialogContentsView:
    //     return QIcon::fromTheme(QStringLiteral("dialog-ok"));
    case SP_FileDialogListView:
        return QIcon::fromTheme(QStringLiteral("view-list-icons"));
    case SP_FileDialogBack:
        return QIcon::fromTheme(QApplication::layoutDirection() == Qt::RightToLeft ?
                     QStringLiteral("go-next") : QStringLiteral("go-previous"));
    case SP_DialogOkButton:
        return QIcon::fromTheme(QStringLiteral("dialog-ok"));
    case SP_DialogCancelButton:
        return QIcon::fromTheme(QStringLiteral("dialog-cancel"));
    case SP_DialogHelpButton:
        return QIcon::fromTheme(QStringLiteral("help-contents"));
    case SP_DialogOpenButton:
        return QIcon::fromTheme(QStringLiteral("document-open"));
    case SP_DialogSaveButton:
        return QIcon::fromTheme(QStringLiteral("document-save"));
    case SP_DialogCloseButton:
        return QIcon::fromTheme(QStringLiteral("dialog-close"));
    case SP_DialogApplyButton:
        return QIcon::fromTheme(QStringLiteral("dialog-ok-apply"));
    case SP_DialogResetButton:
        return QIcon::fromTheme(QStringLiteral("document-revert"));
    // case SP_DialogDiscardButton:
    //     return QIcon::fromTheme(QStringLiteral("dialog-cancel"));
    case SP_DialogYesButton:
        return QIcon::fromTheme(QStringLiteral("dialog-ok"));
    case SP_DialogNoButton:
        return QIcon::fromTheme(QStringLiteral("dialog-cancel"));
    case SP_ArrowUp:
        return QIcon::fromTheme(QStringLiteral("arrow-up"));
    case SP_ArrowDown:
        return QIcon::fromTheme(QStringLiteral("arrow-down"));
    case SP_ArrowLeft:
        return QIcon::fromTheme(QStringLiteral("arrow-left"));
    case SP_ArrowRight:
        return QIcon::fromTheme(QStringLiteral("arrow-right"));
    case SP_ArrowBack:
        return QIcon::fromTheme(QApplication::layoutDirection() == Qt::RightToLeft ?
                     QStringLiteral("go-next") : QStringLiteral("go-previous"));
    case SP_ArrowForward:
        return QIcon::fromTheme(QApplication::layoutDirection() == Qt::RightToLeft ?
                     QStringLiteral("go-previous") : QStringLiteral("go-next"));
    case SP_DirHomeIcon:
        return QIcon::fromTheme(QStringLiteral("user-home"));
    // case SP_CommandLink:
    // case SP_VistaShield:
    case SP_BrowserReload:
        return QIcon::fromTheme(QStringLiteral("view-refresh"));
    case SP_BrowserStop:
        return QIcon::fromTheme(QStringLiteral("process-stop"));
    case SP_MediaPlay:
        return QIcon::fromTheme(QStringLiteral("media-playback-start"));
    case SP_MediaStop:
        return QIcon::fromTheme(QStringLiteral("media-playback-stop"));
    case SP_MediaPause:
        return QIcon::fromTheme(QStringLiteral("media-playback-pause"));
    case SP_MediaSkipForward:
        return QIcon::fromTheme(QStringLiteral("media-skip-forward"));
    case SP_MediaSkipBackward:
        return QIcon::fromTheme(QStringLiteral("media-skip-backward"));
    case SP_MediaSeekForward:
        return QIcon::fromTheme(QStringLiteral("media-seek-forward"));
    case SP_MediaSeekBackward:
        return QIcon::fromTheme(QStringLiteral("media-seek-backward"));
    case SP_MediaVolume:
        return QIcon::fromTheme(QStringLiteral("player-volume"));
    case SP_MediaVolumeMuted:
        return QIcon::fromTheme(QStringLiteral("player-volume-muted"));
#endif
    default:
        break;
    }
    // TODO ?
    return ParentStyleClass::standardIcon(pix, option, widget);
}

int Style::layoutSpacing(QSizePolicy::ControlType control1,
                         QSizePolicy::ControlType control2,
                         Qt::Orientation orientation,
                         const QStyleOption *option, const QWidget *widget) const
{
    Q_UNUSED(control1);
    Q_UNUSED(control2);
    Q_UNUSED(orientation);
    return pixelMetric(PM_DefaultLayoutSpacing, option, widget);
}

// Use 'drawItemTextWithRole' when already know which role to use.
void
Style::drawItemTextWithRole(QPainter *painter, const QRect &rect, int flags,
                            const QPalette &pal, bool enabled,
                            const QString &text,
                            QPalette::ColorRole textRole) const
{
    ParentStyleClass::drawItemText(painter, rect, flags, pal,
                               enabled, text, textRole);
}

void Style::drawSideBarButton(QPainter *painter, const QRect &r, const QStyleOption *option, const QWidget *widget) const
{
    const QPalette &palette(option->palette);
    QRect          r2(r);
    QStyleOption   opt(*option);

    if(r2.height()>r2.width() || (r2.height()<r2.width() && r2.width()<=32))
        opt.state&=~State_Horizontal;
    else
        opt.state|=State_Horizontal;

    const QColor *use(opt.state&State_On ? getSidebarButtons() : buttonColors(option));
    bool         horiz(opt.state&State_Horizontal);

    painter->save();
    if(opt.state&State_On || opt.state&State_MouseOver)
    {
        r2.adjust(-1, -1, 1, 1);
        drawLightBevel(painter, r2, &opt, widget, ROUNDED_NONE,
                       getFill(&opt, use), use, false, WIDGET_MENU_ITEM);
    }
    else
        painter->fillRect(r2, palette.window().color());

    if (opt.state & State_MouseOver && opts.coloredMouseOver) {
        r2 = r;
        if (opts.coloredMouseOver == MO_PLASTIK) {
            if (horiz) {
                r2.adjust(0, 1, 0, -1);
            } else {
                r2.adjust(1, 0, -1, 0);
            }
        } else {
            r2.adjust(1, 1, -1, -1);
        }
        if (opts.coloredMouseOver == MO_GLOW) {
            QColor col(m_mouseOverCols[opt.state&State_On ? 0 : 1]);

            col.setAlphaF(GLOW_ALPHA(false));
            painter->setPen(col);
            drawRect(painter, r);
            col = m_mouseOverCols[opt.state&State_On ? 4 : 3];
            col.setAlphaF(0.8);
            painter->setPen(col);
            drawRect(painter, r2);
        } else {
            painter->setPen(m_mouseOverCols[opt.state&State_On ? 0 : 1]);

            if (horiz || MO_PLASTIK!=opts.coloredMouseOver)
            {
                painter->drawLine(r.x(), r.y(), r.x()+r.width()-1, r.y());
                painter->drawLine(r2.x(), r2.y(), r2.x()+r2.width()-1, r2.y());
            }

            if(!horiz || MO_PLASTIK!=opts.coloredMouseOver)
            {
                painter->drawLine(r.x(), r.y(), r.x(), r.y()+r.height()-1);
                painter->drawLine(r2.x(), r2.y(), r2.x(), r2.y()+r2.height()-1);
                if(MO_PLASTIK!=opts.coloredMouseOver)
                    painter->setPen(m_mouseOverCols[opt.state&State_On ? 1 : 2]);
            }

            if(horiz || MO_PLASTIK!=opts.coloredMouseOver)
            {
                painter->drawLine(r.x(), r.y()+r.height()-1, r.x()+r.width()-1, r.y()+r.height()-1);
                painter->drawLine(r2.x(), r2.y()+r2.height()-1, r2.x()+r2.width()-1, r2.y()+r2.height()-1);
            }

            if(!horiz || MO_PLASTIK!=opts.coloredMouseOver)
            {
                painter->drawLine(r.x()+r.width()-1, r.y(), r.x()+r.width()-1, r.y()+r.height()-1);
                painter->drawLine(r2.x()+r2.width()-1, r2.y(), r2.x()+r2.width()-1, r2.y()+r2.height()-1);
            }
        }
    }

    painter->restore();
}

void Style::drawHighlight(QPainter *p, const QRect &r, bool horiz, bool inc) const
{
    QColor col1(m_mouseOverCols[ORIGINAL_SHADE]);

    col1.setAlphaF(0.5);
    drawFadedLine(p, r, inc ? col1 : m_mouseOverCols[ORIGINAL_SHADE], true, true, horiz);
    drawFadedLine(p, r.adjusted(horiz ? 0 : 1, horiz ? 1 : 0, 0, 0), inc ? m_mouseOverCols[ORIGINAL_SHADE] : col1, true, true, horiz);
}

void
Style::drawFadedLine(QPainter *p, const QRect &r, const QColor &col,
                     bool fadeStart, bool fadeEnd, bool horiz,
                     double fadeSizeStart, double fadeSizeEnd) const
{
    bool            aa(p->testRenderHint(QPainter::Antialiasing));
    QPointF         start(r.x()+(aa ? 0.5 : 0.0), r.y()+(aa ? 0.5 : 0.0)),
        end(r.x()+(horiz ? r.width()-1 : 0)+(aa ? 0.5 : 0.0),
            r.y()+(horiz ? 0 : r.height()-1)+(aa ? 0.5 : 0.0));

    if(opts.fadeLines && (fadeStart || fadeEnd))
    {
        QLinearGradient grad(start, end);
        QColor          fade(col);

        fade.setAlphaF(0.0);
        grad.setColorAt(0, fadeStart && opts.fadeLines ? fade : col);
        if(fadeSizeStart>=0 && fadeSizeStart<=1.0)
            grad.setColorAt(fadeSizeStart, col);
        if(fadeSizeEnd>=0 && fadeSizeEnd<=1.0)
            grad.setColorAt(1.0-fadeSizeEnd, col);
        grad.setColorAt(1, fadeEnd && opts.fadeLines ? fade : col);
        p->setPen(QPen(QBrush(grad), QPENWIDTH1));
    }
    else
        p->setPen(col);
    p->drawLine(start, end);
}

void Style::drawLines(QPainter *p, const QRect &r, bool horiz, int nLines, int offset, const QColor *cols, int startOffset,
                      int dark, ELine type) const
{
    int  space((nLines*2)+(LINE_DASHES!=type ? (nLines-1) : 0)),
        step(LINE_DASHES!=type ? 3 : 2),
        etchedDisp(type == LINE_SUNKEN ? 1 : 0),
        x(horiz ? r.x() : r.x()+((r.width()-space)>>1)),
        y(horiz ? r.y()+((r.height()-space)>>1) : r.y()),
        x2(r.x()+r.width()-1),
        y2(r.y()+r.height()-1),
        i;
    QPen dp(cols[dark], QPENWIDTH1),
        lp(cols[0], QPENWIDTH1);

    if(opts.fadeLines && (horiz ? r.width() : r.height())>16)
    {
        QLinearGradient grad(r.topLeft(), horiz ? r.topRight() : r.bottomLeft());
        QColor          fade(cols[dark]);

        fade.setAlphaF(0.0);
        grad.setColorAt(0, fade);
        grad.setColorAt(0.4, cols[dark]);
        grad.setColorAt(0.6, cols[dark]);
        grad.setColorAt(1, fade);

        dp=QPen(QBrush(grad), QPENWIDTH1);

        if(LINE_FLAT!=type)
        {
            fade=QColor(cols[0]);

            fade.setAlphaF(0.0);
            grad.setColorAt(0, fade);
            grad.setColorAt(0.4, cols[0]);
            grad.setColorAt(0.6, cols[0]);
            grad.setColorAt(1, fade);
            lp=QPen(QBrush(grad), QPENWIDTH1);
        }
    }

    p->setRenderHint(QPainter::Antialiasing, true);
    if(horiz)
    {
        if(startOffset && y+startOffset>0)
            y+=startOffset;

        p->setPen(dp);
        for(i=0; i<space; i+=step)
            drawAaLine(p, x+offset, y+i, x2-offset, y+i);

        if(LINE_FLAT!=type)
        {
            p->setPen(lp);
            x+=etchedDisp;
            x2+=etchedDisp;
            for(i=1; i<space; i+=step)
                drawAaLine(p, x+offset, y+i, x2-offset, y+i);
        }
    }
    else
    {
        if(startOffset && x+startOffset>0)
            x+=startOffset;

        p->setPen(dp);
        for(i=0; i<space; i+=step)
            drawAaLine(p, x+i, y+offset, x+i, y2-offset);

        if(LINE_FLAT!=type)
        {
            p->setPen(lp);
            y+=etchedDisp;
            y2+=etchedDisp;
            for(i=1; i<space; i+=step)
                drawAaLine(p, x+i, y+offset, x+i, y2-offset);
        }
    }
    QPAINTER_RENDERHINT_AA_MAYBE_OFF(p);
}

void Style::drawProgressBevelGradient(QPainter *p, const QRect &origRect, const QStyleOption *option, bool horiz, EAppearance bevApp,
                                      const QColor *cols) const
{
    bool    vertical(!horiz),
        inCache(true);
    QRect   r(0, 0, horiz ? PROGRESS_CHUNK_WIDTH*2 : origRect.width(),
              horiz ? origRect.height() : PROGRESS_CHUNK_WIDTH*2);
    QtcKey  key(createKey(horiz ? r.height() : r.width(), cols[ORIGINAL_SHADE], horiz, bevApp, WIDGET_PROGRESSBAR));
    QPixmap *pix(m_pixmapCache.object(key));

    if(!pix)
    {
        pix=new QPixmap(r.width(), r.height());

        QPainter pixPainter(pix);

        if(qtcIsFlat(bevApp))
            pixPainter.fillRect(r, cols[ORIGINAL_SHADE]);
        else
            drawBevelGradientReal(cols[ORIGINAL_SHADE], &pixPainter, r, horiz, false, bevApp, WIDGET_PROGRESSBAR);

        switch(opts.stripedProgress)
        {
        default:
        case STRIPE_NONE:
            break;
        case STRIPE_PLAIN:
        {
            QRect r2(horiz
                     ? QRect(r.x(), r.y(), PROGRESS_CHUNK_WIDTH, r.height())
                     : QRect(r.x(), r.y(), r.width(), PROGRESS_CHUNK_WIDTH));

            if(qtcIsFlat(bevApp))
                pixPainter.fillRect(r2, cols[1]);
            else
                drawBevelGradientReal(cols[1], &pixPainter, r2, horiz, false, bevApp, WIDGET_PROGRESSBAR);
            break;
        }
        case STRIPE_DIAGONAL:
        {
            QRegion reg;
            int     size(vertical ? origRect.width() : origRect.height());

            for(int offset=0; offset<(size*2); offset+=(PROGRESS_CHUNK_WIDTH*2))
            {
                QPolygon a;

                if(vertical)
                    a.setPoints(4, r.x(),           r.y()+offset,
                                r.x()+r.width(), (r.y()+offset)-size,
                                r.x()+r.width(), (r.y()+offset+PROGRESS_CHUNK_WIDTH)-size,
                                r.x(),           r.y()+offset+PROGRESS_CHUNK_WIDTH);
                else
                    a.setPoints(4, r.x()+offset,                             r.y(),
                                r.x()+offset+PROGRESS_CHUNK_WIDTH,        r.y(),
                                (r.x()+offset+PROGRESS_CHUNK_WIDTH)-size, r.y()+r.height(),
                                (r.x()+offset)-size,                      r.y()+r.height());

                reg+=QRegion(a);
            }

            pixPainter.setClipRegion(reg);
            if(qtcIsFlat(bevApp))
                pixPainter.fillRect(r, cols[1]);
            else
                drawBevelGradientReal(cols[1], &pixPainter, r, horiz, false, bevApp, WIDGET_PROGRESSBAR);
        }
        }

        pixPainter.end();
        int cost(pix->width()*pix->height()*(pix->depth()/8));

        if(cost<m_pixmapCache.maxCost())
            m_pixmapCache.insert(key, pix, cost);
        else
            inCache=false;
    }
    QRect fillRect(origRect);

    if(opts.animatedProgress)
    {
        int animShift=vertical || option->state&STATE_REVERSE ? PROGRESS_CHUNK_WIDTH : -PROGRESS_CHUNK_WIDTH;

        if(vertical || option->state&STATE_REVERSE)
            animShift -= (m_animateStep/2) % (PROGRESS_CHUNK_WIDTH*2);
        else
            animShift += (m_animateStep/2) % (PROGRESS_CHUNK_WIDTH*2);

        if(horiz)
            fillRect.adjust(animShift-PROGRESS_CHUNK_WIDTH, 0, PROGRESS_CHUNK_WIDTH, 0);
        else
            fillRect.adjust(0, animShift-PROGRESS_CHUNK_WIDTH, 0, PROGRESS_CHUNK_WIDTH);
    }

    p->save();
    p->setClipRect(origRect, Qt::IntersectClip);
    p->drawTiledPixmap(fillRect, *pix);
    if (opts.stripedProgress == STRIPE_FADE && fillRect.width() > 4 &&
        fillRect.height() > 4) {
        addStripes(p, QPainterPath(), fillRect, !vertical);
    }
    p->restore();

    if (!inCache) {
        delete pix;
    }
}

void
Style::drawBevelGradient(const QColor &base, QPainter *p, const QRect &origRect,
                         const QPainterPath &path, bool horiz, bool sel,
                         EAppearance bevApp, EWidget w, bool useCache) const
{
    if (origRect.width() < 1 || origRect.height() < 1) {
        return;
    }
    if (qtcIsFlat(bevApp)) {
        if (noneOf(w, WIDGET_TAB_TOP, WIDGET_TAB_BOT) ||
            !qtcIsCustomBgnd(opts) || opts.tabBgnd || !sel) {
            if (path.isEmpty()) {
                p->fillRect(origRect, base);
            } else {
                p->fillPath(path, base);
            }
        }
    } else {
        bool tab = oneOf(w, WIDGET_TAB_TOP, WIDGET_TAB_BOT);
        bool selected = tab ? false : sel;
        EAppearance app =
            (selected ? opts.sunkenAppearance :
             w == WIDGET_LISTVIEW_HEADER && bevApp == APPEARANCE_BEVELLED ?
             APPEARANCE_LV_BEVELLED : bevApp != APPEARANCE_BEVELLED ||
             WIDGET_BUTTON(w) ||
             oneOf(w, WIDGET_LISTVIEW_HEADER, WIDGET_TROUGH,
                   WIDGET_NO_ETCH_BTN, WIDGET_MENU_BUTTON) ? bevApp :
             APPEARANCE_GRADIENT);

        if (w == WIDGET_PROGRESSBAR || !useCache) {
            drawBevelGradientReal(base, p, origRect, path, horiz, sel, app, w);
        } else {
            QRect r(0, 0, horiz ? PIXMAP_DIMENSION : origRect.width(),
                    horiz ? origRect.height() : PIXMAP_DIMENSION);
            QtcKey key(createKey(horiz ? r.height() : r.width(),
                                 base, horiz, app, w));
            QPixmap *pix(m_pixmapCache.object(key));
            bool inCache(true);

            if (!pix) {
                pix = new QPixmap(r.width(), r.height());
                pix->fill(Qt::transparent);

                QPainter pixPainter(pix);

                drawBevelGradientReal(base, &pixPainter, r, horiz, sel, app, w);
                pixPainter.end();

                int cost(pix->width()*pix->height()*(pix->depth()/8));

                if (cost < m_pixmapCache.maxCost()) {
                    m_pixmapCache.insert(key, pix, cost);
                } else {
                    inCache = false;
                }
            }

            if(!path.isEmpty())
            {
                p->save();
                p->setClipPath(path, Qt::IntersectClip);
            }

            p->drawTiledPixmap(origRect, *pix);
            if(!path.isEmpty())
                p->restore();
            if(!inCache)
                delete pix;
        }
    }
}

void
Style::drawBevelGradientReal(const QColor &base, QPainter *p, const QRect &r,
                             const QPainterPath &path, bool horiz, bool sel,
                             EAppearance app, EWidget w) const
{
    bool topTab = (w == WIDGET_TAB_TOP);
    bool botTab = (w == WIDGET_TAB_BOT);
    bool dwt = qtcIsCustomBgnd(opts) && (w == WIDGET_DOCK_WIDGET_TITLE);
    bool titleBar = (opts.windowBorder & WINDOW_BORDER_BLEND_TITLEBAR &&
                     (oneOf(w, WIDGET_MDI_WINDOW, WIDGET_MDI_WINDOW_TITLE) ||
                      (opts.dwtSettings & DWT_COLOR_AS_PER_TITLEBAR &&
                       w == WIDGET_DOCK_WIDGET_TITLE && !dwt)));
    bool reverse = QApplication::layoutDirection() == Qt::RightToLeft;
    const Gradient *grad = qtcGetGradient(app, &opts);
    QLinearGradient g(r.topLeft(), horiz ? r.bottomLeft() : r.topRight());
    GradientStopCont::const_iterator it(grad->stops.begin());
    GradientStopCont::const_iterator end(grad->stops.end());
    int numStops(grad->stops.size());

    for (int i = 0;it != end;++it, ++i) {
        QColor col;

        if ((topTab || botTab || dwt || titleBar) && i == numStops-1) {
            if (titleBar) {
                col = m_backgroundCols[ORIGINAL_SHADE];
                col.setAlphaF(0.0);
            } else {
                col = base;
                if ((sel && opts.tabBgnd == 0 && !reverse) || dwt) {
                    col.setAlphaF(0.0);
                }
            }
        } else {
            shade(base, &col, botTab && opts.invertBotTab ?
                  qMax(INVERT_SHADE((*it).val), 0.9) : (*it).val);
        }
        if (w != WIDGET_TOOLTIP && (*it).alpha < 1.0) {
            col.setAlphaF(col.alphaF() * (*it).alpha);
        }
        g.setColorAt(botTab ? 1.0 - (*it).pos : (*it).pos, col);
    }

    if (app == APPEARANCE_AGUA && !(topTab || botTab || dwt) &&
        (horiz ? r.height() : r.width()) > AGUA_MAX) {
        QColor col;
        double pos = AGUA_MAX/((horiz ? r.height() : r.width())*2.0);
        shade(base, &col, AGUA_MID_SHADE);
        g.setColorAt(pos, col);
        g.setColorAt(1.0-pos, col);
    }

    //p->fillRect(r, base);
    if (path.isEmpty()) {
        p->fillRect(r, QBrush(g));
    } else {
        p->fillPath(path, QBrush(g));
    }
}

void Style::drawSunkenBevel(QPainter *p, const QRect &r, const QColor &col) const
{
    double          radius=opts.titlebarButtons&TITLEBAR_BUTTON_ROUND
        ? r.height()/2.0
        : opts.round>ROUND_FULL
        ? 5.0
        : opts.round>ROUND_SLIGHT
        ? 3.0
        : 2.0;
    QPainterPath    path(buildPath(QRectF(r), WIDGET_OTHER, ROUNDED_ALL, radius));
    QLinearGradient g(r.topLeft(), r.bottomLeft());
    QColor          black(Qt::black),
        white(Qt::white);

    black.setAlphaF(SUNKEN_BEVEL_DARK_ALPHA(col));
    white.setAlphaF(SUNKEN_BEVEL_LIGHT_ALPHA(col));
    g.setColorAt(0, black);
    g.setColorAt(1, white);
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->fillPath(path, QBrush(g));
    p->restore();
}

void
Style::drawLightBevel(QPainter *p, const QRect &r, const QStyleOption *option,
                      const QWidget *widget, int round, const QColor &fill,
                      const QColor *custom, bool doBorder, EWidget w) const
{
    bool onToolbar = (opts.tbarBtnAppearance != APPEARANCE_NONE &&
                      (w == WIDGET_TOOLBAR_BUTTON ||
                       (WIDGET_BUTTON(w) && isOnToolbar(widget))));

    if (oneOf(w, WIDGET_PROGRESSBAR, WIDGET_SB_BUTTON) ||
        (w == WIDGET_SPIN && !opts.unifySpin)/* || !m_usePixmapCache*/) {
        drawLightBevelReal(p, r, option, widget, round, fill, custom,
                           doBorder, w, true, opts.round, onToolbar);
    } else {
        static const int constMaxCachePixmap = 128;

        int endSize = 0;
        int middleSize = 8;
        bool horiz = (CIRCULAR_SLIDER(w) ||
                      isHoriz(option, w, opts.tbarBtns == TBTN_JOINED));
        bool circular = ((w == WIDGET_MDI_WINDOW_BUTTON &&
                          (opts.titlebarButtons & TITLEBAR_BUTTON_ROUND)) ||
                         oneOf(w, WIDGET_RADIO_BUTTON, WIDGET_DIAL) ||
                         CIRCULAR_SLIDER(w));
        double radius = 0;
        ERound realRound = qtcGetWidgetRound(&opts, r.width(), r.height(), w);

        if (!circular) {
            switch (realRound) {
            case ROUND_SLIGHT:
            case ROUND_NONE:
            case ROUND_FULL:
                endSize = (SLIDER(w) && opts.coloredMouseOver == MO_PLASTIK &&
                           option->state&State_MouseOver ? 9 : 5);
                break;
            case ROUND_EXTRA:
                endSize = 7;
                break;
            case ROUND_MAX:
                radius = qtcGetRadius(&opts, r.width(), r.height(),
                                      w, RADIUS_ETCH);
                endSize = (SLIDER(w) ? qMax((opts.sliderWidth / 2) + 1,
                                            int(radius + 1.5)) :
                           int(radius + 2.5));
                middleSize = MIN_ROUND_MAX_WIDTH - (endSize * 2) + 4;
                if (middleSize < 4)
                    middleSize = 4;
                break;
            }
        }

        int size = 2 * endSize + middleSize;

        if (size > constMaxCachePixmap) {
            drawLightBevelReal(p, r, option, widget, round, fill, custom,
                               doBorder, w, true, realRound, onToolbar);
        } else {
            bool small(circular || (horiz ? r.width() : r.height())<(2*endSize));
            QPixmap pix;
            const QSize pixSize(small ? QSize(r.width(), r.height()) :
                                QSize(horiz ? size : r.width(),
                                      horiz ? r.height() : size));
            uint state(option->state&(State_Raised|State_Sunken|State_On|State_Horizontal|State_HasFocus|State_MouseOver|
                                         (WIDGET_MDI_WINDOW_BUTTON==w ? State_Active : State_None)));

            QString key = QStringLiteral("qtc-%1-%2-%3-%4-%5-%6-%7-%8-%9")
                .arg(w, 0, 16).arg(onToolbar ? 1 : 0, 0, 16).arg(round, 0, 16)
                .arg((int)realRound, 0, 16).arg(pixSize.width(), 0, 16)
                .arg(pixSize.height(), 0, 16)
                .arg(state, 0, 16).arg(fill.rgba(), 0, 16).arg((int)(radius * 100), 0, 16);
            if (!m_usePixmapCache || !QPixmapCache::find(key, &pix)) {
                pix = QPixmap(pixSize);
                pix.fill(Qt::transparent);

                QPainter pixPainter(&pix);
                ERound oldRound = opts.round;
                opts.round = realRound;
                pixPainter.setRenderHint(QPainter::Antialiasing, true);
                drawLightBevelReal(&pixPainter, QRect(0, 0, pixSize.width(),
                                                      pixSize.height()), option,
                                   widget, round, fill, custom, doBorder, w,
                                   false, realRound, onToolbar);
                opts.round = oldRound;
                pixPainter.end();

                if (m_usePixmapCache) {
                    QPixmapCache::insert(key, pix);
                }
            }

            if (small) {
                p->drawPixmap(r.topLeft(), pix);
            } else if (horiz) {
                int middle(qMin(r.width()-(2*endSize), middleSize));
                if(middle>0)
                    p->drawTiledPixmap(r.x()+endSize, r.y(), r.width()-(2*endSize), pix.height(), pix.copy(endSize, 0, middle, pix.height()));
                p->drawPixmap(r.x(), r.y(), pix.copy(0, 0, endSize, pix.height()));
                p->drawPixmap(r.x()+r.width()-endSize, r.y(), pix.copy(pix.width()-endSize, 0, endSize, pix.height()));
            } else {
                int middle(qMin(r.height()-(2*endSize), middleSize));
                if (middle > 0) {
                    p->drawTiledPixmap(r.x(), r.y() + endSize, pix.width(),
                                       r.height() - 2 * endSize,
                                       pix.copy(0, endSize,
                                                pix.width(), middle));
                }
                p->drawPixmap(r.x(), r.y(),
                              pix.copy(0, 0, pix.width(), endSize));
                p->drawPixmap(r.x(), r.y() + r.height() - endSize,
                              pix.copy(0, pix.height() - endSize, pix.width(),
                                       endSize));
            }

            if (w == WIDGET_SB_SLIDER && opts.stripedSbar) {
                QRect rx(r.adjusted(1, 1, -1, -1));
                addStripes(p, buildPath(rx, WIDGET_SB_SLIDER, realRound,
                                        qtcGetRadius(&opts, rx.width() - 1,
                                                     rx.height() - 1,
                                                     WIDGET_SB_SLIDER,
                                                     RADIUS_INTERNAL)),
                           rx, horiz);
            }
        }
    }
}

void
Style::drawLightBevelReal(QPainter *p, const QRect &rOrig,
                          const QStyleOption *option, const QWidget *widget,
                          int round, const QColor &fill, const QColor *custom,
                          bool doBorder, EWidget w, bool useCache,
                          ERound realRound, bool onToolbar) const
{
    EAppearance app(qtcWidgetApp(onToolbar ? WIDGET_TOOLBAR_BUTTON : w,
                                  &opts, option->state&State_Active));
    QRect r(rOrig);
    bool bevelledButton((WIDGET_BUTTON(w) || WIDGET_NO_ETCH_BTN == w ||
                         WIDGET_MENU_BUTTON == w) && APPEARANCE_BEVELLED == app);
    bool sunken(option->state &(/*State_Down | */State_On | State_Sunken));
    bool flatWidget((WIDGET_MDI_WINDOW_BUTTON==w &&
                     (opts.round==ROUND_MAX ||
                      opts.titlebarButtons&TITLEBAR_BUTTON_ROUND)) ||
                    (WIDGET_PROGRESSBAR==w && !opts.borderProgress));
    bool lightBorder(!flatWidget && DRAW_LIGHT_BORDER(sunken, w, app));
    bool draw3dfull(!flatWidget && !lightBorder &&
                    DRAW_3D_FULL_BORDER(sunken, app));
    bool draw3d(!flatWidget && (draw3dfull ||
                                (!lightBorder && DRAW_3D_BORDER(sunken, app))));
    bool drawShine(DRAW_SHINE(sunken, app));
    bool doColouredMouseOver(doBorder && option->state&State_Enabled &&
                             WIDGET_MDI_WINDOW_BUTTON!=w && WIDGET_SPIN!=w &&
                             WIDGET_COMBO_BUTTON!=w && WIDGET_SB_BUTTON!=w &&
                             (!SLIDER(w) || !opts.colorSliderMouseOver) &&
                             !(option->state&STATE_KWIN_BUTTON) &&
                             (opts.coloredTbarMo ||
                              !(option->state&STATE_TBAR_BUTTON)) &&
                             opts.coloredMouseOver &&
                             option->state&State_MouseOver &&
                             WIDGET_PROGRESSBAR!=w &&
                             (option->state&STATE_TOGGLE_BUTTON || !sunken));
    bool plastikMouseOver(doColouredMouseOver &&
                          MO_PLASTIK==opts.coloredMouseOver);
    bool colouredMouseOver(doColouredMouseOver && WIDGET_MENU_BUTTON!=w &&
                           (MO_COLORED==opts.coloredMouseOver ||
                            MO_COLORED_THICK==opts.coloredMouseOver ||
                            (MO_GLOW==opts.coloredMouseOver &&
                             !(opts.buttonEffect != EFFECT_NONE)))),
        doEtch(doBorder && ETCH_WIDGET(w) && opts.buttonEffect != EFFECT_NONE),
        glowFocus(doEtch && USE_GLOW_FOCUS(option->state&State_MouseOver) && option->state&State_HasFocus &&
                  option->state&State_Enabled),
        horiz(CIRCULAR_SLIDER(w) || isHoriz(option, w, TBTN_JOINED==opts.tbarBtns)),
        sunkenToggleMo(sunken && !(option->state&State_Sunken) && option->state&(State_MouseOver|STATE_TOGGLE_BUTTON));
    const QColor *cols(custom ? custom : m_backgroundCols),
        *border(colouredMouseOver ? borderColors(option, cols) : cols);

    p->save();

    if (doEtch)
        r.adjust(1, 1, -1, -1);

    if (WIDGET_TROUGH == w && !opts.borderSbarGroove)
        doBorder = false;

    p->setRenderHint(QPainter::Antialiasing, true);

    if (r.width() > 0 && r.height() > 0) {
        if (w == WIDGET_PROGRESSBAR && opts.stripedProgress != STRIPE_NONE) {
            drawProgressBevelGradient(p, opts.borderProgress ?
                                      r.adjusted(1, 1, -1, -1) : r,
                                      option, horiz, app, custom);
        } else {
            drawBevelGradient(fill, p, WIDGET_PROGRESSBAR==w && opts.borderProgress ? r.adjusted(1, 1, -1, -1) : r,
                              doBorder
                              ? buildPath(r, w, round, qtcGetRadius(&opts, r.width()-2, r.height()-2, w, RADIUS_INTERNAL))
                              : buildPath(QRectF(r), w, round, qtcGetRadius(&opts, r.width(), r.height(), w, RADIUS_EXTERNAL)),
                              horiz, sunken, app, w, useCache);

            if(!sunken || sunkenToggleMo)
                if(plastikMouseOver) // && !sunken)
                {
                    p->save();
                    p->setClipPath(buildPath(r.adjusted(0, 0, 0, -1), w, round,
                                             qtcGetRadius(&opts, r.width()-2, r.height()-2, w, RADIUS_INTERNAL)));
                    if (SLIDER(w)) {
                        int len = sbSliderMOLen(opts, horiz ? r.width() :
                                                r.height()) + 1;
                        int so = lightBorder ? SLIDER_MO_PLASTIK_BORDER : 1;
                        int eo = len + so;
                        int col = SLIDER_MO_SHADE;

                        if (horiz) {
                            drawBevelGradient(m_mouseOverCols[col], p, QRect(r.x()+so-1, r.y(), len, r.height()-1), horiz, sunken, app, w, useCache);
                            drawBevelGradient(m_mouseOverCols[col], p, QRect(r.x()+r.width()-eo+1, r.y(), len, r.height()-1), horiz, sunken, app, w, useCache);
                        }
                        else
                        {
                            drawBevelGradient(m_mouseOverCols[col], p, QRect(r.x(), r.y()+so-1, r.width()-1, len), horiz, sunken, app, w, useCache);
                            drawBevelGradient(m_mouseOverCols[col], p, QRect(r.x(), r.y()+r.height()-eo+1, r.width()-1, len), horiz, sunken, app, w, useCache);
                        }
                    }
                    else
                    {
                        bool horizontal((horiz && WIDGET_SB_BUTTON!=w)|| (!horiz && WIDGET_SB_BUTTON==w)),
                            thin(WIDGET_SB_BUTTON==w || WIDGET_SPIN==w || ((horiz ? r.height() : r.width())<16));

                        setPainterPen(p, m_mouseOverCols[MO_PLASTIK_DARK(w)], QPENWIDTH1);
                        if(horizontal)
                        {
                            drawAaLine(p, r.x()+1, r.y()+1, r.x()+r.width()-2, r.y()+1);
                            drawAaLine(p, r.x()+1, r.y()+r.height()-2, r.x()+r.width()-2, r.y()+r.height()-2);
                        }
                        else
                        {
                            drawAaLine(p, r.x()+1, r.y()+1, r.x()+1, r.y()+r.height()-2);
                            drawAaLine(p, r.x()+r.width()-2, r.y()+1, r.x()+r.width()-2, r.y()+r.height()-2);
                        }
                        if(!thin)
                        {
                            setPainterPen(p, m_mouseOverCols[MO_PLASTIK_LIGHT(w)], QPENWIDTH1);
                            if(horizontal)
                            {
                                drawAaLine(p, r.x()+1, r.y()+2, r.x()+r.width()-2, r.y()+2);
                                drawAaLine(p, r.x()+1, r.y()+r.height()-3, r.x()+r.width()-2, r.y()+r.height()-3);
                            }
                            else
                            {
                                drawAaLine(p, r.x()+2, r.y()+1, r.x()+2, r.y()+r.height()-2);
                                drawAaLine(p, r.x()+r.width()-3, r.y()+1, r.x()+r.width()-3, r.y()+r.height()-2);
                            }
                        }
                    }
                    p->restore();
                }
        }

        if(drawShine)
        {
            bool   mo(option->state&State_Enabled && option->state&State_MouseOver && opts.highlightFactor);
            QColor white(Qt::white);

            if(WIDGET_MDI_WINDOW_BUTTON==w || WIDGET_RADIO_BUTTON==w || CIRCULAR_SLIDER(w))
            {
                QRectF          ra(r.x()+0.5, r.y()+0.5, r.width(), r.height());
                double          topSize=(ra.height()*0.4),
                    topWidthAdjust=WIDGET_RADIO_BUTTON==w || WIDGET_SLIDER==w ? 4 : 4.75;
                QRectF          topGradRect(ra.x()+topWidthAdjust, ra.y(),
                                            ra.width()-(topWidthAdjust*2)-1, topSize-1);
                QLinearGradient topGrad(topGradRect.topLeft(), topGradRect.bottomLeft());

                white.setAlphaF(mo ? (opts.highlightFactor>0 ? 0.8 : 0.7) : 0.75);
                topGrad.setColorAt(0.0, white);
                white.setAlphaF(/*mo ? (opts.highlightFactor>0 ? 0.3 : 0.1) : */0.2);
                topGrad.setColorAt(1.0, white);
                p->fillPath(buildPath(topGradRect, w, round, topSize), QBrush(topGrad));
            }
            else
            {
                QRectF ra(r.x()+0.5, r.y()+0.5, r.width(), r.height());
                double size = qtcMin((horiz ? ra.height() : ra.width()) / 2.0,
                                     16),
                    rad=size/2.0;
                int    mod=4;

                if(horiz)
                {
                    if(!(ROUNDED_LEFT&round))
                        ra.adjust(-8, 0, 0, 0);
                    if(!(ROUNDED_RIGHT&round))
                        ra.adjust(0, 0, 8, 0);
                }
                else
                {
                    if(!(ROUNDED_TOP&round))
                        ra.adjust(0, -8, 0, 0);
                    if(!(ROUNDED_BOTTOM&round))
                        ra.adjust(0, 0, 0, 8);
                }

                if (realRound < ROUND_MAX ||
                    (!isMaxRoundWidget(w) && !IS_SLIDER(w))) {
                    rad /= 2.0;
                    mod = mod >> 1;
                }

                QRectF          gr(horiz ? QRectF(ra.x()+mod, ra.y(), ra.width()-(mod*2)-1, size-1)
                                   : QRectF(ra.x(), ra.y()+mod, size-1, ra.height()-(mod*2)-1));
                QLinearGradient g(gr.topLeft(), horiz ? gr.bottomLeft() : gr.topRight());

                white.setAlphaF(mo ? (opts.highlightFactor>0 ? 0.95 : 0.85) : 0.9);
                g.setColorAt(0.0, white);
                white.setAlphaF(mo ? (opts.highlightFactor>0 ? 0.3 : 0.1) : 0.2);
                g.setColorAt(1.0, white);
                if(WIDGET_SB_BUTTON==w)
                {
                    p->save();
                    p->setClipRect(r);
                }
                p->fillPath(buildPath(gr, w, round, rad), QBrush(g));
                if(WIDGET_SB_BUTTON==w)
                    p->restore();
            }
        }
    }

    r.adjust(1, 1, -1, -1);

    if (plastikMouseOver && (!sunken  || sunkenToggleMo)) {
        bool thin = (oneOf(w, WIDGET_SB_BUTTON, WIDGET_SPIN) ||
                     (horiz ? r.height() : r.width()) < 16);
        bool horizontal = (SLIDER(w) ? !horiz :
                           (horiz && w != WIDGET_SB_BUTTON) ||
                           (!horiz && w == WIDGET_SB_BUTTON));
        int len = SLIDER(w) ? sbSliderMOLen(opts, horiz ? r.width() :
                                            r.height()) : (thin ? 1 : 2);

        p->save();
        if (horizontal) {
            p->setClipRect(r.x(), r.y() + len, r.width(), r.height() - len * 2);
        } else {
            p->setClipRect(r.x() + len, r.y(), r.width() - len * 2, r.height());
        }
    }

    if (!colouredMouseOver && lightBorder) {
        setPainterPen(p, cols[LIGHT_BORDER(app)], QPENWIDTH1);
        p->drawPath(buildPath(r, w, round,
                              qtcGetRadius(&opts, r.width(),
                                           r.height(), w, RADIUS_INTERNAL)));
    } else if (colouredMouseOver || (draw3d && option->state & State_Raised)) {
        QPainterPath innerTlPath;
        QPainterPath innerBrPath;
        int dark(/*bevelledButton ? */2/* : 4*/);

        buildSplitPath(r, round, qtcGetRadius(&opts, r.width(), r.height(), w, RADIUS_INTERNAL),
                       innerTlPath, innerBrPath);

        setPainterPen(p, border[colouredMouseOver ? MO_STD_LIGHT(w, sunken) : (sunken ? dark : 0)], QPENWIDTH1);
        p->drawPath(innerTlPath);
        if(colouredMouseOver || bevelledButton || draw3dfull)
        {
            setPainterPen(p, border[colouredMouseOver ? MO_STD_DARK(w) : (sunken ? 0 : dark)], QPENWIDTH1);
            p->drawPath(innerBrPath);
        }
    }
    if(plastikMouseOver && (!sunken  || sunkenToggleMo))
        p->restore();
    QPAINTER_RENDERHINT_AA_MAYBE_OFF(p);

    if(doEtch || glowFocus)
    {
        if( !(opts.thin&THIN_FRAMES) && (!sunken || sunkenToggleMo ||
                                         (sunken && glowFocus && widget && qobject_cast<const QAbstractButton *>(widget) &&
                                          static_cast<const QAbstractButton *>(widget)->isCheckable())) &&
            ((WIDGET_OTHER!=w && WIDGET_SLIDER_TROUGH!=w && MO_GLOW==opts.coloredMouseOver && option->state&State_MouseOver) ||
             (WIDGET_DEF_BUTTON==w && IND_GLOW==opts.defBtnIndicator) ||
             glowFocus) )
            drawGlow(p, rOrig, WIDGET_DEF_BUTTON==w && option->state&State_MouseOver ? WIDGET_STD_BUTTON : w,
                     glowFocus ? m_focusCols : 0L);
        else
            drawEtch(p, rOrig, widget, w, EFFECT_SHADOW==opts.buttonEffect && WIDGET_BUTTON(w) && !sunken);
    }

    if(doBorder)
    {
        const QColor *borderCols=glowFocus || ( (WIDGET_COMBO==w || WIDGET_MENU_BUTTON==w || (WIDGET_NO_ETCH_BTN==w && ROUNDED_ALL!=round)) &&
                                                USE_GLOW_FOCUS(option->state&State_MouseOver) &&
                                                option->state&State_HasFocus && option->state&State_Enabled)
            ? m_focusCols
            : (WIDGET_COMBO==w || WIDGET_COMBO_BUTTON==w) && border==m_comboBtnCols
            ? option->state&State_MouseOver && MO_GLOW==opts.coloredMouseOver && !sunken
            ? m_mouseOverCols
            : m_buttonCols
            : cols;

        r.adjust(-1, -1, 1, 1);
        if(!sunken && option->state&State_Enabled && !glowFocus &&
           ( ( ( (doEtch && WIDGET_OTHER!=w && WIDGET_SLIDER_TROUGH!=w) || SLIDER(w) || WIDGET_COMBO==w || WIDGET_MENU_BUTTON==w ) &&
               (MO_GLOW==opts.coloredMouseOver/* || MO_COLORED==opts.colorMenubarMouseOver*/) && option->state&State_MouseOver) ||
             glowFocus || (doEtch && WIDGET_DEF_BUTTON==w && IND_GLOW==opts.defBtnIndicator)))
            drawBorder(p, r, option, round,
                       WIDGET_DEF_BUTTON==w && IND_GLOW==opts.defBtnIndicator && !(option->state&State_MouseOver)
                       ? m_defBtnCols : m_mouseOverCols, w);
        else
            drawBorder(p, r, option, round,
                       colouredMouseOver && MO_COLORED_THICK==opts.coloredMouseOver ? m_mouseOverCols : borderCols, w);
    }

    p->restore();
}

void Style::drawGlow(QPainter *p, const QRect &r, EWidget w, const QColor *cols) const
{
    bool   def(WIDGET_DEF_BUTTON==w && IND_GLOW==opts.defBtnIndicator),
        defShade=def && (!m_defBtnCols ||
                         (m_defBtnCols[ORIGINAL_SHADE]==m_mouseOverCols[ORIGINAL_SHADE]));
    QColor col(cols ? cols[GLOW_MO]
               : def && m_defBtnCols
               ? m_defBtnCols[GLOW_DEFBTN] : m_mouseOverCols[GLOW_MO]);

    col.setAlphaF(GLOW_ALPHA(defShade));
    p->setBrush(Qt::NoBrush);
    p->setRenderHint(QPainter::Antialiasing, true);
    setPainterPen(p, col, QPENWIDTH1);
    p->drawPath(buildPath(r, w, ROUNDED_ALL, qtcGetRadius(&opts, r.width(), r.height(), w, RADIUS_ETCH)));
    QPAINTER_RENDERHINT_AA_MAYBE_OFF(p);
}

void Style::drawEtch(QPainter *p, const QRect &r, const QWidget *widget,  EWidget w, bool raised, int round) const
{
    QPainterPath tl,
        br;
    QColor       col(Qt::black);

    if(WIDGET_TOOLBAR_BUTTON==w && EFFECT_ETCH==opts.tbarBtnEffect)
        raised=false;

    buildSplitPath(r, round, qtcGetRadius(&opts, r.width(), r.height(), w, RADIUS_ETCH), tl, br);

    col.setAlphaF(USE_CUSTOM_ALPHAS(opts) ? opts.customAlphas[ALPHA_ETCH_DARK] : ETCH_TOP_ALPHA);
    p->setBrush(Qt::NoBrush);
    p->setRenderHint(QPainter::Antialiasing, true);
    setPainterPen(p, col, QPENWIDTH1);

    if(!raised && WIDGET_SLIDER!=w)
    {
        p->drawPath(tl);
        if(WIDGET_SLIDER_TROUGH==w && opts.thinSbarGroove && widget && qobject_cast<const QScrollBar *>(widget))
        {
            QColor col(Qt::white);
            col.setAlphaF(USE_CUSTOM_ALPHAS(opts) ? opts.customAlphas[ALPHA_ETCH_LIGHT] : ETCH_BOTTOM_ALPHA); // 0.25);
            setPainterPen(p, col, QPENWIDTH1);
        }
        else
            setPainterPen(p, getLowerEtchCol(widget), QPENWIDTH1);
    }

    p->drawPath(br);
    QPAINTER_RENDERHINT_AA_MAYBE_OFF(p);
}

void Style::drawBgndRing(QPainter &painter, int x, int y, int size, int size2, bool isWindow) const
{
    double width=(size-size2)/2.0,
        width2=width/2.0;
    QColor col(Qt::white);

    col.setAlphaF(RINGS_INNER_ALPHA(isWindow ? opts.bgndImage.type : opts.menuBgndImage.type));
    if (width == 1) {
        width = QPENWIDTH1;
    }
    painter.setPen(QPen(col, width));
    painter.drawEllipse(QRectF(x+width2, y+width2, size-width, size-width));

    if(IMG_BORDERED_RINGS==(isWindow ? opts.bgndImage.type : opts.menuBgndImage.type))
    {
        col.setAlphaF(RINGS_OUTER_ALPHA);
        painter.setPen(QPen(col, QPENWIDTH1));
        painter.drawEllipse(QRectF(x, y, size, size));
        if(size2)
            painter.drawEllipse(QRectF(x+width, y+width, size2, size2));
    }
}

QPixmap Style::drawStripes(const QColor &color, int opacity) const
{
    QPixmap pix;
    QColor  col(color);

    if(100!=opacity)
        col.setAlphaF(opacity/100.0);

    QString key = QStringLiteral("qtc-stripes-%1").arg(col.rgba(), 0, 16);
    if(!m_usePixmapCache || !QPixmapCache::find(key, &pix))
    {
        pix=QPixmap(QSize(64, 64));

        if(100!=opacity)
            pix.fill(Qt::transparent);

        QPainter pixPainter(&pix);
        QColor   col2(shade(col, BGND_STRIPE_SHADE));

        if(100!=opacity)
        {
            col2.setAlphaF(opacity/100.0);
            pixPainter.setPen(QPen(col, QPENWIDTH1));
            for(int i=0; i<pix.height(); i+=4)
                pixPainter.drawLine(0, i, pix.width()-1, i);
        }
        else
            pixPainter.fillRect(pix.rect(), col);
        pixPainter.setPen(QPen(QColor((3*col.red()+col2.red())/4,
                                 (3*col.green()+col2.green())/4,
                                 (3*col.blue()+col2.blue())/4,
                                 100!=opacity ? col2.alpha() : 255), QPENWIDTH1));

        for(int i=1; i<pix.height(); i+=4)
        {
            pixPainter.drawLine(0, i, pix.width()-1, i);
            pixPainter.drawLine(0, i+2, pix.width()-1, i+2);
        }
        pixPainter.setPen(QPen(col2, QPENWIDTH1));
        for(int i=2; i<pix.height()-1; i+=4)
            pixPainter.drawLine(0, i, pix.width()-1, i);

        if(m_usePixmapCache)
            QPixmapCache::insert(key, pix);
    }

    return pix;
}

void
Style::drawBackground(QPainter *p, const QColor &bgnd, const QRect &r,
                      int opacity, BackgroundType type, EAppearance app,
                      const QPainterPath &path) const
{
    bool isWindow = type != BGND_MENU;

    if (!qtcIsFlatBgnd(app)) {
        static const int constPixmapWidth  = 16;
        static const int constPixmapHeight = 512;

        QColor col(bgnd);
        QPixmap pix;
        QSize scaledSize;
        EGradType grad = isWindow ? opts.bgndGrad : opts.menuBgndGrad;

        if (app == APPEARANCE_STRIPED) {
            pix = drawStripes(col, opacity);
        } else if (app == APPEARANCE_FILE) {
            pix = isWindow ? opts.bgndPixmap.img : opts.menuBgndPixmap.img;
        } else {
            scaledSize = QSize(grad == GT_HORIZ ? constPixmapWidth : r.width(),
                               grad == GT_HORIZ ? r.height() :
                               constPixmapWidth);

            if (opacity != 100)
                col.setAlphaF(opacity / 100.0);

            QString key = QStringLiteral("qtc-bgnd-%1-%2-%3")
                .arg(col.rgba(), 0, 16).arg(grad).arg(app);
            if (!m_usePixmapCache || !QPixmapCache::find(key, &pix)) {
                pix = QPixmap(QSize(grad == GT_HORIZ ? constPixmapWidth :
                                    constPixmapHeight, grad == GT_HORIZ ?
                                    constPixmapHeight : constPixmapWidth));
                pix.fill(Qt::transparent);

                QPainter pixPainter(&pix);
                drawBevelGradientReal(col, &pixPainter,
                                      QRect(0, 0, pix.width(), pix.height()),
                                      grad == GT_HORIZ, false, app,
                                      WIDGET_OTHER);
                pixPainter.end();
                if (m_usePixmapCache) {
                    QPixmapCache::insert(key, pix);
                }
            }
        }

        if (path.isEmpty()) {
            p->drawTiledPixmap(r, oneOf(app, APPEARANCE_STRIPED,
                                        APPEARANCE_FILE) ||
                               scaledSize == pix.size() ? pix :
                               pix.scaled(scaledSize, Qt::IgnoreAspectRatio));
        } else {
            p->save();
            p->setBrushOrigin(r.x(), r.y());
            p->fillPath(path,
                        QBrush(oneOf(app, APPEARANCE_STRIPED,
                                     APPEARANCE_FILE) ||
                               scaledSize == pix.size() ? pix :
                               pix.scaled(scaledSize, Qt::IgnoreAspectRatio)));
            p->restore();
        }

        if (isWindow && noneOf(app, APPEARANCE_STRIPED, APPEARANCE_FILE) &&
            grad == GT_HORIZ &&
            qtcGetGradient(app, &opts)->border == GB_SHINE) {
            int size = qMin(BGND_SHINE_SIZE, qMin(r.height() * 2, r.width()));
            QString key = QStringLiteral("qtc-radial-%1").arg(size / BGND_SHINE_STEPS, 0, 16);
            if (!m_usePixmapCache || !QPixmapCache::find(key, &pix)) {
                size /= BGND_SHINE_STEPS;
                size *= BGND_SHINE_STEPS;
                pix = QPixmap(size, size / 2);
                pix.fill(Qt::transparent);
                QRadialGradient gradient(QPointF(pix.width() / 2.0, 0),
                                         pix.width() / 2.0,
                                         QPointF(pix.width() / 2.0, 0));
                QColor c(Qt::white);
                double alpha = qtcShineAlpha(&col);

                c.setAlphaF(alpha);
                gradient.setColorAt(0, c);
                c.setAlphaF(alpha * 0.625);
                gradient.setColorAt(0.5, c);
                c.setAlphaF(alpha * 0.175);
                gradient.setColorAt(0.75, c);
                c.setAlphaF(0);
                gradient.setColorAt(1, c);
                QPainter pixPainter(&pix);
                pixPainter.fillRect(QRect(0, 0, pix.width(), pix.height()),
                                    gradient);
                pixPainter.end();
                if (m_usePixmapCache) {
                    QPixmapCache::insert(key, pix);
                }
            }
            p->drawPixmap(r.x() + ((r.width() - pix.width()) / 2), r.y(), pix);
        }
    } else {
        QColor col(bgnd);
        if (opacity != 100) {
            col.setAlphaF(opacity / 100.0);
        }
        if (path.isEmpty()) {
            p->fillRect(r, col);
        } else {
            p->save();
            p->setBrushOrigin(r.x(), r.y());
            p->fillPath(path, col);
            p->restore();
        }
    }
}

void
Style::drawBackgroundImage(QPainter *p, bool isWindow, const QRect &r) const
{
    QtCImage &img = isWindow ? opts.bgndImage : opts.menuBgndImage;
    int imgWidth = img.type == IMG_FILE ? img.width : RINGS_WIDTH(img.type);
    int imgHeight = img.type == IMG_FILE ? img.height : RINGS_HEIGHT(img.type);

    switch (img.type) {
    case IMG_NONE:
        break;
    case IMG_FILE:
        qtcLoadBgndImage(&img);
        if (!img.pixmap.img.isNull()) {
            switch (img.pos) {
            case PP_TL:
                p->drawPixmap(r.x(), r.y(), img.pixmap.img);
                break;
            case PP_TM:
                p->drawPixmap(r.x() + (r.width() - img.pixmap.img.width()) / 2,
                              r.y(), img.pixmap.img);
                break;
            default:
            case PP_TR:
                p->drawPixmap(r.right() - img.pixmap.img.width(), r.y(),
                              img.pixmap.img);
                break;
            case PP_BL:
                p->drawPixmap(r.x(), r.bottom() - img.pixmap.img.height(),
                              img.pixmap.img);
                break;
            case PP_BM:
                p->drawPixmap(r.x() + (r.width() - img.pixmap.img.width()) / 2,
                              r.bottom() - img.pixmap.img.height(),
                              img.pixmap.img);
                break;
            case PP_BR:
                p->drawPixmap(r.right() - img.pixmap.img.width(),
                              r.bottom() - img.pixmap.img.height(),
                              img.pixmap.img);
                break;
            case PP_LM:
                p->drawPixmap(r.left(), r.y() + (r.height() -
                                                 img.pixmap.img.height()) / 2,
                              img.pixmap.img);
                break;
            case PP_RM:
                p->drawPixmap(r.right() - img.pixmap.img.width(),
                              r.y() + (r.height() -
                                       img.pixmap.img.height()) / 2,
                              img.pixmap.img);
                break;
            case PP_CENTRED:
                p->drawPixmap(r.x() + (r.width() - img.pixmap.img.width()) / 2,
                              r.y() + (r.height() -
                                       img.pixmap.img.height()) / 2,
                              img.pixmap.img);
            }
        }
        break;
    case IMG_PLAIN_RINGS:
    case IMG_BORDERED_RINGS:
        if (img.pixmap.img.isNull()) {
            img.pixmap.img = QPixmap(imgWidth, imgHeight);
            img.pixmap.img.fill(Qt::transparent);
            QPainter pixPainter(&img.pixmap.img);

            pixPainter.setRenderHint(QPainter::Antialiasing);
            drawBgndRing(pixPainter, 0, 0, 200, 140, isWindow);

            drawBgndRing(pixPainter, 210, 10, 230, 214, isWindow);
            drawBgndRing(pixPainter, 226, 26, 198, 182, isWindow);
            drawBgndRing(pixPainter, 300, 100, 50, 0, isWindow);

            drawBgndRing(pixPainter, 100, 96, 160, 144, isWindow);
            drawBgndRing(pixPainter, 116, 112, 128, 112, isWindow);

            drawBgndRing(pixPainter, 250, 160, 200, 140, isWindow);
            drawBgndRing(pixPainter, 310, 220, 80, 0, isWindow);
            pixPainter.end();
        }
        p->drawPixmap(r.right() - img.pixmap.img.width(),
                      r.y() + 1, img.pixmap.img);
        break;
    case IMG_SQUARE_RINGS:
        if (img.pixmap.img.isNull()) {
            img.pixmap.img = QPixmap(imgWidth, imgHeight);
            img.pixmap.img.fill(Qt::transparent);
            QPainter pixPainter(&img.pixmap.img);
            QColor col(Qt::white);
            double halfWidth = RINGS_SQUARE_LINE_WIDTH / 2.0;

            col.setAlphaF(RINGS_SQUARE_SMALL_ALPHA);
            pixPainter.setRenderHint(QPainter::Antialiasing);
            pixPainter.setPen(QPen(col, RINGS_SQUARE_LINE_WIDTH, Qt::SolidLine,
                                   Qt::SquareCap, Qt::RoundJoin));
            pixPainter.drawPath(buildPath(QRectF(halfWidth + 0.5,
                                                 halfWidth + 0.5,
                                                 RINGS_SQUARE_SMALL_SIZE,
                                                 RINGS_SQUARE_SMALL_SIZE),
                                          WIDGET_OTHER, ROUNDED_ALL,
                                          RINGS_SQUARE_RADIUS));
            pixPainter.drawPath(buildPath(QRectF(halfWidth + 0.5 +
                                                 (imgWidth -
                                                  (RINGS_SQUARE_SMALL_SIZE +
                                                   RINGS_SQUARE_LINE_WIDTH)),
                                                 halfWidth + 0.5 +
                                                 (imgHeight -
                                                  (RINGS_SQUARE_SMALL_SIZE +
                                                   RINGS_SQUARE_LINE_WIDTH)),
                                                 RINGS_SQUARE_SMALL_SIZE,
                                                 RINGS_SQUARE_SMALL_SIZE),
                                          WIDGET_OTHER, ROUNDED_ALL,
                                          RINGS_SQUARE_RADIUS));
            col.setAlphaF(RINGS_SQUARE_LARGE_ALPHA);
            pixPainter.setPen(QPen(col, RINGS_SQUARE_LINE_WIDTH, Qt::SolidLine,
                                   Qt::SquareCap, Qt::RoundJoin));
            pixPainter.drawPath(buildPath(QRectF(halfWidth + 0.5 +
                                                 (imgWidth -
                                                  RINGS_SQUARE_LARGE_SIZE -
                                                  RINGS_SQUARE_LINE_WIDTH) / 2.0,
                                                 halfWidth + 0.5 +
                                                 (imgHeight -
                                                  RINGS_SQUARE_LARGE_SIZE -
                                                  RINGS_SQUARE_LINE_WIDTH) / 2.0,
                                                 RINGS_SQUARE_LARGE_SIZE,
                                                 RINGS_SQUARE_LARGE_SIZE),
                                          WIDGET_OTHER, ROUNDED_ALL,
                                          RINGS_SQUARE_RADIUS));
            pixPainter.end();
        }
        p->drawPixmap(r.right() - img.pixmap.img.width(),
                      r.y() + 1, img.pixmap.img);
        break;
    }
}

void
Style::drawBackground(QPainter *p, const QWidget *widget,
                      BackgroundType type) const
{
    bool isWindow = type != BGND_MENU;
    bool previewMdi = (isWindow && m_isPreview &&
                       qobject_cast<const QMdiSubWindow*>(widget));
    const QWidget *window = m_isPreview ? widget : widget->window();
    int opacity = (type == BGND_MENU ? opts.menuBgndOpacity :
                   type == BGND_DIALOG ? opts.dlgOpacity : opts.bgndOpacity);
    QRect bgndRect = widget->rect();
    QRect imgRect = bgndRect;
    QtcQWidgetProps props(widget);

    if (opacity != 100 && !(qobject_cast<const QMdiSubWindow*>(widget) ||
                            Utils::hasAlphaChannel(window))) {
        opacity = 100;
    }
    if (widget) {
        props->opacity = opacity;
    }

    p->setClipRegion(widget->rect(), Qt::IntersectClip);

    if (isWindow) {
        if (!previewMdi) {
            WindowBorders borders = qtcGetWindowBorderSize(false);
            bgndRect.adjust(-borders.sides, -borders.titleHeight,
                            borders.sides, borders.bottom);
        } else {
            bgndRect.adjust(0, -pixelMetric(PM_TitleBarHeight,
                                            0L, widget), 0, 0);
        }
        if (opts.bgndImage.type == IMG_FILE && opts.bgndImage.onBorder) {
            imgRect = bgndRect;
        }
    }

    drawBackground(p, (isWindow ? window->palette().window().color() :
                       popupMenuCols()[ORIGINAL_SHADE]), bgndRect, opacity,
                   type, (type != BGND_MENU ? opts.bgndAppearance :
                          opts.menuBgndAppearance));
    // FIXME, workaround only, the non transparent part of the image will have
    // a different overall opacity.
    p->save();
    p->setCompositionMode(QPainter::CompositionMode_SourceOver);
    drawBackgroundImage(p, isWindow, imgRect);
    p->restore();
}

QPainterPath
Style::buildPath(const QRectF &r, EWidget w, int round, double radius) const
{
    QPainterPath path;

    if (oneOf(w, WIDGET_RADIO_BUTTON, WIDGET_DIAL) ||
        (w == WIDGET_MDI_WINDOW_BUTTON &&
         opts.titlebarButtons & TITLEBAR_BUTTON_ROUND) || CIRCULAR_SLIDER(w)) {
        path.addEllipse(r);
        return path;
    }

    if (opts.round == ROUND_NONE || radius < 0.01) {
        round = ROUNDED_NONE;
    }

    double diameter = radius * 2;

    if (w != WIDGET_MDI_WINDOW_TITLE && round & CORNER_BR) {
        path.moveTo(r.x() + r.width(), r.y() + r.height() - radius);
    } else {
        path.moveTo(r.x() + r.width(), r.y() + r.height());
    }
    if (round & CORNER_TR) {
        path.arcTo(r.x() + r.width() - diameter, r.y(),
                   diameter, diameter, 0, 90);
    } else {
        path.lineTo(r.x() + r.width(), r.y());
    }
    if (round & CORNER_TL) {
        path.arcTo(r.x(), r.y(), diameter, diameter, 90, 90);
    } else {
        path.lineTo(r.x(), r.y());
    }
    if (w != WIDGET_MDI_WINDOW_TITLE && round & CORNER_BL) {
        path.arcTo(r.x(), r.y() + r.height() - diameter,
                   diameter, diameter, 180, 90);
    } else {
        path.lineTo(r.x(), r.y() + r.height());
    }

    if (w != WIDGET_MDI_WINDOW_TITLE) {
        if (round & CORNER_BR) {
            path.arcTo(r.x() + r.width() - diameter,
                       r.y() + r.height() - diameter,
                       diameter, diameter, 270, 90);
        } else {
            path.lineTo(r.x() + r.width(), r.y() + r.height());
        }
    }
    return path;
}

QPainterPath
Style::buildPath(const QRect &r, EWidget w, int round, double radius) const
{
    return buildPath(QRectF(r.x() + 0.5, r.y() + 0.5,
                            r.width() - 1, r.height() - 1), w, round, radius);
}

void
Style::buildSplitPath(const QRect &r, int round, double radius,
                      QPainterPath &tl, QPainterPath &br) const
{
    double xd = r.x() + 0.5;
    double yd = r.y() + 0.5;
    double diameter = radius * 2;
    bool rounded = diameter > 0.0;
    int width = r.width() - 1;
    int height = r.height() - 1;

    if (rounded && round & CORNER_TR) {
        tl.arcMoveTo(xd + width - diameter, yd, diameter, diameter, 45);
        tl.arcTo(xd + width - diameter, yd, diameter, diameter, 45, 45);
        if (width > diameter) {
            tl.lineTo(xd + width - diameter, yd);
        }
    } else {
        tl.moveTo(xd + width, yd);
    }

    if (rounded && round & CORNER_TL) {
        tl.arcTo(xd, yd, diameter, diameter, 90, 90);
    } else {
        tl.lineTo(xd, yd);
    }

    if (rounded && round & CORNER_BL) {
        tl.arcTo(xd, yd + height - diameter, diameter, diameter, 180, 45);
        br.arcMoveTo(xd, yd + height - diameter, diameter, diameter, 180 + 45);
        br.arcTo(xd, yd + height - diameter, diameter, diameter, 180 + 45, 45);
    } else {
        tl.lineTo(xd, yd + height);
        br.moveTo(xd, yd + height);
    }

    if (rounded && round & CORNER_BR) {
        br.arcTo(xd + width - diameter, yd + height - diameter, diameter,
                 diameter, 270, 90);
    } else {
        br.lineTo(xd + width, yd + height);
    }

    if (rounded && round & CORNER_TR) {
        br.arcTo(xd + width - diameter, yd, diameter, diameter, 0, 45);
    } else {
        br.lineTo(xd + width, yd);
    }
}

void
Style::drawBorder(QPainter *p, const QRect &r, const QStyleOption *option,
                  int round, const QColor *custom, EWidget w,
                  EBorder borderProfile, bool doBlend, int borderVal) const
{
    if(ROUND_NONE==opts.round)
        round=ROUNDED_NONE;

    State        state(option->state);
    bool         enabled(state&State_Enabled),
        entry(WIDGET_ENTRY==w || (WIDGET_SCROLLVIEW==w && opts.highlightScrollViews)),
        hasFocus(enabled && entry && state&State_HasFocus),
        hasMouseOver(enabled && entry && state & State_MouseOver &&
                     opts.unifyCombo && opts.unifySpin);
    const QColor *cols(enabled && hasMouseOver && opts.coloredMouseOver && entry
                       ? m_mouseOverCols
                       : enabled && hasFocus && entry
                       ? m_focusCols
                       : custom
                       ? custom
                       : APP_KRUNNER==theThemedApp ? m_backgroundCols : backgroundColors(option));
    QColor       border(WIDGET_DEF_BUTTON==w && IND_FONT_COLOR==opts.defBtnIndicator && enabled
                        ? option->palette.buttonText().color()
                        : cols[WIDGET_PROGRESSBAR==w
                               ? PBAR_BORDER
                               : !enabled && (WIDGET_BUTTON(w) || WIDGET_SLIDER_TROUGH==w)
                               ? QTC_STD_BORDER
                               : m_mouseOverCols==cols && IS_SLIDER(w)
                               ? SLIDER_MO_BORDER_VAL
                               : borderVal]);

    p->setRenderHint(QPainter::Antialiasing, true);
    p->setBrush(Qt::NoBrush);

    if(WIDGET_TAB_BOT==w || WIDGET_TAB_TOP==w)
        cols=m_backgroundCols;

    if(!(opts.thin&THIN_FRAMES) && (WIDGET_SCROLLVIEW!=w || !(opts.square&SQUARE_SCROLLVIEW) || opts.highlightScrollViews))
        switch(borderProfile)
        {
        case BORDER_FLAT:
            break;
        case BORDER_RAISED:
        case BORDER_SUNKEN:
        case BORDER_LIGHT:
        {
            int          dark=FRAME_DARK_SHADOW;
            QColor       tl(cols[BORDER_RAISED==borderProfile || BORDER_LIGHT==borderProfile ? 0 : dark]),
                br(cols[BORDER_RAISED==borderProfile ? dark : 0]);
            QPainterPath topPath,
                botPath;

            if( ((hasMouseOver || hasFocus) && WIDGET_ENTRY==w) ||
                (hasFocus && WIDGET_SCROLLVIEW==w))
            {
                tl.setAlphaF(ENTRY_INNER_ALPHA);
                br.setAlphaF(ENTRY_INNER_ALPHA);
            }
            else if(doBlend)
            {
                tl.setAlphaF(BORDER_BLEND_ALPHA(w));
                br.setAlphaF(BORDER_SUNKEN==borderProfile ? 0.0 : BORDER_BLEND_ALPHA(w));
            }

            QRect inner(r.adjusted(1, 1, -1, -1));

            buildSplitPath(inner, round, qtcGetRadius(&opts, inner.width(), inner.height(), w, RADIUS_INTERNAL), topPath, botPath);

            setPainterPen(p, (enabled || BORDER_SUNKEN==borderProfile) /*&&
                                                                  (BORDER_RAISED==borderProfile || BORDER_LIGHT==borderProfile || hasFocus || APPEARANCE_FLAT!=app)*/
                      ? tl
                      : option->palette.window().color(), QPENWIDTH1);
            p->drawPath(topPath);
            if(WIDGET_SCROLLVIEW==w || // Because of list view headers, need to draw dark line on right!
               (! ( (WIDGET_ENTRY==w && !hasFocus && !hasMouseOver) ||
                    (WIDGET_ENTRY!=w && doBlend && BORDER_SUNKEN==borderProfile) ) ) )
            {
                if(!hasFocus && !hasMouseOver && BORDER_LIGHT!=borderProfile && WIDGET_SCROLLVIEW!=w)
                    setPainterPen(p, /*WIDGET_SCROLLVIEW==w && !hasFocus
                                ? checkColour(option, QPalette::Window)
                                : WIDGET_ENTRY==w && !hasFocus
                                ? checkColour(option, QPalette::Base)
                                : */enabled && (BORDER_SUNKEN==borderProfile || hasFocus || /*APPEARANCE_FLAT!=app ||*/
                                                WIDGET_TAB_TOP==w || WIDGET_TAB_BOT==w)
                                ? br
                                : checkColour(option, QPalette::Window), QPENWIDTH1);
                p->drawPath(botPath);
            }
        }
        }

    if(BORDER_SUNKEN==borderProfile &&
       (WIDGET_FRAME==w || ((WIDGET_ENTRY==w || WIDGET_SCROLLVIEW==w) && !opts.etchEntry && !hasFocus && !hasMouseOver)))
    {
        QPainterPath topPath,
            botPath;
        QColor       col(border);

        col.setAlphaF(LOWER_BORDER_ALPHA);
        buildSplitPath(r, round, qtcGetRadius(&opts, r.width(), r.height(), w, RADIUS_EXTERNAL), topPath, botPath);
        p->setPen(QPen(/*enabled ? */border/* : col*/, QPENWIDTH1));
        p->drawPath(topPath);
//         if(enabled)
        p->setPen(col);
        p->drawPath(botPath);
    }
    else
    {
        p->setPen(QPen(border, QPENWIDTH1));
        p->drawPath(buildPath(r, w, round, qtcGetRadius(&opts, r.width(), r.height(), w, RADIUS_EXTERNAL)));
    }

    QPAINTER_RENDERHINT_AA_MAYBE_OFF(p);
}

void Style::drawMdiControl(QPainter *p, const QStyleOptionTitleBar *titleBar, SubControl sc, const QWidget *widget,
                           ETitleBarButtons btn, const QColor &iconColor, const QColor *btnCols, const QColor *bgndCols,
                           int adjust, bool activeWindow) const
{
    bool hover((titleBar->activeSubControls&sc) && (titleBar->state&State_MouseOver));

    if(!activeWindow && !hover && opts.titlebarButtons&TITLEBAR_BUTTOM_HIDE_ON_INACTIVE_WINDOW)
        return;

    QRect rect(subControlRect(CC_TitleBar, titleBar, sc, widget));

    if (rect.isValid())
    {
        rect.adjust(adjust, adjust, -adjust, -adjust);

        bool sunken((titleBar->activeSubControls&sc) && (titleBar->state&State_Sunken)),
            colored(coloredMdiButtons(titleBar->state&State_Active, hover)),
            useBtnCols(opts.titlebarButtons&TITLEBAR_BUTTON_STD_COLOR &&
                       (hover ||
                        !(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR_MOUSE_OVER) ||
                        opts.titlebarButtons&TITLEBAR_BUTTON_COLOR));
        const QColor *buttonColors=colored && !(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR_SYMBOL)
            ? m_titleBarButtonsCols[btn] : (useBtnCols ? btnCols : bgndCols);
        const QColor &icnColor=opts.titlebarButtons&TITLEBAR_BUTTON_ICON_COLOR
            ? opts.titlebarButtonColors[btn+(NUM_TITLEBAR_BUTTONS*(titleBar->state&State_Active ? 1 : 2))]
            : colored && opts.titlebarButtons&TITLEBAR_BUTTON_COLOR_SYMBOL
            ? m_titleBarButtonsCols[btn][ORIGINAL_SHADE]
            : SC_TitleBarCloseButton==sc && hover && !sunken && !(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR)
            ? CLOSE_COLOR
            : SC_TitleBarCloseButton!=sc && hover && !sunken &&
            !(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR) &&
            opts.titlebarButtons&TITLEBAR_BUTTON_USE_HOVER_COLOR
            ? m_mouseOverCols[ORIGINAL_SHADE]
            : iconColor;

        bool drewFrame=drawMdiButton(p, rect, hover, sunken, buttonColors);
        drawMdiIcon(p, icnColor, (drewFrame ? buttonColors : bgndCols)[ORIGINAL_SHADE],
                    rect, hover, sunken, subControlToIcon(sc), true, drewFrame);
    }
}

void Style::drawDwtControl(QPainter *p, const State &state, const QRect &rect, ETitleBarButtons btn, Icon icon,
                           const QColor &iconColor, const QColor *btnCols, const QColor *bgndCols) const
{
    bool    sunken(state&State_Sunken),
        hover(state&State_MouseOver),
        colored(coloredMdiButtons(state&State_Active, hover)),
        useBtnCols(opts.titlebarButtons&TITLEBAR_BUTTON_STD_COLOR &&
                   (hover ||
                    !(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR_MOUSE_OVER) ||
                    opts.titlebarButtons&TITLEBAR_BUTTON_COLOR));
    const QColor *buttonColors=colored && !(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR_SYMBOL)
        ? m_titleBarButtonsCols[btn] : (useBtnCols ? btnCols : bgndCols);
    const QColor &icnColor=opts.dwtSettings&DWT_ICON_COLOR_AS_PER_TITLEBAR && opts.titlebarButtons&TITLEBAR_BUTTON_ICON_COLOR
        ? opts.titlebarButtonColors[btn+(NUM_TITLEBAR_BUTTONS/**(titleBar->state&State_Active ? 1 : 2)*/)]
        : colored && opts.titlebarButtons&TITLEBAR_BUTTON_COLOR_SYMBOL
        ? m_titleBarButtonsCols[btn][ORIGINAL_SHADE]
        : (TITLEBAR_CLOSE==btn && !(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR) && (hover || sunken)
           ? CLOSE_COLOR
           : iconColor);

    bool drewFrame=drawMdiButton(p, rect, hover, sunken, buttonColors);
    drawMdiIcon(p, icnColor, (drewFrame ? buttonColors : bgndCols)[ORIGINAL_SHADE], rect, hover, sunken, icon, false, drewFrame);
}

bool Style::drawMdiButton(QPainter *painter, const QRect &r, bool hover, bool sunken, const QColor *cols) const
{
    if(!(opts.titlebarButtons&TITLEBAR_BUTTON_NO_FRAME) &&
       (hover || sunken || !(opts.titlebarButtons&TITLEBAR_BUTTON_HOVER_FRAME)))
    {
        QStyleOption opt;

        opt.rect=r; // .adjusted(1, 1, -1, -1);
        if(opts.titlebarButtons&TITLEBAR_BUTTON_ROUND)
            opt.rect.adjust(1, 1, -1, -1);
        opt.state=State_Enabled|State_Horizontal|State_Raised;
        if(hover)
            opt.state|=State_MouseOver;
        if(sunken)
            opt.state|=State_Sunken;

        drawLightBevel(painter, opt.rect, &opt, 0L, ROUNDED_ALL, getFill(&opt, cols), cols, true, WIDGET_MDI_WINDOW_BUTTON);
        return true;
    }

    return false;
}

void Style::drawMdiIcon(QPainter *painter, const QColor &color, const QColor &bgnd,  const QRect &r, bool hover, bool sunken, Icon icon,
                        bool stdSize, bool drewFrame) const
{
    if(!(opts.titlebarButtons&TITLEBAR_BUTTON_HOVER_SYMBOL_FULL) || hover || sunken)
    {
        bool faded=!sunken && !hover && opts.titlebarButtons&TITLEBAR_BUTTON_HOVER_SYMBOL;

        if(!sunken && !faded && EFFECT_NONE!=opts.titlebarEffect)
            //         // && hover && !(opts.titlebarButtons&TITLEBAR_BUTTON_HOVER_SYMBOL) && !customCol)
        {
            EEffect effect=opts.titlebarEffect;

            if(EFFECT_ETCH==opts.titlebarEffect && drewFrame)
                effect=EFFECT_SHADOW;

            drawIcon(painter, blendColors(WINDOW_SHADOW_COLOR(effect), bgnd, WINDOW_TEXT_SHADOW_ALPHA(effect)),
                     EFFECT_SHADOW==effect
                     ? r.adjusted(1, 1, 1, 1)
                     : r.adjusted(0, 1, 0, 1),
                     sunken, icon, stdSize);
        }

        QColor col(color);

        if(faded)
            col=blendColors(col, bgnd, HOVER_BUTTON_ALPHA(col));

        drawIcon(painter, col, r, sunken, icon, stdSize);
    }
}

void Style::drawIcon(QPainter *painter, const QColor &color, const QRect &r, bool sunken, Icon icon, bool stdSize) const
{
    static const int constIconSize=9;
    static const int constSmallIconSize=7;

    painter->setPen(color);

    QSize    iconSize(stdSize
                      ? constIconSize
                      : constSmallIconSize,
                      stdSize
                      ? constIconSize
                      : (ICN_RESTORE==icon && !(opts.titlebarButtons&TITLEBAR_BUTTOM_ARROW_MIN_MAX)
                         ? constSmallIconSize+1
                         : constSmallIconSize));
    QRect    br(r.x()+((r.width()-iconSize.width())>>1),
                r.y()+((r.height()-iconSize.height())>>1),
                iconSize.width(), iconSize.height());
    if(sunken)
        br.adjust(1, 1, 1, 1);

    switch(icon)
    {
    case ICN_MIN:
        if(opts.titlebarButtons&TITLEBAR_BUTTOM_ARROW_MIN_MAX)
            drawArrow(painter, opts.vArrows ? br.adjusted(0, 1, 0, 1) : br, PE_IndicatorArrowDown, color, false);
        else
            drawRect(painter, QRect(br.left(), br.bottom()-1, br.width(), 2));
        break;
    case ICN_MAX:
        if(opts.titlebarButtons&TITLEBAR_BUTTOM_ARROW_MIN_MAX)
            drawArrow(painter, opts.vArrows ? br.adjusted(0, -1, 0, -1) : br, PE_IndicatorArrowUp, color, false);
        else
        {
            drawRect(painter, br);
            painter->drawLine(br.left() + 1, br.top() + 1,  br.right() - 1, br.top() + 1);
        }
        break;
    case ICN_CLOSE:
        if(stdSize && opts.titlebarButtons&TITLEBAR_BUTTON_SUNKEN_BACKGROUND)
            br.adjust(1, 1, -1, -1);
        painter->save();
        painter->setClipRect(br);
        painter->setPen(QPen(color, 2));
        painter->drawLine(br.left(), br.top(), br.right(), br.bottom());
        painter->drawLine(br.right(), br.top(), br.left(), br.bottom());
        painter->restore();
        break;
    case ICN_RESTORE:
        if(opts.titlebarButtons&TITLEBAR_BUTTOM_ARROW_MIN_MAX)
        {
            painter->drawLine(br.x()+1, br.y(), br.x()+br.width()-2, br.y());
            painter->drawLine(br.x()+1, br.y()+br.height()-1, br.x()+br.width()-2, br.y()+br.height()-1);
            painter->drawLine(br.x(), br.y()+1, br.x(), br.y()+br.height()-2);
            painter->drawLine(br.x()+br.width()-1, br.y()+1, br.x()+br.width()-1, br.y()+br.height()-2);
            drawRect(painter, br.adjusted(1, 1, -1, -1));
        }
        else
        {
            drawRect(painter, QRect(br.x(), br.y()+3, br.width()-2, br.height()-3));
            painter->drawLine(br.x()+1, br.y()+4, br.x()+br.width()-4, br.y()+4);
            painter->drawLine(br.x()+2, br.y(), br.x()+br.width()-1, br.y());
            painter->drawLine(br.x()+2, br.y()+1, br.x()+br.width()-1, br.y()+1);
            painter->drawLine(br.x()+br.width()-1, br.y()+2, br.x()+br.width()-1, br.y()+(stdSize ? 5 : 4));
            painter->drawPoint(br.x()+br.width()-2, br.y()+(stdSize ? 5 : 4));
            painter->drawPoint(br.x()+2, br.y()+2);
        }
        break;
    case ICN_UP:
        drawArrow(painter, br, PE_IndicatorArrowUp, color, false);
        break;
    case ICN_DOWN:
        drawArrow(painter, opts.vArrows ? br.adjusted(0, 1, 0, 1) : br, PE_IndicatorArrowDown, color, false);
        break;
    case ICN_RIGHT:
        drawArrow(painter, br, PE_IndicatorArrowRight, color, false);
        break;
    case ICN_MENU:
        for(int i=1; i<=constIconSize; i+=3)
            painter->drawLine(br.left() + 1, br.top() + i,  br.right() - 1, br.top() + i);
        break;
    case ICN_SHADE:
    case ICN_UNSHADE:
    {
        bool isDwt=opts.dwtSettings&DWT_BUTTONS_AS_PER_TITLEBAR;
        br.adjust(0, -2, 0, 0);
        drawRect(painter, isDwt ? QRect(br.left(), br.bottom(), br.width(), 2) : QRect(br.left()+1, br.bottom()-1, br.width()-2, 2));
        br.adjust(0, ICN_SHADE==icon ? -3 : -5, 0, 0);
        drawArrow(painter, opts.vArrows ? br.adjusted(0, 1, 0, 1) : br,
                  ICN_SHADE==icon ? PE_IndicatorArrowDown : PE_IndicatorArrowUp, color, false);
        break;
    }
    default:
        break;
    }
}

void Style::drawEntryField(QPainter *p, const QRect &rx,  const QWidget *widget, const QStyleOption *option,
                           int round, bool fill, bool doEtch, EWidget w) const
{
    QRect r(rx);

    if(doEtch && opts.etchEntry)
        r.adjust(1, 1, -1, -1);

    p->setRenderHint(QPainter::Antialiasing, true);
    if(fill)
        p->fillPath(buildPath(QRectF(r).adjusted(1, 1, -1, -1), WIDGET_SCROLLVIEW==w ? w : WIDGET_ENTRY, round,
                              qtcGetRadius(&opts, r.width()-2, r.height()-2, WIDGET_SCROLLVIEW==w ? w : WIDGET_ENTRY, RADIUS_INTERNAL)),
                    option->palette.brush(QPalette::Base));
    else
    {
        p->setPen(WIDGET_SCROLLVIEW!=w || !(opts.square&SQUARE_SCROLLVIEW) || opts.highlightScrollViews ? checkColour(option, QPalette::Base)
                  : backgroundColors(option)[ORIGINAL_SHADE]);
        p->drawPath(buildPath(r.adjusted(1, 1, -1, -1), WIDGET_SCROLLVIEW==w ? w : WIDGET_ENTRY, round,
                              qtcGetRadius(&opts, r.width()-2, r.height()-2, WIDGET_SCROLLVIEW==w ? w : WIDGET_ENTRY, RADIUS_INTERNAL)));
    }
    QPAINTER_RENDERHINT_AA_MAYBE_OFF(p);

    if(doEtch && opts.etchEntry)
        drawEtch(p, rx, widget, WIDGET_SCROLLVIEW==w ? w : WIDGET_ENTRY, false);

    drawBorder(p, r, option, round, 0L, w, BORDER_SUNKEN);
}

void Style::drawMenuItem(QPainter *p, const QRect &r, const QStyleOption *option, MenuItemType type, int round, const QColor *cols) const
{
    QColor altCols[TOTAL_SHADES+1];
    if (theThemedApp == APP_KWIN) {
        QWidget *w = qobject_cast<QWidget*>(option->styleObject);
        QPalette::ColorRole role = (opts.useHighlightForMenu || (option->state & (State_On|State_Sunken))) ?
            QPalette::HighlightedText : QPalette::WindowText;
        if (w && w->palette().color(QPalette::Active, role)
                != QApplication::palette().color(QPalette::Active, role)) {
//             qWarning() << "drawMenuItem: widget" << w << "bgCol" << w->palette().color(QPalette::Active, role)
//                 << "differs from app bgCol" << QApplication::palette().color(QPalette::Active, role);
            shadeColors(w->palette().color(QPalette::Active, role), altCols);
            cols = altCols;
        }
    }

    int fill=opts.useHighlightForMenu && ((MENU_BAR!=type) || m_highlightCols==cols || APP_OPENOFFICE==theThemedApp) ? ORIGINAL_SHADE : 4,
        border=opts.borderMenuitems ? 0 : fill;

    if(m_highlightCols!=cols && MENU_BAR==type && !(option->state&(State_On|State_Sunken)) &&
       !opts.colorMenubarMouseOver && (opts.borderMenuitems || !qtcIsFlat(opts.menuitemAppearance)))
        fill=ORIGINAL_SHADE;

    if(MENU_BAR!=type && APPEARANCE_FADE==opts.menuitemAppearance)
    {
        bool reverse = Qt::RightToLeft==option->direction;
        QColor trans(Qt::white);
        QRect r2(opts.round != ROUND_NONE ? r.adjusted(1, 1, -1, -1) : r);
        QRectF rf(r2);
        double fadePercent = ((double)MENUITEM_FADE_SIZE)/rf.width();
        QLinearGradient grad(r2.topLeft(), r2.topRight());

        trans.setAlphaF(0.0);
        grad.setColorAt(0, reverse ? trans : cols[fill]);
        grad.setColorAt(reverse ? fadePercent : 1.0-fadePercent, cols[fill]);
        grad.setColorAt(1, reverse ? cols[fill] : trans);
        if (opts.round != ROUND_NONE) {
            p->save();
            p->setRenderHint(QPainter::Antialiasing, true);
            p->fillPath(buildPath(rf, WIDGET_OTHER,
                                  reverse ? ROUNDED_RIGHT : ROUNDED_LEFT, 4),
                        QBrush(grad));
            p->restore();
        } else {
            p->fillRect(r2, QBrush(grad));
        }
    } else if (MENU_BAR==type || opts.borderMenuitems) {
        bool stdColor(MENU_BAR!=type || (SHADE_BLEND_SELECTED!=opts.shadeMenubars && SHADE_SELECTED!=opts.shadeMenubars));

        QStyleOption opt(*option);

        opt.state|=State_Horizontal|State_Raised;
        opt.state&=~(State_Sunken|State_On);

        if(stdColor && opts.borderMenuitems)
            drawLightBevel(p, r, &opt, 0L, round, cols[fill], cols, stdColor, WIDGET_MENU_ITEM);
        else
        {
            QRect fr(r);

            fr.adjust(1, 1, -1, -1);

            if(fr.width()>0 && fr.height()>0)
                drawBevelGradient(cols[fill], p, fr, true, false, opts.menuitemAppearance, WIDGET_MENU_ITEM);
            drawBorder(p, r, &opt, round, cols, WIDGET_MENU_ITEM, BORDER_FLAT, false, border);
        }
    }
    else
    {
        // For now dont round combos - getting weird effects with shadow/clipping in Gtk2 style :-(
        if (/*MENU_COMBO==type || */opts.square & SQUARE_POPUP_MENUS) {
            drawBevelGradient(cols[fill], p, r, true, false,
                              opts.menuitemAppearance, WIDGET_MENU_ITEM);
        } else {
            p->save();
            p->setRenderHint(QPainter::Antialiasing, true);
            drawBevelGradient(
                cols[fill], p, r,
                buildPath(QRectF(r), WIDGET_OTHER, ROUNDED_ALL,
                          (opts.round >= ROUND_FULL ? 5.0 : 2.5) -
                          (opts.round > ROUND_SLIGHT ? 1.0 : 0.5)), true, false,
                opts.menuitemAppearance, WIDGET_MENU_ITEM, false);
            p->restore();
        }
    }
}

void Style::drawProgress(QPainter *p, const QRect &r, const QStyleOption *option, bool vertical, bool reverse) const
{
    QStyleOption opt(*option);
    QRect        rx(r);

    opt.state|=State_Raised;

    if(vertical)
        opt.state&=~State_Horizontal;
    else
        opt.state|=State_Horizontal;

    if(reverse)
        opt.state|=STATE_REVERSE;
    else
        opt.state&=~STATE_REVERSE;

    if((vertical ? r.height() : r.width())<1)
        return;

    if(vertical && r.height()<3)
        rx.setHeight(3);

    if(!vertical && rx.width()<3)
        rx.setWidth(3);

    // KTorrent's progressbars seem to have state==State_None
    const QColor *use=option->state&State_Enabled || State_None==option->state || ECOLOR_BACKGROUND==opts.progressGrooveColor
        ? m_progressCols
        ? m_progressCols
        : highlightColors(option, true)
        : m_backgroundCols;

    drawLightBevel(p, rx, &opt, 0L, ROUNDED_ALL, use[ORIGINAL_SHADE], use, opts.borderProgress, WIDGET_PROGRESSBAR);

    if(opts.glowProgress && (vertical ? rx.height() : rx.width())>3)
    {
        QRect           ri(opts.borderProgress ? rx.adjusted(1, 1, -1, -1) : rx);
        QLinearGradient grad(0, 0, vertical ? 0 : 1, vertical ? 1 : 0);
        QColor          glow(Qt::white),
            blank(Qt::white);

        blank.setAlphaF(0);
        glow.setAlphaF(GLOW_PROG_ALPHA);
        grad.setCoordinateMode(QGradient::ObjectBoundingMode);
        grad.setColorAt(0, (reverse ? GLOW_END : GLOW_START)==opts.glowProgress ? glow : blank);
        if(GLOW_MIDDLE==opts.glowProgress)
            grad.setColorAt(0.5, glow);
        grad.setColorAt(1, (reverse ? GLOW_START : GLOW_END)==opts.glowProgress ? glow : blank);
        p->fillRect(ri, grad);
    }

    if(!opts.borderProgress)
    {
        p->setPen(use[PBAR_BORDER]);
        if(!vertical)
        {
            p->drawLine(rx.topLeft(), rx.topRight());
            p->drawLine(rx.bottomLeft(), rx.bottomRight());
        }
        else
        {
            p->drawLine(rx.topLeft(), rx.bottomLeft());
            p->drawLine(rx.topRight(), rx.bottomRight());
        }
    }
}

static QPolygon rotate(const QPolygon &p, double angle)
{
    QTransform transform;
    transform.rotate(angle);
    return transform.map(p);
}

void
Style::drawArrow(QPainter *p, const QRect &rx, PrimitiveElement pe,
                 QColor col, bool small, bool kwin) const
{
    QPolygon a;
    QRect r(rx);
    int m = !small && kwin ? ((r.height() - 7) / 2) : 0;

    if(small)
        a.setPoints(opts.vArrows ? 6 : 3,  2,0,  0,-2,  -2,0,   -2,1, 0,-1, 2,1);
    else
        a.setPoints(opts.vArrows ? 8 : 3,  3+m,1+m,  0,-2,  -(3+m),1+m,    -(3+m),2+m,  -(2+m),2+m, 0,0,  2+m,2+m, 3+m,2+m);

    switch (pe) {
    case PE_IndicatorArrowUp:
        if(m)
            r.adjust(0, -m, 0, -m);
        break;
    case PE_IndicatorArrowDown:
        if(m)
            r.adjust(0, m, 0, m);
        a=rotate(a, 180);
        break;
    case PE_IndicatorArrowRight:
        a=rotate(a, 90);
        break;
    case PE_IndicatorArrowLeft:
        a=rotate(a, 270);
        break;
    default:
        return;
    }
    a.translate((r.x()+(r.width()>>1)), (r.y()+(r.height()>>1)));
    p->save();
    col.setAlpha(255);
    p->setPen(col);
    p->setBrush(col);
    // Qt6 changes how coordinates is calculate for non-AA drawing.
    // use QPainter::Qt4CompatiblePainting for now.
    // A better solution should be shifting the coordinates by 0.5
    // or maybe use QPainterPath
    p->setRenderHint(QPainter::Qt4CompatiblePainting, true);
    // Qt >= 4.8.5 has problem drawing polygons correctly. Enabling
    // antialiasing can work around the problem although it will also make
    // the arrow blurry.
    // QtCurve issue:
    // https://github.com/QtCurve/qtcurve-qt4/issues/3.
    // Upstream bug report:
    // https://bugreports.qt-project.org/browse/QTBUG-33512
    p->setRenderHint(QPainter::Antialiasing, opts.vArrows);
    p->drawPolygon(a);
    p->restore();
}

void Style::drawSbSliderHandle(QPainter *p, const QRect &rOrig, const QStyleOption *option, bool slider) const
{
    QStyleOption opt(*option);
    QRect        r(rOrig);

    if(opt.state&(State_Sunken|State_On))
        opt.state|=State_MouseOver;

    if(r.width()>r.height())
        opt.state|=State_Horizontal;

    opt.state &= ~(State_Sunken | State_On);
    opt.state |= State_Raised;

    if (auto slider = styleOptCast<QStyleOptionSlider>(option)) {
        if (slider->minimum == slider->maximum) {
            opt.state &= ~(State_MouseOver | State_Enabled);
        }
    }

    int          min(MIN_SLIDER_SIZE(opts.sliderThumbs));
    const QColor *use(sliderColors(&opt));

    drawLightBevel(p, r, &opt, 0L, (slider && (!(opts.square&SQUARE_SLIDER) ||
                                               (SLIDER_ROUND==opts.sliderStyle || SLIDER_ROUND_ROTATED==opts.sliderStyle)))
#ifndef SIMPLE_SCROLLBARS
                   || (!slider && !(opts.square&SQUARE_SB_SLIDER) && (SCROLLBAR_NONE==opts.scrollbarType || opts.flatSbarButtons))
#endif
                   ? ROUNDED_ALL : ROUNDED_NONE,
                   getFill(&opt, use, false, SHADE_DARKEN==opts.shadeSliders), use, true,
                   slider ? WIDGET_SLIDER : WIDGET_SB_SLIDER);

    if(LINE_NONE!=opts.sliderThumbs && (slider || ((opt.state&State_Horizontal && r.width()>=min)|| r.height()>=min)) &&
       (!slider || SLIDER_CIRCULAR!=opts.sliderStyle))
    {
        const QColor *markers(use);
        bool         horiz(opt.state&State_Horizontal);

        if(LINE_SUNKEN==opts.sliderThumbs)
            if(horiz)
                r.adjust(0, -1, 0, 0);
            else
                r.adjust(-1, 0, 0, 0);
        else
            r.adjust(horiz ? 1 : 0, horiz ? 0 : 1, 0, 0);

        switch(opts.sliderThumbs)
        {
        case LINE_1DOT:
            p->drawPixmap(r.x()+((r.width()-5)/2), r.y()+((r.height()-5)/2), *getPixmap(markers[QTC_STD_BORDER], PIX_DOT, 1.0));
            break;
        case LINE_FLAT:
            drawLines(p, r, !horiz, 3, 5, markers, 0, 5, opts.sliderThumbs);
            break;
        case LINE_SUNKEN:
            drawLines(p, r, !horiz, 4, 3, markers, 0, 3, opts.sliderThumbs);
            break;
        case LINE_DOTS:
        default:
            drawDots(p, r, !horiz, slider ? 3 : 5, slider ? 4 : 2, markers, 0, 5);
        }
    }
}

void Style::drawSliderHandle(QPainter *p, const QRect &r, const QStyleOptionSlider *option) const
{
    bool         horiz(SLIDER_TRIANGULAR==opts.sliderStyle ? r.height()>r.width() : r.width()>r.height());
    QStyleOption opt(*option);

    if(!(option->activeSubControls&SC_SliderHandle) || !(opt.state&State_Enabled))
        opt.state&=~State_MouseOver;

    if (opts.sliderStyle == SLIDER_TRIANGULAR) {
        if (r.width() > r.height()) {
            opt.state |= State_Horizontal;
        }
        opt.state &= ~(State_Sunken | State_On);
        opt.state |= State_Raised;
        const QColor *use = sliderColors(&opt);
        const QColor *border = (opt.state & State_MouseOver &&
                                oneOf(opts.coloredMouseOver, MO_GLOW,
                                      MO_COLORED) ? m_mouseOverCols : use);
        const QColor &fill(getFill(&opt, use, false,
                                   opts.shadeSliders == SHADE_DARKEN));
        int x = r.x();
        int y = r.y();
        PrimitiveElement direction(horiz ? PE_IndicatorArrowDown :
                                   PE_IndicatorArrowRight);
        QPolygon clipRegion;
        bool drawLight(MO_PLASTIK!=opts.coloredMouseOver || !(opt.state&State_MouseOver));
        int size(SLIDER_TRIANGULAR==opts.sliderStyle ? 15 : 13),
            borderVal(m_mouseOverCols==border ? SLIDER_MO_BORDER_VAL : BORDER_VAL(opt.state&State_Enabled));

        if(option->tickPosition & QSlider::TicksBelow)
            direction=horiz ? PE_IndicatorArrowDown : PE_IndicatorArrowRight;
        else if(option->tickPosition & QSlider::TicksAbove)
            direction=horiz ? PE_IndicatorArrowUp : PE_IndicatorArrowLeft;

        if (MO_GLOW==opts.coloredMouseOver && opts.buttonEffect != EFFECT_NONE)
            x++, y++;

        switch(direction)
        {
        default:
        case PE_IndicatorArrowDown:
            clipRegion.setPoints(7,   x, y+2,    x+2, y,   x+8, y,    x+10, y+2,   x+10, y+9,   x+5, y+14,    x, y+9);
            break;
        case PE_IndicatorArrowUp:
            clipRegion.setPoints(7,   x, y+12,   x+2, y+14,   x+8, y+14,   x+10, y+12,   x+10, y+5,   x+5, y,    x, y+5);
            break;
        case PE_IndicatorArrowLeft:
            clipRegion.setPoints(7,   x+12, y,   x+14, y+2,   x+14, y+8,   x+12, y+10,   x+5, y+10,    x, y+5,    x+5, y );
            break;
        case PE_IndicatorArrowRight:
            clipRegion.setPoints(7,   x+2, y,    x, y+2,   x, y+8,    x+2, y+10,   x+9, y+10,   x+14, y+5,    x+9, y);
        }

        p->save();
        p->setClipRegion(QRegion(clipRegion)); // , QPainter::CoordPainter);
        if(qtcIsFlat(opts.sliderAppearance))
        {
            p->fillRect(r, fill);

            if(MO_PLASTIK==opts.coloredMouseOver && opt.state&State_MouseOver && !opts.colorSliderMouseOver)
            {
                int col(SLIDER_MO_SHADE),
                    len(SLIDER_MO_LEN);

                if(horiz)
                {
                    p->fillRect(QRect(x+1, y+1, len, size-2), m_mouseOverCols[col]);
                    p->fillRect(QRect(x+r.width()-(1+len), y+1, len, r.height()-2), m_mouseOverCols[col]);
                }
                else
                {
                    p->fillRect(QRect(x+1, y+1, size-2, len), m_mouseOverCols[col]);
                    p->fillRect(QRect(x+1, y+r.height()-(1+len), r.width()-2, len), m_mouseOverCols[col]);
                }
            }
        }
        else
        {
            drawBevelGradient(fill, p, QRect(x, y, horiz ? r.width()-1 : size, horiz ? size : r.height()-1),
                              horiz, false, MODIFY_AGUA(opts.sliderAppearance));

            if(MO_PLASTIK==opts.coloredMouseOver && opt.state&State_MouseOver && !opts.colorSliderMouseOver)
            {
                int col(SLIDER_MO_SHADE),
                    len(SLIDER_MO_LEN);

                if(horiz)
                {
                    drawBevelGradient(m_mouseOverCols[col], p, QRect(x+1, y+1, len, size-2),
                                      horiz, false, MODIFY_AGUA(opts.sliderAppearance));
                    drawBevelGradient(m_mouseOverCols[col], p,  QRect(x+r.width()-(1+len), y+1, len, size-2),
                                      horiz, false, MODIFY_AGUA(opts.sliderAppearance));
                }
                else
                {
                    drawBevelGradient(m_mouseOverCols[col], p, QRect(x+1, y+1, size-2, len),
                                      horiz, false, MODIFY_AGUA(opts.sliderAppearance));
                    drawBevelGradient(m_mouseOverCols[col], p,QRect(x+1, y+r.height()-(1+len), size-2, len),
                                      horiz, false, MODIFY_AGUA(opts.sliderAppearance));
                }
            }
        }

        p->restore();
        p->save();

        QPainterPath path;
        double       xd(x+0.5),
            yd(y+0.5),
            radius(2.5),
            diameter(radius*2),
            xdg(x-0.5),
            ydg(y-0.5),
            radiusg(radius+1),
            diameterg(radiusg*2);
        bool         glowMo(MO_GLOW==opts.coloredMouseOver && opt.state&State_MouseOver);
        QColor       glowCol(border[GLOW_MO]);

        glowCol.setAlphaF(GLOW_ALPHA(false));

        p->setPen(glowMo ? glowCol : border[borderVal]);

        switch(direction)
        {
        default:
        case PE_IndicatorArrowDown:
            p->setRenderHint(QPainter::Antialiasing, true);
            if(glowMo)
            {
                path.moveTo(xdg+12-radiusg, ydg);
                path.arcTo(xdg, ydg, diameterg, diameterg, 90, 90);
                path.lineTo(xdg, ydg+10.5);
                path.lineTo(xdg+6, ydg+16.5);
                path.lineTo(xdg+12, ydg+10.5);
                path.arcTo(xdg+12-diameterg, ydg, diameterg, diameterg, 0, 90);
                p->drawPath(path);
                path=QPainterPath();
                p->setPen(border[borderVal]);
            }
            path.moveTo(xd+10-radius, yd);
            path.arcTo(xd, yd, diameter, diameter, 90, 90);
            path.lineTo(xd, yd+9);
            path.lineTo(xd+5, yd+14);
            path.lineTo(xd+10, yd+9);
            path.arcTo(xd+10-diameter, yd, diameter, diameter, 0, 90);
            p->drawPath(path);
            QPAINTER_RENDERHINT_AA_MAYBE_OFF(p);
            if(drawLight)
            {
                p->setPen(use[APPEARANCE_DULL_GLASS==opts.sliderAppearance ? 1 : 0]);
                p->drawLine(x+1, y+2, x+1, y+8);
                p->drawLine(x+2, y+1, x+7, y+1);
            }
            break;
        case PE_IndicatorArrowUp:
            p->setRenderHint(QPainter::Antialiasing, true);
            if(glowMo)
            {
                path.moveTo(xdg, ydg+6);
                path.arcTo(xdg, ydg+16-diameterg, diameterg, diameterg, 180, 90);
                path.arcTo(xdg+12-diameterg, ydg+16-diameterg, diameterg, diameterg, 270, 90);
                path.lineTo(xdg+12, ydg+5.5);
                path.lineTo(xdg+6, ydg-0.5);
                path.lineTo(xdg, ydg+5.5);
                p->drawPath(path);
                path=QPainterPath();
                p->setPen(border[borderVal]);
            }
            path.moveTo(xd, yd+5);
            path.arcTo(xd, yd+14-diameter, diameter, diameter, 180, 90);
            path.arcTo(xd+10-diameter, yd+14-diameter, diameter, diameter, 270, 90);
            path.lineTo(xd+10, yd+5);
            path.lineTo(xd+5, yd);
            path.lineTo(xd, yd+5);
            p->drawPath(path);
            QPAINTER_RENDERHINT_AA_MAYBE_OFF(p);
            if(drawLight)
            {
                p->setPen(use[APPEARANCE_DULL_GLASS==opts.sliderAppearance ? 1 : 0]);
                p->drawLine(x+5, y+1, x+1, y+5);
                p->drawLine(x+1, y+5, x+1, y+11);
            }
            break;
        case PE_IndicatorArrowLeft:
            p->setRenderHint(QPainter::Antialiasing, true);
            if(glowMo)
            {
                path.moveTo(xdg+6, ydg+12);
                path.arcTo(xdg+16-diameterg, ydg+12-diameterg, diameterg, diameterg, 270, 90);
                path.arcTo(xdg+16-diameterg, ydg, diameterg, diameterg, 0, 90);
                path.lineTo(xdg+5.5, ydg);
                path.lineTo(xdg-0.5, ydg+6);
                path.lineTo(xdg+5.5, ydg+12);
                p->drawPath(path);
                path=QPainterPath();
                p->setPen(border[borderVal]);
            }
            path.moveTo(xd+5, yd+10);
            path.arcTo(xd+14-diameter, yd+10-diameter, diameter, diameter, 270, 90);
            path.arcTo(xd+14-diameter, yd, diameter, diameter, 0, 90);
            path.lineTo(xd+5, yd);
            path.lineTo(xd, yd+5);
            path.lineTo(xd+5, yd+10);
            p->drawPath(path);
            QPAINTER_RENDERHINT_AA_MAYBE_OFF(p);
            if(drawLight)
            {
                p->setPen(use[APPEARANCE_DULL_GLASS==opts.sliderAppearance ? 1 : 0]);
                p->drawLine(x+1, y+5, x+5, y+1);
                p->drawLine(x+5, y+1, x+11, y+1);
            }
            break;
        case PE_IndicatorArrowRight:
            p->setRenderHint(QPainter::Antialiasing, true);
            if(glowMo)
            {
                path.moveTo(xdg+11, ydg);
                path.arcTo(xdg, ydg, diameterg, diameterg, 90, 90);
                path.arcTo(xdg, ydg+12-diameterg, diameterg, diameterg, 180, 90);
                path.lineTo(xdg+10.5, ydg+12);
                path.lineTo(xdg+16.5, ydg+6);
                path.lineTo(xdg+10.5, ydg);
                p->drawPath(path);
                path=QPainterPath();
                p->setPen(border[borderVal]);
            }
            path.moveTo(xd+9, yd);
            path.arcTo(xd, yd, diameter, diameter, 90, 90);
            path.arcTo(xd, yd+10-diameter, diameter, diameter, 180, 90);
            path.lineTo(xd+9, yd+10);
            path.lineTo(xd+14, yd+5);
            path.lineTo(xd+9, yd);
            p->drawPath(path);
            QPAINTER_RENDERHINT_AA_MAYBE_OFF(p);
            if(drawLight)
            {
                p->setPen(use[APPEARANCE_DULL_GLASS==opts.sliderAppearance ? 1 : 0]);
                p->drawLine(x+2, y+1, x+7, y+1);
                p->drawLine(x+1, y+2, x+1, y+8);
            }
            break;
        }
        p->restore();
    } else {
        if (oneOf(opts.sliderStyle, SLIDER_PLAIN_ROTATED,
                  SLIDER_ROUND_ROTATED)) {
            opt.state ^= State_Horizontal;
        }
        drawSbSliderHandle(p, r, &opt, true);
    }
}

void Style::drawSliderGroove(QPainter *p, const QRect &groove, const QRect &handle,  const QStyleOptionSlider *slider,
                             const QWidget *widget) const
{
    bool               horiz(Qt::Horizontal==slider->orientation);
    QRect              grv(groove);
    QStyleOptionSlider opt(*slider);

    opt.state&=~(State_HasFocus|State_On|State_Sunken|State_MouseOver);

    if(horiz)
    {
        int dh=(grv.height()-5)>>1;
        grv.adjust(0, dh, 0, -dh);
        opt.state|=State_Horizontal;

        if (opts.buttonEffect != EFFECT_NONE)
            grv.adjust(0, -1, 0, 1);
    }
    else
    {
        int dw=(grv.width()-5)>>1;
        grv.adjust(dw, 0, -dw, 0);
        opt.state&=~State_Horizontal;

        if (opts.buttonEffect != EFFECT_NONE)
            grv.adjust(-1, 0, 1, 0);
    }

    if(grv.height()>0 && grv.width()>0)
    {
        drawLightBevel(p, grv, &opt, widget,
                       opts.square&SQUARE_SLIDER ? ROUNDED_NONE : ROUNDED_ALL,
                       m_backgroundCols[slider->state&State_Enabled ? 2 : ORIGINAL_SHADE],
                       m_backgroundCols, true, WIDGET_SLIDER_TROUGH);

        if(opts.fillSlider && slider->maximum!=slider->minimum && slider->state&State_Enabled)
        {
            const QColor *usedCols=m_sliderCols ? m_sliderCols : m_highlightCols;

            if (horiz)
                if (slider->upsideDown)
                    grv=QRect(handle.right()-4, grv.top(), (grv.right()-handle.right())+4, grv.height());
                else
                    grv=QRect(grv.left(), grv.top(), handle.left()+4, grv.height());
            else
                if (slider->upsideDown)
                    grv=QRect(grv.left(), handle.bottom()-4, grv.width(), (grv.height() - handle.bottom())+4);
                else
                    grv=QRect(grv.left(), grv.top(), grv.width(), (handle.top() - grv.top())+4);

            if(grv.height()>0 && grv.width()>0)
                drawLightBevel(p, grv, &opt, widget, opts.square&SQUARE_SLIDER ? ROUNDED_NONE : ROUNDED_ALL,
                               usedCols[ORIGINAL_SHADE], usedCols, true, WIDGET_FILLED_SLIDER_TROUGH);
        }
    }
}

void
Style::drawMenuOrToolBarBackground(const QWidget *widget, QPainter *p,
                                   const QRect &r, const QStyleOption *option,
                                   bool menu, bool horiz) const
{
    // LibreOffice - when drawMenuOrToolBarBackground is called with menuRect,
    // this is empty!
    if (r.width() < 1 || r.height() < 1)
        return;

    EAppearance app = menu ? opts.menubarAppearance : opts.toolbarAppearance;
    if (!qtcIsCustomBgnd(opts) || !qtcIsFlat(app) ||
        (menu && opts.shadeMenubars != SHADE_NONE)) {
        p->save();
#if 0
        // Revert for now
        // This is necessary for correct opacity on the menubar but may
        // break transparent gradient.
        p->setCompositionMode(QPainter::CompositionMode_Source);
#endif
        QRect rx(r);
#ifdef Q_OS_MACOS
        QColor col(menu ?
                   menuColors(option, m_active)[ORIGINAL_SHADE] :
                   option->palette.window().color());
#else
        QColor col(menu && (option->state & State_Enabled ||
                            opts.shadeMenubars != SHADE_NONE) ?
                   menuColors(option, m_active)[ORIGINAL_SHADE] :
                   option->palette.window().color());
#endif
        // TODO: QtQuick
        int opacity = qtcGetOpacity(widget ? widget : getWidget(p));

        if (menu && BLEND_TITLEBAR) {
            rx.adjust(0, -qtcGetWindowBorderSize(false).titleHeight, 0, 0);
        }
        if (opacity < 100) {
            col.setAlphaF(opacity / 100.0);
        }
        drawBevelGradient(col, p, rx, horiz, false, MODIFY_AGUA(app));
        p->restore();
    }
}

void Style::drawHandleMarkers(QPainter *p, const QRect &rx, const QStyleOption *option, bool tb, ELine handles) const
{
    if(rx.width()<2 || rx.height()<2)
        return;

    QRect r(rx);

    if(APP_OPENOFFICE==theThemedApp)
    {
        r.setX(r.x()+2);
        r.setWidth(10);
    }

    // CPD: Mouse over of toolbar handles not working - the whole toolbar seems to be active :-(
    QStyleOption opt(*option);

    opt.state&=~State_MouseOver;

    const QColor *border(borderColors(&opt, m_backgroundCols));

    switch(handles)
    {
    case LINE_NONE:
        break;
    case LINE_1DOT:
        p->drawPixmap(r.x()+((r.width()-5)/2), r.y()+((r.height()-5)/2), *getPixmap(border[QTC_STD_BORDER], PIX_DOT, 1.0));
        break;
    case LINE_DOTS:
        drawDots(p, r, !(option->state&State_Horizontal), 2, tb ? 5 : 3, border, tb ? -2 : 0, 5);
        break;
    case LINE_DASHES:
        if(option->state&State_Horizontal)
            drawLines(p, QRect(r.x()+(tb ? 2 : (r.width()-6)/2), r.y(), 3, r.height()), true, (r.height()-8)/2,
                      tb ? 0 : (r.width()-5)/2, border, 0, 5, handles);
        else
            drawLines(p, QRect(r.x(), r.y()+(tb ? 2 : (r.height()-6)/2), r.width(), 3), false, (r.width()-8)/2,
                      tb ? 0 : (r.height()-5)/2, border, 0, 5, handles);
        break;
    case LINE_FLAT:
        drawLines(p, r, !(option->state&State_Horizontal), 2, tb ? 4 : 2, border, tb ? -2 : 0, 4, handles);
        break;
    default:
        drawLines(p, r, !(option->state&State_Horizontal), 2, tb ? 4 : 2, border, tb ? -2 : 0, 3, handles);
    }
}

void Style::fillTab(QPainter *p, const QRect &r, const QStyleOption *option, const QColor &fill, bool horiz, EWidget tab,
                    bool tabOnly) const
{
    bool   invertedSel=option->state&State_Selected && APPEARANCE_INVERTED==opts.appearance;
    QColor col(invertedSel ? option->palette.window().color() : fill);

    if(opts.tabBgnd && !tabOnly)
        col=shade(col, TO_FACTOR(opts.tabBgnd));

    if(invertedSel)
        p->fillRect(r, col);
    else
    {
        bool        selected(option->state&State_Selected);
        EAppearance app(selected ? SEL_TAB_APP : NORM_TAB_APP);

        drawBevelGradient(col, p, r, horiz, selected, app, tab);
    }
}

void Style::colorTab(QPainter *p, const QRect &r, bool horiz, EWidget tab, int round) const
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    QLinearGradient grad(r.topLeft(), horiz ? r.bottomLeft() : r.topRight());
    QColor          start(m_highlightCols[ORIGINAL_SHADE]),
        end(m_highlightCols[ORIGINAL_SHADE]);

    start.setAlphaF(TO_ALPHA(opts.colorSelTab));
    end.setAlphaF(0.0);
    grad.setColorAt(0, WIDGET_TAB_TOP==tab ? start : end);
    grad.setColorAt(1, WIDGET_TAB_TOP==tab ? end : start);
    p->fillPath(buildPath(r, tab, round, qtcGetRadius(&opts, r.width(), r.height(), tab, RADIUS_EXTERNAL)), grad);
    p->restore();
}

void Style::shadeColors(const QColor &base, QColor *vals) const
{
    bool useCustom(USE_CUSTOM_SHADES(opts));
    double hl=TO_FACTOR(opts.highlightFactor);

    for(int i=0; i<QTC_NUM_STD_SHADES; ++i)
        shade(base, &vals[i], useCustom ? opts.customShades[i] :
              qtcShadeGetIntern(opts.contrast, i,
                                opts.darkerBorders, opts.shading));
    shade(base, &vals[SHADE_ORIG_HIGHLIGHT], hl);
    shade(vals[4], &vals[SHADE_4_HIGHLIGHT], hl);
    shade(vals[2], &vals[SHADE_2_HIGHLIGHT], hl);
    vals[ORIGINAL_SHADE]=base;
}

const QColor * Style::buttonColors(const QStyleOption *option) const
{
    if(option && option->version>=TBAR_VERSION_HACK &&
       option->version<TBAR_VERSION_HACK+NUM_TITLEBAR_BUTTONS &&
       coloredMdiButtons(option->state&State_Active, option->state&(State_MouseOver|State_Sunken)))
        return m_titleBarButtonsCols[option->version-TBAR_VERSION_HACK];

    if(option && option->palette.button()!=m_buttonCols[ORIGINAL_SHADE])
    {
        shadeColors(option->palette.button().color(), m_coloredButtonCols);
        return m_coloredButtonCols;
    }

    return m_buttonCols;
}

QColor Style::titlebarIconColor(const QStyleOption *option) const
{
    if(option && option->version>=TBAR_VERSION_HACK)
    {
        if(opts.titlebarButtons&TITLEBAR_BUTTON_ICON_COLOR && option->version<TBAR_VERSION_HACK+(NUM_TITLEBAR_BUTTONS*3))
            return opts.titlebarButtonColors[option->version-TBAR_VERSION_HACK];
        if(option->version<TBAR_VERSION_HACK+NUM_TITLEBAR_BUTTONS &&
           coloredMdiButtons(option->state&State_Active, option->state&(State_MouseOver|State_Sunken)))
            return m_titleBarButtonsCols[option->version-TBAR_VERSION_HACK][ORIGINAL_SHADE];
    }

    return buttonColors(option)[ORIGINAL_SHADE];
}

const QColor * Style::popupMenuCols(const QStyleOption *option) const
{
    return (opts.lighterPopupMenuBgnd || opts.shadePopupMenu || !option ?
            m_popupMenuCols : backgroundColors(option));
}

const QColor*
Style::checkRadioColors(const QStyleOption *option) const
{
    return (opts.crColor && option && option->state & State_Enabled &&
            (option->state & State_On || option->state & State_NoChange)
            ? m_checkRadioSelCols : buttonColors(option));
}

const QColor * Style::sliderColors(const QStyleOption *option) const
{
    return (option && option->state&State_Enabled)
        ? SHADE_NONE!=opts.shadeSliders && m_sliderCols &&
        (!opts.colorSliderMouseOver || option->state&State_MouseOver)
        ? m_sliderCols
        : m_buttonCols //buttonColors(option)
        : m_backgroundCols;
}

const QColor * Style::backgroundColors(const QColor &col) const
{
    if(col.alpha()!=0 && col!=m_backgroundCols[ORIGINAL_SHADE])
    {
        shadeColors(col, m_coloredBackgroundCols);
        return m_coloredBackgroundCols;
    }

    return m_backgroundCols;
}

const QColor * Style::highlightColors(const QColor &col) const
{
    if(col.alpha()!=0 && col!=m_highlightCols[ORIGINAL_SHADE])
    {
        shadeColors(col, m_coloredHighlightCols);
        return m_coloredHighlightCols;
    }

    return m_highlightCols;
}

const QColor * Style::borderColors(const QStyleOption *option, const QColor *use) const
{
    return opts.coloredMouseOver && option && option->state&State_MouseOver && option->state&State_Enabled ? m_mouseOverCols : use;
}

const QColor * Style::getSidebarButtons() const
{
    if(!m_sidebarButtonsCols)
    {
        if(SHADE_BLEND_SELECTED==opts.shadeSliders)
            m_sidebarButtonsCols=m_sliderCols;
        else if(IND_COLORED==opts.defBtnIndicator)
            m_sidebarButtonsCols=m_defBtnCols;
        else
        {
            m_sidebarButtonsCols=new QColor [TOTAL_SHADES+1];
            shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE], m_buttonCols[ORIGINAL_SHADE]),
                        m_sidebarButtonsCols);
        }
    }

    return m_sidebarButtonsCols;
}

void Style::setMenuColors(const QColor &bgnd)
{
    switch (opts.shadeMenubars) {
    case SHADE_NONE:
        std::copy(m_backgroundCols, m_backgroundCols + TOTAL_SHADES + 1, m_menubarCols);
        break;
    case SHADE_BLEND_SELECTED:
        shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE], m_backgroundCols[ORIGINAL_SHADE]), m_menubarCols);
        break;
    case SHADE_SELECTED:
        shadeColors(IS_GLASS(opts.appearance)
                    ? shade(m_highlightCols[ORIGINAL_SHADE], MENUBAR_GLASS_SELECTED_DARK_FACTOR)
                    : m_highlightCols[ORIGINAL_SHADE],
                    m_menubarCols);
        break;
    case SHADE_CUSTOM:
        shadeColors(opts.customMenubarsColor, m_menubarCols);
        break;
    case SHADE_DARKEN:
        shadeColors(shade(bgnd, MENUBAR_DARK_FACTOR), m_menubarCols);
        break;
    case SHADE_WINDOW_BORDER:
        break;
    }

    QColor *base=opts.shadePopupMenu
        ? SHADE_WINDOW_BORDER==opts.shadeMenubars
        ? (QColor *)getMdiColors(0L, true) // TODO: option!!!
        : m_menubarCols
        : m_backgroundCols;

    if (opts.lighterPopupMenuBgnd) {
        if (!m_popupMenuCols) {
            m_popupMenuCols = new QColor[TOTAL_SHADES + 1];
        }
        shadeColors(shade(base[ORIGINAL_SHADE],
                          TO_FACTOR(opts.lighterPopupMenuBgnd)),
                    m_popupMenuCols);
    } else {
        m_popupMenuCols = base;
    }
}

void Style::setMenuTextColors(QWidget *widget, bool isMenuBar) const
{
    if(SHADE_WINDOW_BORDER==opts.shadeMenubars)
    {
        QPalette pal(widget->palette());
        QStyleOption opt;

        opt.init(widget);
        getMdiColors(&opt, false);

        pal.setBrush(QPalette::Active, QPalette::Foreground, m_activeMdiTextColor);
        pal.setBrush(QPalette::Active, QPalette::Text, pal.brush(QPalette::Active, QPalette::Foreground));
        if(isMenuBar)
        {
            pal.setBrush(QPalette::Inactive, QPalette::Foreground,
                         opts.shadeMenubarOnlyWhenActive ? m_mdiTextColor : m_activeMdiTextColor);
            pal.setBrush(QPalette::Inactive, QPalette::Text, pal.brush(QPalette::Inactive, QPalette::Foreground));
        }
        else if(opts.shadePopupMenu)
        {
            pal.setBrush(QPalette::Disabled, QPalette::Foreground, midColor(m_activeMdiTextColor, popupMenuCols()[ORIGINAL_SHADE]));
            pal.setBrush(QPalette::Disabled, QPalette::Text, pal.brush(QPalette::Disabled, QPalette::Foreground));
        }

        widget->setPalette(pal);
    } else if (opts.customMenuTextColor ||
               oneOf(opts.shadeMenubars, SHADE_BLEND_SELECTED,
                     SHADE_SELECTED) ||
               (opts.shadeMenubars == SHADE_CUSTOM &&
                TOO_DARK(m_menubarCols[ORIGINAL_SHADE]))) {
        QPalette pal(widget->palette());

        pal.setBrush(QPalette::Active, QPalette::Foreground,
                     opts.customMenuTextColor ? opts.customMenuNormTextColor :
                     pal.highlightedText().color());
        pal.setBrush(QPalette::Active, QPalette::Text, pal.brush(QPalette::Active, QPalette::Foreground));

        if(isMenuBar && !opts.shadeMenubarOnlyWhenActive)
        {
            pal.setBrush(QPalette::Inactive, QPalette::Foreground, opts.customMenuTextColor
                         ? opts.customMenuNormTextColor
                         : pal.highlightedText().color());
            pal.setBrush(QPalette::Inactive, QPalette::Text, pal.brush(QPalette::Inactive, QPalette::Foreground));
        }
        else if(!isMenuBar && opts.shadePopupMenu)
        {
            pal.setBrush(QPalette::Disabled, QPalette::Foreground,
                         midColor(pal.brush(QPalette::Active, QPalette::Foreground).color(), popupMenuCols()[ORIGINAL_SHADE]));
            pal.setBrush(QPalette::Disabled, QPalette::Text, pal.brush(QPalette::Disabled, QPalette::Foreground));
        }
        widget->setPalette(pal);
    }
}

const QColor * Style::menuColors(const QStyleOption *option, bool active) const
{
    if(SHADE_WINDOW_BORDER == opts.shadeMenubars)
    {
        return getMdiColors(option, active);
    }
#ifdef Q_OS_MACOS
    else if(opts.shadeMenubarOnlyWhenActive && !active)
#else
    else if(opts.shadeMenubars == SHADE_NONE || (opts.shadeMenubarOnlyWhenActive && !active))
#endif
    {
        return backgroundColors(option);
    }
    return m_menubarCols;
}

bool
Style::coloredMdiButtons(bool active, bool mouseOver) const
{
    return (opts.titlebarButtons & TITLEBAR_BUTTON_COLOR &&
            (active ?
             (mouseOver || !(opts.titlebarButtons &
                             TITLEBAR_BUTTON_COLOR_MOUSE_OVER)) :
             ((opts.titlebarButtons & TITLEBAR_BUTTON_COLOR_MOUSE_OVER &&
               mouseOver) ||
              (!(opts.titlebarButtons & TITLEBAR_BUTTON_COLOR_MOUSE_OVER) &&
               opts.titlebarButtons & TITLEBAR_BUTTON_COLOR_INACTIVE))));
}

const QColor*
Style::getMdiColors(const QStyleOption *option, bool active) const
{
    if (!m_activeMdiColors) {
#ifndef QTC_QT6_ENABLE_KDE
        m_activeMdiTextColor = (option ? option->palette.text().color() :
                                 QApplication::palette().text().color());
        m_mdiTextColor = (option ? option->palette.text().color() :
                           QApplication::palette().text().color());

        QFile f(kdeHome() + "/share/config/kdeglobals");

        if (f.open(QIODevice::ReadOnly)) {
            QTextStream in(&f);
            bool inPal = false;

            while (!in.atEnd()) {
                QString line(in.readLine());
                if (inPal) {
                    if (!m_activeMdiColors &&
                        line.indexOf("activeBackground=") == 0) {
                        QColor col;
                        setRgb(&col, line.mid(17).split(","));
                        if (col != m_highlightCols[ORIGINAL_SHADE]) {
                            m_activeMdiColors = new QColor[TOTAL_SHADES + 1];
                            shadeColors(col, m_activeMdiColors);
                        }
                    } else if (!m_mdiColors &&
                               line.indexOf("inactiveBackground=") == 0) {
                        QColor col;
                        setRgb(&col, line.mid(19).split(","));
                        if (col != m_buttonCols[ORIGINAL_SHADE]) {
                            m_mdiColors = new QColor[TOTAL_SHADES+1];
                            shadeColors(col, m_mdiColors);
                        }
                    } else if(line.indexOf("activeForeground=") == 0) {
                        setRgb(&m_activeMdiTextColor, line.mid(17).split(","));
                    } else if(line.indexOf("inactiveForeground=") == 0) {
                        setRgb(&m_mdiTextColor, line.mid(19).split(","));
                    } else if (line.indexOf('[') != -1) {
                        break;
                    }
                } else if(line.indexOf("[WM]") == 0) {
                    inPal = true;
                }
            }
            f.close();
        }
#else
        Q_UNUSED(option);
        KConfigGroup cg(m_kdeGlobals, "WM");

        QColor col = cg.readEntry("activeBackground", QColor(48, 174, 232));

        if (col != m_backgroundCols[ORIGINAL_SHADE]) {
            m_activeMdiColors = new QColor[TOTAL_SHADES + 1];
            shadeColors(col, m_activeMdiColors);
        }

        col = cg.readEntry("inactiveBackground", QColor(224, 223, 222));;
        if (col != m_backgroundCols[ORIGINAL_SHADE]) {
            m_mdiColors = new QColor[TOTAL_SHADES+1];
            shadeColors(col, m_mdiColors);
        }

        m_activeMdiTextColor = cg.readEntry("activeForeground", QColor(255, 255, 255));
        m_mdiTextColor = cg.readEntry("inactiveForeground", QColor(75, 71, 67));
#endif

        if(!m_activeMdiColors)
            m_activeMdiColors=(QColor *)m_backgroundCols;
        if(!m_mdiColors)
            m_mdiColors=(QColor *)m_backgroundCols;

        if(opts.shadeMenubarOnlyWhenActive && SHADE_WINDOW_BORDER==opts.shadeMenubars &&
           m_activeMdiColors[ORIGINAL_SHADE]==m_mdiColors[ORIGINAL_SHADE])
            opts.shadeMenubarOnlyWhenActive=false;
    }

    return active ? m_activeMdiColors : m_mdiColors;
}

void Style::readMdiPositions() const
{
    if (0==m_mdiButtons[0].size() && 0==m_mdiButtons[1].size()) {
#ifdef Q_OS_MACOS
        // no control over where the system menu appears, so we have little choice
        // but to keep it at its default position. The user can still override this.
        m_mdiButtons[0].append(SC_TitleBarSysMenu);
        m_mdiButtons[0].append(SC_TitleBarCloseButton);
        m_mdiButtons[0].append(SC_TitleBarMinButton);
        m_mdiButtons[0].append(SC_TitleBarMaxButton);

        m_mdiButtons[1].append(SC_TitleBarShadeButton);
        m_mdiButtons[1].append(SC_TitleBarContextHelpButton);
#else
        // Set defaults...
        m_mdiButtons[0].append(SC_TitleBarSysMenu);
        m_mdiButtons[0].append(SC_TitleBarShadeButton);

        m_mdiButtons[1].append(SC_TitleBarContextHelpButton);
        m_mdiButtons[1].append(SC_TitleBarMinButton);
        m_mdiButtons[1].append(SC_TitleBarMaxButton);
        m_mdiButtons[1].append(WINDOWTITLE_SPACER);
        m_mdiButtons[1].append(SC_TitleBarCloseButton);
#endif

#ifdef QTC_QT6_ENABLE_KDE
        KSharedConfigPtr cfg = KSharedConfig::openConfig("kwinrc");
        KConfigGroup grp = cfg->group("org.kde.kdecoration2");

        QString left=grp.readEntry("ButtonsOnLeft", QString()),
            right=grp.readEntry("ButtonsOnRight", QString());

        if(!left.isEmpty() || !right.isEmpty())
            m_mdiButtons[0].clear(), m_mdiButtons[1].clear();

        if(!left.isEmpty())
            parseWindowLine(left, m_mdiButtons[0]);

        if(!right.isEmpty())
            parseWindowLine(right, m_mdiButtons[1]);

        // Designer uses shade buttons, not min/max - so if we don't have shade in our kwin config. 
        // then add this button near the max button...
        if (-1==m_mdiButtons[0].indexOf(SC_TitleBarShadeButton) && -1==m_mdiButtons[1].indexOf(SC_TitleBarShadeButton)) {
            int maxPos=m_mdiButtons[0].indexOf(SC_TitleBarMaxButton);

            if(-1==maxPos) // Left doesnt have max button, assume right does and add shade there
            {
                int minPos=m_mdiButtons[1].indexOf(SC_TitleBarMinButton);
                maxPos=m_mdiButtons[1].indexOf(SC_TitleBarMaxButton);

                m_mdiButtons[1].insert(minPos<maxPos ? (minPos==-1 ? 0 : minPos)
                                        : (maxPos==-1 ? 0 : maxPos), SC_TitleBarShadeButton);
            }
            else // Add to left button
            {
                int minPos=m_mdiButtons[0].indexOf(SC_TitleBarMinButton);

                m_mdiButtons[1].insert(minPos>maxPos ? (minPos==-1 ? 0 : minPos)
                                        : (maxPos==-1 ? 0 : maxPos), SC_TitleBarShadeButton);
            }
        }
#endif
    }
}

const QColor & Style::getFill(const QStyleOption *option, const QColor *use, bool cr, bool darker) const
{
    return !option || !(option->state&State_Enabled)
        ? use[darker ? 2 : ORIGINAL_SHADE]
        : option->state&State_Sunken  // State_Down ????
        ? use[darker ? 5 : 4]
        : option->state&State_MouseOver
        ? !cr && option->state&State_On
        ? use[darker ? 3 : SHADE_4_HIGHLIGHT]
        : use[darker ? SHADE_2_HIGHLIGHT : SHADE_ORIG_HIGHLIGHT]
        : !cr && option->state&State_On
        ? use[darker ? 5 : 4]
        : use[darker ? 2 : ORIGINAL_SHADE];
}

QPixmap * Style::getPixmap(const QColor col, EPixmap p, double shade) const
{
    QtcKey  key(createKey(col, p));
    QPixmap *pix=m_pixmapCache.object(key);

    if (!pix) {
        if (p == PIX_DOT) {
            pix=new QPixmap(5, 5);
            pix->fill(Qt::transparent);

            QColor          c(col);
            QPainter        p(pix);
            QLinearGradient g1(0, 0, 5, 5),
                g2(0, 0, 3, 3);

            g1.setColorAt(0.0, c);
            c.setAlphaF(0.4);
            g1.setColorAt(1.0, c);
            c=Qt::white;
            c.setAlphaF(0.9);
            g2.setColorAt(0.0, c);
            c.setAlphaF(0.7);
            g2.setColorAt(1.0, c);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setPen(Qt::NoPen);
            p.setBrush(g1);
            p.drawEllipse(0, 0, 5, 5);
            p.setBrush(g2);
            p.drawEllipse(1, 1, 4, 4);
            p.end();
        }
        else
        {
            pix=new QPixmap();

            QImage img;

            switch(p)
            {
            case PIX_CHECK:
                if(opts.xCheck) {
                    img = qtc_check_x_on;
                } else {
                    img = qtc_check_on;
                }
                break;
            default:
                break;
            }

            if (img.depth()<32)
                img=img.convertToFormat(QImage::Format_ARGB32);

            qtcAdjustPix(img.bits(), 4, img.width(), img.height(),
                         img.bytesPerLine(), col.red(), col.green(),
                         col.blue(), shade, QTC_PIXEL_QT);
            *pix=QPixmap::fromImage(img);
        }
        m_pixmapCache.insert(key, pix, pix->depth()/8);
    }

    return pix;
}

const QColor & Style::getTabFill(bool current, bool highlight, const QColor *use) const
{
    return (current ? use[ORIGINAL_SHADE] : highlight ?
            use[SHADE_2_HIGHLIGHT] : use[2]);
}

QColor Style::menuStripeCol() const
{
    switch(opts.menuStripe)
    {
    default:
    case SHADE_NONE:
        return m_backgroundCols[ORIGINAL_SHADE];
    case SHADE_CUSTOM:
        return opts.customMenuStripeColor;
    case SHADE_BLEND_SELECTED:
        // Hack! Use opts.customMenuStripeColor to store this setting!
        if (isBlack(opts.customMenuStripeColor)) {
            opts.customMenuStripeColor =
                midColor(m_highlightCols[ORIGINAL_SHADE],
                         popupMenuCols()[ORIGINAL_SHADE]);
        }
        return opts.customMenuStripeColor;
    case SHADE_SELECTED:
        return m_highlightCols[MENU_STRIPE_SHADE];
    case SHADE_DARKEN:
        return popupMenuCols()[MENU_STRIPE_SHADE];
    }
}

const QColor & Style::checkRadioCol(const QStyleOption *opt) const
{
    return opt->state&State_Enabled
        ? m_checkRadioCol
        : opts.crButton
        ? opt->palette.buttonText().color()
        : opt->palette.text().color();
}

QColor Style::shade(const QColor &a, double k) const
{
    QColor mod;
    qtcShade(&a, &mod, k, opts.shading);
    return mod;
}

void Style::shade(const QColor &ca, QColor *cb, double k) const
{
    qtcShade(&ca, cb, k, opts.shading);
}

QColor Style::getLowerEtchCol(const QWidget *widget) const
{
    if(USE_CUSTOM_ALPHAS(opts))
    {
        QColor col(Qt::white);
        col.setAlphaF(opts.customAlphas[ALPHA_ETCH_LIGHT]);
        return col;
    }

    if (qtcIsFlatBgnd(opts.bgndAppearance)) {
        QtcQWidgetProps props(widget);
        bool doEtch = widget && widget->parentWidget() && !props->noEtch;
        // CPD: Don't really want to check here for every widget, when
        // (so far) on problem seems to be in KPackageKit, and thats with
        // its KTextBrowser - so just check when we draw scrollviews...
        // if (doEtch && isInQAbstractItemView(widget->parentWidget())) {
        //     doEtch = false;
        //     props->noEtch = true;
        // }

        if(doEtch)
        {
            QColor bgnd(widget->parentWidget()->palette().color(widget->parentWidget()->backgroundRole()));

            if(bgnd.alpha()>0)
                return shade(bgnd, 1.06);
        }
    }

    QColor col(Qt::white);
    col.setAlphaF(0.1); // qtcIsFlatBgnd(opts.bgndAppearance) ? 0.25 : 0.4);

    return col;
}

int
Style::getFrameRound(const QWidget *widget) const
{
    if (opts.square & SQUARE_FRAME) {
        return ROUNDED_NONE;
    }
    const QWidget *window = widget ? widget->window() : nullptr;

    if (window) {
        if (widget->rect() == window->rect()) {
            return ROUNDED_NONE;
        }
    }

    if ((opts.square & SQUARE_ENTRY) && widget &&
        qobject_cast<const QLabel*>(widget)) {
        return ROUNDED_NONE;
    }
    return ROUNDED_ALL;
}

void
Style::widgetDestroyed(QObject *o)
{
    QWidget *w = static_cast<QWidget*>(o);
    if (theThemedApp == APP_KONTACT) {
        m_sViewContainers.remove(w);
        QMap<QWidget*, QSet<QWidget*> >::Iterator it(m_sViewContainers.begin());
        QMap<QWidget*, QSet<QWidget*> >::Iterator end(m_sViewContainers.end());
        QSet<QWidget*> rem;

        for (;it != end;++it) {
            (*it).remove(w);
            if ((*it).isEmpty()) {
                rem.insert(it.key());
            }
        }
        for (QWidget *widget: const_(rem)) {
            m_sViewContainers.remove(widget);
        }
    }
}

#ifdef QTC_QT6_ENABLE_KDE
// void Style::setupKde4()
// {
//     if(kapp) {
//         setDecorationColors();
//     } else {
//         applyKdeSettings(true);
//         applyKdeSettings(false);
//     }
// }

void
Style::setDecorationColors()
{
    KColorScheme kcs(QPalette::Active);
    if (opts.coloredMouseOver) {
        shadeColors(kcs.decoration(KColorScheme::HoverColor).color(),
                    m_mouseOverCols);
    }
    shadeColors(kcs.decoration(KColorScheme::FocusColor).color(), m_focusCols);
}

// void Style::applyKdeSettings(bool pal)
// {
//     if(pal)
//     {
//         if(!kapp)
//             QApplication::setPalette(standardPalette());
//         setDecorationColors();
//     }
//     else
//     {
//         KConfigGroup g(m_configFile, "General");
//         QFont        mnu=g.readEntry("menuFont", QApplication::font());

//         QApplication::setFont(g.readEntry("font", QApplication::font()));
//         QApplication::setFont(mnu, "QMenuBar");
//         QApplication::setFont(mnu, "QMenu");
//         QApplication::setFont(mnu, "KPopupTitle");
//         QApplication::setFont(g.readEntry("toolBarFont", QApplication::font()), "QToolBar");
//     }
// }
#endif

void Style::kdeGlobalSettingsChange(int type, int)
{
    // TODO
    // KDE5 is not emitting this anymore.
    // We need our own signal from the configure UI
    Q_UNUSED(type);
    // switch(type) {
    // case KGlobalSettings::StyleChanged: {
    //     m_configFile->reparseConfiguration();
    //     if (m_usePixmapCache)
    //         QPixmapCache::clear();
    //     init(false);

    //     for (QWidget *widget: QApplication::topLevelWidgets()) {
    //         widget->update();
    //     }
    //     break;
    // }
    // case KGlobalSettings::PaletteChanged:
    //     m_configFile->reparseConfiguration();
    //     applyKdeSettings(true);
    //     if (m_usePixmapCache)
    //         QPixmapCache::clear();
    //     break;
    // case KGlobalSettings::FontChanged:
    //     m_configFile->reparseConfiguration();
    //     applyKdeSettings(false);
    //     break;
    // }

    m_blurHelper->setEnabled(Utils::compositingActive());
    m_windowManager->initialize(opts.windowDrag);
}

void Style::borderSizesChanged()
{
#ifdef QTC_QT6_ENABLE_KDE
    int old = qtcGetWindowBorderSize(false).titleHeight;

    if (old != qtcGetWindowBorderSize(true).titleHeight) {
        for (QWidget *widget: QApplication::topLevelWidgets()) {
            if (qobject_cast<QMainWindow*>(widget) &&
                static_cast<QMainWindow*>(widget)->menuBar()) {
                static_cast<QMainWindow*>(widget)->menuBar()->update();
            }
        }
    }
#endif
}

static QMainWindow*
getWindow(unsigned int xid)
{
    QTC_RET_IF_FAIL(xid, nullptr);
    for (QWidget *widget: QApplication::topLevelWidgets()) {
        if (qobject_cast<QMainWindow*>(widget) && qtcGetWid(widget) == xid) {
            return static_cast<QMainWindow*>(widget);
        }
    }
    return nullptr;
}

static bool
diffTime(struct timeval *lastTime)
{
    struct timeval now, diff;

    gettimeofday(&now, nullptr);
    timersub(&now, lastTime, &diff);
    *lastTime = now;
    return diff.tv_sec > 0 || diff.tv_usec > 500000;
}

void
Style::toggleMenuBar(unsigned int xid)
{
    static unsigned int lastXid = 0;
    static struct timeval lastTime = {0, 0};

    if (diffTime(&lastTime) || lastXid != xid) {
        QMainWindow *win = getWindow(xid);
        if (win) {
            toggleMenuBar(win);
        }
    }
    lastXid = xid;
}

void
Style::toggleStatusBar(unsigned int xid)
{
    static unsigned int lastXid = 0;
    static struct timeval lastTime = {0, 0};

    if (diffTime(&lastTime) || lastXid != xid) {
        QMainWindow *win = getWindow(xid);
        if (win) {
            toggleStatusBar(win);
        }
    }
    lastXid = xid;
}

void Style::compositingToggled()
{
    for (QWidget *widget: QApplication::topLevelWidgets()) {
        widget->update();
    }
}

void Style::toggleMenuBar(QMainWindow *window)
{
    bool triggeredAction(false);

#ifdef QTC_QT6_ENABLE_KDE
    if (qobject_cast<KXmlGuiWindow*>(window)) {
        KActionCollection *collection=static_cast<KXmlGuiWindow *>(window)->actionCollection();
        QAction           *act=collection ? collection->action(KStandardAction::name(KStandardAction::ShowMenubar)) : 0L;
        if(act)
        {
            act->trigger();
            triggeredAction=true;
        }
    }
#endif
    if(!triggeredAction)
    {
        QWidget *menubar=window->menuWidget();
        if(m_saveMenuBarStatus)
            qtcSetMenuBarHidden(appName, menubar->isVisible());

        window->menuWidget()->setHidden(menubar->isVisible());
    }
}

void Style::toggleStatusBar(QMainWindow *window)
{
    bool triggeredAction(false);

#ifdef QTC_QT6_ENABLE_KDE
    if (qobject_cast<KXmlGuiWindow*>(window)) {
        KActionCollection *collection=static_cast<KXmlGuiWindow *>(window)->actionCollection();
        QAction           *act=collection ? collection->action(KStandardAction::name(KStandardAction::ShowStatusbar)) : 0L;
        if(act)
        {
            act->trigger();
            triggeredAction=true;
            //emitStatusBarState(true); // TODO: ???
        }
    }
#endif
    if (!triggeredAction) {
        QList<QStatusBar*> sb = getStatusBars(window);

        if (sb.count()) {
            if (m_saveStatusBarStatus) {
                qtcSetStatusBarHidden(appName, sb.first()->isVisible());
            }
            for (QStatusBar *statusBar: const_(sb)) {
                statusBar->setHidden(statusBar->isVisible());
            }
            emitStatusBarState(sb.first());
        }
    }
}

void Style::emitMenuSize(QWidget *w, unsigned short size, bool force)
{
    // DO NOT condition compile on QTC_ENABLE_X11.
    // There's no direct linkage on X11 and the following code will just do
    // nothing if X11 is not enabled (either at compile time or at run time).
    QTC_RET_IF_FAIL(qtcX11Enabled());

    if (WId wid = qtcGetWid(w->window())) {
        static const char *constMenuSizeProperty = "qtcMenuSize";
        unsigned short oldSize = 2000;

        if (!force) {
            QVariant prop(w->property(constMenuSizeProperty));
            if (prop.isValid()) {
                bool ok;
                oldSize = prop.toUInt(&ok);
                if (!ok) {
                    oldSize = 2000;
                }
            }
        }

        if (oldSize != size) {
            w->setProperty(constMenuSizeProperty, size);
            qtcX11SetMenubarSize(wid, size);
            if(!m_dBusHelper->m_dBus)
                m_dBusHelper->m_dBus = new QDBusInterface("org.kde.kwin", "/QtCurve",
                                             "org.kde.QtCurve");
            m_dBusHelper->m_dBus->call(QDBus::NoBlock, "menuBarSize",
                          (unsigned int)wid, (int)size);
        }
    }
}

void Style::emitStatusBarState(QStatusBar *sb)
{
    if (opts.statusbarHiding & HIDE_KWIN) {
        if (!m_dBusHelper->m_dBus)
            m_dBusHelper->m_dBus = new QDBusInterface("org.kde.kwin", "/QtCurve",
                                        "org.kde.QtCurve");
        m_dBusHelper->m_dBus->call(QDBus::NoBlock, "statusBarState",
                     (unsigned int)qtcGetWid(sb->window()),
                     sb->isVisible());
    }
}
}
