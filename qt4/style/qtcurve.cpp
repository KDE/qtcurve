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

#include "config.h"
#include <qtcurve-utils/log.h>
#include <qtcurve-utils/qtprops.h>

#include "qtcurve_p.h"
#include "windowmanager.h"
#include "blurhelper.h"
#include "shortcuthandler.h"

#include <QProgressBar>
#include <QFileDialog>
#include <QToolButton>
#include <QAbstractItemView>
#include <QSplitter>
#include <QSettings>
#include <QTreeView>
#include <QPixmapCache>
#include <QMdiSubWindow>
#include <QStatusBar>
#include <QComboBox>
#include <QWidget>
#include <QListView>
#include <QCheckBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QTextEdit>
#include <QDial>
#include <QStackedLayout>
#include <QSpinBox>
#include <QLabel>
#include <QPrintDialog>
#include <QMouseEvent>
#include <QMenuBar>
#include <QWheelEvent>
#include <QScrollBar>
#include <QResizeEvent>
#include <QWizard>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QHeaderView>
#include <QLineEdit>
#include <QFormLayout>
#include <QPainter>
#include <QMainWindow>
#include <QTextStream>
#include <QToolBox>
#include <QDBusConnection>
#include <QDBusInterface>

#include "macmenu.h"
#include "shadowhelper.h"
#include <sys/time.h>
#include <qtcurve-utils/x11qtc.h>

#include <QDebug>

#include <common/config_file.h>
#include "check_on-png.h"
#include "check_x_on-png.h"

#ifndef QTC_QT4_ENABLE_KDE
#  include "dialog_error-png.h"
#  include "dialog_warning-png.h"
#  include "dialog_information-png.h"
#endif

// WebKit seems to just use the values from ::pixelMetric to get button sizes. So, in pixelMetric we add some extra padding to PM_ButtonMargin
// if we're max rounding - this gives a nicer border. However, dont want this on real buttons - so in sizeFromContents we remove this padding
// in CT_PushButton and CT_ComboBox
#define MAX_ROUND_BTN_PAD (ROUND_MAX==opts.round ? 3 : 0)

#ifdef QTC_QT4_ENABLE_KDE
#include <KDE/KApplication>
#include <KDE/KAboutData>
#include <KDE/KGlobalSettings>
#include <KDE/KConfig>
#include <KDE/KConfigGroup>
#include <KDE/KIconLoader>
#include <KDE/KIcon>
#include <KDE/KColorScheme>
#include <KDE/KStandardDirs>
#include <KDE/KComponentData>
#include <KDE/KTitleWidget>
#include <KDE/KTabBar>
#include <KDE/KFileDialog>
#include <KDE/KPassivePopup>
#include <KDE/KXmlGuiWindow>
#include <KDE/KStandardAction>
#include <KDE/KActionCollection>
#include <KDE/KIconEffect>
#include <KDE/KMenu>
#include <KDE/KAboutApplicationDialog>
#endif // QTC_QT4_ENABLE_KDE

#include <qtcurve-utils/color.h>

// TODO! REMOVE THIS WHEN KDE'S ICON SETTINGS ACTUALLY WORK!!!
#define FIX_DISABLED_ICONS

extern QString (*qt_filedialog_existing_directory_hook)(
    QWidget *parent, const QString &caption, const QString &dir,
    QFileDialog::Options options);

extern QString (*qt_filedialog_open_filename_hook)(
    QWidget *parent, const QString &caption, const QString &dir,
    const QString &filter, QString *selectedFilter,
    QFileDialog::Options options);

extern QStringList (*qt_filedialog_open_filenames_hook)(
    QWidget * parent, const QString &caption, const QString &dir,
    const QString &filter, QString *selectedFilter,
    QFileDialog::Options options);

extern QString (*qt_filedialog_save_filename_hook)(
    QWidget *parent, const QString &caption, const QString &dir,
    const QString &filter, QString *selectedFilter,
    QFileDialog::Options options);

#ifndef Q_OS_MAC
#define POPUP_MENUS_SQUARE(opts)    ((opts).square & SQUARE_POPUP_MENUS)
#else
// prevent a rendering glitch in popup menus:
#define POPUP_MENUS_SQUARE(opts)    true
#endif

namespace QtCurve {

#if defined FIX_DISABLED_ICONS && defined QTC_QT4_ENABLE_KDE
static inline QPixmap
getIconPixmap(const QIcon &icon, const QSize &size,
              QIcon::Mode mode, QIcon::State)
{
    QPixmap pix = icon.pixmap(size, QIcon::Normal);

    if (QIcon::Disabled == mode) {
        QImage img = pix.toImage();
        KIconEffect::toGray(img, 1.0);
        KIconEffect::semiTransparent(img);
        pix = QPixmap::fromImage(img);
    }

    return pix;
}
#else
QTC_ALWAYS_INLINE static inline QPixmap
getIconPixmap(const QIcon &icon, const QSize &size,
              QIcon::Mode mode, QIcon::State state=QIcon::Off)
{
    return icon.pixmap(size, mode, state);
}
#endif

QTC_ALWAYS_INLINE static inline QPixmap
getIconPixmap(const QIcon &icon, int size, QIcon::Mode mode,
              QIcon::State state=QIcon::Off)
{
    return getIconPixmap(icon, QSize(size, size), mode, state);
}

QTC_ALWAYS_INLINE static inline QPixmap
getIconPixmap(const QIcon &icon, const QSize &size, int flags,
              QIcon::State state=QIcon::Off)
{
    return getIconPixmap(icon, size, (flags & QStyle::State_Enabled ?
                                      QIcon::Normal : QIcon::Disabled), state);
}

QTC_ALWAYS_INLINE static inline QPixmap
getIconPixmap(const QIcon &icon, int size, int flags,
              QIcon::State state=QIcon::Off)
{
    return getIconPixmap(icon, QSize(size, size), flags, state);
}

QTC_ALWAYS_INLINE static inline Style::Icon
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

QTC_ALWAYS_INLINE static inline Style::Icon
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

QTC_ALWAYS_INLINE static inline void
drawTbArrow(const QStyle *style, const QStyleOptionToolButton *toolbutton,
            const QRect &rect, QPainter *painter, const QWidget *widget=0)
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

#define WINDOWTITLE_SPACER    0x10000000
#define STATE_REVERSE       (QStyle::StateFlag)0x10000000
#define STATE_MENU          (QStyle::StateFlag)0x20000000
#define STATE_VIEW          (QStyle::StateFlag)0x40000000
#define STATE_KWIN_BUTTON   (QStyle::StateFlag)0x40000000
#define STATE_TBAR_BUTTON   (QStyle::StateFlag)0x80000000
#define STATE_DWT_BUTTON    (QStyle::StateFlag)0x20000000
#define STATE_TOGGLE_BUTTON (QStyle::StateFlag)0x10000000

static const int constMenuPixmapWidth = 22;

QtcThemedApp theThemedApp = APP_OTHER;

static QString appName;

static inline bool
isOOWidget(const QWidget *widget)
{
    return APP_OPENOFFICE == theThemedApp && !widget;
}

static inline bool
blendOOMenuHighlight(const QPalette &pal, const QColor &highlight)
{
    QColor text(pal.text().color());
    QColor hl(pal.highlightedText().color());

    return (text.red() < 50 && text.green() < 50 &&
            text.blue() < 50 && hl.red() > 127 && hl.green() > 127 &&
            hl.blue() > 127 && TOO_DARK(highlight));
}

QTC_ALWAYS_INLINE static inline int
toHint(int sc)
{
    switch (sc) {
    case QStyle::SC_TitleBarSysMenu:
        return Qt::WindowSystemMenuHint;
    case QStyle::SC_TitleBarMinButton:
        return Qt::WindowMinimizeButtonHint;
    case QStyle::SC_TitleBarMaxButton:
        return Qt::WindowMaximizeButtonHint;
    case QStyle::SC_TitleBarCloseButton:
        return 0;
    case QStyle::SC_TitleBarNormalButton:
        return 0;
    case QStyle::SC_TitleBarShadeButton:
    case QStyle::SC_TitleBarUnshadeButton:
        return Qt::WindowShadeButtonHint;
    case QStyle::SC_TitleBarContextHelpButton:
        return Qt::WindowContextHelpButtonHint;
    default:
        return 0;
    }
}

static const char *constBoldProperty = "qtc-set-bold";

static void
setBold(QWidget *widget)
{
    QVariant prop(widget->property(constBoldProperty));
    if(!prop.isValid() || !prop.toBool())
    {
        QFont font(widget->font());

        if(!font.bold())
        {
            font.setBold(true);
            widget->setFont(font);
            widget->setProperty(constBoldProperty, true);
        }
    }
}

static void unSetBold(QWidget *widget)
{
    QVariant prop(widget->property(constBoldProperty));

    if(prop.isValid() && prop.toBool())
    {
        QFont font(widget->font());

        font.setBold(false);
        widget->setFont(font);
        widget->setProperty(constBoldProperty, false);
    }
}

static void adjustToolbarButtons(const QWidget *widget, const QToolBar *toolbar, int &leftAdjust, int &topAdjust,
                                 int &rightAdjust, int &bottomAdjust, int &round)
{
    const int constAdjust=6;
    const int d = 1;
    QRect geo(widget->geometry());

    if (Qt::Horizontal==toolbar->orientation())
    {
        bool haveLeft=qobject_cast<QToolButton*>(toolbar->childAt(geo.x()-d, geo.y())),
             haveRight=qobject_cast<QToolButton*>(toolbar->childAt(geo.right()+d, geo.y()));

        if(haveLeft && haveRight)
            leftAdjust=-constAdjust, rightAdjust=constAdjust, round=ROUNDED_NONE;
        else if(haveLeft)
            leftAdjust=-constAdjust, round=ROUNDED_RIGHT;
        else if(haveRight)
            rightAdjust=constAdjust, round=ROUNDED_LEFT;
    }
    else
    {
        bool haveTop=qobject_cast<QToolButton*>(toolbar->childAt(geo.x(), geo.y()-d)),
             haveBot=qobject_cast<QToolButton*>(toolbar->childAt(geo.x(), geo.bottom()+d));

        if(haveTop && haveBot)
            topAdjust=-constAdjust, bottomAdjust=constAdjust, round=ROUNDED_NONE;
        else if(haveTop)
            topAdjust=-constAdjust, round=ROUNDED_BOTTOM;
        else if(haveBot)
            bottomAdjust=constAdjust, round=ROUNDED_TOP;
    }
}

static const QToolBar * getToolBar(const QWidget *w/*, bool checkQ3*/)
{
    return w
            ? qobject_cast<const QToolBar*>(w) // || (checkQ3 && w->inherits("Q3ToolBar"))
                ? static_cast<const QToolBar*>(w)
                : getToolBar(w->parentWidget()/*, checkQ3*/)
            : 0L;
}

static inline QList<QStatusBar*> getStatusBars(QWidget *w)
{
    return w ? w->findChildren<QStatusBar*>() : QList<QStatusBar*>();
}

static QToolBar*
getToolBarChild(QWidget *w)
{
    foreach (QObject *child, w->children()) {
        if (child->isWidgetType()) {
            if (qobject_cast<QToolBar*>(child))
                return static_cast<QToolBar*>(child);
            QToolBar *tb = getToolBarChild((QWidget*)child);
            if (tb) {
                return tb;
            }
        }
    }
    return 0L;
}

static void
setStyleRecursive(QWidget *w, QStyle *s, int minSize)
{
    w->setStyle(s);
    if (qobject_cast<QToolButton*>(w))
        w->setMinimumSize(1, minSize);
    foreach (QObject *child, w->children()) {
        if (child->isWidgetType()) {
            setStyleRecursive((QWidget*)child, s, minSize);
        }
    }
}

static bool
isA(const QObject *w, const char *type)
{
    return w && (oneOf(w->metaObject()->className(), type) ||
                 (w->parent() &&
                  oneOf(w->parent()->metaObject()->className(), type)));
}

static bool
isInQAbstractItemView(const QObject *w)
{
    int level = 8;

    while (w && --level > 0) {
        if (qobject_cast<const QAbstractItemView*>(w))
            return true;
        if (qobject_cast<const QDialog*>(w)/* || qobject_cast<const QMainWindow*>(w)*/)
            return false;
        w = w->parent();
    }

    return false;
}

static bool
isKontactPreviewPane(const QWidget *widget)
{
    return (theThemedApp == APP_KONTACT &&
            qtcCheckType(widget, "KHBox") &&
            qtcCheckType<QSplitter>(widget->parentWidget()) &&
            qtcCheckType(getParent<2>(widget), "KMReaderWin"));
}

static bool isKateView(const QWidget *widget)
{
    return (qtcCheckType<QFrame>(widget) &&
            qtcCheckType(widget->parentWidget(), "KateView"));
}

static bool isNoEtchWidget(const QWidget *widget)
{
    if (APP_KRUNNER == theThemedApp)
        return true;

    if (theThemedApp == APP_PLASMA) {
        const QWidget *top = widget->window();
        return !top || (!qobject_cast<const QDialog*>(top) &&
                        !qobject_cast<const QMainWindow*>(top));
    }

    if(widget && widget->inherits("QWebView"))
        return true;

    // KHTML:  widget -> QWidget       -> QWidget    -> KHTMLView
    const QObject *w=widget && widget->parent() && widget->parent()->parent() ? widget->parent()->parent()->parent() : 0L;

    return (w && isA(w, "KHTMLView")) || (widget && isInQAbstractItemView(widget->parentWidget()));
}

static QWidget * scrollViewFrame(QWidget *widget)
{
    QWidget *w=widget;

    for(int i=0; i<10 && w; ++i, w=w->parentWidget())
    {
        if((qobject_cast<QFrame*>(w) && ((QFrame*)w)->frameWidth()>0) || qobject_cast<QTabWidget*>(w))
            return w;
    }
    return 0L;
}

static QColor checkColour(const QStyleOption *option, QPalette::ColorRole role)
{
    QColor col(option->palette.brush(role).color());

    if (col.alpha() == 255 && isBlack(col))
        return QApplication::palette().brush(role).color();
    return col;
}

static QColor blendColors(const QColor &foreground, const QColor &background, double alpha)
{
#ifdef QTC_QT4_ENABLE_KDE
    return KColorUtils::mix(background, foreground, alpha);
#else
    return qtcColorMix(&background, &foreground, alpha);
#endif
}

static void addStripes(QPainter *p, const QPainterPath &path, const QRect &rect, bool horizontal)
{
    QColor          col(Qt::white);
    QLinearGradient patternGradient(rect.topLeft(), rect.topLeft()+(horizontal ? QPoint(STRIPE_WIDTH, 0) : QPoint(0, STRIPE_WIDTH)));

    col.setAlphaF(0.0);
    patternGradient.setColorAt(0.0, col);
    col.setAlphaF(0.15);
    patternGradient.setColorAt(1.0, col);
    patternGradient.setSpread(QGradient::ReflectSpread);
    if(path.isEmpty())
        p->fillRect(rect, patternGradient);
    else
    {
        p->save();
        p->setRenderHint(QPainter::Antialiasing, true);
        p->fillPath(path, patternGradient);
        p->restore();
    }
}

static QRegion windowMask(const QRect &r, bool full)
{
    int x, y, w, h;
    r.getRect(&x, &y, &w, &h);

    if(full)
    {
        QRegion region(x + 4, y + 0, w-4*2, h-0*2);
        region += QRegion(x + 0, y + 4, w-0*2, h-4*2);
        region += QRegion(x + 2, y + 1, w-2*2, h-1*2);
        region += QRegion(x + 1, y + 2, w-1*2, h-2*2);
        return region;
    }
    else
    {
        QRegion region(x+1, y+1, w-2, h-2);
        region += QRegion(x, y+2, w, h-4);
        region += QRegion(x+2, y, w-4, h);
        return region;
    }
}

enum WindowsStyleConsts
{
    windowsItemFrame      =  2, // menu item frame width
    windowsSepHeight      =  9, // separator item height
    windowsItemHMargin    =  3, // menu item hor text margin
    windowsItemVMargin    =  2, // menu item ver text margin
    windowsRightBorder    = 15, // right border on windows
    windowsCheckMarkWidth = 12, // checkmarks width on windows
    windowsArrowHMargin   =  6  // arrow horizontal margin
};

static const int constWindowMargin   =  2;
static const int constProgressBarFps = 20;
static const int constTabPad         =  6;

static const QLatin1String constDwtClose("qt_dockwidget_closebutton");
static const QLatin1String constDwtFloat("qt_dockwidget_floatbutton");

#define SB_SUB2 ((QStyle::SubControl)(QStyle::SC_ScrollBarGroove << 1))

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
        uint32_t prop =
            (((qtcIsFlatBgnd(app) ? (haveBgndImage ? APPEARANCE_RAISED :
                                     APPEARANCE_FLAT) : app) & 0xFF) |
             (w->palette().background().color().rgb() & 0x00FFFFFF) << 8);
        qtcX11SetBgnd(wid, prop);
    }
}

void
setSbProp(QWidget *w)
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

#ifndef QTC_QT4_ENABLE_KDE
static void setRgb(QColor *col, const QStringList &rgb)
{
    if(3==rgb.size())
        *col=QColor(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt());
}
#endif

#ifdef QTC_QT4_STYLE_SUPPORT

static QString themeFile(const QString &dir, const QString &n, const char *sub)
{
    QString name(dir+sub+n+THEME_SUFFIX);

    return QFile(name).exists() ? name : QString();
}

static QString themeFile(const QString &dir, const QString &n, bool kde3=false)
{
    QString name(themeFile(dir, n, kde3 ? THEME_DIR : THEME_DIR4));

    if(name.isEmpty())
        name=themeFile(dir, n, kde3 ? THEME_DIR4 : THEME_DIR);
    return name;
}
#endif

static inline void drawRect(QPainter *p, const QRect &r)
{
    p->drawRect(r.x(), r.y(), r.width()-1, r.height()-1);
}

static inline void
drawAaLine(QPainter *p, int x1, int y1, int x2, int y2)
{
    p->drawLine(QLineF(x1 + 0.5, y1 + 0.5, x2 + 0.5, y2 + 0.5));
}

static inline void drawAaPoint(QPainter *p, int x, int y)
{
    p->drawPoint(QPointF(x+0.5, y+0.5));
}

static inline void drawAaRect(QPainter *p, const QRect &r)
{
    p->drawRect(QRectF(r.x()+0.5, r.y()+0.5, r.width()-1, r.height()-1));
}

static void drawDots(QPainter *p, const QRect &r, bool horiz, int nLines, int offset,
                     const QColor *cols, int startOffset, int dark)
{
    int space((nLines*2)+(nLines-1)),
        x(horiz ? r.x() : r.x()+((r.width()-space)>>1)),
        y(horiz ? r.y()+((r.height()-space)>>1) : r.y()),
        i, j,
        numDots((horiz ? (r.width()-(2*offset))/3 : (r.height()-(2*offset))/3)+1);

    p->setRenderHint(QPainter::Antialiasing, true);
    if(horiz)
    {
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
    }
    else
    {
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
    p->setRenderHint(QPainter::Antialiasing, false);
}

static bool isHoriz(const QStyleOption *option, EWidget w, bool joinedTBar)
{
    return option->state&QStyle::State_Horizontal ||
           (WIDGET_BUTTON(w) && (!joinedTBar || (WIDGET_TOOLBAR_BUTTON!=w && WIDGET_NO_ETCH_BTN!=w && WIDGET_MENU_BUTTON!=w)));
}

static bool isOnToolbar(const QWidget *widget)
{
    const QWidget *wid=widget ? widget->parentWidget() : 0L;

    while(wid)
    {
        if(qobject_cast<const QToolBar*>(wid) || wid->inherits("Q3ToolBar"))
            return true;
        wid=wid->parentWidget();
    }

    return false;
}

#define PIXMAP_DIMENSION 10

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
enum ECacheType
{
    CACHE_STD,
    CACHE_PBAR,
    CACHE_TAB_TOP,
    CACHE_TAB_BOT
};

static QtcKey createKey(qulonglong size, const QColor &color, bool horiz, int app, EWidget w)
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
    return 1+
           ((color.rgb()&RGB_MASK)<<1)+
           (((qulonglong)(p&0x1F))<<33)+
           (((qulonglong)1)<<38);
}

#ifdef QTC_QT4_ENABLE_KDE
static void parseWindowLine(const QString &line, QList<int> &data)
{
    int len(line.length());

    for(int i=0; i<len; ++i)
        switch(line[i].toLatin1())
        {
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
#endif

static const QWidget*
getWidget(const QPainter *p)
{
    if (p) {
        if (p->device()->devType() == QInternal::Widget) {
            return static_cast<const QWidget*>(p->device());
        } else {
            QPaintDevice *dev = QPainter::redirected(p->device());
            if (dev && dev->devType() == QInternal::Widget) {
                return static_cast<const QWidget*>(dev);
            }
        }
    }
    return 0L;
}

static const QImage * getImage(const QPainter *p)
{
    return p && p->device() && QInternal::Image==p->device()->devType() ? static_cast<const QImage*>(p->device()) : 0L;
}

static const QAbstractButton * getButton(const QWidget *w, const QPainter *p)
{
    const QWidget *widget=w ? w : getWidget(p);
    return widget ? qobject_cast<const QAbstractButton*>(widget) : 0L;
}

inline bool isMultiTabBarTab(const QAbstractButton *button)
{
    // Check for isFlat fails in KDE SC4.5
    return button && ((qobject_cast<const QPushButton*>(button) &&
                       // ((QPushButton*)button)->isFlat() &&
                       button->inherits("KMultiTabBarTab")) ||
                      (theThemedApp == APP_KDEVELOP &&
                       qobject_cast<const QToolButton*>(button) &&
                       button->inherits("Sublime::IdealToolButton")));
}

#ifdef QTC_QT4_STYLE_SUPPORT
Style::Style(const QString &name)
#else
Style::Style()
#endif
      : m_popupMenuCols(0L),
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
        m_pos(-1, -1),
        m_hoverWidget(0L),
        m_dBus(0),
        m_shadowHelper(new ShadowHelper(this)),
        m_sViewSBar(0L),
        m_windowManager(new WindowManager(this)),
        m_blurHelper(new BlurHelper(this)),
        m_shortcutHandler(new ShortcutHandler(this))
#ifdef QTC_QT4_STYLE_SUPPORT
        , m_name(name)
#endif
{
    const char *env = getenv(QTCURVE_PREVIEW_CONFIG);
    if (oneOf(env, QTCURVE_PREVIEW_CONFIG)) {
        // To enable preview of QtCurve settings, the style config module will set QTCURVE_PREVIEW_CONFIG
        // and use CE_QtC_SetOptions to set options. If this is set, we do not use the QPixmapCache as it
        // will interfere with that of the kcm's widgets!
        m_isPreview = PREVIEW_MDI;
        m_usePixmapCache = false;
    } else if (oneOf(env, QTCURVE_PREVIEW_CONFIG_FULL)) {
        // As above, but preview is in window - so can use opacity settings!
        m_isPreview = PREVIEW_WINDOW;
        m_usePixmapCache = false;
    } else {
        init(true);
    }
}

void Style::init(bool initial)
{
    if (!initial)
        freeColors();

#ifdef QTC_QT4_ENABLE_KDE
    if (initial) {
        if (KGlobal::hasMainComponent()) {
            m_componentData = KGlobal::mainComponent();
        } else {
            QString name(QApplication::applicationName());

            if (name.isEmpty()) {
                name = qAppName();
            }
            if (name.isEmpty()) {
                name = "QtApp";
            }
            m_componentData = KComponentData(
                name.toLatin1(), name.toLatin1(),
                KComponentData::SkipMainComponentRegistration);
        }
    }
#endif

    if (m_isPreview) {
        if (PREVIEW_WINDOW != m_isPreview) {
            opts.bgndOpacity = opts.dlgOpacity = opts.menuBgndOpacity = 100;
        }
    } else {
#ifdef QTC_QT4_STYLE_SUPPORT
        QString rcFile;
        if (!m_name.isEmpty()) {
            rcFile = themeFile(Utils::kdeHome(), m_name);
            if (rcFile.isEmpty()) {
                rcFile = themeFile(KDE_PREFIX(4), m_name, false);
            }
        }
        qtcReadConfig(rcFile, &opts);
#else
        qtcReadConfig(QString(), &opts);
#endif

        if (initial) {
#ifdef Q_OS_MAC
            if (opts.nonnativeMenubarApps.contains("kde") || opts.nonnativeMenubarApps.contains(appName)) {
                QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);
            }
#endif
            QDBusConnection::sessionBus().connect(
                QString(), "/KGlobalSettings", "org.kde.KGlobalSettings",
                "notifyChange", this, SLOT(kdeGlobalSettingsChange(int, int)));
            QDBusConnection::sessionBus().connect(
                "org.kde.kwin", "/KWin", "org.kde.KWin",
                "compositingToggled", this, SLOT(compositingToggled()));

            if (!qApp || QString(qApp->argv()[0]) != "kwin") {
                QDBusConnection::sessionBus().connect(
                    "org.kde.kwin", "/QtCurve", "org.kde.QtCurve",
                    "borderSizesChanged", this, SLOT(borderSizesChanged()));
                if (opts.menubarHiding & HIDE_KWIN) {
                    QDBusConnection::sessionBus().connect(
                        "org.kde.kwin", "/QtCurve", "org.kde.QtCurve",
                        "toggleMenuBar", this,
                        SLOT(toggleMenuBar(unsigned int)));
                }

                if (opts.statusbarHiding & HIDE_KWIN) {
                    QDBusConnection::sessionBus().connect(
                        "org.kde.kwin", "/QtCurve", "org.kde.QtCurve",
                        "toggleStatusBar", this,
                        SLOT(toggleStatusBar(unsigned int)));
                }
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
    shadeColors(QApplication::palette().color(QPalette::Active, QPalette::Highlight), m_focusCols);
    shadeColors(QApplication::palette().color(QPalette::Active, QPalette::Highlight), m_mouseOverCols);
// Dont setup KDE4 fonts/colours here - seems to mess things up when using proxy styles.
// See http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=638629
//#ifdef QTC_QT4_ENABLE_KDE
//    setupKde4();
//#endif

    m_windowManager->initialize(opts.windowDrag, opts.windowDragWhiteList.toList(), opts.windowDragBlackList.toList());

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
            if(!m_sliderCols)
                m_sliderCols=new QColor [TOTAL_SHADES+1];
            shadeColors(SHADE_BLEND_SELECTED==opts.shadeSliders
                            ? midColor(m_highlightCols[ORIGINAL_SHADE],
                                       m_buttonCols[ORIGINAL_SHADE])
                            : opts.customSlidersColor,
                        m_sliderCols);
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
                            m_highlightCols[ORIGINAL_SHADE], DEF_BNT_TINT), m_defBtnCols);
            break;
        default:
            break;
        case IND_COLORED:
            if(SHADE_BLEND_SELECTED==opts.shadeSliders)
                m_defBtnCols=m_sliderCols;
            else
            {
                m_defBtnCols=new QColor [TOTAL_SHADES+1];
                shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE],
                                    m_buttonCols[ORIGINAL_SHADE]), m_defBtnCols);
            }
    }

    switch(opts.comboBtn)
    {
        default:
        case SHADE_DARKEN:
        case SHADE_NONE:
            break;
        case SHADE_SELECTED:
            m_comboBtnCols=m_highlightCols;
            break;
        case SHADE_BLEND_SELECTED:
            if (opts.shadeSliders==SHADE_BLEND_SELECTED) {
                m_comboBtnCols=m_sliderCols;
                break;
            }
            QTC_FALLTHROUGH();
        case SHADE_CUSTOM:
            if(opts.shadeSliders==SHADE_CUSTOM && opts.customSlidersColor==opts.customComboBtnColor)
            {
                m_comboBtnCols=m_sliderCols;
                break;
            }
            if(!m_comboBtnCols)
                m_comboBtnCols=new QColor [TOTAL_SHADES+1];
            shadeColors(SHADE_BLEND_SELECTED==opts.comboBtn
                            ? midColor(m_highlightCols[ORIGINAL_SHADE],
                                       m_buttonCols[ORIGINAL_SHADE])
                            : opts.customComboBtnColor,
                        m_comboBtnCols);
    }

    switch(opts.sortedLv)
    {
        case SHADE_DARKEN:
            if(!m_sortedLvColors)
                m_sortedLvColors=new QColor [TOTAL_SHADES+1];
            shadeColors(shade(opts.lvButton ? m_buttonCols[ORIGINAL_SHADE] : m_backgroundCols[ORIGINAL_SHADE], LV_HEADER_DARK_FACTOR), m_sortedLvColors);
            break;
        default:
        case SHADE_NONE:
            break;
        case SHADE_SELECTED:
            m_sortedLvColors=m_highlightCols;
            break;
        case SHADE_BLEND_SELECTED:
            if(SHADE_BLEND_SELECTED==opts.shadeSliders)
            {
                m_sortedLvColors=m_sliderCols;
                break;
            }
            else if(SHADE_BLEND_SELECTED==opts.comboBtn)
            {
                m_sortedLvColors=m_comboBtnCols;
                break;
            }
            QTC_FALLTHROUGH();
        case SHADE_CUSTOM:
            if(opts.shadeSliders==SHADE_CUSTOM && opts.customSlidersColor==opts.customSortedLvColor)
            {
                m_sortedLvColors=m_sliderCols;
                break;
            }
            if(opts.comboBtn==SHADE_CUSTOM && opts.customComboBtnColor==opts.customSortedLvColor)
            {
                m_sortedLvColors=m_comboBtnCols;
                break;
            }
            if(!m_sortedLvColors)
                m_sortedLvColors=new QColor [TOTAL_SHADES+1];
            shadeColors(SHADE_BLEND_SELECTED==opts.sortedLv
                            ? midColor(m_highlightCols[ORIGINAL_SHADE],
                                       (opts.lvButton ? m_buttonCols[ORIGINAL_SHADE] : m_backgroundCols[ORIGINAL_SHADE]))
                            : opts.customSortedLvColor,
                        m_sortedLvColors);
    }

    switch(opts.crColor)
    {
        default:
        case SHADE_NONE:
            m_checkRadioSelCols=m_buttonCols;
            break;
        case SHADE_DARKEN:
            if(!m_checkRadioSelCols)
                m_checkRadioSelCols=new QColor [TOTAL_SHADES+1];
            shadeColors(shade(m_buttonCols[ORIGINAL_SHADE], LV_HEADER_DARK_FACTOR), m_checkRadioSelCols);
            break;
        case SHADE_SELECTED:
            m_checkRadioSelCols=m_highlightCols;
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

    if(IMG_PLAIN_RINGS==opts.bgndImage.type || IMG_BORDERED_RINGS==opts.bgndImage.type ||
       IMG_SQUARE_RINGS==opts.bgndImage.type ||
       IMG_PLAIN_RINGS==opts.menuBgndImage.type || IMG_BORDERED_RINGS==opts.menuBgndImage.type ||
       IMG_SQUARE_RINGS==opts.menuBgndImage.type)
        qtcCalcRingAlphas(&m_backgroundCols[ORIGINAL_SHADE]);

    m_blurHelper->setEnabled(100!=opts.bgndOpacity || 100!=opts.dlgOpacity || 100!=opts.menuBgndOpacity);

#ifdef QTC_QT4_ENABLE_KDE
    // We need to set the decoration colours for the preview now...
    if (m_isPreview) {
        setDecorationColors();
    }
#endif
}

Style::~Style()
{
    freeColors();
    if (m_dBus) {
        delete m_dBus;
    }
}

void Style::freeColor(QSet<QColor*> &freedColors, QColor **cols)
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
        *cols!=m_coloredHighlightCols)
    {
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

static QString getFile(const QString &f)
{
    QString d(f);

    int slashPos(d.lastIndexOf('/'));

    if(slashPos!=-1)
        d.remove(0, slashPos+1);

    return d;
}

void Style::polish(QApplication *app)
{
    appName=getFile(app->argv()[0]);

    if("kwin"==appName)
        theThemedApp=APP_KWIN;
    else if("systemsettings"==appName)
        theThemedApp = APP_SYSTEMSETTINGS;
    else if("plasma"==appName || appName.startsWith("plasma-"))
        theThemedApp = APP_PLASMA;
    else if("krunner"==appName || "krunner_lock"==appName || "kscreenlocker"==appName)
        theThemedApp=APP_KRUNNER;
    else if("kontact"==appName)
        theThemedApp=APP_KONTACT;
    else if("k3b"==appName)
        theThemedApp=APP_K3B;
    else if("arora"==appName)
        theThemedApp=APP_ARORA;
    else if("rekonq"==appName)
        theThemedApp=APP_REKONQ;
    else if("Designer"==QCoreApplication::applicationName())
        theThemedApp=APP_QTDESIGNER;
    else if("QtCreator"==QCoreApplication::applicationName())
        theThemedApp=APP_QTCREATOR;
    else if("kdevelop"==appName || "kdevelop.bin"==appName)
        theThemedApp=APP_KDEVELOP;
    else if("soffice.bin"==appName)
        theThemedApp=APP_OPENOFFICE;
    else if("kdmgreet"==appName)
        opts.forceAlternateLvCols=false;

    qtcInfo("QtCurve: Application name: \"%s\"\n",
            appName.toLatin1().constData());

    if(APP_REKONQ==theThemedApp)
        opts.statusbarHiding=0;
    if(opts.menubarHiding)
        m_saveMenuBarStatus=opts.menubarApps.contains("kde") || opts.menubarApps.contains(appName);
    if(opts.statusbarHiding)
        m_saveStatusBarStatus=opts.statusbarApps.contains("kde") || opts.statusbarApps.contains(appName);

    if(!qtcIsFlatBgnd(opts.bgndAppearance) && opts.noBgndGradientApps.contains(appName))
        opts.bgndAppearance=APPEARANCE_FLAT;
    if(IMG_NONE!=opts.bgndImage.type && opts.noBgndImageApps.contains(appName))
        opts.bgndImage.type=IMG_NONE;
    if(SHADE_NONE!=opts.menuStripe && opts.noMenuStripeApps.contains(appName))
        opts.menuStripe=SHADE_NONE;

    if((100!=opts.bgndOpacity || 100!=opts.dlgOpacity) && (opts.noBgndOpacityApps.contains(appName) || appName.endsWith(".kss")))
        opts.bgndOpacity=opts.dlgOpacity=100;
    if(100!=opts.menuBgndOpacity && opts.noMenuBgndOpacityApps.contains(appName))
        opts.menuBgndOpacity=100;

    if (theThemedApp == APP_KWIN) {
        opts.bgndAppearance = APPEARANCE_FLAT;
    } else if (theThemedApp == APP_OPENOFFICE) {
        opts.scrollbarType = SCROLLBAR_WINDOWS;
        if (opts.menuitemAppearance == APPEARANCE_FADE)
            opts.menuitemAppearance = APPEARANCE_FLAT;
        opts.borderMenuitems = opts.etchEntry = false;

        if(opts.useHighlightForMenu && blendOOMenuHighlight(QApplication::palette(), m_highlightCols[ORIGINAL_SHADE]))
        {
            m_ooMenuCols=new QColor [TOTAL_SHADES+1];
            shadeColors(tint(popupMenuCols()[ORIGINAL_SHADE], m_highlightCols[ORIGINAL_SHADE], 0.5), m_ooMenuCols);
        }
        opts.menubarHiding=opts.statusbarHiding=HIDE_NONE;
        opts.square|=SQUARE_POPUP_MENUS|SQUARE_TOOLTIPS;
        if(!qtcIsFlatBgnd(opts.menuBgndAppearance) && 0==opts.lighterPopupMenuBgnd)
            opts.lighterPopupMenuBgnd=1; // shade so that we dont have 3d-ish borders...
        opts.menuBgndAppearance=APPEARANCE_FLAT;
    }

    if (opts.useQtFileDialogApps.contains(appName)) {
        qt_filedialog_existing_directory_hook = 0L;
        qt_filedialog_open_filename_hook = 0L;
        qt_filedialog_open_filenames_hook = 0L;
        qt_filedialog_save_filename_hook = 0L;
    }

    BaseStyle::polish(app);
    if (opts.hideShortcutUnderline) {
        app->installEventFilter(m_shortcutHandler);
    }
}

void Style::polish(QPalette &palette)
{
    int  contrast(QSettings(QLatin1String("Trolltech")).value("/Qt/KDE/contrast", DEFAULT_CONTRAST).toInt());
    bool newContrast(false);

    if(contrast<0 || contrast>10)
        contrast=DEFAULT_CONTRAST;

    if(contrast!=opts.contrast)
    {
        opts.contrast=contrast;
        newContrast=true;
    }

    bool newHighlight(newContrast ||
                 m_highlightCols[ORIGINAL_SHADE]!=palette.color(QPalette::Active, QPalette::Highlight)),
         newGray(newContrast ||
                 m_backgroundCols[ORIGINAL_SHADE]!=palette.color(QPalette::Active, QPalette::Background)),
         newButton(newContrast ||
                   m_buttonCols[ORIGINAL_SHADE]!=palette.color(QPalette::Active, QPalette::Button)),
         newSlider(m_sliderCols && m_highlightCols!=m_sliderCols && SHADE_BLEND_SELECTED==opts.shadeSliders &&
                   (newButton || newHighlight)),
         newDefBtn(m_defBtnCols && (IND_COLORED!=opts.defBtnIndicator || SHADE_BLEND_SELECTED!=opts.shadeSliders) &&
                   IND_SELECTED!=opts.defBtnIndicator && IND_GLOW!=opts.defBtnIndicator &&
                   (newContrast || newButton || newHighlight)),
         newComboBtn(m_comboBtnCols && m_highlightCols!=m_comboBtnCols && m_sliderCols!=m_comboBtnCols &&
                     SHADE_BLEND_SELECTED==opts.comboBtn &&
                     (newButton || newHighlight)),
         newSortedLv(m_sortedLvColors && ((SHADE_BLEND_SELECTED==opts.sortedLv && m_defBtnCols!=m_sortedLvColors &&
                                            m_sliderCols!=m_sortedLvColors && m_comboBtnCols!=m_sortedLvColors) ||
                                             SHADE_DARKEN==opts.sortedLv) &&
                     (newContrast || (opts.lvButton ? newButton : newGray))),
         newCheckRadioSelCols(m_checkRadioSelCols && ((SHADE_BLEND_SELECTED==opts.crColor && m_defBtnCols!=m_checkRadioSelCols &&
                                                         m_sliderCols!=m_checkRadioSelCols && m_comboBtnCols!=m_checkRadioSelCols &&
                                                         m_sortedLvColors!=m_checkRadioSelCols) ||
                                             SHADE_DARKEN==opts.crColor) &&
                     (newContrast || newButton)),
         newProgressCols(m_progressCols && SHADE_BLEND_SELECTED==opts.progressColor &&
                         m_sliderCols!=m_progressCols && m_comboBtnCols!=m_progressCols &&
                         m_sortedLvColors!=m_progressCols && m_checkRadioSelCols!=m_progressCols && (newContrast || newButton));

    if(newGray)
    {
        shadeColors(palette.color(QPalette::Active, QPalette::Background), m_backgroundCols);
        if(IMG_PLAIN_RINGS==opts.bgndImage.type || IMG_BORDERED_RINGS==opts.bgndImage.type ||
           IMG_SQUARE_RINGS==opts.bgndImage.type ||
           IMG_PLAIN_RINGS==opts.menuBgndImage.type || IMG_BORDERED_RINGS==opts.menuBgndImage.type ||
           IMG_SQUARE_RINGS==opts.menuBgndImage.type)
        {
            qtcCalcRingAlphas(&m_backgroundCols[ORIGINAL_SHADE]);
            if(m_usePixmapCache)
                QPixmapCache::clear();
        }
    }

    if(newButton)
        shadeColors(palette.color(QPalette::Active, QPalette::Button), m_buttonCols);

    if(newHighlight)
        shadeColors(palette.color(QPalette::Active, QPalette::Highlight), m_highlightCols);

// Dont set these here, they will be updated in setDecorationColors()...
//     shadeColors(QApplication::palette().color(QPalette::Active, QPalette::Highlight), m_focusCols);
//     if(opts.coloredMouseOver)
//         shadeColors(QApplication::palette().color(QPalette::Active, QPalette::Highlight), m_mouseOverCols);

    setMenuColors(palette.color(QPalette::Active, QPalette::Background));

    if(newSlider)
        shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE], m_buttonCols[ORIGINAL_SHADE]), m_sliderCols);

    if(newDefBtn)
    {
        if(IND_TINT==opts.defBtnIndicator)
            shadeColors(tint(m_buttonCols[ORIGINAL_SHADE],
                        m_highlightCols[ORIGINAL_SHADE], DEF_BNT_TINT), m_defBtnCols);
        else if(IND_GLOW!=opts.defBtnIndicator)
            shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE],
                        m_buttonCols[ORIGINAL_SHADE]), m_defBtnCols);
    }

    if(newComboBtn)
        shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE], m_buttonCols[ORIGINAL_SHADE]), m_comboBtnCols);

    if(newSortedLv)
    {
        if(SHADE_BLEND_SELECTED==opts.sortedLv)
            shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE],
                        opts.lvButton ? m_buttonCols[ORIGINAL_SHADE] : m_backgroundCols[ORIGINAL_SHADE]), m_sortedLvColors);
        else
            shadeColors(shade(opts.lvButton ? m_buttonCols[ORIGINAL_SHADE] : m_backgroundCols[ORIGINAL_SHADE], LV_HEADER_DARK_FACTOR),
                        m_sortedLvColors);
    }

    if(m_sidebarButtonsCols && SHADE_BLEND_SELECTED!=opts.shadeSliders &&
       IND_COLORED!=opts.defBtnIndicator)
        shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE],
                   m_buttonCols[ORIGINAL_SHADE]), m_sidebarButtonsCols);

    switch(opts.shadeCheckRadio)
    {
        default:
            m_checkRadioCol=palette.color(QPalette::Active, opts.crButton ? QPalette::ButtonText : QPalette::Text);
            break;
        case SHADE_BLEND_SELECTED:
        case SHADE_SELECTED:
            m_checkRadioCol=palette.color(QPalette::Active, QPalette::Highlight);
            break;
        case SHADE_CUSTOM:
             m_checkRadioCol=opts.customCheckRadioColor;
    }

    if(newCheckRadioSelCols)
    {
        if(SHADE_BLEND_SELECTED==opts.crColor)
            shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE], m_buttonCols[ORIGINAL_SHADE]), m_checkRadioSelCols);
        else
            shadeColors(shade(m_buttonCols[ORIGINAL_SHADE], LV_HEADER_DARK_FACTOR), m_checkRadioSelCols);
    }

    if(newProgressCols)
        shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE], m_backgroundCols[ORIGINAL_SHADE]), m_progressCols);

    if(APP_OPENOFFICE==theThemedApp && opts.useHighlightForMenu && (newGray || newHighlight))
    {
        if(blendOOMenuHighlight(palette, m_highlightCols[ORIGINAL_SHADE]))
        {
            if(!m_ooMenuCols)
                m_ooMenuCols=new QColor [TOTAL_SHADES+1];
            shadeColors(tint(popupMenuCols()[ORIGINAL_SHADE], m_highlightCols[ORIGINAL_SHADE], 0.5), m_ooMenuCols);
        }
        else if(m_ooMenuCols)
        {
            delete [] m_ooMenuCols;
            m_ooMenuCols=0L;
        }
    }

    palette.setColor(QPalette::Active, QPalette::Light, m_backgroundCols[0]);
    palette.setColor(QPalette::Active, QPalette::Dark, m_backgroundCols[QTC_STD_BORDER]);
    palette.setColor(QPalette::Inactive, QPalette::Light, m_backgroundCols[0]);
    palette.setColor(QPalette::Inactive, QPalette::Dark, m_backgroundCols[QTC_STD_BORDER]);
    palette.setColor(QPalette::Inactive, QPalette::WindowText, palette.color(QPalette::Active, QPalette::WindowText));
    palette.setColor(QPalette::Disabled, QPalette::Light, m_backgroundCols[0]);
    palette.setColor(QPalette::Disabled, QPalette::Dark, m_backgroundCols[QTC_STD_BORDER]);

    palette.setColor(QPalette::Disabled, QPalette::Base, palette.color(QPalette::Active, QPalette::Background));
    palette.setColor(QPalette::Disabled, QPalette::Background, palette.color(QPalette::Active, QPalette::Background));

    // Fix KDE4's palette...
    if(palette.color(QPalette::Active, QPalette::Highlight)!=palette.color(QPalette::Inactive, QPalette::Highlight))
        m_inactiveChangeSelectionColor=true;

    for(int i=QPalette::WindowText; i<QPalette::NColorRoles; ++i)
        //if(i!=QPalette::Highlight && i!=QPalette::HighlightedText)
            palette.setColor(QPalette::Inactive, (QPalette::ColorRole)i, palette.color(QPalette::Active, (QPalette::ColorRole)i));

    // Force this to be re-generated!
    if(SHADE_BLEND_SELECTED==opts.menuStripe)
        opts.customMenuStripeColor=Qt::black;
#ifdef QTC_QT4_ENABLE_KDE
    // Only set palette here...
    if(kapp)
        setDecorationColors();
#endif
}

void
Style::polish(QWidget *widget)
{
    // TODO Reorganize this polish function
    if (!widget)
        return;

    QtcQWidgetProps qtcProps(widget);
    bool enableMouseOver(opts.highlightFactor || opts.coloredMouseOver);
    if (Log::checkLevel(LogLevel::Info) && qtcGetWid(widget) &&
        widget->windowType() != Qt::Desktop && !qtcProps->prePolished) {
        qDebug() << "Window Created before polishing:" << widget;
    }
    qtcProps->prePolished = true;

    if (opts.buttonEffect != EFFECT_NONE && !USE_CUSTOM_ALPHAS(opts) &&
        isNoEtchWidget(widget)) {
        qtcProps->noEtch = true;
    }

    m_windowManager->registerWidget(widget);
    m_shadowHelper->registerWidget(widget);

    // Need to register all widgets to blur helper, in order to have proper
    // blur_behind region set have proper regions removed for opaque widgets.
    // Note: that the helper does nothing as long as compositing and ARGB
    // are not enabled
    const bool isDialog = qtcIsDialog(widget->window());
    if ((opts.menuBgndOpacity != 100 &&
         (qobject_cast<QMenu*>(widget) ||
          // TODO temporary solution only
          widget->inherits("QComboBoxPrivateContainer"))) ||
        (opts.bgndOpacity != 100 && (!widget->window() || !isDialog)) ||
        (opts.dlgOpacity != 100 && (!widget->window() || isDialog))) {
        m_blurHelper->registerWidget(widget);
    }
    // Sometimes get background errors with QToolBox (e.g. in Bespin config),
    // and setting WA_StyledBackground seems to fix this,..
    if (qtcIsCustomBgnd(opts) ||
        oneOf(opts.groupBox, FRAME_SHADED, FRAME_FADED)) {
        switch (widget->windowType()) {
        case Qt::Window:
        case Qt::Sheet:
        case Qt::Dialog: {
            // For non-transparent widgets, only need to set
            // WA_StyledBackground - and PE_Widget will be called to
            // render background...
            widget->setAttribute(Qt::WA_StyledBackground);
            break;
        }
        case Qt::Popup:
            // we currently don't want that kind of gradient on menus etc
        case Qt::Drawer:
        case Qt::Tool:
            // this we exclude as it is used for dragging of icons etc
        default:
            break;
        }
        if (qobject_cast<QSlider*>(widget)) {
            widget->setBackgroundRole(QPalette::NoRole);
        }
        if (widget->autoFillBackground() && widget->parentWidget() &&
            widget->parentWidget()->objectName() == "qt_scrollarea_viewport" &&
            qtcCheckType<QAbstractScrollArea>(getParent<2>(widget)) &&
            qtcCheckType<QToolBox>(getParent<3>(widget))) {
            widget->parentWidget()->setAutoFillBackground(false);
            widget->setAutoFillBackground(false);
        }
    }

    // Translucent main window seems to cause QMdiSubWindow to not draw its
    // background. Not sure what exactly the problem is but drawing the
    // background ourselves by setting Qt::WA_StyledBackground
    // as well as handling it in drawPrimitive seems to fix/workaround
    // the problem.
    // TODO:
    // Figuring out what's wrong.
    // Check what oxygen does to QMdiSubWindow
    // Check Qt5 version
    if (qobject_cast<QMdiSubWindow*>(widget)) {
        widget->setAttribute(Qt::WA_StyledBackground);
    }

    if (opts.menubarHiding && qobject_cast<QMainWindow*>(widget) &&
        static_cast<QMainWindow*>(widget)->menuWidget()) {
        widget->installEventFilter(this);
        if (m_saveMenuBarStatus) {
            static_cast<QMainWindow*>(widget)->menuWidget()
                ->installEventFilter(this);
        }
        if (m_saveMenuBarStatus && qtcMenuBarHidden(appName)) {
            static_cast<QMainWindow*>(widget)->menuWidget()->setHidden(true);
            if (BLEND_TITLEBAR || opts.menubarHiding & HIDE_KWIN ||
                opts.windowBorder &
                WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR) {
                emitMenuSize(static_cast<QMainWindow*>(widget)->menuWidget(), 0);
            }
        }
    }

    if (opts.statusbarHiding && qobject_cast<QMainWindow*>(widget)) {
        QList<QStatusBar*> sb = getStatusBars(widget);
        if (sb.count()) {
            widget->installEventFilter(this);
            foreach (QStatusBar *statusBar, sb) {
                if (m_saveStatusBarStatus) {
                    statusBar->installEventFilter(this);
                }
                if (m_saveStatusBarStatus && qtcStatusBarHidden(appName)) {
                    statusBar->setHidden(true);
                }
            }
            setSbProp(widget);
            emitStatusBarState(sb.first());
        }
    }

    // Enable hover effects in all itemviews
    if (QAbstractItemView *itemView =
        qobject_cast<QAbstractItemView*>(widget)) {
        QWidget *viewport=itemView->viewport();
        viewport->setAttribute(Qt::WA_Hover);

        if(opts.forceAlternateLvCols &&
           viewport->autoFillBackground() && // Dolphins Folders panel
           //255==viewport->palette().color(itemView->viewport()->backgroundRole()).alpha() && // KFilePlacesView
           !widget->inherits("KFilePlacesView") &&
           // Exclude non-editable combo popup...
           !(opts.gtkComboMenus && widget->inherits("QComboBoxListView") &&
             qtcCheckType<QComboBox>(getParent<2>(widget)) &&
             !static_cast<QComboBox*>(getParent<2>(widget))->isEditable()) &&
           // Exclude KAboutDialog...
           !qtcCheckKDEType(getParent<5>(widget), KAboutApplicationDialog) &&
           (qobject_cast<QTreeView*>(widget) ||
            (qobject_cast<QListView*>(widget) &&
             ((QListView*)widget)->viewMode() != QListView::IconMode))) {
            itemView->setAlternatingRowColors(true);
        }
    }

    if (APP_KONTACT == theThemedApp && qobject_cast<QToolButton*>(widget))
        ((QToolButton*)widget)->setAutoRaise(true);

    if(enableMouseOver &&
       (qobject_cast<QPushButton*>(widget) ||
        qobject_cast<QAbstractButton*>(widget) ||
        qobject_cast<QComboBox*>(widget) ||
        qobject_cast<QAbstractSpinBox*>(widget) ||
        qobject_cast<QCheckBox*>(widget) ||
        qobject_cast<QGroupBox*>(widget) ||
        qobject_cast<QRadioButton*>(widget) ||
        qobject_cast<QSplitterHandle*>(widget) ||
        qobject_cast<QSlider*>(widget) ||
        qobject_cast<QHeaderView*>(widget) ||
        qobject_cast<QTabBar*>(widget) ||
        qobject_cast<QAbstractScrollArea*>(widget) ||
        qobject_cast<QTextEdit*>(widget) ||
        qobject_cast<QLineEdit*>(widget) ||
        qobject_cast<QDial*>(widget) ||
        // qobject_cast<QDockWidget*>(widget) ||
        widget->inherits("QWorkspaceTitleBar") ||
        widget->inherits("QDockSeparator") ||
        widget->inherits("QDockWidgetSeparator") ||
        widget->inherits("Q3DockWindowResizeHandle")))
        widget->setAttribute(Qt::WA_Hover, true);

    if (qobject_cast<QSplitterHandle*>(widget)) {
        widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
    } else if (qobject_cast<QScrollBar*>(widget)) {
        if (enableMouseOver) {
            widget->setAttribute(Qt::WA_Hover, true);
        }
        widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
        if (!opts.gtkScrollViews) {
            widget->installEventFilter(this);
        }
    } else if (qobject_cast<QAbstractScrollArea*>(widget) &&
               widget->inherits("KFilePlacesView")) {
        if (qtcIsCustomBgnd(opts)) {
            polishScrollArea(static_cast<QAbstractScrollArea*>(widget), true);
        }
        widget->installEventFilter(this);
    } else if (qobject_cast<QProgressBar*>(widget)) {
        if (widget->palette().color(QPalette::Inactive,
                                    QPalette::HighlightedText) !=
            widget->palette().color(QPalette::Active,
                                    QPalette::HighlightedText)) {
            QPalette pal(widget->palette());
            pal.setColor(QPalette::Inactive, QPalette::HighlightedText,
                         pal.color(QPalette::Active,
                                   QPalette::HighlightedText));
            widget->setPalette(pal);
        }
        if (opts.boldProgress) {
            setBold(widget);
        }
        widget->installEventFilter(this);
    } else if (widget->inherits("Q3Header")) {
        widget->setMouseTracking(true);
        widget->installEventFilter(this);
    } else if (opts.highlightScrollViews && widget->inherits("Q3ScrollView")) {
        widget->installEventFilter(this);
        widget->setAttribute(Qt::WA_Hover, true);
    } else if (qobject_cast<QMenuBar*>(widget)) {
        if (opts.xbar &&
            (!(oneOf(theThemedApp, APP_QTDESIGNER, APP_KDEVELOP) &&
               widget->inherits("QDesignerMenuBar")))) {
            Bespin::MacMenu::manage((QMenuBar*)widget);
        }
        if (BLEND_TITLEBAR || opts.menubarHiding & HIDE_KWIN ||
            opts.windowBorder & WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR) {
            emitMenuSize(widget, PREVIEW_MDI == m_isPreview ||
                         !widget->isVisible() ? 0 : widget->rect().height());
        }
        if (qtcIsCustomBgnd(opts)) {
            widget->setBackgroundRole(QPalette::NoRole);
        }
        widget->setAttribute(Qt::WA_Hover, true);

        // if(opts.shadeMenubarOnlyWhenActive && SHADE_NONE!=opts.shadeMenubars)
        widget->installEventFilter(this);

        setMenuTextColors(widget, true);
    } else if (qobject_cast<QLabel*>(widget)) {
        widget->installEventFilter(this);
        if (WM_DRAG_ALL == opts.windowDrag &&
            (((QLabel*)widget)->textInteractionFlags()
             .testFlag(Qt::TextSelectableByMouse)) &&
            qtcCheckType<QFrame>(widget->parentWidget()) &&
            qtcCheckKDEType(getParent<2>(widget), KTitleWidget))
            ((QLabel*)widget)->setTextInteractionFlags(((QLabel*)widget)->textInteractionFlags()&~Qt::TextSelectableByMouse);

    } else if (/*!opts.gtkScrollViews && */qobject_cast<QAbstractScrollArea*>(widget)) {
        if(qtcIsCustomBgnd(opts))
            polishScrollArea(static_cast<QAbstractScrollArea*>(widget));
        if (!opts.gtkScrollViews && (((QFrame*)widget)->frameWidth() > 0)) {
            widget->installEventFilter(this);
        }
        if (theThemedApp == APP_KONTACT && widget->parentWidget()) {
            QWidget *frame = scrollViewFrame(widget->parentWidget());

            if (frame) {
                frame->installEventFilter(this);
                m_sViewContainers[frame].insert(widget);
                connect(widget, SIGNAL(destroyed(QObject*)),
                        this, SLOT(widgetDestroyed(QObject*)));
                connect(frame, SIGNAL(destroyed(QObject*)),
                        this, SLOT(widgetDestroyed(QObject*)));
            }
        }
    } else if(qobject_cast<QDialog*>(widget) &&
              widget->inherits("QPrintPropertiesDialog") &&
              widget->parentWidget() && widget->window() &&
              widget->parentWidget()->window() &&
              widget->window()->windowTitle().isEmpty() &&
              !widget->parentWidget()->window()->windowTitle().isEmpty()) {
        widget->window()->setWindowTitle(widget->parentWidget()->window()
                                         ->windowTitle());
    } else if (widget->inherits("QWhatsThat")) {
        QPalette pal(widget->palette());
        QColor shadow(pal.shadow().color());

        shadow.setAlpha(32);
        pal.setColor(QPalette::Shadow, shadow);
        widget->setPalette(pal);
        widget->setMask(QRegion(widget->rect().adjusted(0, 0, -6, -6))+QRegion(widget->rect().adjusted(6, 6, 0, 0)));
    } else if (qobject_cast<QDockWidget*>(widget) &&
               qtcCheckType<QSplitter>(widget->parentWidget()) &&
               qtcCheckType(getParent<2>(widget), "KFileWidget")) {
        ((QDockWidget*)widget)
            ->setTitleBarWidget(new QtCurveDockWidgetTitleBar(widget));
    }

    if (widget->inherits("QTipLabel") &&
        !qtcIsFlat(opts.tooltipAppearance)) {
        widget->setBackgroundRole(QPalette::NoRole);
        // TODO: turn this into addAlphaChannel
        widget->setAttribute(Qt::WA_TranslucentBackground);
    }

    if (!widget->isWindow())
        if (QFrame *frame = qobject_cast<QFrame*>(widget)) {
            // kill ugly frames...
            if (QFrame::Box == frame->frameShape() ||
                QFrame::Panel == frame->frameShape() ||
                QFrame::WinPanel == frame->frameShape())
                frame->setFrameShape(QFrame::StyledPanel);
            // else if (QFrame::HLine==frame->frameShape() || QFrame::VLine==frame->frameShape())
            widget->installEventFilter(this);
            if (qtcCheckKDEType(widget->parent(), KTitleWidget)) {
                if(qtcIsCustomBgnd(opts))
                    frame->setAutoFillBackground(false);
                else
                    frame->setBackgroundRole(QPalette::Window);

                QLayout *layout(frame->layout());

                if (layout) {
                    layout->setMargin(0);
                }
            }

            QComboBox *p = nullptr;
            if (opts.gtkComboMenus &&
                (p = qtcObjCast<QComboBox>(getParent<2>(widget))) &&
                !p->isEditable()) {
                QPalette pal(widget->palette());
                QColor col(popupMenuCols()[ORIGINAL_SHADE]);

                if(!qtcIsFlatBgnd(opts.menuBgndAppearance) || 100!=opts.menuBgndOpacity || !POPUP_MENUS_SQUARE(opts))
                    col.setAlphaF(0);

                pal.setBrush(QPalette::Active, QPalette::Base, col);
                pal.setBrush(QPalette::Active, QPalette::Window, col);
                widget->setPalette(pal);
                if(opts.shadePopupMenu)
                    setMenuTextColors(widget, false);
            }
        }

    if (qobject_cast<QMenu*>(widget)) {
        if (!qtcIsFlatBgnd(opts.menuBgndAppearance) ||
            opts.menuBgndOpacity != 100 ||
            !POPUP_MENUS_SQUARE(opts)) {
            widget->installEventFilter(this);
            // Set WA_NoSystemBackground or a square background will be drawn.
            widget->setAttribute(Qt::WA_NoSystemBackground);
        }
        if (opts.lighterPopupMenuBgnd || opts.shadePopupMenu) {
            QPalette pal = widget->palette();

            pal.setBrush(QPalette::Active, QPalette::Window,
                         popupMenuCols()[ORIGINAL_SHADE]);
            widget->setPalette(pal);
            if (opts.shadePopupMenu)
                setMenuTextColors(widget, false);
            if (IMG_NONE != opts.menuBgndImage.type) {
                widget->installEventFilter(this);
            }
        }
    }

    if ((!qtcIsFlatBgnd(opts.menuBgndAppearance) ||
         opts.menuBgndOpacity != 100 || !POPUP_MENUS_SQUARE(opts)) &&
        widget->inherits("QComboBoxPrivateContainer")) {
        widget->installEventFilter(this);
        // Set WA_NoSystemBackground or a square background will be drawn.
        widget->setAttribute(Qt::WA_NoSystemBackground);
    }

    bool parentIsToolbar(false);

    // Using dark menubars - konqueror's combo box texts get messed up. Seems to be when a plain QWidget has widget->setBackgroundRole(QPalette::Window);
    // and widget->setAutoFillBackground(false); set (below). These onyl happen if 'parentIsToolbar' - so dont bather detecting this if the widget
    // is a plain QWidget
    //
    // QWidget QComboBoxListView QComboBoxPrivateContainer SearchBarCombo KToolBar KonqMainWindow
    // QWidget KCompletionBox KLineEdit SearchBarCombo KToolBar KonqMainWindow
    if (noneOf(widget->metaObject()->className(), "QWidget")) {
        QWidget *wid = widget ? widget->parentWidget() : nullptr;
        while (wid && !parentIsToolbar) {
            parentIsToolbar = (qobject_cast<QToolBar*>(wid) ||
                               wid->inherits("Q3ToolBar"));
            wid = wid->parentWidget();
        }
    }

    if (theThemedApp == APP_QTCREATOR &&
        qobject_cast<QMainWindow*>(widget) &&
        static_cast<QMainWindow*>(widget)->menuWidget()) {
        static_cast<QMainWindow*>(widget)->menuWidget()->setStyle(this);
    }

    if (theThemedApp == APP_QTCREATOR && qobject_cast<QDialog*>(widget) &&
        qtcCheckKDEType(widget, KFileDialog)) {
        if (QToolBar *tb = getToolBarChild(widget)) {
            int size = pixelMetric(PM_ToolBarIconSize);
            tb->setIconSize(QSize(size, size));
            tb->setMinimumSize(QSize(size+14, size+14));
            setStyleRecursive(tb, this, size+4);
        }
    }

    if(parentIsToolbar && (qobject_cast<QComboBox*>(widget) ||
                           qobject_cast<QLineEdit*>(widget)))
        widget->setFont(QApplication::font());

    if (qobject_cast<QMenuBar*>(widget) || widget->inherits("Q3ToolBar") || qobject_cast<QToolBar*>(widget) || parentIsToolbar)
        widget->setBackgroundRole(QPalette::Window);

    if(!qtcIsFlat(opts.toolbarAppearance) && parentIsToolbar)
        widget->setAutoFillBackground(false);

    if (APP_SYSTEMSETTINGS == theThemedApp &&
        qtcCheckType<QFrame>(widget) &&
        QFrame::NoFrame != ((QFrame*)widget)->frameShape() &&
        qtcCheckType<QFrame>(widget->parentWidget()) &&
        qtcCheckType<QTabWidget>(getParent<2>(widget)))
        ((QFrame*)widget)->setFrameShape(QFrame::NoFrame);

    if (QLayout *layout = widget->layout()) {
        // explicitly check public layout classes,
        // QMainWindowLayout doesn't work here
        if (qobject_cast<QBoxLayout*>(layout) ||
            qobject_cast<QFormLayout*>(layout) ||
            qobject_cast<QGridLayout*>(layout) ||
            qobject_cast<QStackedLayout*>(layout)) {
            polishLayout(layout);
        }
    }

    if((APP_K3B==theThemedApp && widget->inherits("K3b::ThemedHeader") && qobject_cast<QFrame*>(widget)) ||
        widget->inherits("KColorPatch"))
    {
        ((QFrame*)widget)->setLineWidth(0);
        ((QFrame*)widget)->setFrameShape(QFrame::NoFrame);
    }

    if(APP_KDEVELOP==theThemedApp && !opts.stdSidebarButtons && widget->inherits("Sublime::IdealButtonBarWidget") && widget->layout())
    {
        widget->layout()->setSpacing(0);
        widget->layout()->setMargin(0);
    }

    QWidget *window = widget->window();

    if ((opts.bgndOpacity != 100 && qtcIsWindow(window)) ||
        (opts.dlgOpacity != 100 && qtcIsDialog(window))) {
        widget->installEventFilter(this);

        if (widget->inherits("KFilePlacesView")) {
            widget->setAutoFillBackground(false);
            widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
        }
    }

#ifdef QTC_QT4_ENABLE_KDE
    // Make file selection button in QPrintDialog appear more KUrlRequester like.
    if (qobject_cast<QToolButton*>(widget) &&
        qtcCheckType<QGroupBox>(widget->parentWidget()) &&
        qtcCheckType<QPrintDialog>(getParent<3>(widget)) &&
        static_cast<QToolButton*>(widget)->text() == QLatin1String("...")) {
        static_cast<QToolButton*>(widget)->setIcon(KIcon("document-open"));
        static_cast<QToolButton*>(widget)->setAutoRaise(false);
    }
#endif
}

static QFontMetrics styledFontMetrics(const QStyleOption *option, const QWidget *widget)
{
    return (option ? option->fontMetrics :
            widget ? widget->fontMetrics() : qApp->fontMetrics());
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
        if (qobject_cast<QLabel*>(label))
            labelHeight = label->sizeHint().height() + addedHeight;
        else if (qobject_cast<QCheckBox*>(label))
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
        if (qobject_cast<QCheckBox*>(label))
            label->setMinimumHeight(labelHeight);
        else
        {
#if QT_VERSION >= QT_VERSION_CHECK(4, 6, 2)
            label->setMinimumHeight((labelHeight * 4 + 6) / 7);
#else
            // QFormLayout determines label size as height * 5 / 4, so revert that
            label->setMinimumHeight((labelHeight * 4 + 4) / 5);
#endif
        }
    }
}

void Style::polishLayout(QLayout *layout)
{
    if (QFormLayout *formLayout = qobject_cast<QFormLayout*>(layout))
        polishFormLayout(formLayout);
    // recurse into layouts
    for (int i = 0; i < layout->count(); ++i)
        if (QLayout *l = layout->itemAt(i)->layout())
            polishLayout(l);
}

// Taken from oxygen!
void Style::polishScrollArea(QAbstractScrollArea *scrollArea, bool isKFilePlacesView) const
{
    if(!scrollArea)
        return;

    // HACK: add exception for KPIM transactionItemView, which is an overlay widget and must have filled background. This is a temporary workaround
    // until a more robust solution is found.
    if (scrollArea->inherits("KPIM::TransactionItemView")) {
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
    if(!(viewport && QPalette::Window==viewport->backgroundRole()) && !isKFilePlacesView)
        return;

    // change viewport autoFill background.
    // do the same for children if the background role is QPalette::Window
    viewport->setAutoFillBackground(false);
    foreach (QWidget *child, viewport->findChildren<QWidget*>()) {
        if (child->parent() == viewport &&
            QPalette::Window == child->backgroundRole()) {
            child->setAutoFillBackground(false);
        }
    }
}

void Style::unpolish(QApplication *app)
{
    if (opts.hideShortcutUnderline)
        app->removeEventFilter(m_shortcutHandler);
    BaseStyle::unpolish(app);
}

void Style::unpolish(QWidget *widget)
{
    if (!widget)
        return;
    widget->removeEventFilter(this);
    m_windowManager->unregisterWidget(widget);
    m_shadowHelper->unregisterWidget(widget);
    m_blurHelper->unregisterWidget(widget);

    // Sometimes get background errors with QToolBox (e.g. in Bespin config),
    // and setting WA_StyledBackground seems to fix this,..
    if (qtcIsCustomBgnd(opts) || FRAME_SHADED == opts.groupBox ||
        FRAME_FADED == opts.groupBox) {
        switch (widget->windowType()) {
        case Qt::Window:
        case Qt::Sheet:
        case Qt::Dialog:
            widget->setAttribute(Qt::WA_StyledBackground, false);
            break;
        case Qt::Popup:
            // we currently don't want that kind of gradient on menus etc
        case Qt::Drawer:
        case Qt::Tool:
            // this we exclude as it is used for dragging of icons etc
        default:
            break;
        }
        if (qobject_cast<QSlider*>(widget)) {
            widget->setBackgroundRole(QPalette::Window);
        }
    }

    if (// m_isPreview &&
        qobject_cast<QMdiSubWindow*>(widget))
        widget->setAttribute(Qt::WA_StyledBackground, false);

    if (opts.menubarHiding && qobject_cast<QMainWindow*>(widget) &&
        static_cast<QMainWindow*>(widget)->menuWidget()) {
        if (m_saveMenuBarStatus) {
            static_cast<QMainWindow*>(widget)->menuWidget()
                ->removeEventFilter(this);
        }
    }

    if (opts.statusbarHiding && qobject_cast<QMainWindow*>(widget)) {
        if (m_saveStatusBarStatus) {
            foreach (QStatusBar *statusBar, getStatusBars(widget)) {
                statusBar->removeEventFilter(this);
            }
        }
    }

    if(qobject_cast<QPushButton*>(widget) ||
       qobject_cast<QComboBox*>(widget) ||
       qobject_cast<QAbstractSpinBox*>(widget) ||
       qobject_cast<QCheckBox*>(widget) ||
       qobject_cast<QGroupBox*>(widget) ||
       qobject_cast<QRadioButton*>(widget) ||
       qobject_cast<QSplitterHandle*>(widget) ||
       qobject_cast<QSlider*>(widget) ||
       qobject_cast<QHeaderView*>(widget) ||
       qobject_cast<QTabBar*>(widget) ||
       qobject_cast<QAbstractScrollArea*>(widget) ||
       qobject_cast<QTextEdit*>(widget) ||
       qobject_cast<QLineEdit*>(widget) ||
       qobject_cast<QDial*>(widget) ||
       // qobject_cast<QDockWidget*>(widget) ||
       widget->inherits("QWorkspaceTitleBar") ||
       widget->inherits("QDockSeparator") ||
       widget->inherits("QDockWidgetSeparator") ||
       widget->inherits("Q3DockWindowResizeHandle"))
        widget->setAttribute(Qt::WA_Hover, false);
    if (qobject_cast<QScrollBar*>(widget)) {
        widget->setAttribute(Qt::WA_Hover, false);
        if (opts.round != ROUND_NONE && !opts.flatSbarButtons) {
            widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
        }
    } else if (qobject_cast<QProgressBar*>(widget)) {
        if(opts.boldProgress)
            unSetBold(widget);
        m_progressBars.remove((QProgressBar*)widget);
    } else if (widget->inherits("Q3Header")) {
        widget->setMouseTracking(false);
    } else if (qobject_cast<QMenuBar*>(widget)) {
        if (opts.xbar) {
            Bespin::MacMenu::release((QMenuBar*)widget);
        }
        widget->setAttribute(Qt::WA_Hover, false);

        if(qtcIsCustomBgnd(opts))
            widget->setBackgroundRole(QPalette::Background);

        if (SHADE_WINDOW_BORDER==opts.shadeMenubars || opts.customMenuTextColor || SHADE_BLEND_SELECTED==opts.shadeMenubars ||
           SHADE_SELECTED==opts.shadeMenubars || (SHADE_CUSTOM==opts.shadeMenubars &&TOO_DARK(m_menubarCols[ORIGINAL_SHADE])))
            widget->setPalette(QApplication::palette());
    } else if(/*!opts.gtkScrollViews && */qobject_cast<QAbstractScrollArea*>(widget))
    {
        if(APP_KONTACT==theThemedApp && widget->parentWidget())
        {
            QWidget *frame=scrollViewFrame(widget->parentWidget());

            if (frame) {
                if (m_sViewContainers.contains(frame)) {
                    m_sViewContainers[frame].remove(widget);
                    if (m_sViewContainers[frame].count() == 0) {
                        frame->removeEventFilter(this);
                        m_sViewContainers.remove(frame);
                        disconnect(frame, SIGNAL(destroyed(QObject*)),
                                   this, SLOT(widgetDestroyed(QObject*)));
                    }
                }
            }
        }
    } else if(qobject_cast<QDockWidget*>(widget) &&
              ((QDockWidget*)widget)->titleBarWidget() &&
              qobject_cast<QtCurveDockWidgetTitleBar*>(((QDockWidget*)widget)
                                                       ->titleBarWidget()) &&
              qtcCheckType<QSplitter>(widget->parentWidget()) &&
              qtcCheckType(getParent<2>(widget), "KFileWidget")) {
        delete ((QDockWidget*)widget)->titleBarWidget();
        ((QDockWidget*)widget)->setTitleBarWidget(0L);
    }
    else if(opts.boldProgress && "CE_CapacityBar"==widget->objectName())
        unSetBold(widget);

    if (widget->inherits("QTipLabel") && !qtcIsFlat(opts.tooltipAppearance)) {
        widget->setAttribute(Qt::WA_NoSystemBackground, false);
        widget->clearMask();
    }

    if (!widget->isWindow())
        if (QFrame *frame = qobject_cast<QFrame*>(widget)) {
            if (qtcCheckKDEType(widget->parent(), KTitleWidget)) {
                if(qtcIsCustomBgnd(opts))
                    frame->setAutoFillBackground(true);
                else
                    frame->setBackgroundRole(QPalette::Base);

                QLayout *layout(frame->layout());

                if(layout)
                    layout->setMargin(6);
            }

            QWidget *p=0L;

            if (opts.gtkComboMenus && widget->parentWidget() &&
                (p = getParent<2>(widget)) && qobject_cast<QComboBox*>(p) &&
                !((QComboBox*)(p))->isEditable()) {
                widget->setPalette(QApplication::palette());
            }
        }

    if (qobject_cast<QMenu*>(widget)) {
        widget->setAttribute(Qt::WA_NoSystemBackground, false);
        widget->clearMask();
        if (opts.lighterPopupMenuBgnd || opts.shadePopupMenu) {
            widget->setPalette(QApplication::palette());
        }
    }

    if ((!qtcIsFlatBgnd(opts.menuBgndAppearance) ||
         100 != opts.menuBgndOpacity || !POPUP_MENUS_SQUARE(opts)) &&
        widget->inherits("QComboBoxPrivateContainer")) {
        widget->setAttribute(Qt::WA_NoSystemBackground, false);
        widget->clearMask();
    }

    if (qobject_cast<QMenuBar*>(widget) ||
        widget->inherits("Q3ToolBar") ||
        qobject_cast<QToolBar*>(widget) ||
        (widget && qobject_cast<QToolBar*>(widget->parent())))
        widget->setBackgroundRole(QPalette::Button);
}

//
// QtCurve's menu's have a 2 pixel border all around - but want the top, and left edges to
// active the nearest menu item. Therefore, when we get a mouse event in that region then
// adjsut its position...
static bool updateMenuBarEvent(QMouseEvent *event, QMenuBar *menu)
{
    struct HackEvent : public QMouseEvent {
        bool
        adjust()
        {
            bool res = false;
            if (p.x() < 2) {
                p.rx() += 2;
                g.rx() += 2;
                res = true;
            }
            if (p.y() < 2) {
                p.ry() += 2;
                g.ry() += 2;
                res = true;
            }
            return res;
        }
    };

    if (((HackEvent*)event)->adjust()) {
        static_cast<QObject*>(menu)->event(event);
        return true;
    }
    return false;
}

bool Style::eventFilter(QObject *object, QEvent *event)
{
    if (qobject_cast<QMenuBar*>(object) && dynamic_cast<QMouseEvent*>(event)) {
        if (updateMenuBarEvent((QMouseEvent*)event, (QMenuBar*)object)) {
            return true;
        }
    }

    if (event->type() == QEvent::Show &&
        qobject_cast<QAbstractScrollArea*>(object) &&
        object->inherits("KFilePlacesView")) {
        QWidget *view = ((QAbstractScrollArea*)object)->viewport();
        QPalette palette = view->palette();
        QColor color = ((QWidget*)object)->palette().background().color();

        if (qtcIsCustomBgnd(opts))
            color.setAlphaF(0.0);

        palette.setColor(view->backgroundRole(), color);
        view->setPalette(palette);
        object->removeEventFilter(this);
    }

    bool isSViewCont = (theThemedApp == APP_KONTACT &&
                        m_sViewContainers.contains((QWidget*)object));
    if ((!opts.gtkScrollViews &&
         qobject_cast<QAbstractScrollArea*>(object)) || isSViewCont) {
        QPoint pos;
        switch (event->type()) {
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
            pos = ((QMouseEvent*)event)->pos();
            break;
        case QEvent::Wheel:
            pos = ((QWheelEvent*)event)->pos();
        default:
            break;
        }

        if (!pos.isNull()) {
            QAbstractScrollArea *area=0L;
            QPoint mapped(pos);

            if (isSViewCont) {
                QSet<QWidget*>::ConstIterator it(m_sViewContainers[(QWidget*)object].begin()),
                                               end(m_sViewContainers[(QWidget*)object].end());

                for(; it!=end && !area; ++it)
                    if((*it)->isVisible())
                    {
                        mapped=(*it)->mapFrom((QWidget*)object, pos);
                        if((*it)->rect().adjusted(0, 0, 4, 4).contains(mapped))
                            area=(QAbstractScrollArea*)(*it);
                    }
            } else {
                area = (QAbstractScrollArea*)object;
            }

            if (area) {
                QScrollBar *sbars[2]={area->verticalScrollBar(), area->horizontalScrollBar() };

                for(int i=0; i<2; ++i)
                    if(sbars[i])
                    {
                        QRect r(i ? 0 : area->rect().right()-3,   i ? area->rect().bottom()-3 : 0,
                                sbars[i]->rect().width(), sbars[i]->rect().height());

                        if(r.contains(pos) ||
                            (sbars[i]==m_sViewSBar &&
                             (QEvent::MouseMove==event->type() ||
                              QEvent::MouseButtonRelease==event->type())))
                        {
                            if(QEvent::Wheel!=event->type())
                            {
                                struct HackEvent : public QMouseEvent
                                {
                                    void set(const QPoint &mapped, bool vert)
                                    {
                                        p=QPoint(vert ? 0 : mapped.x(), vert ? mapped.y() : 0);
                                        g=QPoint(g.x()+(vert ? 0 : -3), g.y()+(vert ? -3 : 0));
                                    }
                                };

                                ((HackEvent*)event)->set(mapped, 0==i);
                            }
                            sbars[i]->event(event);
                            if(QEvent::MouseButtonPress==event->type())
                                m_sViewSBar=sbars[i];
                            else if(QEvent::MouseButtonRelease==event->type())
                                m_sViewSBar=0L;
                            return true;
                        }
                    }
            }
        }
    }

    switch((int)(event->type()))
    {
        case QEvent::Timer:
        case QEvent::Move:
            return false; // just for performance - they can occur really often
        case QEvent::Resize:
            if (!POPUP_MENUS_SQUARE(opts) &&
                object->inherits("QComboBoxPrivateContainer")) {
                QWidget *widget = static_cast<QWidget*>(object);
                if (Utils::hasAlphaChannel(widget)) {
                    widget->clearMask();
                } else {
                    widget->setMask(windowMask(widget->rect(),
                                               opts.round > ROUND_SLIGHT));
                }
                return false;
            } else if ((BLEND_TITLEBAR ||
                        opts.windowBorder &
                        WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR ||
                        opts.menubarHiding & HIDE_KWIN) &&
                       qobject_cast<QMenuBar*>(object)) {
                QResizeEvent *re = static_cast<QResizeEvent*>(event);

                if (re->size().height() != re->oldSize().height())
                    emitMenuSize((QMenuBar*)object, PREVIEW_MDI==m_isPreview || !((QMenuBar*)object)->isVisible()
                                    ? 0 : re->size().height());
            }
            break;
        case QEvent::ShortcutOverride:
            if((opts.menubarHiding || opts.statusbarHiding) && qobject_cast<QMainWindow*>(object))
            {
                QMainWindow *window=static_cast<QMainWindow*>(object);

                if(window->isVisible())
                {
                    if(opts.menubarHiding&HIDE_KEYBOARD && window->menuWidget())
                    {
                        QKeyEvent *k=static_cast<QKeyEvent*>(event);

                        if(k->modifiers()&Qt::ControlModifier && k->modifiers()&Qt::AltModifier && Qt::Key_M==k->key())
                            toggleMenuBar(window);
                    }
                    if(opts.statusbarHiding&HIDE_KEYBOARD)
                    {
                        QKeyEvent *k=static_cast<QKeyEvent*>(event);

                        if(k->modifiers()&Qt::ControlModifier && k->modifiers()&Qt::AltModifier && Qt::Key_S==k->key())
                            toggleStatusBar(window);
                    }
                }
            }
            break;
        case QEvent::ShowToParent:
            if(qobject_cast<QMenuBar*>(object)) {
                if(opts.menubarHiding && m_saveMenuBarStatus && qtcMenuBarHidden(appName))
                    static_cast<QMenuBar*>(object)->setHidden(true);
#ifdef Q_OS_MAC
                if (opts.nonnativeMenubarApps.contains(appName)) {
                    QMenuBar *mnb = static_cast<QMenuBar*>(object);
                    if (mnb->isNativeMenuBar()) {
                        mnb->setNativeMenuBar(false);
                        mnb->setHidden(false);
                        mnb->setVisible(true);
                    }
                } else {
                    static_cast<QMenuBar*>(object)->setNativeMenuBar(true);
                }
#endif
            }
            if(opts.statusbarHiding && m_saveStatusBarStatus && qobject_cast<QStatusBar*>(object) &&
               qtcStatusBarHidden(appName))
                static_cast<QStatusBar*>(object)->setHidden(true);
            break;
        case QEvent::PaletteChange: {
            QWidget *widget = qtcToWidget(object);

            if (widget && widget->isWindow() &&
                (qtcIsDialog(widget) || qtcIsWindow(widget))) {
                setBgndProp(widget, opts.bgndAppearance,
                            IMG_NONE != opts.bgndImage.type);
            }
            break;
        }
        case QEvent::Paint: {
            if ((!qtcIsFlatBgnd(opts.menuBgndAppearance) ||
                 opts.menuBgndImage.type != IMG_NONE ||
                 opts.menuBgndOpacity != 100 ||
                 !POPUP_MENUS_SQUARE(opts)) &&
                (qobject_cast<QMenu*>(object) ||
                 (object->inherits("QComboBoxPrivateContainer")))) {
                QWidget *widget = qtcToWidget(object);
                QPainter p(widget);
                QRect r(widget->rect());
                double radius = opts.round >= ROUND_FULL ? 5.0 : 2.5;
                QStyleOption opt;
                opt.init(widget);
                const QColor *use = popupMenuCols(&opt);

                // Qt4 does not clear the background if the attribute
                // Qt::WA_NoSystemBackground is not set on the widget.
                p.setCompositionMode(QPainter::CompositionMode_Source);
                p.setClipRegion(static_cast<QPaintEvent*>(event)->region());
                if (!opts.popupBorder) {
                    p.setRenderHint(QPainter::Antialiasing, true);
                    p.setPen(use[ORIGINAL_SHADE]);
                    p.drawPath(buildPath(r, WIDGET_OTHER, ROUNDED_ALL, radius));
                    p.setRenderHint(QPainter::Antialiasing, false);
                }
                if (!POPUP_MENUS_SQUARE(opts)) // && !isCombo)
                    p.setClipRegion(windowMask(r, opts.round > ROUND_SLIGHT),
                                    Qt::IntersectClip);

                // In case the gradient uses alpha, we need to fill with
                // the background colour - this makes it consistent with Gtk.
                if (opts.menuBgndOpacity == 100) {
                    p.fillRect(r, opt.palette.brush(QPalette::Background));
                }
                drawBackground(&p, widget, BGND_MENU);
                if (opts.popupBorder) {
                    EGradientBorder border =
                        qtcGetGradient(opts.menuBgndAppearance, &opts)->border;

                    p.setClipping(false);
                    p.setPen(use[QTC_STD_BORDER]);
                    // For now dont round combos - getting weird effects with
                    // shadow/clipping in Gtk2 style :-(
                    if (POPUP_MENUS_SQUARE(opts)) {
                        drawRect(&p, r);
                    } else {
                        p.setRenderHint(QPainter::Antialiasing, true);
                        p.drawPath(buildPath(r, WIDGET_OTHER,
                                             ROUNDED_ALL, radius));
                    }

                    if (qtcUseBorder(border) &&
                        APPEARANCE_FLAT != opts.menuBgndAppearance) {
                        QRect ri(r.adjusted(1, 1, -1, -1));

                        p.setPen(use[0]);
                        if (border == GB_LIGHT) {
                            if(POPUP_MENUS_SQUARE(opts)) // || isCombo)
                                drawRect(&p, ri);
                            else
                                p.drawPath(buildPath(ri, WIDGET_OTHER, ROUNDED_ALL, radius-1.0));
                        } else if (POPUP_MENUS_SQUARE(opts)) {
                            if (GB_3D != border) {
                                p.drawLine(ri.x(), ri.y(),
                                           ri.x() + ri.width() - 1,  ri.y());
                                p.drawLine(ri.x(), ri.y(), ri.x(),
                                           ri.y() + ri.height() - 1);
                            }
                            p.setPen(use[FRAME_DARK_SHADOW]);
                            p.drawLine(ri.x(), ri.y()+ri.height()-1, ri.x()+ri.width()-1,  ri.y()+ri.height()-1);
                            p.drawLine(ri.x()+ri.width()-1, ri.y(), ri.x()+ri.width()-1,  ri.y()+ri.height()-1);
                        }
                        else
                        {
                            QPainterPath tl,
                                         br;

                            buildSplitPath(ri, ROUNDED_ALL, radius-1.0, tl, br);
                            if(GB_3D!=border)
                                p.drawPath(tl);
                            p.setPen(use[FRAME_DARK_SHADOW]);
                            p.drawPath(br);
                        }
                    }
                }
            }
            else if(m_clickedLabel==object && qobject_cast<QLabel*>(object) && ((QLabel*)object)->buddy() && ((QLabel*)object)->buddy()->isEnabled())
            {
                // paint focus rect
                QLabel                *lbl = (QLabel*)object;
                QPainter              painter(lbl);
                QStyleOptionFocusRect opts;

                opts.palette = lbl->palette();
                opts.rect    = QRect(0, 0, lbl->width(), lbl->height());
                drawPrimitive(PE_FrameFocusRect, &opts, &painter, lbl);
            }
            else
            {
                QFrame *frame = qobject_cast<QFrame*>(object);

                if (frame)
                {
                    if(QFrame::HLine==frame->frameShape() || QFrame::VLine==frame->frameShape())
                    {
                        QPainter painter(frame);
                        QRect    r(QFrame::HLine==frame->frameShape()
                                    ? QRect(frame->rect().x(), frame->rect().y()+ (frame->rect().height()/2), frame->rect().width(), 1)
                                    : QRect(frame->rect().x()+(frame->rect().width()/2),  frame->rect().y(), 1, frame->rect().height()));

                        drawFadedLine(&painter, r, backgroundColors(frame->palette().window().color())[QTC_STD_BORDER], true, true,
                                      QFrame::HLine==frame->frameShape());
                        return true;
                    }
                    else
                        return false;
                }
            }
            break;
        }
        case QEvent::MouseButtonPress:
            if(dynamic_cast<QMouseEvent*>(event) && qobject_cast<QLabel*>(object) && ((QLabel*)object)->buddy())
            {
                QLabel      *lbl = (QLabel*)object;
                QMouseEvent *mev = (QMouseEvent*)event;

                if (lbl->rect().contains(mev->pos()))
                {
                    m_clickedLabel=lbl;
                    lbl->repaint();
                }
            }
            break;
        case QEvent::MouseButtonRelease:
            if (dynamic_cast<QMouseEvent*>(event) && qobject_cast<QLabel*>(object) && ((QLabel*)object)->buddy())
            {
                QLabel      *lbl = (QLabel*)object;
                QMouseEvent *mev = (QMouseEvent*)event;

                if(m_clickedLabel)
                {
                    m_clickedLabel=0;
                    lbl->update();
                }

                // set focus to the buddy...
                if (lbl->rect().contains(mev->pos()))
                    ((QLabel*)object)->buddy()->setFocus(Qt::ShortcutFocusReason);
            }
            break;
        case QEvent::StyleChange:
        case QEvent::Show:
        {
            QProgressBar *bar = qobject_cast<QProgressBar*>(object);

            if(bar)
            {
                m_progressBars.insert(bar);
                if (!m_progressBarAnimateTimer) {
                    if (opts.animatedProgress || (0 == bar->minimum() && 0 == bar->maximum())) {
                        // we know we'll need a timer, start it at once
                        if (m_timer.isNull()) {
                            m_timer.start();
                        }
                        m_progressBarAnimateFps = constProgressBarFps;
                        m_progressBarAnimateTimer = startTimer(1000/m_progressBarAnimateFps);
                    }
                }
            } else if(!POPUP_MENUS_SQUARE(opts) &&
                      object->inherits("QComboBoxPrivateContainer")) {
                QWidget *widget = static_cast<QWidget*>(object);
                if (Utils::hasAlphaChannel(widget)) {
                    widget->clearMask();
                } else {
                    widget->setMask(windowMask(widget->rect(),
                                               opts.round > ROUND_SLIGHT));
                }
                return false;
            } else if ((BLEND_TITLEBAR || opts.windowBorder&WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR || opts.menubarHiding&HIDE_KWIN) &&
                    qobject_cast<QMenuBar*>(object))
            {
                QMenuBar *mb=(QMenuBar*)object;
                emitMenuSize((QMenuBar*)mb, PREVIEW_MDI==m_isPreview || !((QMenuBar*)mb)->isVisible() ? 0 : mb->size().height(), true);
            } else if (QEvent::Show == event->type()) {
                QWidget *widget = qtcToWidget(object);

                if (widget && widget->isWindow() &&
                    (qtcIsWindow(widget) || qtcIsDialog(widget))) {
                    setBgndProp(widget, opts.bgndAppearance,
                                IMG_NONE != opts.bgndImage.type);

                    int opacity = (qtcIsDialog(widget) ? opts.dlgOpacity :
                                   opts.bgndOpacity);
                    setOpacityProp(widget, (unsigned short)opacity);
                }
            }
            break;
        }
        case QEvent::Close:
        case QEvent::Destroy:
        case QEvent::Hide: {
            if((BLEND_TITLEBAR || opts.windowBorder&WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR || opts.menubarHiding&HIDE_KWIN) &&
                qobject_cast<QMenuBar*>(object))
            {
                QMenuBar *mb=(QMenuBar*)object;
                emitMenuSize((QMenuBar*)mb, 0);
            }
            if(m_hoverWidget && object==m_hoverWidget)
            {
                m_pos.setX(-1);
                m_pos.setY(-1);
                m_hoverWidget=0L;
            }

            // The Destroy event is sent from ~QWidget, which happens after ~QProgressBar - therefore, we can't cast to a QProgressBar.
            // So we have to check on object.
            if(object && !m_progressBars.isEmpty())
            {
                m_progressBars.remove(reinterpret_cast<QProgressBar*>(object));
                if (m_progressBars.isEmpty() && m_progressBarAnimateTimer)
                {
                    killTimer(m_progressBarAnimateTimer);
                    m_progressBarAnimateTimer = 0;
                }
            }
            break;
        }
        case QEvent::Enter:
            if(object->isWidgetType() && object->inherits("Q3Header"))
            {
                m_hoverWidget=(QWidget*)object;

                if(m_hoverWidget && !m_hoverWidget->isEnabled())
                    m_hoverWidget=0L;
            }
            break;
        case QEvent::Leave:
            if(m_hoverWidget && object==m_hoverWidget)
            {
                m_pos.setX(-1);
                m_pos.setY(-1);
                m_hoverWidget=0L;
                ((QWidget*)object)->repaint();
            }
            break;
        case QEvent::MouseMove:  // Only occurs for widgets with mouse tracking enabled
        {
            QMouseEvent *me = dynamic_cast<QMouseEvent*>(event);

            if(me && m_hoverWidget && object->isWidgetType() && object->inherits("Q3Header"))
            {
                if(!me->pos().isNull() && me->pos()!=m_pos)
                    m_hoverWidget->repaint();
                m_pos=me->pos();
            }
            break;
        }
        case QEvent::FocusIn:
        case QEvent::FocusOut:
            if(opts.highlightScrollViews && object->isWidgetType() && object->inherits("Q3ScrollView"))
            {
                ((QWidget*)object)->update();
                return false;
            }
            break;
        case QEvent::WindowActivate:
            if(opts.shadeMenubarOnlyWhenActive && SHADE_NONE!=opts.shadeMenubars && qobject_cast<QMenuBar*>(object))
            {
                m_active=true;
                ((QWidget*)object)->repaint();
                return false;
            }
            break;
        case QEvent::WindowDeactivate:
            if(opts.shadeMenubarOnlyWhenActive && SHADE_NONE!=opts.shadeMenubars && qobject_cast<QMenuBar*>(object))
            {
                m_active=false;
                ((QWidget*)object)->repaint();
                return false;
            }
            break;
        default:
            break;
    }

    return BaseStyle::eventFilter(object, event);
}

void Style::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_progressBarAnimateTimer) {
        bool hasAnimation = false;
        m_animateStep = m_timer.elapsed() / (1000 / constProgressBarFps);
        foreach (QProgressBar *bar, const_(m_progressBars)) {
            if ((opts.animatedProgress && 0 == m_animateStep % 2 &&
                 bar->value() != bar->minimum() &&
                 bar->value() != bar->maximum()) ||
                (0 == bar->minimum() && 0 == bar->maximum())) {
                bar->update();
                hasAnimation = true;
            }
        }
        if (Q_UNLIKELY(!hasAnimation && m_progressBarAnimateFps == constProgressBarFps)) {
            // go back to "idling frequency" mode.
            killTimer(m_progressBarAnimateTimer);
            m_progressBarAnimateTimer = 0;
        }
    }

    event->ignore();
}

int Style::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    prePolish(widget);
    switch((unsigned)metric)
    {
        case PM_ToolTipLabelFrameWidth:
            return !(opts.round != ROUND_NONE) || opts.square&SQUARE_TOOLTIPS ? BaseStyle::pixelMetric(metric, option, widget) : 3;
        case PM_MdiSubWindowFrameWidth:
            return 3;
        case PM_DockWidgetTitleMargin:
            return !(opts.dwtSettings&DWT_TEXT_ALIGN_AS_PER_TITLEBAR) || ALIGN_LEFT==opts.titlebarAlignment ? 4 : 0;
        case PM_DockWidgetTitleBarButtonMargin:
            return 4;
        case PM_DockWidgetFrameWidth:
            return 2;
        case PM_ToolBarExtensionExtent:
            return 15;
#ifndef QTC_QT4_ENABLE_KDE
        case PM_SmallIconSize:
            return 16;
        case PM_ToolBarIconSize:
            return 22;
        case PM_IconViewIconSize:
        case PM_LargeIconSize:
            return 32;
#else
        case PM_TabCloseIndicatorWidth:
        case PM_TabCloseIndicatorHeight:
        case PM_SmallIconSize:
        case PM_ButtonIconSize:
            return KIconLoader::global()->currentSize(KIconLoader::Small);
        case PM_ToolBarIconSize:
            return KIconLoader::global()->currentSize(KIconLoader::Toolbar);
        case PM_IconViewIconSize:
        case PM_LargeIconSize:
            return KIconLoader::global()->currentSize(KIconLoader::Dialog);
        case PM_MessageBoxIconSize:
            // TODO return KIconLoader::global()->currentSize(KIconLoader::MessageBox);
            return KIconLoader::SizeHuge;
#endif
        case PM_SubMenuOverlap:
            return -2;
        case PM_ScrollView_ScrollBarSpacing:
            return opts.etchEntry ? 2 : 3;
        case PM_MenuPanelWidth:
            return opts.popupBorder ? pixelMetric(PM_DefaultFrameWidth, option, widget) : 0;
        case PM_SizeGripSize:
            return SIZE_GRIP_SIZE;
        case PM_TabBarScrollButtonWidth:
            return 18;
        case PM_HeaderMargin:
            return 3;
        case PM_DefaultChildMargin:
            return isOOWidget(widget)
                    ? /*opts.round>=ROUND_FULL && !(opts.square&SQUARE_SCROLLVIEW)
                        ?*/ 2
                        /*: 1*/
                    : 6;
        case PM_DefaultTopLevelMargin:
            return 9;
        case PM_LayoutHorizontalSpacing:
        case PM_LayoutVerticalSpacing:
            return -1; // use layoutSpacingImplementation
        case PM_DefaultLayoutSpacing:
            return 6;
        case PM_LayoutLeftMargin:
        case PM_LayoutTopMargin:
        case PM_LayoutRightMargin:
        case PM_LayoutBottomMargin:
            return pixelMetric((option && (option->state&QStyle::State_Window)) || (widget && widget->isWindow())
                                ? PM_DefaultTopLevelMargin
                                : PM_DefaultChildMargin, option, widget);
        case PM_MenuBarItemSpacing:
            return 0;
        case PM_ToolBarItemMargin:
            return 0;
        case PM_ToolBarItemSpacing:
            return TBTN_JOINED==opts.tbarBtns ? 0 : 1;
        case PM_ToolBarFrameWidth:
            // Remove because, in KDE4 at least, if have two locked toolbars together then the last/first items are too close
            return /*TB_NONE==opts.toolbarBorders ? 0 : */1;
        case PM_FocusFrameVMargin:
        case PM_FocusFrameHMargin:
            return 2;
        case PM_MenuBarVMargin:
        case PM_MenuBarHMargin:
            // Bangarang (media player) has a 4 pixel high menubar at the top - when it doesn't actually have a menubar!
            // Seems to be because of the return 2 below (which was previously always returned unless XBar support and
            // size was 0). So, if we are askes for these metrics for a widet whose size<6, then return 0.
            return widget && widget->size().height() < 6 ? 0 : 2;
        case PM_MenuHMargin:
        case PM_MenuVMargin:
            return 0;
        case PM_MenuButtonIndicator:
            return (opts.buttonEffect != EFFECT_NONE ? 10 : 9)+
                    (!widget || qobject_cast<const QToolButton*>(widget) ? 6 : 0);
        case PM_ButtonMargin:
            return (opts.buttonEffect != EFFECT_NONE ?
                    (opts.thin & THIN_BUTTONS) ? 4 : 6 :
                    (opts.thin & THIN_BUTTONS) ? 2 : 4) + MAX_ROUND_BTN_PAD;
        case PM_TabBarTabShiftVertical:
            return 2;
        case PM_TabBarTabShiftHorizontal:
            return 0;
        case PM_ButtonShiftHorizontal:
            // return Qt::RightToLeft==QApplication::layoutDirection() ? -1 : 1;
        case PM_ButtonShiftVertical:
            return APP_KDEVELOP==theThemedApp && !opts.stdSidebarButtons && widget && isMultiTabBarTab(getButton(widget, 0L)) ? 0 : 1;
        case PM_ButtonDefaultIndicator:
            return 0;
        case PM_DefaultFrameWidth:
            if ((/*!opts.popupBorder || */opts.gtkComboMenus) && widget && widget->inherits("QComboBoxPrivateContainer"))
                return opts.gtkComboMenus ? (opts.borderMenuitems || !POPUP_MENUS_SQUARE(opts) ? 2 : 1) : 0;

            if ((!opts.gtkScrollViews || (opts.square&SQUARE_SCROLLVIEW)) && isKateView(widget))
                return (opts.square&SQUARE_SCROLLVIEW) ? 1 : 0;

            if ((opts.square&SQUARE_SCROLLVIEW) && widget && !opts.etchEntry &&
                (qobject_cast<const QAbstractScrollArea*>(widget) ||
                 isKontactPreviewPane(widget) ||
                 widget->inherits("Q3ScrollView")))
                return (opts.gtkScrollViews || opts.thinSbarGroove ||
                        !opts.borderSbarGroove) &&
                    (!opts.highlightScrollViews) ? 1 : 2;

            if (!qtcDrawMenuBorder(opts) && !opts.borderMenuitems &&
                POPUP_MENUS_SQUARE(opts) &&
                qobject_cast<const QMenu*>(widget)) {
                return 1;
            }

            if (opts.buttonEffect != EFFECT_NONE && opts.etchEntry &&
                (!widget || // !isFormWidget(widget) &&
                qobject_cast<const QLineEdit*>(widget) || qobject_cast<const QAbstractScrollArea*>(widget) ||
                widget->inherits("Q3ScrollView") /*|| isKontactPreviewPane(widget)*/))
                return 3;
            else
                return 2;
        case PM_SpinBoxFrameWidth:
            return opts.buttonEffect != EFFECT_NONE && opts.etchEntry ? 3 : 2;
        case PM_IndicatorWidth:
        case PM_IndicatorHeight:
        case PM_ExclusiveIndicatorWidth:
        case PM_ExclusiveIndicatorHeight:
        case PM_CheckListControllerSize:
        case PM_CheckListButtonSize:
            return (opts.buttonEffect != EFFECT_NONE ?
                    opts.crSize + 2 : opts.crSize);
        case PM_TabBarTabOverlap:
            return TAB_MO_GLOW==opts.tabMouseOver ? 0 : 1;
        case PM_ProgressBarChunkWidth:
            return 4;
        // case PM_DockWindowHandleExtent:
        //     return 10;
        case PM_DockWidgetSeparatorExtent:
        case PM_SplitterWidth:
            return LINE_1DOT==opts.splitters ? 7 : 6;
        case PM_ToolBarHandleExtent:
            return LINE_1DOT==opts.handles ? 7 : 8;
        case PM_ScrollBarSliderMin:
            // Leave a minimum of 21 pixels (which is the size used by Oxygen)
            // See https://github.com/QtCurve/qtcurve-qt4/issues/7
            // and https://bugs.kde.org/show_bug.cgi?id=317690
            return qtcMax(opts.sliderWidth, 20) + 1;
        case PM_SliderThickness: {
            int glowSize = (opts.buttonEffect != EFFECT_NONE &&
                            opts.coloredMouseOver == MO_GLOW ? 2 : 0);
            if (opts.sliderStyle == SLIDER_CIRCULAR) {
                return CIRCULAR_SLIDER_SIZE + 6 + glowSize;
            } else if (opts.sliderStyle == SLIDER_TRIANGULAR) {
                return 19 + glowSize;
            } else {
                return (SLIDER_SIZE + glowSize +
                        (oneOf(opts.sliderStyle, SLIDER_PLAIN_ROTATED,
                               SLIDER_ROUND_ROTATED) ? 11 : 6));
            }
        }
        case PM_SliderControlThickness: {
            int glowSize = (opts.buttonEffect != EFFECT_NONE &&
                            opts.coloredMouseOver == MO_GLOW ? 2 : 0);
            if (opts.sliderStyle == SLIDER_CIRCULAR) {
                return CIRCULAR_SLIDER_SIZE + glowSize;
            } else if (opts.sliderStyle == SLIDER_TRIANGULAR) {
                return 11 + glowSize;
            } else {
                return (SLIDER_SIZE + glowSize +
                        (oneOf(opts.sliderStyle, SLIDER_PLAIN_ROTATED,
                               SLIDER_ROUND_ROTATED) ? 6 : -2));
            }
        }
        case PM_SliderTickmarkOffset:
            return SLIDER_TRIANGULAR==opts.sliderStyle ? 5 : 4;
        case PM_SliderSpaceAvailable:
            if (auto slider = styleOptCast<QStyleOptionSlider>(option)) {
                int size(pixelMetric(PM_SliderControlThickness, slider, widget));

                if (slider->tickPosition & QSlider::TicksBelow)
                    ++size;
                if (slider->tickPosition & QSlider::TicksAbove)
                    ++size;
                return size;
            }
            return BaseStyle::pixelMetric(metric, option, widget);
        case PM_SliderLength: {
            int glowSize = (opts.buttonEffect != EFFECT_NONE &&
                            opts.coloredMouseOver == MO_GLOW ? 2 : 0);
            if (opts.sliderStyle == SLIDER_CIRCULAR) {
                return CIRCULAR_SLIDER_SIZE + glowSize;
            } else if (opts.sliderStyle == SLIDER_TRIANGULAR) {
                return 11 + glowSize;
            } else {
                return (SLIDER_SIZE + glowSize +
                        (oneOf(opts.sliderStyle, SLIDER_PLAIN_ROTATED,
                               SLIDER_ROUND_ROTATED) ? -2 : 6));
            }
        }
        case PM_ScrollBarExtent:
            return opts.sliderWidth;
        case PM_MaximumDragDistance:
            return -1;
        case PM_TabBarTabHSpace:
            return 14;
        case PM_TabBarTabVSpace:
            return opts.highlightTab ? 10 : 8;
        case PM_TitleBarHeight:
            return qMax(widget ? widget->fontMetrics().lineSpacing()
                               : option ? option->fontMetrics.lineSpacing()
                                        : 0, 24);
        case PM_MenuBarPanelWidth:
            return 0;
        case QtC_Round:
            return (int)((opts.square&SQUARE_WINDOWS && opts.round>ROUND_SLIGHT) ? ROUND_SLIGHT : opts.round);
        case QtC_WindowBorder:
            return opts.windowBorder;
        case QtC_CustomBgnd:
            return qtcIsCustomBgnd(opts);
        case QtC_TitleAlignment:
            switch(opts.titlebarAlignment)
            {
                default:
                case ALIGN_LEFT:
                    return Qt::AlignLeft;
                case ALIGN_CENTER:
                    return Qt::AlignHCenter|Qt::AlignVCenter;
                case ALIGN_FULL_CENTER:
                    return Qt::AlignHCenter;
                case ALIGN_RIGHT:
                    return Qt::AlignRight;
            }
        case QtC_TitleBarButtons:
            return opts.titlebarButtons;
        case QtC_TitleBarIcon:
            return opts.titlebarIcon;
        case QtC_TitleBarIconColor:
            return titlebarIconColor(option).rgb();
        case QtC_TitleBarEffect:
            return opts.titlebarEffect;
        case QtC_BlendMenuAndTitleBar:
            return BLEND_TITLEBAR;
        case QtC_ShadeMenubarOnlyWhenActive:
            return opts.shadeMenubarOnlyWhenActive;
        case QtC_ToggleButtons:
            return (opts.menubarHiding&HIDE_KWIN   ? 0x1 : 0)+
                   (opts.statusbarHiding&HIDE_KWIN ? 0x2 : 0);
        case QtC_MenubarColor:
            return m_menubarCols[ORIGINAL_SHADE].rgb();
        case QtC_TitleBarApp:
            return !option || option->state&State_Active ? opts.titlebarAppearance : opts.inactiveTitlebarAppearance;
// The following is a somewhat hackyish fix for konqueror's show close button on tab setting...
// ...its hackish in the way that I'm assuming when KTabBar is positioning the close button and it
// asks for these options, it only passes in a QStyleOption  not a QStyleOptionTab
//.........
        case PM_TabBarBaseHeight:
            if (qtcCheckKDEType(widget, KTabBar) &&
                !styleOptCast<QStyleOptionTab>(option)) {
                return 10;
            }
            return BaseStyle::pixelMetric(metric, option, widget);
        case PM_TabBarBaseOverlap:
            if (qtcCheckKDEType(widget, KTabBar) &&
                !styleOptCast<QStyleOptionTab>(option)) {
                return 0;
            }
// Fall through!
//.........
        default:
            return BaseStyle::pixelMetric(metric, option, widget);
    }
}

int Style::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const
{
    prePolish(widget);
    switch (hint)
    {
        case SH_ToolTip_Mask:
        case SH_Menu_Mask:
            if ((SH_ToolTip_Mask == hint && (opts.square & SQUARE_TOOLTIPS)) ||
                (SH_Menu_Mask == hint && POPUP_MENUS_SQUARE(opts))) {
                return BaseStyle::styleHint(hint, option, widget, returnData);
            } else {
                if (!Utils::hasAlphaChannel(widget) &&
                    (!widget || widget->isWindow())) {
                    if (auto mask =
                        styleOptCast<QStyleHintReturnMask>(returnData)) {
                        mask->region = windowMask(option->rect,
                                                  opts.round > ROUND_SLIGHT);
                    }
                }
                return true;
            }
        case SH_ComboBox_ListMouseTracking:
        case SH_PrintDialog_RightAlignButtons:
        case SH_ItemView_ArrowKeysNavigateIntoChildren:
        case SH_ToolBox_SelectedPageTitleBold:
        case SH_ScrollBar_MiddleClickAbsolutePosition:
        case SH_SpinControls_DisableOnBounds:
        case SH_Slider_SnapToValue:
        case SH_FontDialog_SelectAssociatedText:
        case SH_Menu_MouseTracking:
            return true;
        case SH_UnderlineShortcut:
            return widget && opts.hideShortcutUnderline ? m_shortcutHandler->showShortcut(widget) : true;
        case SH_GroupBox_TextLabelVerticalAlignment:
            if (auto frame = styleOptCast<QStyleOptionGroupBox>(option)) {
                if (frame->features & QStyleOptionFrameV2::Flat) {
                    return Qt::AlignVCenter;
                }
            }
            return opts.gbLabel&GB_LBL_INSIDE
                    ? Qt::AlignBottom
                    : opts.gbLabel&GB_LBL_OUTSIDE
                        ? Qt::AlignTop
                        : Qt::AlignVCenter;
        case SH_MessageBox_CenterButtons:
        case SH_ProgressDialog_CenterCancelButton:
        case SH_DitherDisabledText:
        case SH_EtchDisabledText:
        case SH_Menu_AllowActiveAndDisabled:
        case SH_ItemView_ShowDecorationSelected: // Controls whether the highlighting of listview/treeview items highlights whole line.
        case SH_MenuBar_AltKeyNavigation:
            return false;
        case SH_ItemView_ChangeHighlightOnFocus: // gray out selected items when losing focus.
            return false;
        case SH_WizardStyle:
            return QWizard::ClassicStyle;
        case SH_RubberBand_Mask: {
            auto opt = styleOptCast<QStyleOptionRubberBand>(option);
            if (!opt)
                return true;
            if (auto mask = styleOptCast<QStyleHintReturnMask>(returnData)) {
                mask->region = option->rect;
                mask->region -= option->rect.adjusted(1,1,-1,-1);
            }
            return true;
        }
        case SH_Menu_SubMenuPopupDelay:
            return opts.menuDelay;
        case SH_ToolButton_PopupDelay:
            return 250;
        case SH_ComboBox_PopupFrameStyle:
            return opts.popupBorder || !POPUP_MENUS_SQUARE(opts) ? QFrame::StyledPanel|QFrame::Plain : QFrame::NoFrame;
        case SH_TabBar_Alignment:
            return Qt::AlignLeft;
        case SH_Header_ArrowAlignment:
            return Qt::AlignLeft;
        case SH_WindowFrame_Mask:
            if (auto mask = styleOptCast<QStyleHintReturnMask>(returnData)) {
                QRect r(option->rect);

                switch((opts.square&SQUARE_WINDOWS && opts.round>ROUND_SLIGHT) ? ROUND_SLIGHT : opts.round)
                {
                    case ROUND_NONE:
                        mask->region=r;
                        break;
                    case ROUND_SLIGHT:
                        mask->region=QRegion(r.x()+1, r.y(), r.width()-2, r.height());
                        mask->region += QRegion(r.x()+0, r.y()+1, 1, r.height()-2);
                        mask->region += QRegion(r.x()+r.width()-1, r.y()+1, 1, r.height()-2);
                        break;
                    default: // ROUND_FULL
                        mask->region=QRegion(r.x()+5, r.y(), r.width()-10, r.height());
                        mask->region += QRegion(r.x()+0, r.y()+5, 1, r.height()-5);
                        mask->region += QRegion(r.x()+1, r.y()+3, 1, r.height()-2);
                        mask->region += QRegion(r.x()+2, r.y()+2, 1, r.height()-1);
                        mask->region += QRegion(r.x()+3, r.y()+1, 2, r.height());
                        mask->region += QRegion(r.x()+r.width()-1, r.y()+5, 1, r.height()-5);
                        mask->region += QRegion(r.x()+r.width()-2, r.y()+3, 1, r.height()-2);
                        mask->region += QRegion(r.x()+r.width()-3, r.y()+2, 1, r.height()-1);
                        mask->region += QRegion(r.x()+r.width()-5, r.y()+1, 2, r.height()-0);
                }
            }
            return 1;
        case SH_TitleBar_NoBorder:
        case SH_TitleBar_AutoRaise:
            return 1;
        case SH_MainWindow_SpaceBelowMenuBar:
            if(opts.xbar)
                if (const QMenuBar *menubar = qobject_cast<const QMenuBar*>(widget))
                    if (0==menubar->height() && !menubar->actions().isEmpty())
                    {   // we trick menubars if we use macmenus - hehehe...
                        // NOTICE the final result NEEDS to be > "0" (i.e. "1") to avoid side effects...
                        return -menubar->actionGeometry(menubar->actions().first()).height() + 1;
                    }
            return 0;
        case SH_DialogButtonLayout:
            return opts.gtkButtonOrder ? QDialogButtonBox::GnomeLayout : QDialogButtonBox::KdeLayout;
        case SH_MessageBox_TextInteractionFlags:
            return Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse;
        case SH_LineEdit_PasswordCharacter:
            if(opts.passwordChar)
            {
                int                chars[4]={opts.passwordChar, 0x25CF, 0x2022, 0};
                const QFontMetrics &fm(option ? option->fontMetrics
                                        : (widget ? widget->fontMetrics() : QFontMetrics(QFont())));
                for(int i=0; chars[i]; ++i)
                    if (fm.inFont(QChar(chars[i])))
                        return chars[i];
                return '*';
            }
            else
                return '\0';
        case SH_MenuBar_MouseTracking:
            // Always return 1, as setting to 0 dissables the effect when a menu is shown.
            return 1; // opts.menubarMouseOver ? 1 : 0;
        case SH_ScrollView_FrameOnlyAroundContents:
            return widget && widget->isWindow()
                    ? false
                    : opts.gtkScrollViews && (!widget || !widget->inherits("QComboBoxListView"));
        case SH_ComboBox_Popup:
            if (opts.gtkComboMenus) {
                if (widget && widget->inherits("Q3ComboBox"))
                    return 0;
                if (auto cmb = styleOptCast<QStyleOptionComboBox>(option)) {
                    return !cmb->editable;
                }
            }
            return 0;
        case SH_FormLayoutFormAlignment:
            // KDE4 HIG, align the contents in a form layout to the left
            return Qt::AlignLeft | Qt::AlignTop;
        case SH_FormLayoutLabelAlignment:
            // KDE4  HIG, align the labels in a form layout to the right
            return Qt::AlignRight;
        case SH_FormLayoutFieldGrowthPolicy:
            return QFormLayout::ExpandingFieldsGrow;
        case SH_FormLayoutWrapPolicy:
            return QFormLayout::DontWrapRows;
#ifdef QTC_QT4_ENABLE_KDE
        case SH_DialogButtonBox_ButtonsHaveIcons:
            return KGlobalSettings::showIconsOnPushButtons();
        case SH_ItemView_ActivateItemOnSingleClick:
            return KGlobalSettings::singleClick();
#endif
        default:
#ifdef QTC_QT4_ENABLE_KDE
            // Tell the calling app that we can handle certain custom widgets...
            if(hint>=SH_CustomBase && widget)
                if("CE_CapacityBar"==widget->objectName())
                {
                    if (opts.boldProgress)
                        setBold((QWidget*)widget);
                    return CE_QtC_KCapacityBar;
                }
#endif
            return BaseStyle::styleHint(hint, option, widget, returnData);
   }
}

QPalette Style::standardPalette() const
{
#ifdef QTC_QT4_ENABLE_KDE
    return KGlobalSettings::createApplicationPalette(KSharedConfig::openConfig(m_componentData));
#else
    return BaseStyle::standardPalette();
#endif
}

QIcon Style::standardIconImplementation(StandardPixmap pix, const QStyleOption *option, const QWidget *widget) const
{
    switch(pix)
    {
//         case SP_TitleBarMenuButton:
//         case SP_TitleBarMinButton:
//         case SP_TitleBarMaxButton:
//         case SP_TitleBarContextHelpButton:
        case SP_TitleBarNormalButton:
        case SP_TitleBarShadeButton:
        case SP_TitleBarUnshadeButton:
        case SP_DockWidgetCloseButton:
        case SP_TitleBarCloseButton:
        {
            QPixmap pm(13, 13);

            pm.fill(Qt::transparent);

            QPainter painter(&pm);

            drawIcon(&painter, Qt::color1, QRect(0, 0, pm.width(), pm.height()), false, pix2Icon(pix),
                     SP_TitleBarShadeButton==pix || SP_TitleBarUnshadeButton==pix);
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
#ifndef QTC_QT4_ENABLE_KDE
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
            return KIcon("dialog-information");
        case SP_MessageBoxWarning:
            return KIcon("dialog-warning");
        case SP_MessageBoxCritical:
            return KIcon("dialog-error");
        case SP_MessageBoxQuestion:
            return KIcon("dialog-information");
        case SP_DesktopIcon:
            return KIcon("user-desktop");
        case SP_TrashIcon:
            return KIcon("user-trash");
        case SP_ComputerIcon:
            return KIcon("computer");
        case SP_DriveFDIcon:
            return KIcon("media-floppy");
        case SP_DriveHDIcon:
            return KIcon("drive-harddisk");
        case SP_DriveCDIcon:
        case SP_DriveDVDIcon:
            return KIcon("media-optical");
        case SP_DriveNetIcon:
            return KIcon("network-server");
        case SP_DirOpenIcon:
            return KIcon("document-open");
        case SP_DirIcon:
        case SP_DirClosedIcon:
            return KIcon("folder");
//         case SP_DirLinkIcon:
        case SP_FileIcon:
            return KIcon("application-x-zerosize");
//         case SP_FileLinkIcon:
        case SP_FileDialogStart:
            return KIcon(Qt::RightToLeft==QApplication::layoutDirection() ? "go-edn" : "go-first");
        case SP_FileDialogEnd:
            return KIcon(Qt::RightToLeft==QApplication::layoutDirection() ? "go-first" : "go-end");
        case SP_FileDialogToParent:
            return KIcon("go-up");
        case SP_FileDialogNewFolder:
            return KIcon("folder-new");
        case SP_FileDialogDetailedView:
            return KIcon("view-list-details");
//         case SP_FileDialogInfoView:
//             return KIcon("dialog-ok");
//         case SP_FileDialogContentsView:
//             return KIcon("dialog-ok");
        case SP_FileDialogListView:
            return KIcon("view-list-icons");
        case SP_FileDialogBack:
            return KIcon(Qt::RightToLeft==QApplication::layoutDirection() ? "go-next" : "go-previous");
        case SP_DialogOkButton:
            return KIcon("dialog-ok");
        case SP_DialogCancelButton:
            return KIcon("dialog-cancel");
        case SP_DialogHelpButton:
            return KIcon("help-contents");
        case SP_DialogOpenButton:
            return KIcon("document-open");
        case SP_DialogSaveButton:
            return KIcon("document-save");
        case SP_DialogCloseButton:
            return KIcon("dialog-close");
        case SP_DialogApplyButton:
            return KIcon("dialog-ok-apply");
        case SP_DialogResetButton:
            return KIcon("document-revert");
//         case SP_DialogDiscardButton:
//              return KIcon("dialog-cancel");
        case SP_DialogYesButton:
            return KIcon("dialog-ok");
        case SP_DialogNoButton:
            return KIcon("dialog-cancel");
        case SP_ArrowUp:
            return KIcon("arrow-up");
        case SP_ArrowDown:
            return KIcon("arrow-down");
        case SP_ArrowLeft:
            return KIcon("arrow-left");
        case SP_ArrowRight:
            return KIcon("arrow-right");
        case SP_ArrowBack:
            return KIcon(Qt::RightToLeft==QApplication::layoutDirection() ? "go-next" : "go-previous");
        case SP_ArrowForward:
            return KIcon(Qt::RightToLeft==QApplication::layoutDirection() ? "go-previous" : "go-next");
        case SP_DirHomeIcon:
            return KIcon("user-home");
//         case SP_CommandLink:
//         case SP_VistaShield:
        case SP_BrowserReload:
            return KIcon("view-refresh");
        case SP_BrowserStop:
            return KIcon("process-stop");
        case SP_MediaPlay:
            return KIcon("media-playback-start");
        case SP_MediaStop:
            return KIcon("media-playback-stop");
        case SP_MediaPause:
            return KIcon("media-playback-pause");
        case SP_MediaSkipForward:
            return KIcon("media-skip-forward");
        case SP_MediaSkipBackward:
            return KIcon("media-skip-backward");
        case SP_MediaSeekForward:
            return KIcon("media-seek-forward");
        case SP_MediaSeekBackward:
            return KIcon("media-seek-backward");
        case SP_MediaVolume:
            return KIcon("player-volume");
        case SP_MediaVolumeMuted:
            return KIcon("player-volume-muted");
#endif
        default:
            break;
    }
    return BaseStyle::standardIconImplementation(pix, option, widget);
}

int Style::layoutSpacingImplementation(QSizePolicy::ControlType control1, QSizePolicy::ControlType control2, Qt::Orientation orientation,
                                       const QStyleOption *option, const QWidget *widget) const
{
    Q_UNUSED(control1); Q_UNUSED(control2); Q_UNUSED(orientation);

    return pixelMetric(PM_DefaultLayoutSpacing, option, widget);
}

void Style::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    prePolish(widget);
    QRect r(option->rect);
    QFlags<State> state(option->state);
    const QPalette &palette(option->palette);
    bool reverse(Qt::RightToLeft==option->direction);

    switch ((unsigned)element) {
#if defined QTC_QT4_ENABLE_KDE
    case PE_IndicatorTabClose: {
        int size(pixelMetric(QStyle::PM_SmallIconSize));
        QIcon::Mode mode(state&State_Enabled ? state& State_Raised ?
                         QIcon::Active : QIcon::Normal : QIcon::Disabled);

        if (!(state & State_Raised) && !(state & State_Sunken) &&
            !(state & QStyle::State_Selected))
            mode = QIcon::Disabled;

        drawItemPixmap(painter, r, Qt::AlignCenter,
                       KIcon("dialog-close").pixmap(size, mode,
                                                    state & State_Sunken ?
                                                    QIcon::On : QIcon::Off));
        break;
    }
#endif
    case PE_Widget:
        // TODO: handle widget == nullptr
        if (widget && widget->testAttribute(Qt::WA_StyledBackground) &&
            ((!widget->testAttribute(Qt::WA_NoSystemBackground) &&
              (qtcIsDialog(widget) || qtcIsWindow(widget)) &&
              widget->isWindow()) ||
             qobject_cast<const QMdiSubWindow*>(widget))) {
            bool isDialog = qobject_cast<const QDialog*>(widget);

            if (qtcIsCustomBgnd(opts) || m_isPreview ||
                (isDialog && opts.dlgOpacity != 100) ||
                (!isDialog && opts.bgndOpacity != 100)) {
                painter->save();
                // Blur and shadow here?
                if (!(widget && qobject_cast<const QMdiSubWindow*>(widget))) {
                    painter->setCompositionMode(
                        QPainter::CompositionMode_Source);
                }
                drawBackground(painter, widget,
                               isDialog ? BGND_DIALOG : BGND_WINDOW);
                painter->restore();
            }
        }
        break;
    case PE_PanelScrollAreaCorner:
        // disable painting of PE_PanelScrollAreaCorner
        // the default implementation fills the rect with the window background color which does not work for windows that have gradients.
        // ...but need to for WebView!!!
        if (!opts.gtkScrollViews || !qtcIsCustomBgnd(opts) ||
            (widget && widget->inherits("WebView")))
            painter->fillRect(r, palette.brush(QPalette::Window));
        break;
    case PE_IndicatorBranch: {
        int middleH((r.x() + r.width() / 2) - 1);
        int middleV(r.y() + r.height() / 2);
#if 0
        int beforeH(middleH);
#endif
        int beforeV(middleV);
        int afterH(middleH);
        int afterV(middleV);

        painter->save();

        if (state & State_Children) {
            QRect ar(r.x() + ((r.width() - (LV_SIZE + 4)) >> 1),
                     r.y() + ((r.height() - (LV_SIZE + 4)) >> 1),
                     LV_SIZE + 4, LV_SIZE + 4);
            if (/*LV_OLD == */opts.lvLines) {
                beforeV = ar.y()-1;
                afterH = ar.x() + LV_SIZE + 4;
                afterV = ar.y() + LV_SIZE + 4;
#if 0
                beforeH = ar.x();
                int lo = opts.round != ROUND_NONE ? 2 : 0;

                painter->setPen(palette.mid().color());
                painter->drawLine(ar.x() + lo, ar.y(),
                                  (ar.x() + ar.width() - 1) - lo, ar.y());
                painter->drawLine(ar.x() + lo, ar.y() + ar.height() - 1,
                                  (ar.x() + ar.width() - 1) - lo,
                                  ar.y() + ar.height() - 1);
                painter->drawLine(ar.x(), ar.y() + lo, ar.x(),
                                  (ar.y() + ar.height() - 1) - lo);
                painter->drawLine(ar.x() + ar.width() - 1,
                                  ar.y() + lo, ar.x() + ar.width() - 1,
                                  (ar.y() + ar.height() - 1) - lo);

                if (opts.round != ROUND_NONE) {
                    painter->drawPoint(ar.x() + 1, ar.y() + 1);
                    painter->drawPoint(ar.x() + 1, ar.y() + ar.height() - 2);
                    painter->drawPoint(ar.x() + ar.width() - 2, ar.y() + 1);
                    painter->drawPoint(ar.x() + ar.width() - 2,
                                       ar.y() + ar.height() - 2);

                    QColor col(palette.mid().color());

                    col.setAlphaF(0.5);
                    painter->setPen(col);
                    painter->drawLine(ar.x() + 1, ar.y() + 1,
                                      ar.x() + 2, ar.y());
                    painter->drawLine(ar.x() + ar.width() - 2,
                                      ar.y(), ar.x() + ar.width() - 1,
                                      ar.y() + 1);
                    painter->drawLine(ar.x() + 1, ar.y() + ar.height() - 2,
                                      ar.x() + 2, ar.y() + ar.height() - 1);
                    painter->drawLine(ar.x() + ar.width() - 2,
                                      ar.y() + ar.height() - 1,
                                      ar.x() + ar.width() - 1,
                                      ar.y() + ar.height() - 2);
                }
#endif
            }
            drawArrow(painter, ar, state&State_Open ?
                      PE_IndicatorArrowDown : reverse ?
                      PE_IndicatorArrowLeft : PE_IndicatorArrowRight,
                      MOArrow(state, palette, QPalette::ButtonText));
        }

        const int constStep = /*LV_OLD == */opts.lvLines ? 0
            : widget && qobject_cast<const QTreeView*>(widget) ?
            ((QTreeView*)widget)->indentation() : 20;

        if (opts.lvLines /*&& (LV_OLD == opts.lvLines ||
                           (r.x() >= constStep && constStep > 0))*/) {
            painter->setPen(palette.mid().color());
            if (state & State_Item) {
                if (reverse) {
                        painter->drawLine(r.left(), middleV, afterH, middleV);
                } else {
#if 0
                    if (LV_NEW == opts.lvLines) {
                        if(state & State_Children) {
                            painter->drawLine(middleH - constStep, middleV,
                                              r.right() - constStep, middleV);
                        } else {
                            drawFadedLine(painter,
                                          QRect(middleH - constStep, middleV,
                                                r.right() - (middleH - constStep),
                                                middleV), palette.mid().color(),
                                          false, true, true);
                        }
                    } else
#endif
                        painter->drawLine(afterH, middleV, r.right(), middleV);
                }
            }
            if (state & State_Sibling && afterV < r.bottom())
                painter->drawLine(middleH - constStep, afterV,
                                  middleH - constStep, r.bottom());
            if (state & (State_Open | State_Children | State_Item | State_Sibling) && (/*LV_NEW==opts.lvLines || */beforeV>r.y()))
                painter->drawLine(middleH-constStep, r.y(), middleH-constStep, beforeV);
        }
        painter->restore();
        break;
    }
    case PE_IndicatorViewItemCheck: {
        QStyleOption opt(*option);

        opt.state &= ~State_MouseOver;
        opt.state |= STATE_VIEW;
        drawPrimitive(PE_IndicatorCheckBox, &opt, painter, widget);
        break;
    }
    case PE_IndicatorHeaderArrow:
        if (auto header = styleOptCast<QStyleOptionHeader>(option)) {
            drawArrow(painter, r,
                      header->sortIndicator & QStyleOptionHeader::SortUp ?
                      PE_IndicatorArrowUp : PE_IndicatorArrowDown,
                      MOArrow(state, palette, QPalette::ButtonText));
        }
        break;
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowLeft:
    case PE_IndicatorArrowRight:
        if (State_None==state)
            state|=State_Enabled;
        if((QStyle::State_Enabled|QtC_StateKWin)==state) {
            drawArrow(painter, r, element, Qt::color1, false, true);
        } else {
            QColor col = MOArrow(state, palette, QPalette::Text);
            if (state & (State_Sunken | State_On) &&
               !(widget &&
                 ((opts.unifySpin && qobject_cast<const QSpinBox*>(widget)) ||
                  (opts.unifyCombo && qobject_cast<const QComboBox*>(widget) &&
                   ((const QComboBox*)widget)->isEditable())))) {
                r.adjust(1, 1, 1, 1);
            }
            if (col.alpha() < 255 &&
                PE_IndicatorArrowRight == element &&
                widget && widget->inherits("KUrlButton"))
                col = blendColors(col, palette.background().color(),
                                  col.alphaF());

            drawArrow(painter, r, element, col, false, false);
        }
        break;
    case PE_IndicatorSpinMinus:
    case PE_IndicatorSpinPlus:
    case PE_IndicatorSpinUp:
    case PE_IndicatorSpinDown: {
            QRect sr(r);
            const QColor *use(buttonColors(option));
            const QColor &col = MOArrow(state, palette, QPalette::ButtonText);
            bool down(PE_IndicatorSpinDown==element || PE_IndicatorSpinMinus==element);

            if((!opts.unifySpinBtns || state&State_Sunken) && !opts.unifySpin)
                drawLightBevel(painter, sr, option, widget, down
                                                    ? reverse
                                                            ? ROUNDED_BOTTOMLEFT
                                                            : ROUNDED_BOTTOMRIGHT
                                                        : reverse
                                                            ? ROUNDED_TOPLEFT
                                                            : ROUNDED_TOPRIGHT,
                               getFill(option, use), use, true, WIDGET_SPIN);

            if(PE_IndicatorSpinUp==element || PE_IndicatorSpinDown==element)
            {
                sr.setY(sr.y()+(down ? -2 : 1));

                if (opts.unifySpin) {
                    sr.adjust(reverse ? 1 : -1, 0, reverse ? 1 : -1, 0);
                    if (!opts.vArrows) {
                        sr.setY(sr.y()+(down ? -2 : 2));
                    }
                } else if(state&State_Sunken) {
                    sr.adjust(1, 1, 1, 1);
                }

                drawArrow(painter, sr, PE_IndicatorSpinUp==element ? PE_IndicatorArrowUp : PE_IndicatorArrowDown,
                          col, !opts.unifySpin);
            }
            else
            {
                int    l(qMin(r.width()-6, r.height()-6));
                QPoint c(r.x()+(r.width()/2), r.y()+(r.height()/2));

                l/=2;
                if(l%2 != 0)
                    --l;

                if(state&State_Sunken && !opts.unifySpin)
                    c+=QPoint(1, 1);

                painter->setPen(col);
                painter->drawLine(c.x()-l, c.y(), c.x()+l, c.y());
                if (!down) {
                    painter->drawLine(c.x(), c.y() - l, c.x(), c.y() + l);
                }
            }
            break;
        }
        case PE_IndicatorToolBarSeparator:
        {
            painter->save();
            switch(opts.toolbarSeparators)
            {
                case LINE_NONE:
                    break;
                case LINE_FLAT:
                case LINE_SUNKEN:
                    if(r.width()<r.height())
                    {
                        int x(r.x()+((r.width()-2) / 2));
                        drawFadedLine(painter, QRect(x, r.y()+TOOLBAR_SEP_GAP, 1, r.height()-(TOOLBAR_SEP_GAP*2)),
                                      m_backgroundCols[LINE_SUNKEN==opts.toolbarSeparators ? 3 : 4], true, true, false);

                        if(LINE_SUNKEN==opts.toolbarSeparators)
                            drawFadedLine(painter, QRect(x+1, r.y()+6, 1, r.height()-12), m_backgroundCols[0], true, true, false);
                    }
                    else
                    {
                        int y(r.y()+((r.height()-2) / 2));

                        drawFadedLine(painter, QRect(r.x()+TOOLBAR_SEP_GAP, y, r.width()-(TOOLBAR_SEP_GAP*2), 1),
                                      m_backgroundCols[LINE_SUNKEN==opts.toolbarSeparators ? 3 : 4], true, true, true);
                        if(LINE_SUNKEN==opts.toolbarSeparators)
                            drawFadedLine(painter, QRect(r.x()+TOOLBAR_SEP_GAP, y+1, r.width()-(TOOLBAR_SEP_GAP*2), 1),
                                          m_backgroundCols[0], true, true, true);
                    }
                    break;
                default:
                case LINE_DOTS:
                    drawDots(painter, r, !(state&State_Horizontal), 1, 5, m_backgroundCols, 0, 5);
            }
            painter->restore();
            break;
        }
        case PE_FrameGroupBox:
            if(FRAME_NONE==opts.groupBox)
                break;
            if (auto frame = styleOptCast<QStyleOptionFrame>(option)) {
                QStyleOptionFrameV2 frameV2(*frame);
                if (frameV2.features & QStyleOptionFrameV2::Flat || FRAME_LINE==opts.groupBox)
                    drawFadedLine(painter, QRect(r.x(), r.y(), r.width(), 1), backgroundColors(option)[QTC_STD_BORDER],
                                  opts.gbLabel&GB_LBL_CENTRED || reverse, opts.gbLabel&GB_LBL_CENTRED || !reverse, true);
                else
                {
                    if(opts.gbLabel&GB_LBL_OUTSIDE)
                        r.adjust(0, 2, 0, 0);

                    if(FRAME_SHADED==opts.groupBox || FRAME_FADED==opts.groupBox)
                    {
                        int          round=opts.square&SQUARE_FRAME ? ROUNDED_NONE : ROUNDED_ALL;
                        QPainterPath path(buildPath(r, WIDGET_FRAME, round,
                                                    ROUNDED_ALL==round
                                                        ? qtcGetRadius(&opts, r.width(), r.height(), WIDGET_FRAME, RADIUS_EXTERNAL)
                                                        : 0.0));

                        painter->save();
                        painter->setClipping(false);
                        if(0!=opts.gbFactor)
                        {
                            QColor col(opts.gbFactor<0 ? Qt::black : Qt::white);

                            col.setAlphaF(TO_ALPHA(opts.gbFactor));
                            if(FRAME_SHADED==opts.groupBox)
                                painter->fillPath(path, col);
                            else
                            {
                                QLinearGradient grad(r.topLeft(), r.bottomLeft());

                                grad.setColorAt(0, col);
                                col.setAlphaF(0.0);
                                grad.setColorAt(1, col);
                                painter->fillPath(path, grad);
                            }
                        }

                        if(!(opts.gbLabel&(GB_LBL_INSIDE|GB_LBL_OUTSIDE)))
                            painter->restore();

                        if(FRAME_SHADED==opts.groupBox)
                            drawBorder(painter, r, option, round, backgroundColors(option), WIDGET_FRAME,
                                        /*state&State_Raised && opts.gbFactor<0 ? BORDER_RAISED : */BORDER_SUNKEN);
                        else
                        {
                            QColor          col(backgroundColors(option)[QTC_STD_BORDER]);
                            QLinearGradient grad(r.topLeft(), r.bottomLeft());

                            col.setAlphaF(1.0);
                            grad.setColorAt(0, col);
                            col.setAlphaF(0.0);
                            grad.setColorAt(1, col);
                            painter->setRenderHint(QPainter::Antialiasing, true);
                            painter->setPen(QPen(QBrush(grad), 1));
                            painter->drawPath(path);
                        }
                        if(opts.gbLabel&(GB_LBL_INSIDE|GB_LBL_OUTSIDE))
                            painter->restore();
                    }
                    else
                    {
                        frameV2.state &= ~(State_Sunken | State_HasFocus);
                        frameV2.rect=r;
                        drawPrimitive(PE_Frame, &frameV2, painter, widget);
                    }
                }
            }
            break;
        case PE_Frame:
        {
            // Dont draw OO.o status bar frames...
            if(isOOWidget(widget) && r.height()<22)
                break;

            if (widget && qtcCheckKDEType(widget->parent(), KTitleWidget)) {
                break;
            } else if (widget && widget->parent() &&
                       qobject_cast<const QComboBox*>(widget->parent())) {
                if(opts.gtkComboMenus && !((QComboBox*)(widget->parent()))->isEditable())
                    drawPrimitive(PE_FrameMenu, option, painter, widget);
                else if(POPUP_MENUS_SQUARE(opts))
                {
                    const QColor *use(APP_KRUNNER==theThemedApp ? m_backgroundCols : backgroundColors(option));

                    painter->save();
                    painter->setPen(use[QTC_STD_BORDER]);
                    drawRect(painter, r);
                    painter->setPen(palette.base().color());
                    drawRect(painter, r.adjusted(1, 1, -1, -1));
                    painter->restore();
                }
            } else {
                auto fo = styleOptCast<QStyleOptionFrame>(option);

                if(APP_K3B==theThemedApp && !(state&(State_Sunken|State_Raised)) && fo && 1==fo->lineWidth)
                {
                    painter->save();
                    painter->setPen(backgroundColors(option)[QTC_STD_BORDER]);
                    drawRect(painter, r);
                    painter->restore();
                }
                else if((QtC_StateKWin==state || (QtC_StateKWin|State_Active)==state) && fo && 1==fo->lineWidth && 1==fo->midLineWidth)
                {
                    QColor border;
                    if(fo->version==(TBAR_BORDER_VERSION_HACK+2))
                        border=palette.color(QPalette::Active, QPalette::Shadow);
                    else
                    {
                        const QColor *borderCols(opts.windowBorder&WINDOW_BORDER_COLOR_TITLEBAR_ONLY
                                                    ? backgroundColors(palette.color(QPalette::Active, QPalette::Window))
                                                    : theThemedApp==APP_KWIN
                                                        ? buttonColors(option)
                                                        : getMdiColors(option, state&State_Active));
                        border=borderCols[fo->version==TBAR_BORDER_VERSION_HACK ? 0 : QTC_STD_BORDER];
                    }

                    border.setAlphaF(1.0);
                    painter->save();
                    painter->setRenderHint(QPainter::Antialiasing, false);
                    painter->setPen(border);
                    drawRect(painter, r);
                    painter->restore();
                }
                else
                {
                    bool kateView(isKateView(widget)),
                         kontactPreview(!kateView && isKontactPreviewPane(widget)),
                         sv(isOOWidget(widget) ||
                            qobject_cast<const QAbstractScrollArea*>(widget) ||
                            (widget && widget->inherits("Q3ScrollView")) ||
                            ((opts.square&SQUARE_SCROLLVIEW) && (kateView || kontactPreview))),
                        squareSv(sv && ((opts.square&SQUARE_SCROLLVIEW) || (widget && widget->isWindow()))),
                        inQAbstractItemView(widget && widget->parentWidget() && isInQAbstractItemView(widget->parentWidget()));
                    QtcQWidgetProps props(widget);

                    if (sv && (opts.etchEntry || squareSv ||
                               isOOWidget(widget))) {
                        // For some reason, in KPackageKit, the KTextBrower when polished is not in the scrollview,
                        // but is when painted. So check here if it should not be etched.
                        // Also, see not in getLowerEtchCol()
                        if (opts.buttonEffect != EFFECT_NONE &&
                            !USE_CUSTOM_ALPHAS(opts) && widget &&
                            widget->parentWidget() &&
                            !props->noEtch && inQAbstractItemView) {
                            props->noEtch = true;
                        }

                        // If we are set to have sunken scrollviews, then the frame width is set to 3.
                        // ...but it we are a scrollview within a scrollview, then we dont draw sunken, therefore
                        // need to draw inner border...
                        bool doEtch = (opts.buttonEffect != EFFECT_NONE &&
                                       opts.etchEntry);
                        bool noEtchW = (doEtch &&
                                        !USE_CUSTOM_ALPHAS(opts) &&
                                        props->noEtch);

                        if(doEtch && noEtchW)
                        {
                            painter->setPen(palette.brush(QPalette::Base).color());
                            drawRect(painter, r.adjusted(2, 2, -2, -2));
                        }

                        if(!opts.highlightScrollViews && fo)
                        {
                            QStyleOptionFrame opt(*fo);
                            opt.state&=~State_HasFocus;
                            drawEntryField(painter, r, widget, &opt, squareSv ? ROUNDED_NONE : ROUNDED_ALL, false, doEtch && !noEtchW, WIDGET_SCROLLVIEW);
                        }
                        else
                            drawEntryField(painter, r, widget, option, squareSv ? ROUNDED_NONE : ROUNDED_ALL, false,
                                           doEtch && !noEtchW, WIDGET_SCROLLVIEW);
                    }
                    // K3b's Disk usage status bar, etc...
//                     else if(APP_K3B==theThemedApp && widget && widget->inherits("K3b::FillStatusDisplay"))
                    else if (fo && fo->lineWidth > 0) {
                        bool kwinTab(theThemedApp == APP_KWIN && widget &&
                                     !widget->parentWidget() &&
                                     oneOf(widget->metaObject()->className(),
                                           "KWin::TabBox"));
                        QStyleOption opt(*option);

                        painter->save();

                        if(kwinTab)
                            r.adjust(-1, -1, 1, 1);

                        if(!opts.highlightScrollViews)
                            opt.state&=~State_HasFocus;

                        if(opts.round && qtcIsFlatBgnd(opts.bgndAppearance) && 100==opts.bgndOpacity &&
                           widget && widget->parentWidget() && !inQAbstractItemView/* &&
                           widget->palette().background().color()!=widget->parentWidget()->palette().background().color()*/)
                        {
                            painter->setPen(widget->parentWidget()->palette().background().color());
                            painter->drawRect(r);
                            painter->drawRect(r.adjusted(1, 1, -1, -1));
                        }

                        if(sv || kateView || kontactPreview)
                        {
                            painter->setRenderHint(QPainter::Antialiasing, true);
                            painter->setPen(option->palette.brush(opts.thin&THIN_FRAMES && !(opts.square&SQUARE_SCROLLVIEW)
                                                                    ? QPalette::Window
                                                                    : QPalette::Base).color());
                            painter->drawPath(buildPath(r.adjusted(1, 1, -1, -1), WIDGET_SCROLLVIEW, ROUNDED_ALL,
                                                        qtcGetRadius(&opts, r.width()-2, r.height()-2, WIDGET_SCROLLVIEW, RADIUS_INTERNAL)));
                            painter->setRenderHint(QPainter::Antialiasing, false);
                        }

                        drawBorder(painter, r, &opt,
                                   opts.round ?  getFrameRound(widget) : ROUND_NONE, backgroundColors(option),
                                   sv || kateView || kontactPreview ? WIDGET_SCROLLVIEW : WIDGET_FRAME,
                                   state&State_Sunken || state&State_HasFocus
                                        ? BORDER_SUNKEN
                                        : state&State_Raised
                                            ? BORDER_RAISED
                                            : BORDER_FLAT);
                        painter->restore();
                    }
                }
            }
            break;
        }
        case PE_PanelMenuBar:
            if (widget && widget->parentWidget() && (qobject_cast<const QMainWindow*>(widget->parentWidget()) ||
                                                     widget->parentWidget()->inherits("Q3MainWindow")))
            {
                painter->save();
                if (!opts.xbar ||
                    (!widget || noneOf(widget->metaObject()->className(),
                                       "QWidget")))
                    drawMenuOrToolBarBackground(widget, painter, r, option);
                if (opts.toolbarBorders != TB_NONE) {
                    const QColor *use = (m_active ? m_menubarCols :
                                         backgroundColors(option));
                    bool dark(oneOf(opts.toolbarBorders, TB_DARK, TB_DARK_ALL));

                    if (oneOf(opts.toolbarBorders, TB_DARK_ALL, TB_LIGHT_ALL)) {
                        painter->setPen(use[0]);
                        painter->drawLine(r.x(), r.y(),
                                          r.x() + r.width() - 1, r.y());
                        painter->drawLine(r.x(), r.y(), r.x(),
                                          r.y() + r.height() - 1);
                        painter->setPen(use[dark ? 3 : 4]);
                        painter->drawLine(r.x(), r.y() + r.height() - 1,
                                          r.x() + r.width() - 1,
                                          r.y() + r.height() - 1);
                        painter->drawLine(r.x() + r.width() - 1, r.y(),
                                          r.x() + r.width() - 1,
                                          r.y() + r.height() - 1);
                    } else {
                        painter->setPen(use[dark ? 3 : 4]);
                        painter->drawLine(r.x(), r.y() + r.height() - 1,
                                          r.x() + r.width() - 1,
                                          r.y() + r.height() - 1);
                    }
                }
                painter->restore();
            }
            break;
        case PE_FrameTabBarBase:
            if (auto tbb = styleOptCast<QStyleOptionTabBarBase>(option)) {
                if(tbb->shape != QTabBar::RoundedNorth && tbb->shape != QTabBar::RoundedWest &&
                   tbb->shape != QTabBar::RoundedSouth && tbb->shape != QTabBar::RoundedEast)
                    BaseStyle::drawPrimitive(element, option, painter, widget);
                else
                {
                    static const int constSidePad=16*2;
                    const QColor *use(backgroundColors(option));
                    QRegion      region(tbb->rect);
                    QLine        topLine(tbb->rect.bottomLeft() - QPoint(0, 1), tbb->rect.bottomRight() - QPoint(0, 1)),
                                 bottomLine(tbb->rect.bottomLeft(), tbb->rect.bottomRight());
                    bool         horiz(QTabBar::RoundedNorth==tbb->shape || QTabBar::RoundedSouth==tbb->shape);
                    double       size=horiz ? tbb->rect.width() : tbb->rect.height(),
                                 tabRectSize=horiz ? tbb->tabBarRect.width() : tbb->tabBarRect.height(),
                                 tabFadeSize=tabRectSize+constSidePad > size ? 0.0 : 1.0-((tabRectSize+constSidePad)/size),
                                 minFadeSize=1.0-((size-constSidePad)/size),
                                 fadeSizeStart=minFadeSize,
                                 fadeSizeEnd=tabFadeSize<minFadeSize ? minFadeSize : (tabFadeSize>FADE_SIZE ? FADE_SIZE : tabFadeSize);

                    if(reverse && horiz)
                        fadeSizeStart=fadeSizeEnd, fadeSizeEnd=minFadeSize;

                    region -= tbb->tabBarRect;

                    painter->save();
                    painter->setClipRegion(region);
                    bool fadeState=true, fadeEnd=true;

                    // Dont fade start/end of tabbar in KDevelop's menubar
                    if (theThemedApp == APP_KDEVELOP &&
                        qtcCheckType<QMenuBar>(getParent<2>(widget)) &&
                        qobject_cast<const QTabBar*>(widget))
                        fadeState=fadeEnd=false;

                    drawFadedLine(painter, QRect(topLine.p1(), topLine.p2()),
                                  QTabBar::RoundedSouth==tbb->shape && APPEARANCE_FLAT==opts.appearance
                                    ? palette.background().color()
                                    : use[QTabBar::RoundedNorth==tbb->shape ? QTC_STD_BORDER
                                                                            : (opts.borderTab ? 0 : FRAME_DARK_SHADOW)],
                                  fadeState, fadeEnd, horiz, fadeSizeStart, fadeSizeEnd);
                    if(!(opts.thin&THIN_FRAMES))
                        drawFadedLine(painter, QRect(bottomLine.p1(), bottomLine.p2()),
                                      use[QTabBar::RoundedNorth==tbb->shape ? 0 : QTC_STD_BORDER],
                                      fadeState, fadeEnd, horiz, fadeSizeStart, fadeSizeEnd);
                    painter->restore();
                }
            }
            break;
        case PE_FrameStatusBar:
            if(!opts.drawStatusBarFrames)
                break;
            QTC_FALLTHROUGH();
        case PE_FrameMenu:
            if(POPUP_MENUS_SQUARE(opts) &&
                (qtcIsFlatBgnd(opts.menuBgndAppearance) ||
                (opts.gtkComboMenus && widget && widget->parent() && qobject_cast<const QComboBox*>(widget->parent()))))
            {
                const QColor    *use(popupMenuCols(option));
                EGradientBorder border=qtcGetGradient(opts.menuBgndAppearance, &opts)->border;
                painter->save();
                painter->setPen(use[QTC_STD_BORDER]);
                drawRect(painter, r);

                if(qtcUseBorder(border) && APPEARANCE_FLAT!=opts.menuBgndAppearance)
                {
                    painter->setPen(use[0]);
                    if(GB_LIGHT==border)
                         drawRect(painter, r.adjusted(1, 1, -1, -1));
                    else
                    {
                        if(GB_3D!=border)
                        {
                            painter->drawLine(r.x()+1, r.y()+1, r.x()+r.width()-2,  r.y()+1);
                            painter->drawLine(r.x()+1, r.y()+1, r.x()+1,  r.y()+r.height()-2);
                        }
                        painter->setPen(use[FRAME_DARK_SHADOW]);
                        painter->drawLine(r.x()+1, r.y()+r.height()-2, r.x()+r.width()-2,  r.y()+r.height()-2);
                        painter->drawLine(r.x()+r.width()-2, r.y()+1, r.x()+r.width()-2,  r.y()+r.height()-2);
                    }
                }
                painter->restore();
            }
            break;
        case PE_FrameDockWidget:
        {
            const QColor *use(backgroundColors(option));

            painter->save();
            painter->setPen(use[0]);
            painter->drawLine(r.x(), r.y(), r.x()+r.width()-1, r.y());
            painter->drawLine(r.x(), r.y(), r.x(), r.y()+r.height()-1);
            painter->setPen(use[APPEARANCE_FLAT==opts.appearance ? ORIGINAL_SHADE : QTC_STD_BORDER]);
            painter->drawLine(r.x(), r.y()+r.height()-1, r.x()+r.width()-1, r.y()+r.height()-1);
            painter->drawLine(r.x()+r.width()-1, r.y(), r.x()+r.width()-1, r.y()+r.height()-1);
            painter->restore();
            break;
        }
        case PE_FrameButtonTool:
        case PE_PanelButtonTool:
            if(isMultiTabBarTab(getButton(widget, painter)))
            {
                if(!opts.stdSidebarButtons)
                    drawSideBarButton(painter, r, option, widget);
                else if((state&State_Enabled) || !(state&State_AutoRaise) )
                {
                    QStyleOption opt(*option);
                    opt.state|=STATE_TBAR_BUTTON;
                    drawPrimitive(PE_PanelButtonCommand, &opt, painter, widget);
                }
                break;
            }
            QTC_FALLTHROUGH();
        case PE_IndicatorButtonDropDown: // This should never be called, but just in case - draw as a normal toolbutton...
        {
            bool dwt(widget && widget->inherits("QDockWidgetTitleButton")),
                 koDwt(!dwt && widget && widget->parentWidget() && widget->parentWidget()->inherits("KoDockWidgetTitleBar"));

            if(((state&State_Enabled) || !(state&State_AutoRaise)) &&
               (!widget || !(dwt || koDwt)|| (state&State_MouseOver)) )
            {
                QStyleOption opt(*option);

                if(dwt || koDwt)
                    opt.state|=STATE_DWT_BUTTON;
                drawPrimitive(PE_PanelButtonCommand, &opt, painter, widget);
            }
            break;
        }
        case PE_IndicatorDockWidgetResizeHandle:
        {
            QStyleOption dockWidgetHandle = *option;
            bool horizontal = state&State_Horizontal;
            if (horizontal)
                dockWidgetHandle.state &= ~State_Horizontal;
            else
                dockWidgetHandle.state |= State_Horizontal;
            drawControl(CE_Splitter, &dockWidgetHandle, painter, widget);
            break;
        }
        case PE_PanelLineEdit:
            if (auto panel = styleOptCast<QStyleOptionFrame>(option)) {
                if(panel->lineWidth > 0)
                {
                    QRect r2(r.adjusted(1, 1, -1, opts.buttonEffect !=
                                        EFFECT_NONE ? -2 : -1));
                    painter->fillPath(buildPath(r2, WIDGET_ENTRY, ROUNDED_ALL, qtcGetRadius(&opts, r2.width(), r2.height(), WIDGET_ENTRY, RADIUS_INTERNAL)),
                                      palette.brush(QPalette::Base));
                    drawPrimitive(PE_FrameLineEdit, option, painter, widget);
                }
                else
                    painter->fillRect(r.adjusted(2, 2, -2, -2), palette.brush(QPalette::Base));
            }
            break;
        case PE_FrameLineEdit:
            if (auto lineEdit = styleOptCast<QStyleOptionFrame>(option)) {
                if ((lineEdit->lineWidth>0 || isOOWidget(widget)) &&
                    !(widget &&
                     (qobject_cast<const QComboBox*>(widget->parentWidget()) ||
                      qobject_cast<const QAbstractSpinBox*>(widget->parentWidget()))))
                {
                    QStyleOptionFrame opt(*lineEdit);

                    if(opt.state&State_Enabled && state&State_ReadOnly)
                        opt.state^=State_Enabled;

                    if (opts.buttonEffect != EFFECT_NONE && opts.etchEntry &&
                        theThemedApp == APP_ARORA && widget &&
                        widget->parentWidget() &&
                        oneOf(widget->metaObject()->className(), "LocationBar")) {
                        if (const QToolBar *tb =
                            getToolBar(widget->parentWidget()/*, false*/)) {
                            QRect r2(r);
                            struct TB: public QToolBar {
                                void
                                initOpt(QStyleOptionToolBar *opt)
                                {
                                    initStyleOption(opt);
                                }
                            };

                            QStyleOptionToolBar opt;

                            ((TB*)tb)->initOpt(&opt);

                            painter->save();

                            // Only need to adjust coords if toolbar has a gradient...
                            if (!qtcIsFlat(opts.toolbarAppearance)) {
                                r2.setY(-widget->mapTo((QWidget*)tb, QPoint(r.x(), r.y())).y());
                                r2.setHeight(tb->rect().height());
                            }
                            painter->setClipRegion(QRegion(r2).subtract(QRegion(r2.adjusted(2, 2, -2, -2))));
                            drawMenuOrToolBarBackground(widget, painter, r2, &opt, false, true);
                            painter->restore();
                        }
                    }
                    painter->save();
                    bool  isOO(isOOWidget(widget));
                    QRect rect(r);
                    int   round(ROUNDED_ALL);

                    if(isOO)
                    {
                        // This (hopefull) checks is we're OO.o 3.2 - in which case no adjustment is required...
                        const QImage *img=getImage(painter);

                        if(!img || img->rect()!=r) // OO.o 3.1?
                            rect.adjust(1, 2, -1, -2);
                        else
                        {
                            round=ROUNDED_NONE;
                            painter->fillRect(r, palette.brush(QPalette::Window));
                            rect.adjust(1, 1, -1, -1);
                        }
                    }

                    drawEntryField(painter, rect, widget, &opt, round, isOO,
                                   !isOO && opts.buttonEffect != EFFECT_NONE);
                    painter->restore();
                }
            }
            break;
        case PE_Q3CheckListIndicator:
            if (auto lv = styleOptCast<QStyleOptionQ3ListView>(option)) {
                if(lv->items.isEmpty())
                    break;

                QStyleOptionQ3ListViewItem item(lv->items.at(0));
                int                        x(lv->rect.x()),
                                           w(lv->rect.width()),
                                           marg(lv->itemMargin);

                if (state & State_Selected && !lv->rootIsDecorated && !(item.features & QStyleOptionQ3ListViewItem::ParentControl))
                    painter->fillRect(0, 0, x + marg + w + 4, item.height, palette.brush(QPalette::Highlight));
            }

            r.setX(r.x()+((r.width()-opts.crSize)/2)-1);
            r.setY(r.y()+((r.height()-opts.crSize)/2)-1);
            r.setWidth(opts.crSize);
            r.setHeight(opts.crSize);
            QTC_FALLTHROUGH();
        case PE_IndicatorMenuCheckMark:
        case PE_IndicatorCheckBox:
        {
            bool  menu(state&STATE_MENU),
                  view(state&STATE_VIEW),
                  doEtch(opts.buttonEffect != EFFECT_NONE &&
                         (opts.crButton || (PE_IndicatorMenuCheckMark!=element && !menu &&
                                            r.width()>=opts.crSize+2 && r.height()>=opts.crSize+2))),
                  isOO(isOOWidget(widget)),
                  selectedOOMenu(isOO && (r==QRect(0, 0, 15, 15) || r==QRect(0, 0, 14, 15)) &&  // OO.o 3.2 =14x15?
                                ((State_Sunken|State_Enabled)==state || (State_Sunken|State_Enabled|State_Selected)==state));
            int   crSize(opts.crSize+(doEtch ? 2 : 0));
            QRect rect(r.x(), r.y()+(view ? -1 : 0), crSize, crSize);
            // render checkable menu items with only a tick; should become an option
            bool onlyTicksInMenu = menu && opts.onlyTicksInMenu;

            painter->save();

            // For OO.o 3.2 need to fill widget background!
            if(isOO)
                painter->fillRect(r, palette.brush(QPalette::Window));

            if(selectedOOMenu)
            {
                if(r==QRect(0, 0, 14, 15)) // OO.o 3.2 =14x15?
                    rect.adjust(-1, -1, -1, -1);
                painter->setPen(option ? option->palette.text().color() : QApplication::palette().text().color());
                drawRect(painter, r);
                // LibreOffice its 15x15 - and arrow is not centred, so adjust this...
                if(r==QRect(0, 0, 15, 15))
                    rect.adjust(-1, -1, -1, -1);
            }
            else if (!onlyTicksInMenu)
            {
                if(isOO && r==QRect(0, 0, opts.crSize, opts.crSize))
                    rect.adjust(0, -1, 0, -1);

                if(CR_SMALL_SIZE!=opts.crSize)
                {
                    if(menu)
                        rect.adjust(0, -1, 0, -1);
                    else if(r.height()>crSize)   // Can only adjust position if there is space!
                        rect.adjust(0, 1, 0, 1); // ...when used in a listview, usually there is no space.
                }

                if(opts.crButton)
                {
                    const QColor *use(checkRadioColors(option));
                    QStyleOption opt(*option);

                    if(menu || selectedOOMenu)
                        opt.state&=~(State_MouseOver|State_Sunken);
                    opt.state&=~State_On;
                    opt.state|=State_Raised;
                    opt.rect=rect;
                    drawLightBevel(painter, rect, &opt, widget, ROUNDED_ALL, getFill(&opt, use, true, false),
                                   use, true, WIDGET_CHECKBOX);
                }
                else
                {
                    bool         sunken(!menu && !selectedOOMenu && (state&State_Sunken)),
                                 mo(!sunken && state&State_MouseOver && state&State_Enabled),
                                 glow(doEtch && MO_GLOW==opts.coloredMouseOver && mo);
                    const QColor *bc(sunken ? 0L : borderColors(option, 0L)),
                                 *btn(checkRadioColors(option)),
                                 *use(bc ? bc : btn);
                    const QColor &bgnd(state&State_Enabled && !sunken
                                            ? MO_NONE==opts.coloredMouseOver && !opts.crHighlight && mo
                                                ? use[CR_MO_FILL]
                                                : palette.base().color()
                                            : palette.background().color());
                    bool         lightBorder=DRAW_LIGHT_BORDER(false, WIDGET_TROUGH, APPEARANCE_INVERTED);

                    rect=QRect(doEtch ? rect.adjusted(1, 1, -1, -1) : rect);

                    if(qtcIsFlat(opts.appearance))
                        painter->fillRect(rect.adjusted(1, 1, -1, -1), bgnd);
                    else
                        drawBevelGradient(bgnd, painter, rect.adjusted(1, 1, -1, -1), true, false, APPEARANCE_INVERTED, WIDGET_TROUGH);

                    if(MO_NONE!=opts.coloredMouseOver && !glow && mo)
                    {
                        painter->setRenderHint(QPainter::Antialiasing, true);
                        painter->setPen(use[CR_MO_FILL]);
                        drawAaRect(painter, rect.adjusted(1, 1, -1, -1));
                        painter->setRenderHint(QPainter::Antialiasing, false);
                    }
                    else
                    {
                        painter->setPen(midColor(state&State_Enabled ? palette.base().color()
                                                                    : palette.background().color(), use[3]));
                        if(lightBorder)
                            drawRect(painter, rect.adjusted(1, 1, -1, -1));
                        else
                        {
                            painter->drawLine(rect.x()+1, rect.y()+1, rect.x()+1, rect.y()+rect.height()-2);
                            painter->drawLine(rect.x()+1, rect.y()+1, rect.x()+rect.width()-2, rect.y()+1);
                        }
                    }

                    if(doEtch && !view)
                    {
                        if(glow && !(opts.thin&THIN_FRAMES))
                            drawGlow(painter, r, WIDGET_CHECKBOX);
                        else
                            drawEtch(painter, r, widget, WIDGET_CHECKBOX, opts.crButton && EFFECT_SHADOW==opts.buttonEffect ? !sunken : false);
                    }

                    drawBorder(painter, rect, option, ROUNDED_ALL, use, WIDGET_CHECKBOX);
                }
            }

            if(state&State_On || selectedOOMenu)
            {
                // widget can be NULL in LibreOffice: see http://crashreport.libreoffice.org/stats/signature/qtcurve.so
                // In a pure Qt build we don't really have a way of knowing what font to use without a widget,
                // but in a KDE build we can use KGlobalSettings::menuFont()
                if (onlyTicksInMenu
#ifndef QTC_QT4_ENABLE_KDE
                    && widget
#endif
                ) {
                    // only tickmarks (= without the box) in menu; adjust its horizontal position
                    rect.adjust(6, 0, 0, 0);
                    // get the font for adjusting; widget will be the menu holding the menuitem
                    // but that uses the same font as the menuitem.
#ifdef QTC_QT4_ENABLE_KDE
                    QFont font(widget ? widget->font() : KGlobalSettings::menuFont());
#else
                    QFont font(widget->font());
#endif
#ifndef Q_OS_MAC
                    font.setBold(true);
#else
                    // not that there is much variation in the CheckMark symbol on OS X, but
                    // we pick one from a font that's always supposed to be there and that's
                    // a good choice functionally speaking.
                    font.setFamily("Apple Symbols");
#endif
                    // adjust the size so the tickmark looks just about right
                    font.setPointSizeF(font.pointSizeF() * 1.3);
                    painter->save();
                    painter->setFont(font);
                    // render the tickmark using the Unicode "Check Mark" symbol (✓)
                    drawItemTextWithRole(painter, rect, Qt::AlignHCenter|Qt::AlignVCenter, palette, true,
                                     QString(QChar(0x2713)), QPalette::Text);
                    painter->restore();
                }
                else
                {
                        QPixmap *pix(getPixmap(checkRadioCol(option), PIX_CHECK, 1.0));
                        painter->drawPixmap(rect.center().x()-(pix->width()/2), rect.center().y()-(pix->height()/2), *pix);
                }
            }
            else if (state&State_NoChange)    // tri-state
            {
                int x(rect.center().x()), y(rect.center().y());

                painter->setPen(checkRadioCol(option));
                painter->drawLine(x-3, y, x+3, y);
                painter->drawLine(x-3, y+1, x+3, y+1);
            }

            painter->restore();
            break;
        }
        case PE_Q3CheckListExclusiveIndicator:
            if (auto lv = styleOptCast<QStyleOptionQ3ListView>(option)) {
                if(lv->items.isEmpty())
                    break;

                QStyleOptionQ3ListViewItem item(lv->items.at(0));
                int                        x(lv->rect.x()),
                                           w(lv->rect.width()),
                                           marg(lv->itemMargin);

                if (state & State_Selected && !lv->rootIsDecorated && !(item.features & QStyleOptionQ3ListViewItem::ParentControl))
                    painter->fillRect(0, 0, x + marg + w + 4, item.height, palette.brush(QPalette::Highlight));
            }

            r.setX(r.x()+((r.width()-opts.crSize)/2)-1);
            r.setY(r.y()+((r.height()-opts.crSize)/2)-1);
            r.setWidth(opts.crSize);
            r.setHeight(opts.crSize);
            QTC_FALLTHROUGH();
        case PE_IndicatorRadioButton:
        {
            bool isOO(isOOWidget(widget)),
                 selectedOOMenu(isOO && (r==QRect(0, 0, 15, 15) || r==QRect(0, 0, 14, 15)) &&  // OO.o 3.2 =14x15?
                                ((State_Sunken|State_Enabled)==state || (State_Sunken|State_Enabled|State_Selected)==state));

            if(isOO)
                painter->fillRect(r, palette.brush(QPalette::Background));

            if(selectedOOMenu)
                drawPrimitive(PE_IndicatorCheckBox, option, painter, widget);
            else
            {
                bool menu(state&STATE_MENU);
                int  x(r.x()), y(r.y());

                painter->save();

                if(opts.crButton)
                {
                    const QColor *use(checkRadioColors(option));
                    QStyleOption opt(*option);
                    bool         doEtch(opts.buttonEffect != EFFECT_NONE);
                    QRect        rect(r.x(), r.y(), opts.crSize+(doEtch ? 2 : 0), opts.crSize+(doEtch ? 2 : 0));

                    if(CR_SMALL_SIZE!=opts.crSize && menu)
                        rect.adjust(0, -1, 0, -1), y++;

                    if(isOO && r==QRect(0, 0, opts.crSize, opts.crSize))
                        rect.adjust(-1, -1, -1, -1), --x, --y;

                    if(menu || selectedOOMenu)
                        opt.state&=~(State_MouseOver|State_Sunken);
                    opt.state&=~State_On;
                    opt.state|=State_Raised;
                    opt.rect=rect;

                    if(doEtch)
                        x++, y++;
                    if(CR_SMALL_SIZE!=opts.crSize && menu)
                        y-=2;

                    drawLightBevel(painter, rect, &opt, widget, ROUNDED_ALL, getFill(&opt, use, true, false), use, true, WIDGET_RADIO_BUTTON);
                }
                else
                {
                    bool         sunken(!menu && !selectedOOMenu && (state&State_Sunken)),
                                 doEtch(!menu
                                        && r.width()>=opts.crSize+2 && r.height()>=opts.crSize+2
                                        && opts.buttonEffect != EFFECT_NONE),
                                 mo(!sunken && state&State_MouseOver && state&State_Enabled),
                                 glow(doEtch && MO_GLOW==opts.coloredMouseOver && mo),
                                 coloredMo(MO_NONE!=opts.coloredMouseOver && !glow && mo && !sunken);
                    bool         lightBorder=DRAW_LIGHT_BORDER(false, WIDGET_TROUGH, APPEARANCE_INVERTED),
                                 doneShadow=false;
                    QRect        rect(doEtch ? r.adjusted(1, 1, -1, -1) : r);
                    const QColor *bc(sunken ? 0L : borderColors(option, 0L)),
                                 *btn(checkRadioColors(option)),
                                 *use(bc ? bc : btn);

                    if(doEtch)
                        x++, y++;

                    const QColor &bgnd(state&State_Enabled && !sunken
                                            ? MO_NONE==opts.coloredMouseOver && !opts.crHighlight && mo
                                                ? use[CR_MO_FILL]
                                                : palette.base().color()
                                            : palette.background().color());
                    QPainterPath path;

                    path.addEllipse(QRectF(rect).adjusted(0.5, 0.5, -1.0, -1.0));
                    drawBevelGradient(bgnd, painter, rect.adjusted(1, 1, -1, -1), path, true, false, APPEARANCE_INVERTED, WIDGET_TROUGH);
                    painter->setRenderHint(QPainter::Antialiasing, true);
                    if(coloredMo)
                    {
                        painter->setBrush(Qt::NoBrush);
                        painter->setPen(use[CR_MO_FILL]);
                        painter->drawArc(QRectF(x+1, y+1, opts.crSize-2, opts.crSize-2), 0, 360*16);
                        painter->drawArc(QRectF(x+2, y+2, opts.crSize-4, opts.crSize-4), 0, 360*16);
                    }

                    painter->setBrush(Qt::NoBrush);
                    if(!doneShadow && doEtch && (glow || EFFECT_NONE!=opts.buttonEffect || sunken))
                    {
                        QColor topCol(glow ? m_mouseOverCols[GLOW_MO] : Qt::black);

                        if(!glow)
                            topCol.setAlphaF(ETCH_RADIO_TOP_ALPHA);

                        painter->setPen(topCol);
                        painter->drawArc(QRectF(x-0.5, y-0.5, opts.crSize+1, opts.crSize+1), 45*16, 180*16);
                        if(!glow)
                            painter->setPen(getLowerEtchCol(widget));
                        painter->drawArc(QRectF(x-0.5, y-0.5, opts.crSize+1, opts.crSize+1), 225*16, 180*16);
                    }

                    painter->setPen(use[BORDER_VAL(state&State_Enabled)]);
                    painter->drawArc(QRectF(x+0.25, y+0.25, opts.crSize-0.5, opts.crSize-0.5), 0, 360*16);
                    if(!coloredMo)
                    {
                        painter->setPen(btn[state&State_MouseOver ? 3 : 4]);
                        painter->drawArc(QRectF(x+0.75, y+0.75, opts.crSize-1.5, opts.crSize-1.5), lightBorder ? 0 : 45*16, lightBorder ? 360*16 : 180*16);
                    }
                }
                if(state&State_On || selectedOOMenu)
                {
                    QPainterPath path;
                    double       radius=opts.smallRadio ? 2.75 : 3.75,
                                offset=(opts.crSize/2.0)-radius;

                    path.addEllipse(QRectF(x+offset, y+offset, radius*2.0, radius*2.0));
                    painter->setRenderHint(QPainter::Antialiasing, true);
                    painter->fillPath(path, checkRadioCol(option));
                }

                painter->restore();
            }
            break;
        }
        case PE_IndicatorToolBarHandle:
            painter->save();
            drawHandleMarkers(painter, r, option, true, opts.handles);
            painter->restore();
            break;
        case PE_FrameFocusRect:
            if (FOCUS_NONE==opts.focus)
                return;
            if (auto focusFrame = styleOptCast<QStyleOptionFocusRect>(option)) {
                if (!(focusFrame->state&State_KeyboardFocusChange) ||
                    (widget && widget->inherits("QComboBoxListView")))
                    return;

                if(widget && FOCUS_GLOW==opts.focus)
                {
                    if(qobject_cast<const QAbstractButton*>(widget))
                    {
                        if(!qobject_cast<const QToolButton*>(widget) || !static_cast<const QToolButton*>(widget)->autoRaise())
                            return;
                    }
                    else if(qobject_cast<const QComboBox*>(widget) || qobject_cast<const QGroupBox*>(widget) ||
                            qobject_cast<const QDial*>(widget))
                        return;
                }

                QRect r2(r);

                if(widget && (qobject_cast<const QCheckBox*>(widget) || qobject_cast<const QRadioButton*>(widget)) &&
                   ((QAbstractButton*)widget)->text().isEmpty() && r.height()<=widget->rect().height()-2 && r.width()<=widget->rect().width()-2 &&
                   r.x()>=1 && r.y()>=1)
                {
                    int adjust=qMin(qMin(abs(widget->rect().x()-r.x()), 2), abs(widget->rect().y()-r.y()));
                    r2.adjust(-adjust, -adjust, adjust, adjust);
                }

                if(widget && qobject_cast<const QGroupBox*>(widget))
                   r2.adjust(0, 2, 0, 0);

                if(FOCUS_STANDARD==opts.focus)
                {
                    // Taken from QWindowsStyle...
                    painter->save();
                    painter->setBackgroundMode(Qt::TransparentMode);
                    QColor bgCol(focusFrame->backgroundColor);
                    if (!bgCol.isValid())
                        bgCol = painter->background().color();
                    // Create an "XOR" color.
                    QColor patternCol((bgCol.red() ^ 0xff) & 0xff,
                                      (bgCol.green() ^ 0xff) & 0xff,
                                      (bgCol.blue() ^ 0xff) & 0xff);
                    painter->setBrush(QBrush(patternCol, Qt::Dense4Pattern));
                    painter->setBrushOrigin(r.topLeft());
                    painter->setPen(Qt::NoPen);
                    painter->drawRect(r.left(), r.top(), r.width(), 1);    // Top
                    painter->drawRect(r.left(), r.bottom(), r.width(), 1); // Bottom
                    painter->drawRect(r.left(), r.top(), 1, r.height());   // Left
                    painter->drawRect(r.right(), r.top(), 1, r.height());  // Right
                    painter->restore();
                }
                else
                {
                    //Figuring out in what beast we are painting...
                    bool view(state&State_Item ||
                              ((widget && ((qobject_cast<const QAbstractScrollArea*>(widget)) ||
                                        widget->inherits("Q3ScrollView"))) ||
                                         (widget && widget->parent() &&
                                            ((qobject_cast<const QAbstractScrollArea*>(widget->parent())) ||
                                              widget->parent()->inherits("Q3ScrollView")))) );

                    if(!view && !widget)
                    {
                        // Try to determine if we are in a KPageView...
                        const QWidget *wid=getWidget(painter);

                        if(wid && wid->parentWidget())
                        {
                            if(wid->parentWidget()->inherits("KDEPrivate::KPageListView"))
                            {
                                r2.adjust(2, 2, -2, -2);
                                view=true;
                            }
                            else if(APP_KONTACT==theThemedApp && (wid->parentWidget()->inherits("KMail::MainFolderView") ||
                                                                  wid->parentWidget()->inherits("MessageList::Core::View")))
                            {
                                view=true;
                            }
                        }
                    }
                    painter->save();
                    QColor c(view && state&State_Selected
                                  ? palette.highlightedText().color()
                                  : m_focusCols[FOCUS_SHADE(state&State_Selected)]);

                    if(FOCUS_LINE==opts.focus || FOCUS_GLOW==opts.focus)
                    {
                        if(!(state&State_Horizontal) && widget && qobject_cast<const QTabBar*>(widget))
                            drawFadedLine(painter, QRect(r2.x()+r2.width()-1, r2.y(), 1, r2.height()), c, true, true, false);
                        else
                            drawFadedLine(painter, QRect(r2.x(), r2.y()+r2.height()-(view ? 3 : 1), r2.width(), 1), c, true, true, true);
                    }
                    else
                    {
                        //if(FOCUS_GLOW==opts.focus)
                        //    c.setAlphaF(FOCUS_GLOW_LINE_ALPHA);
                        painter->setPen(c);
                        if(FOCUS_FILLED==opts.focus)
                        {
                            c.setAlphaF(FOCUS_ALPHA);
                            painter->setBrush(c);
                        }

                        if (opts.round != ROUND_NONE) {
                            bool square((opts.square&SQUARE_LISTVIEW_SELECTION) &&
                                        ((/*(!widget && r.height()<=40 && r.width()>=48) || */
                                          (widget && !widget->inherits("KFilePlacesView") &&
                                          (qobject_cast<const QTreeView*>(widget) ||
                                            (qobject_cast<const QListView*>(widget) &&
                                             QListView::IconMode!=((const QListView*)widget)->viewMode())))) ||
                                          (!widget && view) ) );

                            painter->setRenderHint(QPainter::Antialiasing, true);
                            painter->drawPath(buildPath(r2, WIDGET_SELECTION, ROUNDED_ALL,
                                                        square ? SLIGHT_INNER_RADIUS : qtcGetRadius(&opts, r2.width(), r2.height(), WIDGET_OTHER,
                                                                                                    oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED) ? RADIUS_EXTERNAL : RADIUS_SELECTION)));
                        }
                        else
                            drawRect(painter, r2);
                    }
                    painter->restore();
                }
            }
            break;
        case PE_FrameButtonBevel:
        case PE_PanelButtonBevel:
        case PE_PanelButtonCommand:
        {
            if(state&STATE_DWT_BUTTON && (opts.dwtSettings&DWT_BUTTONS_AS_PER_TITLEBAR))
                break;

            bool doEtch = opts.buttonEffect != EFFECT_NONE;

            // This fixes the "Sign in" button at mail.lycos.co.uk
            // ...basically if KHTML gices us a fully transparent background colour, then
            // dont paint the button.
            if(0==option->palette.button().color().alpha())
            {
                if(state&State_MouseOver && state&State_Enabled && MO_GLOW==opts.coloredMouseOver && doEtch && !(opts.thin&THIN_FRAMES))
                    drawGlow(painter, r, WIDGET_STD_BUTTON);
                return;
            }

            if(!widget)
                widget=getWidget(painter);

            const QColor *use(buttonColors(option));
            bool         isDefault(false),
                         isFlat(false),
                         isKWin(state&QtC_StateKWin),
                         isDown(state&State_Sunken || state&State_On),
                         isOnListView(!isKWin && widget && qobject_cast<const QAbstractItemView*>(widget));
            QStyleOption opt(*option);

            if(PE_PanelButtonBevel==element)
                opt.state|=State_Enabled;

            if (auto button = styleOptCast<QStyleOptionButton>(option)) {
                isDefault = (button->features & QStyleOptionButton::DefaultButton) && (button->state&State_Enabled);
                isFlat = (button->features & QStyleOptionButton::Flat);
            }

            if(!(opt.state&State_Enabled))
                opt.state&=~State_MouseOver;

            // For some reason with OO.o not all buttons are set as raised!
            if(!(opt.state&State_AutoRaise))
                opt.state|=State_Raised;

            isDefault = (isDefault ||
                         (doEtch && opts.coloredMouseOver == MO_GLOW &&
                          oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED) &&
                          opt.state & State_HasFocus &&
                          opt.state & State_Enabled));
            if(isFlat && !isDown && !(opt.state&State_MouseOver))
                return;

            painter->save();

            if(isOnListView)
                opt.state|=State_Horizontal|State_Raised;

            if(isDefault && state&State_Enabled && (IND_TINT==opts.defBtnIndicator || IND_SELECTED==opts.defBtnIndicator))
                use=m_defBtnCols;
            else if(state&STATE_DWT_BUTTON && widget && opts.titlebarButtons&TITLEBAR_BUTTON_COLOR &&
                    coloredMdiButtons(state&State_Active, state&State_MouseOver) &&
                    !(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR_SYMBOL))
            {
                if(constDwtClose==widget->objectName())
                    use=m_titleBarButtonsCols[TITLEBAR_CLOSE];
                else if (constDwtFloat==widget->objectName()) {
                    use=m_titleBarButtonsCols[TITLEBAR_MAX];
                } else if (qtcCheckType<QDockWidget>(getParent<2>(widget)) &&
                           getParent(widget)
                           ->inherits("KoDockWidgetTitleBar")) {
                    QDockWidget *dw = (QDockWidget*)getParent<2>(widget);;
                    QWidget *koDw = widget->parentWidget();
                    int fw = (dw->isFloating() ?
                              pixelMetric(QStyle::PM_DockWidgetFrameWidth,
                                          0, dw) : 0);
                    QRect geom(widget->geometry());
                    QStyleOptionDockWidgetV2 dwOpt;
                    dwOpt.initFrom(dw);
                    dwOpt.rect = QRect(QPoint(fw, fw), QSize(koDw->geometry().width() - (fw * 2),
                                                             koDw->geometry().height() - (fw * 2)));
                    dwOpt.title = dw->windowTitle();
                    dwOpt.closable = (dw->features()&QDockWidget::DockWidgetClosable)==QDockWidget::DockWidgetClosable;
                    dwOpt.floatable = (dw->features()&QDockWidget::DockWidgetFloatable)==QDockWidget::DockWidgetFloatable;

                    if(dwOpt.closable &&
                       subElementRect(QStyle::SE_DockWidgetCloseButton, &dwOpt,
                                      getParent(widget)) == geom)
                        use=m_titleBarButtonsCols[TITLEBAR_CLOSE];
                    else if(dwOpt.floatable &&
                       subElementRect(QStyle::SE_DockWidgetFloatButton, &dwOpt,
                                      getParent(widget)) == geom)
                        use=m_titleBarButtonsCols[TITLEBAR_MAX];
                    else
                        use=m_titleBarButtonsCols[TITLEBAR_SHADE];
                }
            }

            if(isKWin)
                opt.state|=STATE_KWIN_BUTTON;

            bool coloredDef=isDefault && state&State_Enabled && IND_COLORED==opts.defBtnIndicator;

            if(widget && qobject_cast<const QAbstractButton*>(widget) && (static_cast<const QAbstractButton*>(widget))->isCheckable())
                opt.state|=STATE_TOGGLE_BUTTON;

            drawLightBevel(painter, r, &opt, widget, ROUNDED_ALL,
                           coloredDef ? m_defBtnCols[MO_DEF_BTN]
                                      : getFill(&opt, use, false,
                                                isDefault && state&State_Enabled && IND_DARKEN==opts.defBtnIndicator),
                           coloredDef ? m_defBtnCols : use,
                           true, isKWin || state&STATE_DWT_BUTTON
                                    ? WIDGET_MDI_WINDOW_BUTTON
                                    : isOnListView
                                        ? WIDGET_NO_ETCH_BTN
                                        : isDefault && state&State_Enabled
                                            ? WIDGET_DEF_BUTTON
                                            : state&STATE_TBAR_BUTTON
                                                ? WIDGET_TOOLBAR_BUTTON
                                                : WIDGET_STD_BUTTON);

            if (isDefault && state&State_Enabled)
                switch(opts.defBtnIndicator)
                {
                    case IND_CORNER:
                    {
                        QPainterPath path;
                        int          offset(isDown ? 5 : 4),
                                     etchOffset(doEtch ? 1 : 0);
                        double       xd(r.x()+0.5),
                                     yd(r.y()+0.5);
                        const QColor *cols = m_focusCols;
                        // Was:
                        // const QColor *cols = (m_focusCols ? m_focusCols :
                        //                       m_highlightCols);
                        // which is always m_focusCols

                        path.moveTo(xd+offset+etchOffset, yd+offset+etchOffset);
                        path.lineTo(xd+offset+6+etchOffset, yd+offset+etchOffset);
                        path.lineTo(xd+offset+etchOffset, yd+offset+6+etchOffset);
                        path.lineTo(xd+offset+etchOffset, yd+offset+etchOffset);
                        painter->setBrush(cols[isDown ? 0 : 4]);
                        painter->setPen(cols[isDown ? 0 : 4]);
                        painter->setRenderHint(QPainter::Antialiasing, true);
                        painter->drawPath(path);
                        painter->setRenderHint(QPainter::Antialiasing, false);
                        break;
                    }
                    case IND_COLORED:
                    {
                        int   offset=COLORED_BORDER_SIZE+(doEtch ? 1 : 0);
                        QRect r2(r.adjusted(offset, offset, -offset, -offset));

                        drawBevelGradient(getFill(&opt, use), painter, r2, true, state &(State_On | State_Sunken), opts.appearance, WIDGET_STD_BUTTON);
                    }
                    default:
                        break;
                }
            painter->restore();
            break;
        }
        case PE_FrameDefaultButton:
            break;
        case PE_FrameWindow:
        {
            bool         colTbarOnly=opts.windowBorder&WINDOW_BORDER_COLOR_TITLEBAR_ONLY,
                         fillBgnd=!(state&QtC_StateKWin) && !m_isPreview && !qtcIsFlatBgnd(opts.bgndAppearance);
            const QColor *bgndCols(colTbarOnly || fillBgnd ? backgroundColors(palette.color(QPalette::Active, QPalette::Window)) : 0L),
                         *borderCols(colTbarOnly
                                        ? bgndCols
                                        : theThemedApp==APP_KWIN
                                            ? buttonColors(option)
                                            : getMdiColors(option, state&State_Active));
            QColor       light(borderCols[0]),
                         dark(option->version==(TBAR_BORDER_VERSION_HACK+2)
                                ? palette.color(QPalette::Active, QPalette::Shadow)
                                : borderCols[option && option->version==TBAR_BORDER_VERSION_HACK ? 0 : QTC_STD_BORDER]);
            bool         isKWin=state&QtC_StateKWin,
                         addLight=opts.windowBorder&WINDOW_BORDER_ADD_LIGHT_BORDER && (!isKWin || qtcGetWindowBorderSize(false).sides>1);

            light.setAlphaF(1.0);
            dark.setAlphaF(1.0);

            painter->save();

            if(fillBgnd)
                painter->fillRect(r, bgndCols[ORIGINAL_SHADE]);
            if(opts.round<ROUND_SLIGHT || !isKWin || (state&QtC_StateKWinNotFull && state&QtC_StateKWin))
            {
                painter->setRenderHint(QPainter::Antialiasing, false);

                if(addLight)
                {
                    painter->setPen(light);
                    painter->drawLine(r.x()+1, r.y(), r.x()+1, r.y()+r.height()-1);
                }
                painter->setPen(dark);
                drawRect(painter, r);
            }
            else
            {
                if(addLight)
                {
                    painter->setRenderHint(QPainter::Antialiasing, false);
                    painter->setPen(light);
                    painter->drawLine(r.x()+1, r.y(), r.x()+1, r.y()+r.height()-(1+(opts.round>ROUND_SLIGHT && state&QtC_StateKWin ? 3 : 1)));
                }
                painter->setRenderHint(QPainter::Antialiasing, true);
                painter->setPen(dark);
                painter->drawPath(buildPath(r, WIDGET_OTHER, ROUNDED_ALL,
                                            opts.round>ROUND_SLIGHT && state&QtC_StateKWin
                                                ? 6.0
                                                : 2.0));

                if (opts.round >= ROUND_FULL &&
                    !(state & QtC_StateKWinCompositing)) {
                    QColor col(opts.windowBorder&WINDOW_BORDER_COLOR_TITLEBAR_ONLY
                                ? backgroundColors(option)[QTC_STD_BORDER]
                                : buttonColors(option)[QTC_STD_BORDER]);

                    painter->setRenderHint(QPainter::Antialiasing, false);
                    painter->setPen(col);
                    painter->drawPoint(r.x()+2, r.y()+r.height()-3);
                    painter->drawPoint(r.x()+r.width()-3, r.y()+r.height()-3);
                    painter->drawLine(r.x()+1, r.y()+r.height()-5, r.x()+1, r.y()+r.height()-4);
                    painter->drawLine(r.x()+3, r.y()+r.height()-2, r.x()+4, r.y()+r.height()-2);
                    painter->drawLine(r.x()+r.width()-2, r.y()+r.height()-5, r.x()+r.width()-2, r.y()+r.height()-4);
                    painter->drawLine(r.x()+r.width()-4, r.y()+r.height()-2, r.x()+r.width()-5, r.y()+r.height()-2);
                }
            }
            painter->restore();
            break;
        }
        case PE_FrameTabWidget:
        {
            int round(opts.square&SQUARE_TAB_FRAME ? ROUNDED_NONE : ROUNDED_ALL);

            painter->save();

            if (auto twf = styleOptCast<QStyleOptionTabWidgetFrame>(option))
                if((opts.round || (/*qtcIsCustomBgnd(opts) && */0==opts.tabBgnd)) &&
                    widget && qobject_cast<const QTabWidget*>(widget))
                {
                    struct QtcTabWidget : public QTabWidget
                    {
                        bool  tabsVisible()    const { return tabBar() && tabBar()->isVisible(); }
                        QRect currentTabRect() const { return tabBar()->tabRect(tabBar()->currentIndex()); }
                    };

                    const QTabWidget *tw((const QTabWidget*)widget);

                    if(tw->count()>0 && ((const QtcTabWidget*)widget)->tabsVisible())
                    {
                        if(!reverse && /*qtcIsCustomBgnd(opts) && */0==opts.tabBgnd) // Does not work for reverse :-(
                        {
                            QRect tabRect(((const QtcTabWidget*)widget)->currentTabRect());
                            int   adjust(TAB_MO_GLOW==opts.tabMouseOver && !(opts.thin&THIN_FRAMES) ? 2 : 1);

                            switch(tw->tabPosition())
                            {
                                case QTabWidget::South:
                                    tabRect=QRect(tabRect.x()+adjust, r.y()+r.height()-2, tabRect.width()-(2*adjust), 4);
                                    break;
                                case QTabWidget::North:
                                {
                                    int leftAdjust=twf->leftCornerWidgetSize.width()>0 ? twf->leftCornerWidgetSize.width() : 0;
                                    tabRect.adjust(leftAdjust+adjust, 0, leftAdjust-adjust, 2);
                                    break;
                                }
                                case QTabWidget::West:
                                    tabRect.adjust(0, adjust, 2, -adjust);
                                    break;
                                case QTabWidget::East:
                                    tabRect=QRect(r.x()+r.width()-2, tabRect.y()+adjust, 4, tabRect.height()-(2*adjust));
                                    break;
                            }

                            painter->setClipRegion(QRegion(r).subtract(tabRect), Qt::IntersectClip);
                        }

                        if(!(opts.square&SQUARE_TAB_FRAME) && 0==tw->currentIndex())
                        {
                            bool reverse(Qt::RightToLeft==twf->direction);

                            switch(tw->tabPosition())
                            {
                                case QTabWidget::North:
                                    if(reverse && twf->rightCornerWidgetSize.isEmpty())
                                        round-=CORNER_TR;
                                    else if(!reverse && twf->leftCornerWidgetSize.isEmpty())
                                        round-=CORNER_TL;
                                    break;
                                case QTabWidget::South:
                                    if(reverse && twf->rightCornerWidgetSize.isEmpty())
                                        round-=CORNER_BR;
                                    else if(!reverse && twf->leftCornerWidgetSize.isEmpty())
                                        round-=CORNER_BL;
                                    break;
                                case QTabWidget::West:
                                    round-=CORNER_TL;
                                    break;
                                case QTabWidget::East:
                                    round-=CORNER_TR;
                                    break;
                            }
                        }
                    }
                }

            QStyleOption opt(*option);
            const QColor *use=backgroundColors(option);

            opt.state|=State_Enabled;
            if(0!=opts.tabBgnd)
            {
                QColor bgnd(shade(use[ORIGINAL_SHADE], TO_FACTOR(opts.tabBgnd)));

                painter->fillRect(r.adjusted(0, 1, 0, -1), bgnd);
                painter->fillRect(r.adjusted(1, 0, -1, 0), bgnd);
            }
            drawBorder(painter, r, &opt, round, use, WIDGET_TAB_FRAME, opts.borderTab ? BORDER_LIGHT : BORDER_RAISED, false);
            painter->restore();
            break;
        }
        case PE_PanelItemViewItem: {
            auto v4Opt = styleOptCast<QStyleOptionViewItemV4>(option);
            const QAbstractItemView *view = qobject_cast<const QAbstractItemView*>(widget);
            bool                         hover = state&State_MouseOver && state&State_Enabled && (!view ||
                                                 QAbstractItemView::NoSelection!=view->selectionMode()),
                                         hasCustomBackground = v4Opt->backgroundBrush.style() != Qt::NoBrush &&
                                                               !(option->state & State_Selected),
                                         hasSolidBackground = !hasCustomBackground || Qt::SolidPattern==v4Opt->backgroundBrush.style();

            if (!hover && !(state & State_Selected) && !hasCustomBackground &&
                !(v4Opt->features & QStyleOptionViewItemV2::Alternate))
                break;

            QPalette::ColorGroup cg(state&State_Enabled
                                        ? state&State_Active
                                            ? QPalette::Normal
                                            : QPalette::Inactive
                                        : QPalette::Disabled);

            if (v4Opt && (v4Opt->features & QStyleOptionViewItemV2::Alternate))
                painter->fillRect(r, option->palette.brush(cg, QPalette::AlternateBase));

            if (!hover && !(state&State_Selected) && !hasCustomBackground)
                break;

            if (hasCustomBackground) {
                painter->save();
                painter->setBrushOrigin(r.topLeft());
                painter->fillRect(r, v4Opt->backgroundBrush);
                painter->restore();
            }

            if(state&State_Selected || hover)
            {
                if(!widget)
                {
                    widget=getWidget(painter);
                    if(widget)
                        widget=widget->parentWidget();
                }

                QColor color(hasCustomBackground && hasSolidBackground
                                ? v4Opt->backgroundBrush.color()
                                : palette.color(cg, QPalette::Highlight));
                bool   square((opts.square&SQUARE_LISTVIEW_SELECTION) &&
                              (/*(!widget && r.height()<=40 && r.width()>=48) || */
                               (widget && !widget->inherits("KFilePlacesView") &&
                                (qobject_cast<const QTreeView*>(widget) ||
                                  (qobject_cast<const QListView*>(widget) &&
                                  QListView::IconMode!=((const QListView*)widget)->viewMode()))))),
                       modAlpha(!(state&State_Active) && m_inactiveChangeSelectionColor);

                if (hover && !hasCustomBackground) {
                    if (!(state & State_Selected)) {
                        color.setAlphaF(theThemedApp == APP_PLASMA && !widget ?
                                        0.5 * (modAlpha ? 0.75 : 1.0) : 0.20);
                    } else {
                        color = color.lighter(110);
                        if (modAlpha) {
                            color.setAlphaF(INACTIVE_SEL_ALPHA);
                        }
                    }
                } else if (modAlpha) {
                    color.setAlphaF(color.alphaF() * INACTIVE_SEL_ALPHA);
                }

                if(square)
                    drawBevelGradient(color, painter, r, true, false, opts.selectionAppearance, WIDGET_SELECTION);
                else
                {
                    QPixmap pix;
                    QString key;

                    key.sprintf("qtc-sel-%x-%x", r.height(), color.rgba());
                    if(!m_usePixmapCache || !QPixmapCache::find(key, pix))
                    {
                        pix=QPixmap(QSize(24, r.height()));
                        pix.fill(Qt::transparent);

                        QPainter pixPainter(&pix);
                        QRect    border(0, 0, pix.width(), pix.height());
                        double   radius(qtcGetRadius(&opts, r.width(), r.height(), WIDGET_OTHER, RADIUS_SELECTION));

                        pixPainter.setRenderHint(QPainter::Antialiasing, true);
                        drawBevelGradient(color, &pixPainter, border, buildPath(QRectF(border), WIDGET_OTHER, ROUNDED_ALL, radius), true,
                                          false, opts.selectionAppearance, WIDGET_SELECTION, false);
                        if(opts.borderSelection)
                        {
                            pixPainter.setBrush(Qt::NoBrush);
                            pixPainter.setPen(color);
                            pixPainter.drawPath(buildPath(border, WIDGET_SELECTION, ROUNDED_ALL, radius));
                        }
                        pixPainter.end();
                        if(m_usePixmapCache)
                            QPixmapCache::insert(key, pix);
                    }

                    bool roundedLeft  = false,
                         roundedRight = false;

                    if (v4Opt)
                    {
                        roundedLeft  = (QStyleOptionViewItemV4::Beginning==v4Opt->viewItemPosition);
                        roundedRight = (QStyleOptionViewItemV4::End==v4Opt->viewItemPosition);
                        if (QStyleOptionViewItemV4::OnlyOne==v4Opt->viewItemPosition ||
                            QStyleOptionViewItemV4::Invalid==v4Opt->viewItemPosition ||
                            (view && view->selectionBehavior() != QAbstractItemView::SelectRows))
                        {
                            roundedLeft=roundedRight=true;
                        }
                    }

                    int size(roundedLeft && roundedRight ? qMin(8, r.width()/2) : 8);

                    if (!reverse ? roundedLeft : roundedRight)
                    {
                        painter->drawPixmap(r.topLeft(), pix.copy(0, 0, size, r.height()));
                        r.adjust(size, 0, 0, 0);
                    }
                    if (!reverse ? roundedRight : roundedLeft)
                    {
                        painter->drawPixmap(r.right() - size + 1, r.top(), pix.copy(24-size, 0, size, r.height()));
                        r.adjust(0, 0, -size, 0);
                    }
                    if (r.isValid())
                        painter->drawTiledPixmap(r, pix.copy(7, 0, 8, r.height()));
                }
            }
            break;
        }
        case QtC_PE_DrawBackground:
            if (auto bgnd = styleOptCast<BgndOption>(option))
                if(state&QtC_StateKWin)
                {
                    QColor col(palette.brush(QPalette::Window).color());
                    int    opacity(col.alphaF()*100);

                    col.setAlphaF(1.0);
                    drawBackground(painter, col, r, opacity, BGND_WINDOW, bgnd->app, bgnd->path);
                     // APPEARANCE_RAISED is used to signal flat background, but have background image!
                    if(APPEARANCE_FLAT!=bgnd->app)
                    {
                        painter->save();
                        painter->setClipRect(bgnd->rect, Qt::IntersectClip);
                        drawBackgroundImage(painter, true,
                                            (opts.bgndImage.type == IMG_FILE &&
                                             opts.bgndImage.onBorder) ?
                                            bgnd->rect : bgnd->widgetRect);
                        painter->restore();
                    }
                }
            break;
        // TODO: This is the only part left from QWindowsStyle - but I dont think its actually used!
        // case PE_IndicatorProgressChunk:
        case PE_PanelTipLabel: {
            bool haveAlpha = Utils::hasAlphaChannel(widget);
            bool rounded = !(opts.square & SQUARE_TOOLTIPS);
            QPainterPath path = rounded ? buildPath(QRectF(r), WIDGET_OTHER, ROUNDED_ALL, opts.round >= ROUND_FULL ? 5.0 : 2.5) : QPainterPath();
            QColor       col=palette.toolTipBase().color();

            if(widget && widget->window())
                m_shadowHelper->registerWidget(widget->window());
            painter->save();
            if(rounded)
                painter->setRenderHint(QPainter::Antialiasing, true);
            if(haveAlpha)
                col.setAlphaF(0.875);
            drawBevelGradient(col, painter, r, path, true, false, opts.tooltipAppearance, WIDGET_TOOLTIP, !haveAlpha);
            if(qtcIsFlat(opts.tooltipAppearance))
            {
                painter->setPen(QPen(palette.toolTipText(), 0));
                drawRect(painter, r);
            }
            painter->restore();
            break;
        }
            // Fall through!
        default:
            BaseStyle::drawPrimitive(element, option, painter, widget);
            break;
    }
}

void Style::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    prePolish(widget);
    QRect               r(option->rect);
    const QFlags<State> &state(option->state);
    const QPalette      &palette(option->palette);
    bool                reverse(Qt::RightToLeft==option->direction);

    switch((unsigned)element)
    {
        case CE_QtC_SetOptions:
            if (auto preview = styleOptCast<PreviewOption>(option)) {
                if(!painter && widget && QLatin1String("QtCurveConfigDialog")==widget->objectName())
                {
                    Style *that=(Style*)this;
                    opts=preview->opts;
                    qtcCheckConfig(&opts);
                    that->init(true);
                }
            }
            break;
        case CE_QtC_Preview:
            if (auto preview = styleOptCast<PreviewOption>(option)) {
                if(widget && widget && QLatin1String("QtCurveConfigDialog-GradientPreview")==widget->objectName())
                {
                    Options      old=opts;
                    const QColor *use(buttonColors(option));
                    opts=preview->opts;

                    drawLightBevelReal(painter, r, option, widget, ROUNDED_ALL, getFill(option, use, false, false), use,
                                       true, WIDGET_STD_BUTTON, false, opts.round, false);
                    opts=old;
                }
            }
            break;
        case CE_QtC_KCapacityBar:
            if (auto bar = styleOptCast<QStyleOptionProgressBar>(option)) {
                QStyleOptionProgressBar mod(*bar);

                if(mod.rect.height()>16 && widget->parentWidget() &&
                   (qobject_cast<const QStatusBar*>(widget->parentWidget()) ||
                    widget->parentWidget()->inherits("DolphinStatusBar")))
                {
                    int m=(mod.rect.height()-16)/2;
                    mod.rect.adjust(0, m, 0, -m);
                }
                drawControl(CE_ProgressBarGroove, &mod, painter, widget);
                if (opts.buttonEffect != EFFECT_NONE && opts.borderProgress)
                    mod.rect.adjust(1, 1, -1, -1);
                drawControl(CE_ProgressBarContents, &mod, painter, widget);
                drawControl(CE_ProgressBarLabel, &mod, painter, widget);
            }
            break;
        case CE_ToolBoxTabShape:
        {
            auto tb = styleOptCast<QStyleOptionToolBox>(option);
            if(!(tb && widget))
                break;

            const QColor *use = backgroundColors(widget->palette().color(QPalette::Window));
            QPainterPath path;
            int          y = r.height()*15/100;

            painter->save();
            if (reverse)
            {
                path.moveTo(r.left()+52, r.top());
                path.cubicTo(QPointF(r.left()+50-8, r.top()), QPointF(r.left()+50-10, r.top()+y), QPointF(r.left()+50-10, r.top()+y));
                path.lineTo(r.left()+18+9, r.bottom()-y);
                path.cubicTo(QPointF(r.left()+18+9, r.bottom()-y), QPointF(r.left()+19+6, r.bottom()-1-0.3), QPointF(r.left()+19, r.bottom()-1-0.3));
            }
            else
            {
                path.moveTo(r.right()-52, r.top());
                path.cubicTo(QPointF(r.right()-50+8, r.top()), QPointF(r.right()-50+10, r.top()+y), QPointF(r.right()-50+10, r.top()+y));
                path.lineTo(r.right()-18-9, r.bottom()-y);
                path.cubicTo(QPointF(r.right()-18-9, r.bottom()-y), QPointF(r.right()-19-6, r.bottom()-1-0.3), QPointF(r.right()-19, r.bottom()-1-0.3));
            }

            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->translate(0, 1);
            painter->setPen(use[0]);
            painter->drawPath(path);
            painter->translate(0, -1);
            painter->setPen(use[4]);
            painter->drawPath(path);
            painter->setRenderHint(QPainter::Antialiasing, false);
            if (reverse)
            {
                painter->drawLine(r.left()+50-1, r.top(), r.right(), r.top());
                painter->drawLine(r.left()+20, r.bottom()-2, r.left(), r.bottom()-2);
                painter->setPen(use[0]);
                painter->drawLine(r.left()+50, r.top()+1, r.right(), r.top()+1);
                painter->drawLine(r.left()+20, r.bottom()-1, r.left(), r.bottom()-1);
            }
            else
            {
                painter->drawLine(r.left(), r.top(), r.right()-50+1, r.top());
                painter->drawLine(r.right()-20, r.bottom()-2, r.right(), r.bottom()-2);
                painter->setPen(use[0]);
                painter->drawLine(r.left(), r.top()+1, r.right()-50, r.top()+1);
                painter->drawLine(r.right()-20, r.bottom()-1, r.right(), r.bottom()-1);
            }
            painter->restore();
            break;
        }
        case CE_MenuScroller:
        {
            const QColor *use(popupMenuCols());
            painter->fillRect(r, use[ORIGINAL_SHADE]);
            painter->setPen(use[QTC_STD_BORDER]);
            drawRect(painter, r);
            drawPrimitive(((state&State_DownArrow) ? PE_IndicatorArrowDown : PE_IndicatorArrowUp), option, painter, widget);
            break;
        }
        case CE_RubberBand: // Rubber band used in such things as iconview.
        {
            if(r.width()>0 && r.height()>0)
            {
                painter->save();
                QColor c(m_highlightCols[ORIGINAL_SHADE]);

                painter->setClipRegion(r);
                painter->setPen(c);
                c.setAlpha(50);
                painter->setBrush(c);
                drawRect(painter, r);
                painter->restore();
            }
            break;
        }
        case CE_Splitter:
        {
            const QColor *use(buttonColors(option));
            const QColor *border(borderColors(option, use));
            // In Amarok nightly (2.2) State_Horizontal doesn't seem to
            // always be set...
            bool horiz(state & State_Horizontal ||
                       (r.height() > 6 && r.height() > r.width()));

            painter->save();
            if(/*qtcIsFlatBgnd(opts.bgndAppearance) || */state&State_MouseOver && state&State_Enabled)
            {
                QColor color(palette.color(QPalette::Active, QPalette::Window));

                if(state&State_MouseOver && state&State_Enabled && opts.splitterHighlight)
                    if(ROUND_NONE!=opts.round)
                    {
                        painter->save();
                        painter->setRenderHint(QPainter::Antialiasing, true);
                        double   radius(qtcGetRadius(&opts, r.width(), r.height(), WIDGET_OTHER, RADIUS_SELECTION));

                        drawBevelGradient(shade(palette.background().color(), TO_FACTOR(opts.splitterHighlight)),
                                          painter, r, buildPath(QRectF(r), WIDGET_OTHER, ROUNDED_ALL, radius),
                                          !(state&State_Horizontal), false, opts.selectionAppearance, WIDGET_SELECTION, false);
                        painter->restore();
                    }
                    else
                        drawBevelGradient(shade(palette.background().color(), TO_FACTOR(opts.splitterHighlight)), painter,
                                          r, !(state&State_Horizontal), false, opts.selectionAppearance, WIDGET_SELECTION);
                else
                    painter->fillRect(r, color);
            }

            switch(opts.splitters)
            {
                case LINE_NONE:
                    break;
                case LINE_1DOT:
                    painter->drawPixmap(r.x()+((r.width()-5)/2), r.y()+((r.height()-5)/2), *getPixmap(border[QTC_STD_BORDER], PIX_DOT, 1.0));
                    break;
                default:
                case LINE_DOTS:
                    drawDots(painter, r, horiz, NUM_SPLITTER_DASHES, 1, border, 0, 5);
                    break;
                case LINE_FLAT:
                case LINE_SUNKEN:
                case LINE_DASHES:
                    drawLines(painter, r, horiz, NUM_SPLITTER_DASHES, 3, border, 0, 3, opts.splitters);
            }
            painter->restore();
            break;
        }
        case CE_SizeGrip:
        {
            QPolygon   triangle(3);
            Qt::Corner corner;
            int        size=SIZE_GRIP_SIZE-2;

            if (auto sgrp = styleOptCast<QStyleOptionSizeGrip>(option))
                corner = sgrp->corner;
            else if (Qt::RightToLeft==option->direction)
                corner = Qt::BottomLeftCorner;
            else
                corner = Qt::BottomRightCorner;

            switch(corner)
            {
                case Qt::BottomLeftCorner:
                    triangle.putPoints(0, 3, 0,0, size,size, 0,size);
                    triangle.translate(r.x(), r.y()+(r.height()-(SIZE_GRIP_SIZE-1)));
                    break;
                case Qt::BottomRightCorner:
                    triangle.putPoints(0, 3, size,0, size,size, 0,size);
                    triangle.translate(r.x()+(r.width()-(SIZE_GRIP_SIZE-1)), r.y()+(r.height()-(SIZE_GRIP_SIZE-1)));
                    break;
                case Qt::TopRightCorner:
                    triangle.putPoints(0, 3, 0,0, size,0, size,size);
                    triangle.translate(r.x()+(r.width()-(SIZE_GRIP_SIZE-1)), r.y());
                    break;
                case Qt::TopLeftCorner:
                    triangle.putPoints(0, 3, 0,0, size,0, 0,size);
                    triangle.translate(r.x(), r.y());
            }
            painter->save();
            painter->setPen(m_backgroundCols[2]);
            painter->setBrush(m_backgroundCols[2]);
            painter->drawPolygon(triangle);
            painter->restore();
            break;
        }
        case CE_ToolBar:
            if (auto toolbar = styleOptCast<QStyleOptionToolBar>(option)) {
                if(!widget || !widget->parent() || qobject_cast<QMainWindow*>(widget->parent()))
                {
                    painter->save();
                    drawMenuOrToolBarBackground(widget, painter, r, option, false, Qt::NoToolBarArea==toolbar->toolBarArea ||
                                                                                   Qt::BottomToolBarArea==toolbar->toolBarArea ||
                                                                                   Qt::TopToolBarArea==toolbar->toolBarArea);
                    if(TB_NONE!=opts.toolbarBorders)
                    {
                        const QColor *use=/*PE_PanelMenuBar==pe && m_active
                                            ? m_menubarCols
                                            : */ backgroundColors(option);
                        bool         dark(TB_DARK==opts.toolbarBorders || TB_DARK_ALL==opts.toolbarBorders);

                        if(TB_DARK_ALL==opts.toolbarBorders || TB_LIGHT_ALL==opts.toolbarBorders)
                        {
                            painter->setPen(use[0]);
                            painter->drawLine(r.x(), r.y(), r.x()+r.width()-1, r.y());
                            painter->drawLine(r.x(), r.y(), r.x(), r.y()+r.height()-1);
                            painter->setPen(use[dark ? 3 : 4]);
                            painter->drawLine(r.x(), r.y()+r.height()-1, r.x()+r.width()-1, r.y()+r.height()-1);
                            painter->drawLine(r.x()+r.width()-1, r.y(), r.x()+r.width()-1, r.y()+r.height()-1);
                        }
                        else
                        {
                            bool paintH(true),
                                 paintV(true);

                            switch (toolbar->toolBarArea)
                            {
                                case Qt::BottomToolBarArea:
                                case Qt::TopToolBarArea:
                                    paintV=false;
                                    break;
                                case Qt::RightToolBarArea:
                                case Qt::LeftToolBarArea:
                                    paintH=false;
                                default:
                                    break;
                            }

                            painter->setPen(use[0]);
                            if(paintH)
                                painter->drawLine(r.x(), r.y(), r.x()+r.width()-1, r.y());
                            if(paintV)
                                painter->drawLine(r.x(), r.y(), r.x(), r.y()+r.height()-1);
                            painter->setPen(use[dark ? 3 : 4]);
                            if(paintH)
                                painter->drawLine(r.x(), r.y()+r.height()-1, r.x()+r.width()-1, r.y()+r.height()-1);
                            if(paintV)
                                painter->drawLine(r.x()+r.width()-1, r.y(), r.x()+r.width()-1, r.y()+r.height()-1);
                        }
                    }
                    painter->restore();
                }
            }
            break;
        case CE_DockWidgetTitle:
            if (auto dwOpt = styleOptCast<QStyleOptionDockWidget>(option)) {
                auto v2 = styleOptCast<QStyleOptionDockWidgetV2>(dwOpt);
                bool verticalTitleBar(v2 == 0 ? false : v2->verticalTitleBar);
                bool isKOffice(widget && widget->inherits("KoDockWidgetTitleBar"));
                QRect fillRect(r);

                // This fixes the look of KOffice's dock widget titlebars...
                if(isKOffice)
                    fillRect.adjust(-r.x(), -r.y(), 0, 0);

                if(!qtcIsFlat(opts.dwtAppearance))
                {
                    painter->save();

                    QColor col((opts.dwtSettings&DWT_COLOR_AS_PER_TITLEBAR)
                                ? getMdiColors(option, state&State_Active)[ORIGINAL_SHADE]
                                : palette.background().color());
                    if(opts.round<ROUND_FULL)
                        drawBevelGradient(col, painter, fillRect, !verticalTitleBar,
                                          false, opts.dwtAppearance, WIDGET_DOCK_WIDGET_TITLE);
                    else
                    {
                        double radius(qtcGetRadius(&opts, fillRect.width(), fillRect.height(), WIDGET_OTHER, RADIUS_EXTERNAL));
                        int    round=ROUNDED_ALL;

                        if(opts.dwtSettings&DWT_ROUND_TOP_ONLY)
                            round=verticalTitleBar ? ROUNDED_LEFT : ROUNDED_TOP;
                        painter->setRenderHint(QPainter::Antialiasing, true);
                        drawBevelGradient(col, painter, fillRect, buildPath(QRectF(fillRect), WIDGET_OTHER, round, radius), !verticalTitleBar,
                                          false, opts.dwtAppearance, WIDGET_DOCK_WIDGET_TITLE, false);
                    }

                    painter->restore();
                }

                if (!dwOpt->title.isEmpty())
                {
                    QRect titleRect(subElementRect(SE_DockWidgetTitleBarText, option, widget));

                    if (verticalTitleBar)
                    {
                        QRect rVert(r);
                        QSize s(rVert.size());

                        s.transpose();
                        rVert.setSize(s);

                        titleRect = QRect(rVert.left() + r.bottom() - titleRect.bottom(),
                                        rVert.top() + titleRect.left() - r.left(),
                                        titleRect.height(), titleRect.width());

                        painter->translate(rVert.left(), rVert.top() + rVert.width());
                        painter->rotate(-90);
                        painter->translate(-rVert.left(), -rVert.top());
                    }
#ifdef QTC_QT4_ENABLE_KDE
                    if(opts.dwtSettings&DWT_FONT_AS_PER_TITLEBAR)
                        painter->setFont(KGlobalSettings::windowTitleFont());
#endif
                    QFontMetrics fm(painter->fontMetrics());
                    QString      title(fm.elidedText(dwOpt->title, Qt::ElideRight, titleRect.width(), QPalette::WindowText));
                    painter->save();
                    getMdiColors(option, state&State_Active);

                    QColor textColor((opts.dwtSettings&DWT_COLOR_AS_PER_TITLEBAR)
                                        ? state&State_Active
                                            ? m_activeMdiTextColor
                                            : m_mdiTextColor
                                        : palette.color(QPalette::WindowText)),
                           shadow(WINDOW_SHADOW_COLOR(opts.titlebarEffect));
                    int textOpt(Qt::AlignVCenter); // TODO: dwtPosAsPerTitleBar ?

                    if(opts.dwtSettings&DWT_TEXT_ALIGN_AS_PER_TITLEBAR)
                        switch(opts.titlebarAlignment)
                        {
                            case ALIGN_FULL_CENTER:
                                if (!verticalTitleBar && !reverse) {
                                    QFontMetrics fm(painter->fontMetrics());
                                    int          width=fm.boundingRect(title).width();

                                    if(((fillRect.width()+width)/2)<=titleRect.width()+(isKOffice ? r.x() : 0))
                                    {
                                        titleRect=fillRect;
                                        textOpt|=Qt::AlignHCenter;
                                    }
                                    else
                                        textOpt|=Qt::AlignRight;
                                    break;
                                }
                                QTC_FALLTHROUGH();
                            case ALIGN_CENTER:
                                textOpt|=Qt::AlignHCenter;
                                break;
                            case ALIGN_RIGHT:
                                textOpt|=Qt::AlignRight;
                                break;
                            default:
                            case ALIGN_LEFT:
                                textOpt|=Qt::AlignLeft;
                        }
                    else
                        textOpt|=Qt::AlignLeft;

                    if (!styleHint(SH_UnderlineShortcut, dwOpt, widget))
                        textOpt|=Qt::TextHideMnemonic;
                    else
                        textOpt|=Qt::TextShowMnemonic;

                    if((opts.dwtSettings&DWT_EFFECT_AS_PER_TITLEBAR) &&
                       EFFECT_NONE!=opts.titlebarEffect)
                    {
                        shadow.setAlphaF(WINDOW_TEXT_SHADOW_ALPHA(opts.titlebarEffect));
                        painter->setPen(shadow);
                        painter->drawText(titleRect.adjusted(1, 1, 1, 1), textOpt, title);

                        if (!(state&State_Active) && DARK_WINDOW_TEXT(textColor))
                            textColor.setAlpha((textColor.alpha() * 180) >> 8);
                    }
                    painter->setPen(textColor);
                    painter->drawText(titleRect, textOpt, title);
                    painter->restore();
                }
            }
            break;
        case CE_HeaderEmptyArea:
        {
            auto ho = styleOptCast<QStyleOptionHeader>(option);
            bool horiz(ho ? Qt::Horizontal==ho->orientation : state&State_Horizontal);
            QStyleOption opt(*option);
            const QColor             *use(opts.lvButton ? buttonColors(option) : backgroundColors(option));

            opt.state&=~State_MouseOver;
            painter->save();

            drawBevelGradient(getFill(&opt, use), painter, r, horiz, false, opts.lvAppearance, WIDGET_LISTVIEW_HEADER);

            painter->setRenderHint(QPainter::Antialiasing, true);
            if(APPEARANCE_RAISED==opts.lvAppearance)
            {
                painter->setPen(use[4]);
                if(horiz)
                    drawAaLine(painter, r.x(), r.y()+r.height()-2, r.x()+r.width()-1, r.y()+r.height()-2);
                else
                    drawAaLine(painter, r.x()+r.width()-2, r.y(), r.x()+r.width()-2, r.y()+r.height()-1);
            }

            painter->setPen(use[QTC_STD_BORDER]);
            if(horiz)
                drawAaLine(painter, r.x(), r.y()+r.height()-1, r.x()+r.width()-1, r.y()+r.height()-1);
            else if(reverse)
                drawAaLine(painter, r.x(), r.y(), r.x(), r.y()+r.height()-1);
            else
                drawAaLine(painter, r.x()+r.width()-1, r.y(), r.x()+r.width()-1, r.y()+r.height()-1);
            painter->setRenderHint(QPainter::Antialiasing, false);
            painter->restore();
            break;
        }
        case CE_HeaderSection:
            if (auto ho = styleOptCast<QStyleOptionHeader>(option)) {
                const QColor *use(state&State_Enabled && m_sortedLvColors &&
                                  ho->sortIndicator != QStyleOptionHeader::None ?
                                  m_sortedLvColors : opts.lvButton ?
                                  buttonColors(option) :
                                  backgroundColors(option));

                painter->save();

                if (state & (State_Raised | State_Sunken)) {
                    /* State_Down | State_On */
                    bool sunken = (state & State_Sunken);
                    bool q3Header = widget && widget->inherits("Q3Header");
                    QStyleOption opt(*option);

                    opt.state &= ~State_On;
                    if (q3Header && widget && widget->underMouse() &&
                        m_hoverWidget && r.contains(m_pos)) {
                        opt.state |= State_MouseOver;
                    }

                    if (ho->section == -1 && !(state & State_Enabled) &&
                        widget && widget->isEnabled()) {
                        opt.state |= State_Enabled;
                    }

                    drawBevelGradient(getFill(&opt, use), painter, r,
                                      ho->orientation == Qt::Horizontal,
                                      sunken, opts.lvAppearance,
                                      WIDGET_LISTVIEW_HEADER);

                    painter->setRenderHint(QPainter::Antialiasing, true);
                    if (opts.lvAppearance == APPEARANCE_RAISED) {
                        painter->setPen(use[4]);
                        if (ho->orientation == Qt::Horizontal) {
                            drawAaLine(painter, r.x(), r.y() + r.height() - 2,
                                       r.x() + r.width() - 1,
                                       r.y() + r.height() - 2);
                        } else {
                            drawAaLine(painter, r.x() + r.width() - 2, r.y(),
                                       r.x() + r.width() - 2,
                                       r.y() + r.height() - 1);
                        }
                    }

                    if (ho->orientation == Qt::Horizontal) {
                        painter->setPen(use[QTC_STD_BORDER]);
                        drawAaLine(painter, r.x(), r.y() + r.height() - 1,
                                   r.x() + r.width() - 1,
                                   r.y() + r.height() - 1);
                        if (opts.coloredMouseOver && state & State_MouseOver &&
                            state & State_Enabled)
                            drawHighlight(painter,
                                          QRect(r.x(), r.y() + r.height() - 2,
                                                r.width(), 2), true, true);

                        if (q3Header ||
                            noneOf(ho->position, QStyleOptionHeader::End,
                                   QStyleOptionHeader::OnlyOneSection)) {
                            // Draw header separator
                            int sep_x = reverse ? r.x() : r.x() + r.width() - 2;
                            drawFadedLine(painter,
                                          QRect(sep_x, r.y() + 5, 1,
                                                r.height() - 10),
                                          use[QTC_STD_BORDER],
                                          true, true, false);
                            drawFadedLine(painter,
                                          QRect(sep_x + 1, r.y() + 5, 1,
                                                r.height() - 10),
                                          use[0], true, true, false);
                        }
                    } else {
                        painter->setPen(use[QTC_STD_BORDER]);
                        if (reverse) {
                            drawAaLine(painter, r.x(), r.y(), r.x(),
                                       r.y()+r.height()-1);
                        } else {
                            drawAaLine(painter, r.x() + r.width() - 1, r.y(),
                                       r.x() + r.width() - 1,
                                       r.y() + r.height() - 1);
                        }

                        if (q3Header ||
                            noneOf(ho->position, QStyleOptionHeader::End,
                                   QStyleOptionHeader::OnlyOneSection)) {
                            // Draw header separator
                            drawFadedLine(painter,
                                          QRect(r.x() + 5,
                                                r.y() + r.height() - 2,
                                                r.width() - 10, 1),
                                          use[QTC_STD_BORDER], true, true, true);
                            drawFadedLine(painter,
                                          QRect(r.x() + 5,
                                                r.y() + r.height() - 1,
                                                r.width() - 10, 1),
                                          use[0], true, true, true);
                        }
                        if (opts.coloredMouseOver && state & State_MouseOver &&
                            state & State_Enabled) {
                            drawHighlight(painter,
                                          QRect(r.x(), r.y() + r.height() - 3,
                                                r.width(), 2), true, true);
                        }
                    }
                    painter->setRenderHint(QPainter::Antialiasing, false);
                } else if (!qtcIsFlat(opts.lvAppearance) && !reverse &&
                           (state == (State_Enabled | State_Active) ||
                            state == State_Enabled)) {
                    QPolygon top;
                    const QColor &col(getFill(option, use));

                    top.setPoints(3, r.x(), r.y(), r.x() + r.width(), r.y(),
                                  r.x() + r.width(), r.y() + r.height());
                    painter->setClipRegion(QRegion(top));
                    drawBevelGradient(col, painter, r, true, false,
                                      opts.lvAppearance, WIDGET_LISTVIEW_HEADER);
                    painter->setClipRegion(QRegion(r).eor(QRegion(top)));
                    drawBevelGradient(col, painter, r, false, false,
                                      opts.lvAppearance, WIDGET_LISTVIEW_HEADER);
                } else {
                    painter->fillRect(r, getFill(option, use));
                }
                painter->restore();
            }
            break;
        case CE_HeaderLabel:
            if (auto header = styleOptCast<QStyleOptionHeader>(option)) {
                if (!header->icon.isNull())
                {
                    QPixmap pixmap(getIconPixmap(header->icon, pixelMetric(PM_SmallIconSize), header->state));
                    int     pixw(pixmap.width());
                    QRect   aligned(alignedRect(header->direction, QFlag(header->iconAlignment), pixmap.size(), r)),
                            inter(aligned.intersected(r));

                    painter->drawPixmap(inter.x(), inter.y(), pixmap, inter.x() - aligned.x(), inter.y() - aligned.y(), inter.width(), inter.height());

                    if (header->direction == Qt::LeftToRight)
                        r.setLeft(r.left() + pixw + 2);
                    else
                        r.setRight(r.right() - pixw - 2);
                }
                drawItemTextWithRole(painter, r, header->textAlignment, palette,
                                     state&State_Enabled, header->text,
                                     QPalette::ButtonText);
            }
            break;
        case CE_ProgressBarGroove: {
            bool doEtch = (opts.buttonEffect != EFFECT_NONE &&
                           opts.borderProgress);
            bool horiz = true;
            QColor col;

            if (auto bar = styleOptCast<QStyleOptionProgressBarV2>(option)) {
                horiz = bar->orientation == Qt::Horizontal;
            }
            painter->save();

            if(doEtch)
                r.adjust(1, 1, -1, -1);

            switch(opts.progressGrooveColor)
            {
                default:
                case ECOLOR_BASE:
                    col=palette.base().color();
                    break;
                case ECOLOR_BACKGROUND:
                    col=palette.background().color();
                    break;
                case ECOLOR_DARK:
                    col=m_backgroundCols[2];
            }

            drawBevelGradient(col, painter, r, opts.borderProgress
                                ? buildPath(r, WIDGET_PBAR_TROUGH, ROUNDED_ALL,
                                            qtcGetRadius(&opts, r.width(), r.height(), WIDGET_PBAR_TROUGH, RADIUS_EXTERNAL))
                                : QPainterPath(),
                              horiz, false, opts.progressGrooveAppearance, WIDGET_PBAR_TROUGH);

            if(doEtch)
                drawEtch(painter, r.adjusted(-1, -1, 1, 1), widget, WIDGET_PBAR_TROUGH);
            else if(!opts.borderProgress)
            {
                painter->setPen(m_backgroundCols[QTC_STD_BORDER]);
                if(horiz)
                {
                    painter->drawLine(r.topLeft(), r.topRight());
                    painter->drawLine(r.bottomLeft(), r.bottomRight());
                }
                else
                {
                    painter->drawLine(r.topLeft(), r.bottomLeft());
                    painter->drawLine(r.topRight(), r.bottomRight());
                }
            }

            if(opts.borderProgress)
                drawBorder(painter, r, option, ROUNDED_ALL, backgroundColors(option), WIDGET_PBAR_TROUGH,
                           qtcIsFlat(opts.progressGrooveAppearance) && ECOLOR_DARK!=opts.progressGrooveColor ? BORDER_SUNKEN : BORDER_FLAT);
            painter->restore();
            break;
        }
        case CE_ProgressBarContents:
            if (auto bar = styleOptCast<QStyleOptionProgressBar>(option)) {
                bool vertical(false),
                     inverted(false),
                     indeterminate(0==bar->minimum && 0==bar->maximum);

                // Get extra style options if version 2
                if (auto bar2 =
                    styleOptCast<QStyleOptionProgressBarV2>(option)) {
                    vertical = Qt::Vertical==bar2->orientation;
                    inverted = bar2->invertedAppearance;
                }

                if (!indeterminate && -1==bar->progress)
                    break;

                bool reverse = (!vertical && (bar->direction == Qt::RightToLeft)) || vertical;

                if (inverted)
                    reverse = !reverse;

                painter->save();

                if (!m_progressBarAnimateTimer && (opts.animatedProgress || indeterminate)) {
                    if (m_timer.isNull()) {
                        m_timer.start();
                    }
                    // now we'll need a timer, start it at the regular frequency
                    m_progressBarAnimateFps = constProgressBarFps;
                    m_progressBarAnimateTimer = const_cast<Style*>(this)->startTimer(1000/m_progressBarAnimateFps);
                }

                if(indeterminate) //Busy indicator
                {
                    int chunkSize(PROGRESS_CHUNK_WIDTH*3.4),
                        measure(vertical ? r.height() : r.width());

                    if(chunkSize>(measure/2))
                        chunkSize=measure/2;

                    int          step(m_animateStep % ((measure-chunkSize) * 2));
                    QStyleOption opt(*option);

                    if (step > (measure-chunkSize))
                        step = 2 * (measure-chunkSize) - step;

                    opt.state|=State_Raised|State_Horizontal;
                    drawProgress(painter, vertical ? QRect(r.x(), r.y()+step, r.width(), chunkSize) : QRect(r.x()+step, r.y(), chunkSize, r.height()),
                                 option, vertical);
                }
                else if(r.isValid() && bar->progress>0)
                {
                    qint64 progress = qMax<qint64>(bar->progress, bar->minimum); // workaround for bug in QProgressBar
                    double pg = ((progress - qint64(bar->minimum)) /
                                qMax(double(1.0), double(qint64(bar->maximum) - qint64(bar->minimum))));

                    if(vertical)
                    {
                        int height(qMin(r.height(), (int)(pg * r.height())));

                        if(inverted)
                            drawProgress(painter, QRect(r.x(), r.y(), r.width(), height), option, true);
                        else
                            drawProgress(painter, QRect(r.x(), r.y()+(r.height()-height), r.width(), height), option, true);
                    }
                    else
                    {
                        int width(qMin(r.width(), (int)(pg * r.width())));

                        if(reverse || inverted)
                            drawProgress(painter, QRect(r.x()+(r.width()-width), r.y(), width, r.height()), option, false, true);
                        else
                            drawProgress(painter, QRect(r.x(), r.y(), width, r.height()), option);
                    }
                }

                painter->restore();
            }
            break;
        case CE_ProgressBarLabel:
            if (auto bar = styleOptCast<QStyleOptionProgressBar>(option)) {
                // The busy indicator doesn't draw a label
                if (0==bar->minimum && 0==bar->maximum)
                    return;

                bool vertical(false),
                     inverted(false),
                     bottomToTop(false);

                // Get extra style options if version 2
                if (auto bar2 =
                    styleOptCast<QStyleOptionProgressBarV2>(option)) {
                    vertical = bar2->orientation == Qt::Vertical;
                    inverted = bar2->invertedAppearance;
                    bottomToTop = bar2->bottomToTop;
                }

                painter->save();
                painter->setRenderHint(QPainter::Antialiasing, true);

                if (vertical)
                {
                    r = QRect(r.left(), r.top(), r.height(), r.width()); // flip width and height

                    QTransform m;
                    if (bottomToTop)
                    {
                        m.translate(0.0, r.width());
                        m.rotate(-90);
                    }
                    else
                    {
                        m.translate(r.height(), 0.0);
                        m.rotate(90);
                    }
                    painter->setTransform(m);
                }

                int                  progressIndicatorPos = (bar->progress - qreal(bar->minimum)) /
                                                             qMax(qreal(1.0), qreal(bar->maximum) - bar->minimum) * r.width();
                bool                 flip((!vertical && (((Qt::RightToLeft==bar->direction) && !inverted) ||
                                                        ((Qt::LeftToRight==bar->direction) && inverted))) ||
                                          (vertical && ((!inverted && !bottomToTop) || (inverted && bottomToTop))));
                QRect                leftRect;
                QRegion              rightRect(r);
                QPalette::ColorGroup cg=state&State_Enabled || State_None==state ? QPalette::Active : QPalette::Current;

                if (flip)
                {
                    int indicatorPos(r.width() - progressIndicatorPos);

                    if (indicatorPos >= 0 && indicatorPos <= r.width())
                    {
                        painter->setPen(palette.brush(cg, QPalette::Base).color());
                        leftRect = QRect(r.left(), r.top(), indicatorPos, r.height());
                        //rightRect = QRect(r.left()+indicatorPos, r.top(), r.width()-indicatorPos, r.height());
                    }
                    else if (indicatorPos > r.width())
                        painter->setPen(palette.brush(cg, QPalette::Text).color());
                    else
                        painter->setPen(palette.brush(cg, QPalette::HighlightedText).color());
                }
                else
                {
                    if (progressIndicatorPos >= 0 && progressIndicatorPos <= r.width())
                    {
                        leftRect = QRect(r.left(), r.top(), progressIndicatorPos, r.height());
                        //rightRect = QRect(r.left()+progressIndicatorPos, r.top(), r.width()-progressIndicatorPos, r.height());
                    }
                    else if (progressIndicatorPos > r.width())
                        painter->setPen(palette.brush(cg, QPalette::HighlightedText).color());
                    else
                        painter->setPen(palette.brush(cg, QPalette::Text).color());
                }

                QString text = bar->fontMetrics.elidedText(bar->text, Qt::ElideRight, r.width());

                rightRect = rightRect.subtracted(leftRect);
                painter->setClipRegion(rightRect);
                painter->drawText(r, text, QTextOption(Qt::AlignAbsolute | Qt::AlignHCenter | Qt::AlignVCenter));
                if (!leftRect.isNull())
                {
                    painter->setPen(palette.brush(cg, flip ? QPalette::Text : QPalette::HighlightedText).color());
                    painter->setClipRect(leftRect);
                    painter->drawText(r, text, QTextOption(Qt::AlignAbsolute | Qt::AlignHCenter | Qt::AlignVCenter));
                }

                painter->restore();
            }
            break;
        case CE_MenuBarItem:
            if (auto mbi = styleOptCast<QStyleOptionMenuItem>(option)) {
                bool    down(state&(State_On|State_Sunken)),
                        active(state&State_Enabled && (down || (state&State_Selected && opts.menubarMouseOver)));
                uint    alignment(Qt::AlignCenter|Qt::TextShowMnemonic|Qt::TextDontClip|Qt::TextSingleLine);
                QPixmap pix(getIconPixmap(mbi->icon, pixelMetric(PM_SmallIconSize), mbi->state));

                if (!styleHint(SH_UnderlineShortcut, mbi, widget))
                    alignment|=Qt::TextHideMnemonic;

                painter->save();

                if (!opts.xbar ||
                    (!widget || oneOf("QWidget",
                                      widget->metaObject()->className()))) {
                    drawMenuOrToolBarBackground(widget, painter,
                                                mbi->menuRect, option);
                }

                if(active)
                    drawMenuItem(painter, !opts.roundMbTopOnly && !POPUP_MENUS_SQUARE(opts) ? r.adjusted(1, 1, -1, -1) : r,
                                 option, MENU_BAR,
                                 (down || APP_OPENOFFICE==theThemedApp) && opts.roundMbTopOnly ? ROUNDED_TOP : ROUNDED_ALL,
                                 opts.useHighlightForMenu && (opts.colorMenubarMouseOver || down || APP_OPENOFFICE==theThemedApp)
                                            ? (m_ooMenuCols ? m_ooMenuCols : m_highlightCols) : m_backgroundCols);

                if (!pix.isNull())
                    drawItemPixmap(painter, mbi->rect, alignment, pix);
                else
                {
                    const QColor &col=state&State_Enabled
                                        ? ((opts.colorMenubarMouseOver && active) || (!opts.colorMenubarMouseOver && down))
                                            ? opts.customMenuTextColor
                                                ? opts.customMenuSelTextColor
                                                : opts.useHighlightForMenu
                                                    ? palette.highlightedText().color()
                                                    : palette.foreground().color()
                                            : palette.foreground().color()
                                         : palette.foreground().color();

                    painter->setPen(col);
                    painter->drawText(r, alignment, mbi->text);
                }
                painter->restore();
            }
            break;
        case CE_MenuItem:
            if (auto menuItem = styleOptCast<QStyleOptionMenuItem>(option)) {
                bool           comboMenu(qobject_cast<const QComboBox*>(widget)),
                               reverse(Qt::RightToLeft==menuItem->direction),
                               isOO(isOOWidget(widget));
                int            checkcol(qMax(menuItem->maxIconWidth, 20)),
                               stripeWidth(qMax(checkcol, constMenuPixmapWidth)-2);
                const QColor * use(popupMenuCols(option));

                QRect rx(r);

                if(isOO)
                {
                    if(opts.borderMenuitems)
                        r.adjust(2, 0, -2, 0);
                    else if(APPEARANCE_FADE==opts.menuitemAppearance)
                        r.adjust(1, 0, -1, 0);
                }

                painter->save();

                if (QStyleOptionMenuItem::Separator==menuItem->menuItemType)
                {
                    bool isMenu(!widget || qobject_cast<const QMenu*>(widget)),
                         doStripe(isMenu && opts.menuStripe && !comboMenu);

                    if(doStripe)
                        drawBevelGradient(menuStripeCol(), painter, QRect(reverse ? r.right()-stripeWidth : r.x(), r.y(),
                                                                          stripeWidth, r.height()),
                                          false, false, opts.menuStripeAppearance, WIDGET_OTHER);

                    if(!menuItem->text.isEmpty())
                    {
                        int textAlignment;
                        QFont font(menuItem->font);
                        QRect textRect;
                        if (opts.buttonStyleMenuSections) {
                            QStyleOption opt;
                            opt.rect = r.adjusted(2, 2, -3, -2);
                            opt.state = State_Raised | State_Enabled | State_Horizontal;
                            drawLightBevel(painter, opt.rect, &opt, widget, ROUNDED_ALL,
                                           getFill(&opt, use), use, true,
                                           WIDGET_NO_ETCH_BTN);

                            font.setBold(true);
                            painter->setFont(font);
                            textAlignment = Qt::AlignHCenter | Qt::AlignVCenter;
                            textRect = r;
                        } else {
                            font.setBold(true);
                            font.setUnderline(true);
#ifdef Q_OS_MACOS
                            if (QGuiApplication::platformName() == QLatin1String("cocoa")) {
                                font.setLetterSpacing(QFont::PercentageSpacing, 95);
                            }
#endif
                            painter->setFont(font);
                            QRect miRect(menuItem->rect.left() + 5 +
                                         (!reverse && doStripe ? stripeWidth : 0),
                                         menuItem->rect.center().y() - painter->fontMetrics().ascent() / 2 - 5,
                                         menuItem->rect.width() -
                                         (9 + (doStripe ? stripeWidth : 0)), 1);
                            QColor lineCol = use[MENU_SEP_SHADE];
                            if (miRect.y() >= rx.height() / 2) {
                                drawFadedLine(painter, miRect, lineCol, reverse,
                                          !reverse, true);
                            }
                            textAlignment = Qt::AlignLeft | Qt::AlignBottom;
                            textRect = r.adjusted(5, 5, -5, -5);
                        }
                        drawItemTextWithRole(painter, textRect,
                                             textAlignment,
                                             palette, state & State_Enabled,
                                             menuItem->text, QPalette::Text);
                    }
                    else
                    {
                        QRect miRect(menuItem->rect.left() + 3 +
                                        (!reverse && doStripe ? stripeWidth : 0),
                                        menuItem->rect.center().y(),
                                        menuItem->rect.width() - (7 + (doStripe ? stripeWidth : 0)),
                                        1);
                        drawFadedLine(painter, miRect, use[MENU_SEP_SHADE], true, true, true);
                    }

                    if(isOO)
                    {
                        painter->setPen(use[QTC_STD_BORDER]);
                        painter->drawLine(rx.topLeft(), rx.bottomLeft());
                        painter->drawLine(rx.topRight(), rx.bottomRight());
                    }
                    painter->restore();
                    break;
                }

                bool selected(state&State_Selected),
                     checkable(QStyleOptionMenuItem::NotCheckable!=menuItem->checkType),
                     checked(menuItem->checked),
                     enabled(state&State_Enabled);

                if(opts.menuStripe && !comboMenu)
                    drawBevelGradient(menuStripeCol(), painter,
                                      QRect(reverse ? r.right()-stripeWidth : r.x(), r.y(), stripeWidth, r.height()),
                                      false, false, opts.menuStripeAppearance, WIDGET_OTHER);

                if (selected && enabled)
                    drawMenuItem(painter, r, option, /*comboMenu ? MENU_COMBO : */MENU_POPUP, ROUNDED_ALL,
                                 opts.useHighlightForMenu ? (m_ooMenuCols ? m_ooMenuCols : m_highlightCols) : use);

                if(comboMenu)
                {
                    if (menuItem->icon.isNull())
                        checkcol = 0;
                    else
                        checkcol = menuItem->maxIconWidth;
                }
                else
                {
                    // Check
                    QRect checkRect(r.left() + 3, r.center().y() - 6, opts.crSize, opts.crSize);
                    checkRect = visualRect(menuItem->direction, menuItem->rect, checkRect);
                    if (checkable)
                    {
                        if (!opts.onlyTicksInMenu && (menuItem->checkType & QStyleOptionMenuItem::Exclusive) && menuItem->icon.isNull())
                        {
                            QStyleOptionButton button;
                            button.rect = checkRect;
                            button.state = menuItem->state|STATE_MENU;
                            if (checked)
                                button.state |= State_On;
                            button.palette = palette;
                            drawPrimitive(PE_IndicatorRadioButton, &button, painter, widget);
                        }
                        else
                        {
                            if (menuItem->icon.isNull() || !opts.menuIcons)
                            {
                                QStyleOptionButton button;
                                button.rect = checkRect;
                                button.state = menuItem->state|STATE_MENU;
                                if (checked)
                                    button.state |= State_On;
                                button.palette = palette;
                                drawPrimitive(PE_IndicatorMenuCheckMark, &button, painter, widget);
                            }
                            else if (checked)
                            {
                                int          iconSize(qMax(menuItem->maxIconWidth, 20));
                                QRect        sunkenRect(r.left() + 1, r.top() + (r.height() - iconSize) / 2,
                                                        iconSize, iconSize);
                                QStyleOption opt(*option);

                                sunkenRect = visualRect(menuItem->direction, menuItem->rect, sunkenRect);
                                opt.state = menuItem->state;
                                opt.state|=State_Raised|State_Horizontal|State_On;
                                drawLightBevel(painter, sunkenRect, &opt, widget, ROUNDED_ALL, getFill(&opt, m_buttonCols), m_buttonCols);
                            }
                        }
                    }
                }

                // Text and icon, ripped from windows style
                bool  dis(!(state&State_Enabled)),
                      act(state&State_Selected);
                QRect vCheckRect(visualRect(option->direction, menuItem->rect,
                                            QRect(menuItem->rect.x(), menuItem->rect.y(), checkcol, menuItem->rect.height())));

                if (opts.menuIcons && !menuItem->icon.isNull())
                {
                    QIcon::Mode mode(dis ? QIcon::Disabled : QIcon::Normal);

                    if (act && !dis)
                        mode = QIcon::Active;

                    QPixmap pixmap(getIconPixmap(menuItem->icon, pixelMetric(PM_SmallIconSize), mode,
                                   checked ? QIcon::On : QIcon::Off));

                    int     pixw(pixmap.width()),
                            pixh(pixmap.height());
                    QRect   pmr(0, 0, pixw, pixh);

                    pmr.moveCenter(vCheckRect.center());
                    painter->setPen(palette.text().color());
                    if (checkable && checked)
                        painter->drawPixmap(QPoint(pmr.left() + 1, pmr.top() + 1), pixmap);
                    else
                        painter->drawPixmap(pmr.topLeft(), pixmap);
                }

                painter->setPen(dis
                                    ? palette.text().color()
                                    : selected && opts.useHighlightForMenu && !m_ooMenuCols
                                        ? palette.highlightedText().color()
                                        : palette.foreground().color());

                int x, y, w, h,
                    tab(menuItem->tabWidth);

                menuItem->rect.getRect(&x, &y, &w, &h);

                int     xm(windowsItemFrame + checkcol + windowsItemHMargin -2),
                        xpos(menuItem->rect.x() + xm);
                QRect   textRect(xpos, y + windowsItemVMargin,
                                 opts.menuIcons ? (w - xm - windowsRightBorder - tab + 1)
                                                : (w - ((xm*2) + tab)),
                                 h - 2 * windowsItemVMargin),
                        vTextRect = visualRect(option->direction, menuItem->rect, textRect);
                QString s(menuItem->text);

                if (!s.isEmpty())                      // draw text
                {
                    int t(s.indexOf(QLatin1Char('\t'))),
                        textFlags(Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine);

                    if (!styleHint(SH_UnderlineShortcut, menuItem, widget))
                        textFlags |= Qt::TextHideMnemonic;
                    textFlags |= Qt::AlignLeft;

                    if (t >= 0)
                    {
                        QRect vShortcutRect(visualRect(option->direction, menuItem->rect,
                                                    QRect(textRect.topRight(), QPoint(menuItem->rect.right(), textRect.bottom()))));

                        painter->drawText(vShortcutRect, textFlags, s.mid(t + 1));
                        s = s.left(t);
                    }

                    QFont font(menuItem->font);

                    if (menuItem->menuItemType == QStyleOptionMenuItem::DefaultItem)
                        font.setBold(true);

                    painter->setFont(font);
                    painter->drawText(vTextRect, textFlags, s.left(t));
                }

                // Arrow
                if (QStyleOptionMenuItem::SubMenu==menuItem->menuItemType) // draw sub menu arrow
                {
                    int              dim((menuItem->rect.height() - 4) / 2),
                                     xpos(menuItem->rect.left() + menuItem->rect.width() - 3 - dim);
                    PrimitiveElement arrow(Qt::RightToLeft==option->direction ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight);
                    QRect            vSubMenuRect(visualRect(option->direction, menuItem->rect,
                                                             QRect(xpos, menuItem->rect.top() + menuItem->rect.height() / 2 - dim / 2, dim, dim)));

                    drawArrow(painter, vSubMenuRect, arrow,
                              opts.useHighlightForMenu && state&State_Enabled && state&State_Selected && !m_ooMenuCols
                                ? palette.highlightedText().color()
                                : palette.text().color());
                }

                if(isOO)
                {
                    painter->setPen(use[QTC_STD_BORDER]);
                    painter->drawLine(rx.topLeft(), rx.bottomLeft());
                    painter->drawLine(rx.topRight(), rx.bottomRight());
                }
                painter->restore();
            }
            break;
        case CE_MenuHMargin:
        case CE_MenuVMargin:
        case CE_MenuEmptyArea:
            break;
        case CE_PushButton:
            if (auto btn = styleOptCast<QStyleOptionButton>(option)) {
                // For OO.o 3.2 need to fill widget background!
                if(isOOWidget(widget))
                    painter->fillRect(r, palette.brush(QPalette::Window));
                drawControl(CE_PushButtonBevel, btn, painter, widget);

                QStyleOptionButton subopt(*btn);

                subopt.rect = subElementRect(SE_PushButtonContents, btn, widget);
                drawControl(CE_PushButtonLabel, &subopt, painter, widget);

                if (state & State_HasFocus &&
                    !(state & State_MouseOver &&
                      oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED) &&
                      opts.coloredMouseOver != MO_NONE)) {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*btn);
                    fropt.rect = subElementRect(SE_PushButtonFocusRect, btn,
                                                widget);
                    drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
                }
            }
            break;
        case CE_PushButtonBevel:
            if (auto btn = styleOptCast<QStyleOptionButton>(option)) {
                int dbi(pixelMetric(PM_ButtonDefaultIndicator, btn, widget));

                if (btn->features & QStyleOptionButton::DefaultButton)
                    drawPrimitive(PE_FrameDefaultButton, option, painter, widget);
                if (btn->features & QStyleOptionButton::AutoDefaultButton)
                    r.setCoords(r.left() + dbi, r.top() + dbi, r.right() - dbi, r.bottom() - dbi);
                if (!(btn->features &
                      (QStyleOptionButton::Flat |
                       QStyleOptionButton::CommandLinkButton)) ||
                    state & (State_Sunken | State_On | State_MouseOver)) {
                    QStyleOptionButton tmpBtn(*btn);

                    tmpBtn.rect = r;
                    drawPrimitive(PE_PanelButtonCommand, &tmpBtn, painter, widget);
                }
                if (btn->features & QStyleOptionButton::HasMenu)
                {
                    int   mbi(pixelMetric(PM_MenuButtonIndicator, btn, widget));
                    QRect ar(Qt::LeftToRight==btn->direction
                                ? btn->rect.right() - (mbi+6)
                                : btn->rect.x() + 6,
                             ((btn->rect.height() - mbi)/2),
                             mbi, mbi);

                    if(option->state &(State_On | State_Sunken))
                        ar.adjust(1, 1, 1, 1);

                    drawArrow(painter, ar, PE_IndicatorArrowDown,
                              MOArrow(state, palette, QPalette::ButtonText));
                }
            }
            break;
        case CE_PushButtonLabel:
            if (auto button = styleOptCast<QStyleOptionButton>(option)) {
                uint tf(Qt::AlignVCenter | Qt::TextShowMnemonic);

                if (!styleHint(SH_UnderlineShortcut, button, widget))
                    tf |= Qt::TextHideMnemonic;

                if (!button->icon.isNull())
                {
                    //Center both icon and text
                    QIcon::Mode mode(button->state&State_Enabled ? QIcon::Normal : QIcon::Disabled);

                    if (QIcon::Normal==mode && button->state&State_HasFocus)
                        mode = QIcon::Active;

                    QIcon::State state((button->state&State_On) || (button->state&State_Sunken) ? QIcon::On : QIcon::Off);
                    QPixmap      pixmap(getIconPixmap(button->icon, button->iconSize, mode, state));
                    int          labelWidth(pixmap.width()),
                                 labelHeight(pixmap.height()),
                                 iconSpacing (4);//### 4 is currently hardcoded in QPushButton::sizeHint()

                    if (!button->text.isEmpty())
                        labelWidth += (button->fontMetrics.boundingRect(r, tf, button->text).width() + iconSpacing);

                    QRect iconRect(r.x() + (r.width() - labelWidth) / 2,
                                   r.y() + (r.height() - labelHeight) / 2,
                                   pixmap.width(), pixmap.height());

                    iconRect = visualRect(button->direction, r, iconRect);

                    tf |= Qt::AlignLeft; //left align, we adjust the text-rect instead

                    if (Qt::RightToLeft==button->direction)
                        r.setRight(iconRect.left() - iconSpacing);
                    else
                        r.setLeft(iconRect.left() + iconRect.width() + iconSpacing);

                    if (button->state & (State_On|State_Sunken))
                        iconRect.translate(pixelMetric(PM_ButtonShiftHorizontal, option, widget),
                                           pixelMetric(PM_ButtonShiftVertical, option, widget));
                    painter->drawPixmap(iconRect, pixmap);
                }
                else
                    tf |= Qt::AlignHCenter;

                if (button->state & (State_On|State_Sunken))
                    r.translate(pixelMetric(PM_ButtonShiftHorizontal, option, widget),
                                pixelMetric(PM_ButtonShiftVertical, option, widget));

                // The following is mainly for DejaVu Sans 11...
                if(button->fontMetrics.height()==19 && r.height()==(23+((opts.thin&THIN_BUTTONS) ? 0 : 2)))
                    r.translate(0, 1);

                if (button->features & QStyleOptionButton::HasMenu) {
                    int mbi(pixelMetric(PM_MenuButtonIndicator, button, widget));

                    if (Qt::LeftToRight == button->direction) {
                        r = r.adjusted(0, 0, -mbi, 0);
                    } else {
                        r = r.adjusted(mbi, 0, 0, 0);
                    }
                }

                int num(opts.embolden && button->features&QStyleOptionButton::DefaultButton ? 2 : 1);

                for(int i=0; i<num; ++i)
                    drawItemTextWithRole(painter, r.adjusted(i, 0, i, 0), tf, palette, (button->state&State_Enabled),
                                         button->text, QPalette::ButtonText);
            }
            break;
        case CE_ComboBoxLabel:
            if (auto comboBox = styleOptCast<QStyleOptionComboBox>(option)) {
                QRect editRect = subControlRect(CC_ComboBox, comboBox, SC_ComboBoxEditField, widget);
                bool  sunken=!comboBox->editable && (state&(State_On|State_Sunken));
                int   shiftH=sunken ? pixelMetric(PM_ButtonShiftHorizontal, option, widget) : 0,
                      shiftV=sunken ? pixelMetric(PM_ButtonShiftVertical, option, widget) : 0;

                painter->save();

                if (!comboBox->currentIcon.isNull())
                {
                    QPixmap pixmap = getIconPixmap(comboBox->currentIcon, comboBox->iconSize, state);
                    QRect   iconRect(editRect);

                    iconRect.setWidth(comboBox->iconSize.width() + 5);
                    if(!comboBox->editable)
                        iconRect = alignedRect(QApplication::layoutDirection(), Qt::AlignLeft|Qt::AlignVCenter,
                                               iconRect.size(), editRect);
                    if (comboBox->editable)
                    {
                        int adjust=opts.etchEntry ? 2 : 1;

                        if(opts.square&SQUARE_ENTRY || opts.round<ROUND_FULL)
                            painter->fillRect(iconRect.adjusted(adjust-1, adjust, -(adjust-1), -adjust), palette.brush(QPalette::Base));
                        else
                        {
                            painter->fillRect(iconRect.adjusted(1, adjust, -1, -adjust), palette.brush(QPalette::Base));
                            painter->fillRect(iconRect.adjusted(0, adjust+1, 0, -(adjust+1)), palette.brush(QPalette::Base));
                        }
                    }

                    if (sunken)
                        iconRect.translate(shiftH, shiftV);

                    drawItemPixmap(painter, iconRect, Qt::AlignCenter, pixmap);

                    if (reverse)
                        editRect.translate(-4 - comboBox->iconSize.width(), 0);
                    else
                        editRect.translate(comboBox->iconSize.width() + 4, 0);
                }

                if (!comboBox->currentText.isEmpty() && !comboBox->editable)
                {
                    if (sunken)
                        editRect.translate(shiftH, shiftV);

                    int margin=comboBox->frame && widget && widget->rect().height()<(opts.buttonEffect != EFFECT_NONE ? 22 : 20)  ? 4 : 0;
                    editRect.adjust(1, -margin, -1, margin);
                    painter->setClipRect(editRect);
                    drawItemTextWithRole(painter, editRect, Qt::AlignLeft|Qt::AlignVCenter, palette,
                                         state&State_Enabled, comboBox->currentText, QPalette::ButtonText);
                }
                painter->restore();
            }
            break;
        case CE_MenuBarEmptyArea: {
            painter->save();
            if (!opts.xbar ||
                (!widget || noneOf(widget->metaObject()->className(),
                                   "QWidget"))) {
                drawMenuOrToolBarBackground(widget, painter, r, option);
            }
            if (TB_NONE != opts.toolbarBorders && widget &&
                widget->parentWidget() &&
                (qobject_cast<const QMainWindow*>(widget->parentWidget()) ||
                 widget->parentWidget()->inherits("Q3MainWindow"))) {
                const QColor *use = menuColors(option, m_active);
                bool dark = (TB_DARK == opts.toolbarBorders ||
                             TB_DARK_ALL == opts.toolbarBorders);

                if (TB_DARK_ALL == opts.toolbarBorders ||
                    TB_LIGHT_ALL == opts.toolbarBorders) {
                    painter->setPen(use[0]);
                    painter->drawLine(r.x(), r.y(),
                                      r.x() + r.width() - 1, r.y());
                    painter->drawLine(r.x(), r.y(), r.x(),
                                      r.y() + r.width() - 1);
                    painter->setPen(use[dark ? 3 : 4]);
                    painter->drawLine(r.x(), r.y() + r.height() - 1,
                                      r.x() + r.width() - 1,
                                      r.y() + r.height() - 1);
                    painter->drawLine(r.x() + r.width() - 1, r.y(),
                                      r.x() + r.width() - 1,
                                      r.y() + r.height() - 1);
                } else {
                    painter->setPen(use[dark ? 3 : 4]);
                    painter->drawLine(r.x(), r.y() + r.height() - 1,
                                      r.x() + r.width() - 1,
                                      r.y() + r.height() - 1);
                }
            }
            painter->restore();
            break;
        }
        case CE_TabBarTabLabel:
            if (auto tab = styleOptCast<QStyleOptionTab>(option)) {
                QStyleOptionTabV3 tabV2(*tab);
                bool verticalTabs(QTabBar::RoundedEast==tabV2.shape || QTabBar::RoundedWest==tabV2.shape ||
                                  QTabBar::TriangularEast==tabV2.shape || QTabBar::TriangularWest==tabV2.shape),
                     toolbarTab=!opts.toolbarTabs && widget && widget->parentWidget() &&
                                qobject_cast<QToolBar*>(widget->parentWidget());

                if (verticalTabs)
                {
                    painter->save();
                    int newX, newY, newRot;
                    if (QTabBar::RoundedEast==tabV2.shape || QTabBar::TriangularEast==tabV2.shape)
                    {
                        newX = r.width();
                        newY = r.y();
                        newRot = 90;
                    }
                    else
                    {
                        newX = 0;
                        newY = r.y() + r.height();
                        newRot = -90;
                    }
                    r.setRect(0, 0, r.height(), r.width());

                    QTransform m;
                    m.translate(newX, newY);
                    m.rotate(newRot);
                    painter->setTransform(m, true);
                }

                int alignment(Qt::AlignVCenter | Qt::TextShowMnemonic | (opts.centerTabText ? Qt::AlignHCenter : Qt::AlignLeft));

                if (!styleHint(SH_UnderlineShortcut, option, widget))
                    alignment |= Qt::TextHideMnemonic;

                if(toolbarTab)
                    tabV2.state&=~State_Selected;
                r = subElementRect(SE_TabBarTabText, &tabV2, widget);
                if (!tabV2.icon.isNull())
                {
                    QSize iconSize(tabV2.iconSize);
                    if (!iconSize.isValid())
                    {
                        int iconExtent(pixelMetric(PM_SmallIconSize));
                        iconSize = QSize(iconExtent, iconExtent);
                    }

                    QPixmap tabIcon(getIconPixmap(tabV2.icon, iconSize, state&State_Enabled));
                    QSize   tabIconSize = tabV2.icon.actualSize(iconSize, tabV2.state&State_Enabled
                                                                  ? QIcon::Normal
                                                                  : QIcon::Disabled);

                    int offset = 4,
                        left = option->rect.left();
                    if (tabV2.leftButtonSize.isNull() || tabV2.leftButtonSize.width()<=0)
                        offset += 2;
                    else
                        left += tabV2.leftButtonSize.width() + 2;
                    QRect iconRect = QRect(left + offset, r.center().y() - tabIcon.height() / 2,
                                           tabIconSize.width(), tabIconSize.height());
                    if (!verticalTabs)
                        iconRect = visualRect(option->direction, option->rect, iconRect);
                    painter->drawPixmap(iconRect.x(), iconRect.y(), tabIcon);
                }

                if(!tab->text.isEmpty())
                {
                    drawItemTextWithRole(painter, r, alignment, tab->palette, tab->state&State_Enabled, tab->text,
                                         !opts.stdSidebarButtons && toolbarTab && state&State_Selected
                                            ? QPalette::HighlightedText : QPalette::WindowText);
                }

                if (verticalTabs)
                    painter->restore();

                if (tabV2.state & State_HasFocus)
                {
                    const int constOffset = 1 + pixelMetric(PM_DefaultFrameWidth);

                    int                   x1=tabV2.rect.left(),
                                          x2=tabV2.rect.right() - 1;
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*tab);
                    fropt.rect.setRect(x1 + 1 + constOffset, tabV2.rect.y() + constOffset,
                                       x2 - x1 - 2*constOffset, tabV2.rect.height() - 2*constOffset);

                    fropt.state|=State_Horizontal;
                    if(FOCUS_LINE!=opts.focus)
                    {
                        if(QTabBar::RoundedNorth==tabV2.shape || QTabBar::TriangularNorth==tabV2.shape)
                            fropt.rect.adjust(0, 1, 0, 0);
                    }
                    else if(TAB_MO_BOTTOM==opts.tabMouseOver && FOCUS_LINE==opts.focus)
                        switch(tabV2.shape)
                        {
                            case QTabBar::RoundedNorth:
                            case QTabBar::TriangularNorth:
                                fropt.rect.adjust(0, 0, 0, 1);
                                break;
                            case QTabBar::RoundedEast:
                            case QTabBar::TriangularEast:
                                fropt.rect.adjust(-2, 0, -(fropt.rect.width()+1), 0);
                                fropt.state&=~State_Horizontal;
                                break;
                            case QTabBar::RoundedSouth:
                            case QTabBar::TriangularSouth:
                                fropt.rect.adjust(0, 0, 0, 1);
                                break;
                            case QTabBar::RoundedWest:
                            case QTabBar::TriangularWest:
                                fropt.rect.adjust(0, 0, 2, 0);
                                fropt.state&=~State_Horizontal;
                            default:
                                break;
                        }

                    drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
                }
            }
            break;
        case CE_TabBarTabShape:
            if(!opts.toolbarTabs && widget && widget->parentWidget() && qobject_cast<QToolBar*>(widget->parentWidget()))
            {
                QStyleOption opt(*option);
                if(state&State_Selected)
                    opt.state|=State_On;
                if(opts.stdSidebarButtons)
                {
                    if(state&(State_Selected|State_MouseOver))
                    {
                        opt.state|=STATE_TBAR_BUTTON;
                        drawPrimitive(PE_PanelButtonTool, &opt, painter, widget);
                    }
                }
                else
                    drawSideBarButton(painter, r, &opt, widget);
            } else if (auto tab = styleOptCast<QStyleOptionTab>(option)) {
                bool onlyTab(widget && widget->parentWidget()
                                ? qobject_cast<const QTabWidget*>(widget->parentWidget()) ? false : true
                                : false),
                     selected(state&State_Selected),
                     horiz(QTabBar::RoundedNorth==tab->shape || QTabBar::RoundedSouth==tab->shape);

                QStyleOptionTabV3 tabV3(*tab);
                QRect        r2(r);
                bool         rtlHorTabs(Qt::RightToLeft==tab->direction && horiz),
                             oneTab(QStyleOptionTab::OnlyOneTab==tab->position),
                             leftCornerWidget(tab->cornerWidgets&QStyleOptionTab::LeftCornerWidget),
                             rightCornerWidget(tab->cornerWidgets&QStyleOptionTab::RightCornerWidget),
                             firstTab((tab->position == (Qt::LeftToRight==tab->direction || !horiz ?
                                       QStyleOptionTab::Beginning : QStyleOptionTab::End)) || oneTab),
                             lastTab((tab->position == (Qt::LeftToRight==tab->direction  || !horiz ?
                                     QStyleOptionTab::End : QStyleOptionTab::Beginning)) || oneTab);
                int          tabBarAlignment(styleHint(SH_TabBar_Alignment, tab, widget)),
                             tabOverlap(oneTab ? 0 : pixelMetric(PM_TabBarTabOverlap, option, widget)),
                             moOffset(ROUND_NONE==opts.round || TAB_MO_TOP!=opts.tabMouseOver ? 1 : opts.round),
                             highlightOffset(opts.highlightTab && opts.round>ROUND_SLIGHT ? 2 : 1),
                             highlightBorder(opts.round>ROUND_FULL ? 4 : 3),
                             sizeAdjust(!selected && TAB_MO_GLOW==opts.tabMouseOver ? 1 : 0);
                bool         leftAligned((!rtlHorTabs && Qt::AlignLeft==tabBarAlignment) ||
                                 (rtlHorTabs && Qt::AlignRight==tabBarAlignment)),
                             rightAligned((!rtlHorTabs && Qt::AlignRight==tabBarAlignment) ||
                                          (rtlHorTabs && Qt::AlignLeft==tabBarAlignment)),
                             docMode(tabV3.documentMode),
                             docFixLeft(!leftCornerWidget && leftAligned && firstTab && (docMode || onlyTab)),
                             fixLeft(!onlyTab && !leftCornerWidget && leftAligned && firstTab && !docMode),
                             fixRight(!onlyTab && !rightCornerWidget && rightAligned && lastTab && !docMode),
                             mouseOver(state&State_Enabled && state&State_MouseOver),
                             glowMo(!selected && mouseOver && opts.coloredMouseOver && TAB_MO_GLOW==opts.tabMouseOver),
                             thin(opts.thin&THIN_FRAMES),
                             drawOuterGlow(glowMo && !thin);
                const QColor *use(backgroundColors(option));
                QColor       fill(getTabFill(selected, mouseOver, use));
                double       radius=qtcGetRadius(&opts, r.width(), r.height(), WIDGET_TAB_TOP, RADIUS_EXTERNAL);
                EBorder      borderProfile(selected || opts.borderInactiveTab
                                            ? opts.borderTab
                                                ? BORDER_LIGHT
                                                : BORDER_RAISED
                                            : BORDER_FLAT);

                painter->save();

                if(!selected && (100!=opts.bgndOpacity || 100!=opts.dlgOpacity))
                {
                    QWidget *top = widget ? widget->window() : 0L;
                    bool isDialog = top && qtcIsDialog(top);

                    // Note: opacity is divided by 150 to make dark inactive tabs more translucent
                    if(isDialog && 100!=opts.dlgOpacity)
                        fill.setAlphaF(opts.dlgOpacity/150.0);
                    else if(!isDialog && 100!=opts.bgndOpacity)
                        fill.setAlphaF(opts.bgndOpacity/150.0);
                }

                switch(tab->shape)
                {
                    case QTabBar::RoundedNorth:
                    case QTabBar::TriangularNorth:
                    {
                        int round=selected || oneTab || TAB_MO_GLOW==opts.tabMouseOver || opts.roundAllTabs
                                        ? ROUNDED_TOP
                                        : firstTab
                                            ? ROUNDED_TOPLEFT
                                            : lastTab
                                                ? ROUNDED_TOPRIGHT
                                                : ROUNDED_NONE;
                        if(!selected)
                            r.adjust(0, 2, 0, -2);

                        if(!firstTab)
                            r.adjust(-tabOverlap, 0, 0, 0);
                        painter->setClipPath(buildPath(r.adjusted(0, 0, 0, 4), WIDGET_TAB_TOP, round, radius));
                        fillTab(painter, r.adjusted(1+sizeAdjust, 1, -(1+sizeAdjust), 0), option, fill, true, WIDGET_TAB_TOP, (docMode || onlyTab));
                        // This clipping (for selected) helps with plasma's tabs and nvidia
                        if(selected || thin)
                            painter->setClipRect(r2.adjusted(-1, 0, 1, -1));
                        else
                            painter->setClipping(false);
                        drawBorder(painter, r.adjusted(sizeAdjust, 0, -sizeAdjust, 4), option, round, glowMo ? m_mouseOverCols : 0L,
                                   WIDGET_TAB_TOP, borderProfile, false);
                        if(drawOuterGlow)
                            drawGlow(painter, r.adjusted(0, -1, 0, 5), WIDGET_TAB_TOP);

                        if(selected || thin)
                            painter->setClipping(false);

                        if(selected)
                        {
                            if(!thin)
                            {
                                painter->setPen(use[0]);

                                // The point drawn below is because of the clipping above...
                                if(fixLeft)
                                    painter->drawPoint(r2.x()+1, r2.y()+r2.height()-1);
                                else
                                    painter->drawLine(r2.left()-1, r2.bottom(), r2.left(), r2.bottom());
                                if(!fixRight)
                                    painter->drawLine(r2.right()-1, r2.bottom(), r2.right(), r2.bottom());
                            }

                            if(docFixLeft)
                            {
                                QColor col(use[QTC_STD_BORDER]);
                                col.setAlphaF(0.5);
                                painter->setPen(col);
                                painter->drawPoint(r2.x(), r2.y()+r2.height()-1);
                            }
                        }
                        else
                        {
                            int l(fixLeft ? r2.left()+(opts.round>ROUND_SLIGHT && !(opts.square&SQUARE_TAB_FRAME) ? 2 : 1) : r2.left()-1),
                                r(fixRight ? r2.right()-2 : r2.right()+1);
                            painter->setPen(use[QTC_STD_BORDER]);
                            painter->drawLine(l, r2.bottom()-1, r, r2.bottom()-1);
                            if(!thin)
                            {
                                painter->setPen(use[0]);
                                painter->drawLine(l, r2.bottom(), r, r2.bottom());
                            }
                        }

                        if(selected)
                        {
                            if(opts.highlightTab)
                            {
                                QColor col(m_highlightCols[0]);
                                painter->setRenderHint(QPainter::Antialiasing, true);
                                painter->setPen(col);
                                drawAaLine(painter, r.left()+highlightOffset, r.top()+1, r.right()-highlightOffset, r.top()+1);
                                col.setAlphaF(0.5);
                                painter->setPen(col);
                                drawAaLine(painter, r.left()+1, r.top()+2, r.right()-1, r.top()+2);
                                painter->setRenderHint(QPainter::Antialiasing, false);
                                painter->setClipRect(QRect(r.x(), r.y(), r.width(), highlightBorder));
                                drawBorder(painter, r, option, ROUNDED_ALL, m_highlightCols, WIDGET_TAB_TOP, BORDER_FLAT, false, 3);
                            }

                            if(opts.colorSelTab)
                                colorTab(painter, r.adjusted(1+sizeAdjust, 1, -(1+sizeAdjust), 0), true, WIDGET_TAB_TOP, round);
                        }
                        else if(mouseOver && opts.coloredMouseOver && TAB_MO_GLOW!=opts.tabMouseOver)
                            drawHighlight(painter, QRect(r.x()+(firstTab ? moOffset : 1),
                                                         r.y()+(TAB_MO_TOP==opts.tabMouseOver ? 0 : r.height()-1),
                                                         r.width()-(firstTab || lastTab ? moOffset : 1), 2),
                                          true, TAB_MO_TOP==opts.tabMouseOver);
                        break;
                    }
                    case QTabBar::RoundedSouth:
                    case QTabBar::TriangularSouth:
                    {
                        int round=selected || oneTab || TAB_MO_GLOW==opts.tabMouseOver || opts.roundAllTabs
                                        ? ROUNDED_BOTTOM
                                        : firstTab
                                            ? ROUNDED_BOTTOMLEFT
                                            : lastTab
                                                ? ROUNDED_BOTTOMRIGHT
                                                : ROUNDED_NONE;
                        if(!selected)
                            r.adjust(0, 2, 0, -2);
                        if(!firstTab)
                            r.adjust(-tabOverlap, 0, 0, 0);

                        painter->setClipPath(buildPath(r.adjusted(0, -4, 0, 0), WIDGET_TAB_BOT, round, radius));
                        fillTab(painter, r.adjusted(1+sizeAdjust, 0, -(1+sizeAdjust), -1), option, fill, true, WIDGET_TAB_BOT, (docMode || onlyTab));
                        if(thin)
                            painter->setClipRect(r2.adjusted(0, 1, 0, 0));
                        else
                            painter->setClipping(false);
                        drawBorder(painter, r.adjusted(sizeAdjust, -4, -sizeAdjust, 0), option, round, glowMo ? m_mouseOverCols : 0L,
                                   WIDGET_TAB_BOT, borderProfile, false);
                        if(thin)
                            painter->setClipping(false);
                        if(drawOuterGlow)
                            drawGlow(painter, r.adjusted(0, -5, 0, 1), WIDGET_TAB_BOT);

                        if(selected)
                        {
                            if(!thin)
                            {
                                painter->setPen(use[opts.borderTab ? 0 : FRAME_DARK_SHADOW]);
                                if(!fixLeft)
                                    painter->drawPoint(r2.left()-(TAB_MO_GLOW==opts.tabMouseOver ? 0 : 1), r2.top());
                                if(!fixRight)
                                    painter->drawLine(r2.right()-(TAB_MO_GLOW==opts.tabMouseOver ? 0 : 1), r2.top(), r2.right(), r2.top());
                            }
                            if(docFixLeft)
                            {
                                QColor col(use[QTC_STD_BORDER]);
                                col.setAlphaF(0.5);
                                painter->setPen(col);
                                painter->drawPoint(r2.x(), r2.y());
                            }
                        }
                        else
                        {
                            int l(fixLeft ? r2.left()+(opts.round>ROUND_SLIGHT && !(opts.square&SQUARE_TAB_FRAME)? 2 : 1) : r2.left()-1),
                                r(fixRight ? r2.right()-2 : r2.right());
                            painter->setPen(use[QTC_STD_BORDER]);
                            painter->drawLine(l, r2.top()+1, r, r2.top()+1);
                            if(!thin)
                            {
                                painter->setPen(use[opts.borderTab ? 0 : FRAME_DARK_SHADOW]);
                                painter->drawLine(l, r2.top(), r, r2.top());
                            }
                        }

                        if(selected)
                        {
                            if(opts.highlightTab)
                            {
                                QColor col(m_highlightCols[0]);
                                painter->setRenderHint(QPainter::Antialiasing, true);
                                painter->setPen(col);
                                drawAaLine(painter, r.left()+highlightOffset, r.bottom()-1, r.right()-highlightOffset, r.bottom()-1);
                                col.setAlphaF(0.5);
                                painter->setPen(col);
                                drawAaLine(painter, r.left()+1, r.bottom()-2, r.right()-1, r.bottom()-2);
                                painter->setRenderHint(QPainter::Antialiasing, false);
                                painter->setClipRect(QRect(r.x(), r.y()+r.height()-highlightBorder, r.width(), r.y()+r.height()-1));
                                drawBorder(painter, r, option, ROUNDED_ALL, m_highlightCols, WIDGET_TAB_BOT, BORDER_FLAT, false, 3);
                            }

                            if(opts.colorSelTab)
                                colorTab(painter, r.adjusted(1+sizeAdjust, 0, -(1+sizeAdjust), -1), true, WIDGET_TAB_BOT, round);
                        }
                        else if(mouseOver && opts.coloredMouseOver && TAB_MO_GLOW!=opts.tabMouseOver)
                            drawHighlight(painter, QRect(r.x()+(firstTab ? moOffset : 1),
                                                         r.y()+(TAB_MO_TOP==opts.tabMouseOver ? r.height()-2 : -1),
                                                         r.width()-(firstTab || lastTab ? moOffset : 1), 2),
                                          true, TAB_MO_TOP!=opts.tabMouseOver);
                        break;
                    }
                    case QTabBar::RoundedWest:
                    case QTabBar::TriangularWest:
                    {
                        int round=selected || oneTab || TAB_MO_GLOW==opts.tabMouseOver || opts.roundAllTabs
                                        ? ROUNDED_LEFT
                                        : firstTab
                                            ? ROUNDED_TOPLEFT
                                            : lastTab
                                                ? ROUNDED_BOTTOMLEFT
                                                : ROUNDED_NONE;
                        if(!selected)
                            r.adjust(2, 0, -2, 0);

                        if(!firstTab)
                            r.adjust(0, -tabOverlap, 0, 0);
                        painter->setClipPath(buildPath(r.adjusted(0, 0, 4, 0), WIDGET_TAB_TOP, round, radius));
                        fillTab(painter, r.adjusted(1, sizeAdjust, 0, -(1+sizeAdjust)), option, fill, false, WIDGET_TAB_TOP, (docMode || onlyTab));
                        if(thin)
                            painter->setClipRect(r2.adjusted(0, 0, -1, 0));
                        else
                            painter->setClipping(false);
                        drawBorder(painter, r.adjusted(0, sizeAdjust, 4, -sizeAdjust), option, round, glowMo ? m_mouseOverCols : 0L,
                                   WIDGET_TAB_TOP, borderProfile, false);
                        if(thin)
                            painter->setClipping(false);
                        if(drawOuterGlow)
                            drawGlow(painter, r.adjusted(-1, 0, 5, 0), WIDGET_TAB_TOP);

                        if(selected)
                        {
                            if(!thin)
                            {
                                painter->setPen(use[0]);
                                if(!firstTab)
                                    painter->drawPoint(r2.right(), r2.top()-(TAB_MO_GLOW==opts.tabMouseOver ? 0 : 1));
                                painter->drawLine(r2.right(), r2.bottom()-1, r2.right(), r2.bottom());
                            }
                        }
                        else
                        {
                            int t(firstTab ? r2.top()+(opts.round>ROUND_SLIGHT && !(opts.square&SQUARE_TAB_FRAME)? 2 : 1) : r2.top()-1),
                                b(/*lastTab ? r2.bottom()-2 : */ r2.bottom()+1);

                            painter->setPen(use[QTC_STD_BORDER]);
                            painter->drawLine(r2.right()-1, t, r2.right()-1, b);
                            if(!thin)
                            {
                                painter->setPen(use[0]);
                                painter->drawLine(r2.right(), t, r2.right(), b);
                            }
                        }

                        if(selected)
                        {
                            if(opts.highlightTab)
                            {
                                QColor col(m_highlightCols[0]);
                                painter->setRenderHint(QPainter::Antialiasing, true);
                                painter->setPen(col);
                                drawAaLine(painter, r.left()+1, r.top()+highlightOffset, r.left()+1, r.bottom()-highlightOffset);
                                col.setAlphaF(0.5);
                                painter->setPen(col);
                                drawAaLine(painter, r.left()+2, r.top()+1, r.left()+2, r.bottom()-1);
                                painter->setRenderHint(QPainter::Antialiasing, false);
                                painter->setClipRect(QRect(r.x(), r.y(), highlightBorder, r.height()));
                                drawBorder(painter, r, option, ROUNDED_ALL, m_highlightCols, WIDGET_TAB_TOP, BORDER_FLAT, false, 3);
                            }

                            if(opts.colorSelTab)
                                colorTab(painter, r.adjusted(1, sizeAdjust, 0, -(1+sizeAdjust)), false, WIDGET_TAB_TOP, round);
                        }
                        else if(mouseOver && opts.coloredMouseOver && TAB_MO_GLOW!=opts.tabMouseOver)
                            drawHighlight(painter, QRect(r.x()+(TAB_MO_TOP==opts.tabMouseOver ? 0 : r.width()-1),
                                                         r.y()+(firstTab ? moOffset : 1),
                                                         2, r.height()-(firstTab || lastTab ? moOffset : 1)),
                                          false, TAB_MO_TOP==opts.tabMouseOver);
                        break;
                    }
                    case QTabBar::RoundedEast:
                    case QTabBar::TriangularEast:
                    {
                        int round=selected || oneTab || TAB_MO_GLOW==opts.tabMouseOver || opts.roundAllTabs
                                        ? ROUNDED_RIGHT
                                        : firstTab
                                            ? ROUNDED_TOPRIGHT
                                            : lastTab
                                                ? ROUNDED_BOTTOMRIGHT
                                                : ROUNDED_NONE;
                        if(!selected)
                            r.adjust(2, 0, -2, 0);

                        if(!firstTab)
                            r.adjust(0, -tabOverlap, 0, 0);
                        painter->setClipPath(buildPath(r.adjusted(-4, 0, 0, 0), WIDGET_TAB_BOT, round, radius));
                        fillTab(painter, r.adjusted(0, sizeAdjust, -1, -(1+sizeAdjust)), option, fill, false, WIDGET_TAB_BOT, (docMode || onlyTab));
                        if(thin)
                            painter->setClipRect(r2.adjusted(1, 0, 0, 0));
                        else
                            painter->setClipping(false);
                        drawBorder(painter, r.adjusted(-4, sizeAdjust, 0, -sizeAdjust), option, round, glowMo ? m_mouseOverCols : 0L,
                                   WIDGET_TAB_BOT, borderProfile, false);
                        if(thin)
                            painter->setClipping(false);
                        if(drawOuterGlow)
                            drawGlow(painter, r.adjusted(-5, 0, 1, 0), WIDGET_TAB_BOT);

                        if(selected)
                        {
                            if(!thin)
                            {
                                painter->setPen(use[opts.borderTab ? 0 : FRAME_DARK_SHADOW]);
                                if(!firstTab)
                                    painter->drawPoint(r2.left(), r2.top()-(TAB_MO_GLOW==opts.tabMouseOver ? 0 : 1));
                                painter->drawLine(r2.left(), r2.bottom()-(TAB_MO_GLOW==opts.tabMouseOver ? 0 : 1), r2.left(), r2.bottom());
                            }
                        }
                        else
                        {
                            int t(firstTab ? r2.top()+(opts.round>ROUND_SLIGHT && !(opts.square&SQUARE_TAB_FRAME)? 2 : 1) : r2.top()-1),
                                b(/*lastTab ? r2.bottom()-2 : */ r2.bottom()+1);

                            painter->setPen(use[QTC_STD_BORDER]);
                            painter->drawLine(r2.left()+1, t, r2.left()+1, b);
                            if(!thin)
                            {
                                painter->setPen(use[opts.borderTab ? 0 : FRAME_DARK_SHADOW]);
                                painter->drawLine(r2.left(), t, r2.left(), b);
                            }
                        }

                        if(selected)
                        {
                            if(opts.highlightTab)
                            {
                                QColor col(m_highlightCols[0]);
                                painter->setRenderHint(QPainter::Antialiasing, true);
                                painter->setPen(col);
                                drawAaLine(painter, r.right()-1, r.top()+highlightOffset, r.right()-1, r.bottom()-highlightOffset);
                                col.setAlphaF(0.5);
                                painter->setPen(col);
                                drawAaLine(painter, r.right()-2, r.top()+1, r.right()-2, r.bottom()-1);
                                painter->setRenderHint(QPainter::Antialiasing, false);
                                painter->setClipRect(QRect(r.x()+r.width()-highlightBorder, r.y(), r.x()+r.width()-1, r.height()));
                                drawBorder(painter, r, option, ROUNDED_ALL, m_highlightCols, WIDGET_TAB_TOP, BORDER_FLAT, false, 3);
                            }

                            if(opts.colorSelTab)
                                colorTab(painter, r.adjusted(0, sizeAdjust, -1, -(1+sizeAdjust)), false, WIDGET_TAB_BOT, round);
                        }
                        else if(mouseOver && opts.coloredMouseOver && TAB_MO_GLOW!=opts.tabMouseOver)
                            drawHighlight(painter, QRect(r.x()+(TAB_MO_TOP==opts.tabMouseOver ? r.width()-2 : -1),
                                                         r.y()+(firstTab ? moOffset : 1),
                                                         2, r.height()-(firstTab || lastTab ? moOffset : 1)),
                                          false, TAB_MO_TOP!=opts.tabMouseOver);
                        break;
                    }
                }
                painter->restore();
            }
            break;
        case CE_ScrollBarAddLine:
        case CE_ScrollBarSubLine:
        {
            QRect            br(r),
                             ar(r);
            const QColor     *use(state&State_Enabled ? m_buttonCols : m_backgroundCols); // buttonColors(option));
            bool             reverse(option && Qt::RightToLeft==option->direction);
            PrimitiveElement pe=state&State_Horizontal
                                 ? CE_ScrollBarAddLine==element ? (reverse ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight)
                                                                : (reverse ? PE_IndicatorArrowRight : PE_IndicatorArrowLeft)
                                 : CE_ScrollBarAddLine==element ? PE_IndicatorArrowDown : PE_IndicatorArrowUp;
            int              round=PE_IndicatorArrowRight==pe ? ROUNDED_RIGHT :
                                   PE_IndicatorArrowLeft==pe ? ROUNDED_LEFT :
                                   PE_IndicatorArrowDown==pe ? ROUNDED_BOTTOM :
                                   PE_IndicatorArrowUp==pe ? ROUNDED_TOP : ROUNDED_NONE;

            switch(opts.scrollbarType)
            {
            default:
            case SCROLLBAR_WINDOWS:
                break;
            case SCROLLBAR_KDE:
            case SCROLLBAR_PLATINUM:
                if (!reverse && PE_IndicatorArrowLeft == pe && r.x() > 3) {
                    round = ROUNDED_NONE;
                    br.adjust(0, 0, 1, 0);
                    if (opts.flatSbarButtons || !opts.vArrows) {
                        ar.adjust(1, 0, 1, 0);
                    }
                } else if(reverse && PE_IndicatorArrowRight == pe && r.x() > 3) {
                    if (SCROLLBAR_PLATINUM == opts.scrollbarType) {
                        round = ROUNDED_NONE;
                        br.adjust(-1, 0, 0, 0);
                        if (opts.flatSbarButtons || !opts.vArrows) {
                            ar.adjust(-1, 0, -1, 0);
                        }
                    } else {
                        if(r.x() < pixelMetric(PM_ScrollBarExtent, option, widget)+2)
                            round = ROUNDED_NONE;
                        br.adjust(0, 0, 1, 0);
                        if (opts.flatSbarButtons || !opts.vArrows) {
                            ar.adjust(1, 0, 1, 0);
                        }
                    }
                } else if (PE_IndicatorArrowUp == pe && r.y() > 3) {
                    round = ROUNDED_NONE;
                    br.adjust(0, 0, 0, 1);
                    if (opts.flatSbarButtons || !opts.vArrows) {
                            ar.adjust(0, 1, 0, 1);
                    }
                }
                break;
            case SCROLLBAR_NEXT:
                if (!reverse && PE_IndicatorArrowRight == pe) {
                    round = ROUNDED_NONE;
                    br.adjust(-1, 0, 0, 0);
                    if (opts.flatSbarButtons || !opts.vArrows) {
                        ar.adjust(-1, 0, 0, -1);
                    }
                } else if (reverse && PE_IndicatorArrowLeft == pe) {
                    round = ROUNDED_NONE;
                    br.adjust(0, 0, 1, 0);
                    if (opts.flatSbarButtons || !opts.vArrows) {
                        ar.adjust(-1, 0, 0, 1);
                    }
                } else if (PE_IndicatorArrowDown == pe) {
                    round = ROUNDED_NONE;
                    br.adjust(0, -1, 0, 0);
                    if (opts.flatSbarButtons || !opts.vArrows) {
                        ar.adjust(0, -1, 0, -1);
                    }
                }
                break;
            }

            painter->save();
            if(opts.flatSbarButtons && !qtcIsFlat(opts.sbarBgndAppearance) /*&& SCROLLBAR_NONE!=opts.scrollbarType*/)
                drawBevelGradientReal(palette.brush(QPalette::Background).color(), painter, r, state&State_Horizontal, false,
                                      opts.sbarBgndAppearance, WIDGET_SB_BGND);

            QStyleOption opt(*option);

            opt.state|=State_Raised;

            if (auto slider = styleOptCast<QStyleOptionSlider>(option)) {
                if((CE_ScrollBarSubLine==element && slider->sliderValue==slider->minimum) ||
                   (CE_ScrollBarAddLine==element && slider->sliderValue==slider->maximum))
                    opt.state&=~(State_MouseOver|State_Sunken|State_On);

                if(slider->minimum==slider->maximum && opt.state&State_Enabled)
                    opt.state^=State_Enabled;
            }

            if(opts.flatSbarButtons)
                opt.state&=~(State_Sunken|State_On);
            else
                drawLightBevel(painter, br, &opt, widget, round, getFill(&opt, use), use, true, WIDGET_SB_BUTTON);

            opt.rect = ar;

            if(!(opt.state&State_Enabled))
                opt.palette.setCurrentColorGroup(QPalette::Disabled);

            if(opt.palette.text().color()!=opt.palette.buttonText().color()) // The following fixes gwenviews scrollbars...
                opt.palette.setColor(QPalette::Text, opt.palette.buttonText().color());

            drawPrimitive(pe, &opt, painter, widget);
            painter->restore();
            break;
        }
        case CE_ScrollBarSubPage:
        case CE_ScrollBarAddPage:
        {
            const QColor *use(m_backgroundCols); // backgroundColors(option));
            int          borderAdjust(0);

            painter->save();
#ifndef SIMPLE_SCROLLBARS
            if (opts.round != ROUND_NONE && (SCROLLBAR_NONE==opts.scrollbarType || opts.flatSbarButtons))
                painter->fillRect(r, palette.background().color());
#endif

            switch(opts.scrollbarType)
            {
                case SCROLLBAR_KDE:
                case SCROLLBAR_WINDOWS:
                    borderAdjust=1;
                    break;
                case SCROLLBAR_PLATINUM:
                    if(CE_ScrollBarAddPage==element)
                        borderAdjust=1;
                    break;
                case SCROLLBAR_NEXT:
                    if(CE_ScrollBarSubPage==element)
                        borderAdjust=1;
                default:
                    break;
            }

            if(state&State_Horizontal)
            {
                if(qtcIsFlat(opts.appearance))
                    painter->fillRect(r.x(), r.y()+1, r.width(), r.height()-2, use[2]);
                else
                    drawBevelGradient(use[2], painter, QRect(r.x(), r.y()+1, r.width(), r.height()-2),
                                      true, false, opts.grooveAppearance, WIDGET_TROUGH);

#ifndef SIMPLE_SCROLLBARS
                if (opts.round != ROUND_NONE && (SCROLLBAR_NONE==opts.scrollbarType || opts.flatSbarButtons))
                {
                    if(CE_ScrollBarAddPage==element)
                        drawBorder(painter, r.adjusted(-5, 0, 0, 0), option, ROUNDED_RIGHT, use, WIDGET_TROUGH);
                    else
                        drawBorder(painter, r.adjusted(0, 0, 5, 0), option, ROUNDED_LEFT, use, WIDGET_TROUGH);
                }
                else
#endif
                    if(CE_ScrollBarAddPage==element)
                        drawBorder(painter, r.adjusted(-5, 0, borderAdjust, 0), option, ROUNDED_NONE, use, WIDGET_TROUGH);
                    else
                        drawBorder(painter, r.adjusted(-borderAdjust, 0, 5, 0), option, ROUNDED_NONE, use, WIDGET_TROUGH);
            }
            else
            {
                if(qtcIsFlat(opts.appearance))
                    painter->fillRect(r.x()+1, r.y(), r.width()-2, r.height(), use[2]);
                else
                    drawBevelGradient(use[2], painter, QRect(r.x()+1, r.y(), r.width()-2, r.height()),
                                      false, false, opts.grooveAppearance, WIDGET_TROUGH);

#ifndef SIMPLE_SCROLLBARS
                if (opts.round != ROUND_NONE && (SCROLLBAR_NONE==opts.scrollbarType || opts.flatSbarButtons))
                {
                    if(CE_ScrollBarAddPage==element)
                        drawBorder(painter, r.adjusted(0, -5, 0, 0), option, ROUNDED_BOTTOM, use, WIDGET_TROUGH);
                    else
                        drawBorder(painter, r.adjusted(0, 0, 0, 5), option, ROUNDED_TOP, use, WIDGET_TROUGH);
                }
                else
#endif
                    if(CE_ScrollBarAddPage==element)
                        drawBorder(painter, r.adjusted(0, -5, 0, borderAdjust), option, ROUNDED_NONE, use, WIDGET_TROUGH);
                    else
                        drawBorder(painter, r.adjusted(0, -borderAdjust, 0, 5), option, ROUNDED_NONE, use, WIDGET_TROUGH);
            }
            painter->restore();
            break;
        }
        case CE_ScrollBarSlider:
            painter->save();
            drawSbSliderHandle(painter, r, option);
            painter->restore();
            break;
#ifdef FIX_DISABLED_ICONS
        // Taken from QStyle - only required so that we can corectly set the disabled icon!!!
        case CE_ToolButtonLabel:
            if (auto tb = styleOptCast<QStyleOptionToolButton>(option)) {
                int shiftX = 0,
                    shiftY = 0;
                if (state & (State_Sunken|State_On))
                {
                    shiftX = pixelMetric(PM_ButtonShiftHorizontal, tb, widget);
                    shiftY = pixelMetric(PM_ButtonShiftVertical, tb, widget);
                }

                // Arrow type always overrules and is always shown
                bool hasArrow = tb->features & QStyleOptionToolButton::Arrow;

                if (((!hasArrow && tb->icon.isNull()) && !tb->text.isEmpty()) || Qt::ToolButtonTextOnly==tb->toolButtonStyle)
                {
                    int alignment = Qt::AlignCenter|Qt::TextShowMnemonic;

                    if (!styleHint(SH_UnderlineShortcut, option, widget))
                        alignment |= Qt::TextHideMnemonic;

                    r.translate(shiftX, shiftY);

                    drawItemTextWithRole(painter, r, alignment, palette, state&State_Enabled, tb->text, QPalette::ButtonText);
                }
                else
                {
                    QPixmap pm;
                    QSize   pmSize = tb->iconSize;
                    QRect   pr = r;

                    if (!tb->icon.isNull())
                    {
                        QIcon::State state = tb->state & State_On ? QIcon::On : QIcon::Off;
                        QIcon::Mode  mode=!(tb->state & State_Enabled)
                                            ? QIcon::Disabled
                                            : (state&State_MouseOver) && (state&State_AutoRaise)
                                                ? QIcon::Active
                                                : QIcon::Normal;
                        QSize        iconSize = tb->iconSize;

                        if (!iconSize.isValid())
                        {
                            int iconExtent = pixelMetric(PM_ToolBarIconSize);
                            iconSize = QSize(iconExtent, iconExtent);
                        }
                        /* Not required?
                        else if(iconSize.width()>iconSize.height())
                            iconSize.setWidth(iconSize.height());
                        else if(iconSize.width()<iconSize.height())
                            iconSize.setHeight(iconSize.width());
                        */

                        if(iconSize.width()>tb->rect.size().width())
                            iconSize=QSize(tb->rect.size().width(), tb->rect.size().width());
                        if(iconSize.height()>tb->rect.size().height())
                            iconSize=QSize(tb->rect.size().height(), tb->rect.size().height());

                        pm=getIconPixmap(tb->icon, iconSize, mode, state);
                        pmSize = pm.size(); // tb->icon.actualSize(iconSize, mode);
                        /*if(pmSize.width()<pm.width())
                            pr.setX(pr.x()+((pm.width()-pmSize.width())));
                        if(pmSize.height()<pm.height())
                            pr.setY(pr.y()+((pm.height()-pmSize.height())));
                        */
                    }

                    if (Qt::ToolButtonIconOnly!=tb->toolButtonStyle)
                    {
                        QRect tr = r;
                        int   alignment = Qt::TextShowMnemonic;

                        painter->setFont(tb->font);
                        if (!styleHint(SH_UnderlineShortcut, option, widget))
                            alignment |= Qt::TextHideMnemonic;

                        if (Qt::ToolButtonTextUnderIcon==tb->toolButtonStyle)
                        {
                            pr.setHeight(pmSize.height() + 6);

                            tr.adjust(0, pr.bottom()-3, 0, 0); // -3);
                            pr.translate(shiftX, shiftY);
                            if (hasArrow)
                                drawTbArrow(this, tb, pr, painter, widget);
                            else
                                drawItemPixmap(painter, pr, Qt::AlignCenter, pm);
                            alignment |= Qt::AlignCenter;
                        }
                        else
                        {
                            pr.setWidth(pmSize.width() + 8);
                            tr.adjust(pr.right(), 0, 0, 0);
                            pr.translate(shiftX, shiftY);
                            if (hasArrow)
                                drawTbArrow(this, tb, pr, painter, widget);
                            else
                                drawItemPixmap(painter, QStyle::visualRect(option->direction, r, pr), Qt::AlignCenter, pm);
                            alignment |= Qt::AlignLeft | Qt::AlignVCenter;
                        }
                        tr.translate(shiftX, shiftY);
                        drawItemTextWithRole(painter, QStyle::visualRect(option->direction, r, tr), alignment, palette,
                                             state & State_Enabled, tb->text, QPalette::ButtonText);
                    }
                    else
                    {
                        pr.translate(shiftX, shiftY);

                        if (hasArrow)
                            drawTbArrow(this, tb, pr, painter, widget);
                        else
                        {
                            if (!(tb->subControls&SC_ToolButtonMenu) && tb->features&QStyleOptionToolButton::HasMenu &&
                                pr.width()>pm.width() && ((pr.width()-pm.width())>LARGE_ARR_WIDTH))
                                pr.adjust(-LARGE_ARR_WIDTH, 0, 0, 0);
                            drawItemPixmap(painter, pr, Qt::AlignCenter, pm);
                        }
                    }
                }
            }
            break;
        case CE_RadioButtonLabel:
        case CE_CheckBoxLabel:
            if (auto btn = styleOptCast<QStyleOptionButton>(option)) {
                uint    alignment = visualAlignment(btn->direction, Qt::AlignLeft | Qt::AlignVCenter);
                QPixmap pix;
                QRect   textRect = r;

                if (!styleHint(SH_UnderlineShortcut, btn, widget))
                    alignment |= Qt::TextHideMnemonic;

                if (!btn->icon.isNull())
                {
                    pix = getIconPixmap(btn->icon, btn->iconSize, btn->state);
                    drawItemPixmap(painter, r, alignment, pix);
                    if (reverse)
                        textRect.setRight(textRect.right() - btn->iconSize.width() - 4);
                    else
                        textRect.setLeft(textRect.left() + btn->iconSize.width() + 4);
                }
                if (!btn->text.isEmpty())
                    drawItemTextWithRole(painter, textRect, alignment | Qt::TextShowMnemonic,
                                         palette, state&State_Enabled, btn->text, QPalette::WindowText);
            }
            break;
        case CE_ToolBoxTabLabel:
            if (auto tb = styleOptCast<QStyleOptionToolBox>(option)) {
                bool    enabled = state & State_Enabled,
                        selected = state & State_Selected;
                QPixmap pm = getIconPixmap(tb->icon, pixelMetric(QStyle::PM_SmallIconSize, tb, widget) ,state);
                QRect   cr = subElementRect(QStyle::SE_ToolBoxTabContents, tb, widget);
                QRect   tr, ir;
                int     ih = 0;

                if (pm.isNull())
                {
                    tr = cr;
                    tr.adjust(4, 0, -8, 0);
                }
                else
                {
                    int iw = pm.width() + 4;
                    ih = pm.height();
                    ir = QRect(cr.left() + 4, cr.top(), iw + 2, ih);
                    tr = QRect(ir.right(), cr.top(), cr.width() - ir.right() - 4, cr.height());
                }

                if (selected && styleHint(QStyle::SH_ToolBox_SelectedPageTitleBold, tb, widget))
                {
                    QFont f(painter->font());
                    f.setBold(true);
                    painter->setFont(f);
                }

                QString txt = tb->fontMetrics.elidedText(tb->text, Qt::ElideRight, tr.width());

                if (ih)
                    painter->drawPixmap(ir.left(), (tb->rect.height() - ih) / 2, pm);

                int alignment = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic;
                if (!styleHint(QStyle::SH_UnderlineShortcut, tb, widget))
                    alignment |= Qt::TextHideMnemonic;
                drawItemTextWithRole(painter, tr, alignment, tb->palette, enabled, txt, QPalette::ButtonText);

                if (!txt.isEmpty() && state&State_HasFocus)
                {
                    QStyleOptionFocusRect opt;
                    opt.rect = tr;
                    opt.palette = palette;
                    opt.state = QStyle::State_None;
                    drawPrimitive(PE_FrameFocusRect, &opt, painter, widget);
                }
            }
            break;
#endif
        case CE_RadioButton:
        case CE_CheckBox:
            if (opts.crHighlight && (r.width()>opts.crSize*2))
                if (auto button = styleOptCast<QStyleOptionButton>(option)) {
                    QStyleOptionButton copy(*button);

                    copy.rect.adjust(2, 0, -2, 0);

                    if(button->state&State_MouseOver && button->state&State_Enabled)
                    {
                        QRect highlightRect(subElementRect(CE_RadioButton==element ? SE_RadioButtonFocusRect : SE_CheckBoxFocusRect,
                                                           option, widget));

                        if(Qt::RightToLeft==button->direction)
                            highlightRect.setRight(r.right());
                        else
                            highlightRect.setX(r.x());
                        highlightRect.setWidth(highlightRect.width()+1);

                        if(ROUND_NONE!=opts.round)
                        {
                            painter->save();
                            painter->setRenderHint(QPainter::Antialiasing, true);
                            double   radius(qtcGetRadius(&opts, highlightRect.width(), highlightRect.height(),
                                                      WIDGET_OTHER, RADIUS_SELECTION));

                            drawBevelGradient(shade(palette.background().color(), TO_FACTOR(opts.crHighlight)),
                                              painter, highlightRect,
                                              buildPath(QRectF(highlightRect), WIDGET_OTHER, ROUNDED_ALL, radius), true,
                                              false, opts.selectionAppearance, WIDGET_SELECTION, false);
                            painter->restore();
                        }
                        else
                            drawBevelGradient(shade(palette.background().color(), TO_FACTOR(opts.crHighlight)), painter,
                                              highlightRect, true, false, opts.selectionAppearance, WIDGET_SELECTION);
                    }
                    BaseStyle::drawControl(element, &copy, painter, widget);
                    break;
                }
            // Fall through!
        default:
            BaseStyle::drawControl(element, option, painter, widget);
    }
}

void
Style::drawComplexControl(ComplexControl control,
                          const QStyleOptionComplex *option,
                          QPainter *painter, const QWidget *widget) const
{
    prePolish(widget);
    QRect r(option->rect);
    const QFlags<State> &state(option->state);
    const QPalette &palette(option->palette);
    bool reverse = option->direction == Qt::RightToLeft;

    switch (control) {
        case CC_Dial:
            if (auto slider = styleOptCast<QStyleOptionSlider>(option)) {
                r.adjust(1, 1, -1, -1);

                QStyleOptionComplex opt(*option);
                bool mo(state&State_Enabled && state&State_MouseOver);
                QRect outer(r);
                int sliderWidth = /*qMin(2*r.width()/5, */CIRCULAR_SLIDER_SIZE/*)*/;
#ifdef DIAL_DOT_ON_RING
                int halfWidth = sliderWidth / 2;
#endif

                opt.state|=State_Horizontal;

                // Outer circle...
                if (outer.width() > outer.height())
                {
                    outer.setLeft(outer.x()+(outer.width()-outer.height())/2);
                    outer.setWidth(outer.height());
                }
                else
                {
                    outer.setTop(outer.y()+(outer.height()-outer.width())/2);
                    outer.setHeight(outer.width());
                }

                opt.state&=~State_MouseOver;
#ifdef DIAL_DOT_ON_RING
                opt.rect=outer.adjusted(halfWidth, halfWidth, -halfWidth, -halfWidth);
#else
                opt.rect=outer;
#endif
                drawLightBevel(painter, opt.rect, &opt, widget, ROUNDED_ALL,
                               getFill(&opt, m_backgroundCols), m_backgroundCols,
                               true, WIDGET_DIAL);

                // Inner 'dot'
                if(mo)
                    opt.state|=State_MouseOver;

                // angle calculation from qcommonstyle.cpp (c)
                // Trolltech 1992-2007, ASA.
                qreal angle(0);
                if (slider->maximum == slider->minimum) {
                    angle = M_PI / 2;
                } else {
                    const qreal fraction(qreal(slider->sliderValue - slider->minimum)/
                                         qreal(slider->maximum - slider->minimum));
                    if(slider->dialWrapping)
                        angle = 1.5*M_PI - fraction*2*M_PI;
                    else
                        angle = (M_PI*8 - fraction*10*M_PI)/6;
                }

                QPoint center = outer.center();
#ifdef DIAL_DOT_ON_RING
                const qreal radius=0.5*(outer.width() - sliderWidth);
#else
                const qreal radius=0.5*(outer.width() - 2*sliderWidth);
#endif
                center += QPoint(radius*cos(angle), -radius*sin(angle));

                opt.rect=QRect(outer.x(), outer.y(), sliderWidth, sliderWidth);
                opt.rect.moveCenter(center);

                const QColor *use(buttonColors(option));

                drawLightBevel(painter, opt.rect, &opt, widget, ROUNDED_ALL,
                               getFill(&opt, use), use, true, WIDGET_RADIO_BUTTON);

                // Draw value...
#ifdef DIAL_DOT_ON_RING
                drawItemTextWithRole(painter, outer.adjusted(sliderWidth, sliderWidth, -sliderWidth, -sliderWidth),
                                     Qt::AlignCenter, palette, state&State_Enabled,
                                     QString::number(slider->sliderValue), QPalette::ButtonText);
#else
                int adjust=2*sliderWidth;
                drawItemTextWithRole(painter, outer.adjusted(adjust, adjust, -adjust, -adjust),
                                     Qt::AlignCenter, palette, state&State_Enabled,
                                     QString::number(slider->sliderValue), QPalette::ButtonText);
#endif

                if(state&State_HasFocus)
                {
                    QStyleOptionFocusRect fr;
                    fr.rect = outer.adjusted(-1, -1, 1, 1);
                    drawPrimitive(PE_FrameFocusRect, &fr, painter, widget);
                }
            }
            break;
        case CC_ToolButton:
            // For OO.o 3.2 need to fill widget background!
            if (isOOWidget(widget))
                painter->fillRect(r, palette.brush(QPalette::Window));
            if (auto toolbutton =
                styleOptCast<QStyleOptionToolButton>(option)) {
                int widthAdjust = 0;
                int heightAdjust = 0;

                if (widget) {
                    if ((opts.dwtSettings & DWT_BUTTONS_AS_PER_TITLEBAR) &&
                        (widget->inherits("QDockWidgetTitleButton") ||
                         qtcCheckType(getParent(widget),
                                      "KoDockWidgetTitleBar"))) {
                        ETitleBarButtons btn = TITLEBAR_CLOSE;
                        Icon icon = ICN_CLOSE;

                        if(constDwtFloat == widget->objectName())
                            btn=TITLEBAR_MAX, icon=ICN_RESTORE;
                        else if (constDwtClose != widget->objectName() &&
                                 qtcCheckType<QDockWidget>(
                                     getParent<2>(widget)) &&
                                 qtcCheckType(widget->parentWidget(),
                                              "KoDockWidgetTitleBar")) {
                            QDockWidget *dw =
                                (QDockWidget*)getParent<2>(widget);
                            QWidget *koDw = widget->parentWidget();
                            int fw =
                                (dw->isFloating() ?
                                 pixelMetric(QStyle::PM_DockWidgetFrameWidth,
                                             0, dw) : 0);
                            QRect geom(widget->geometry());
                            QStyleOptionDockWidgetV2 dwOpt;
                            dwOpt.initFrom(dw);
                            dwOpt.rect = QRect(QPoint(fw, fw), QSize(koDw->geometry().width() - (fw * 2),
                                                                    koDw->geometry().height() - (fw * 2)));
                            dwOpt.title = dw->windowTitle();
                            dwOpt.closable = (dw->features()&QDockWidget::DockWidgetClosable)==QDockWidget::DockWidgetClosable;
                            dwOpt.floatable =
                                (dw->features() &
                                 QDockWidget::DockWidgetFloatable) ==
                                QDockWidget::DockWidgetFloatable;
                            if (dwOpt.closable &&
                                subElementRect(QStyle::SE_DockWidgetCloseButton,
                                               &dwOpt, getParent<2>(widget)) == geom) {
                                btn = TITLEBAR_CLOSE;
                                icon = ICN_CLOSE;
                            } else if (dwOpt.floatable &&
                                       subElementRect(QStyle::SE_DockWidgetFloatButton, &dwOpt,
                                                      getParent<2>(widget)) == geom)
                                btn=TITLEBAR_MAX, icon=ICN_RESTORE;
                            else
                                btn=TITLEBAR_SHADE, icon=dw && dw->widget() && dw->widget()->isVisible()
                                        ? ICN_SHADE
                                        : ICN_UNSHADE;
                        }

                        QColor        shadow(WINDOW_SHADOW_COLOR(opts.titlebarEffect));
                        const QColor *bgndCols((opts.dwtSettings&DWT_COLOR_AS_PER_TITLEBAR)
                                                ? getMdiColors(option, state&State_Active)
                                                : buttonColors(option)),
                                     *btnCols((opts.dwtSettings&DWT_COLOR_AS_PER_TITLEBAR)
                                                ? opts.titlebarButtons&TITLEBAR_BUTTON_STD_COLOR
                                                    ? buttonColors(option)
                                                    : getMdiColors(option, state&State_Active)
                                                : bgndCols);

                        drawDwtControl(painter, state, r.adjusted(-1, -1, 1, 1), btn, icon, option->palette.color(QPalette::WindowText), btnCols,
                                       bgndCols);
                        break;
                    }
                    if (qobject_cast<QTabBar*>(widget->parentWidget())) {
                        QStyleOptionToolButton btn(*toolbutton);

                        if (oneOf(toolbutton->arrowType, Qt::LeftArrow,
                                  Qt::RightArrow)) {
                            btn.rect.adjust(0, 4, 0, -4);
                        } else {
                            btn.rect.adjust(4, 0, -4, 0);
                        }
                        if(!(btn.state&State_Enabled))
                            btn.state&=~State_MouseOver;
                        drawPrimitive(PE_PanelButtonTool, &btn, painter, widget);
                        if (opts.vArrows) {
                            switch (toolbutton->arrowType) {
                            case Qt::LeftArrow:
                                btn.rect.adjust(-1, 0, -1, 0);
                                break;
                            case Qt::RightArrow:
                                btn.rect.adjust(1, 0, 1, 0);
                                break;
                            case Qt::UpArrow:
                                btn.rect.adjust(0, -1, 0, -1);
                                break;
                            case Qt::DownArrow:
                                btn.rect.adjust(0, 1, 0, 1);
                            default:
                                break;
                            }
                        }
                        drawTbArrow(this, &btn, btn.rect, painter, widget);
                        break;
                    }

                    const QToolButton *btn = qobject_cast<const QToolButton*>(widget);

                    if(btn && btn->isDown() && Qt::ToolButtonTextBesideIcon==btn->toolButtonStyle() &&
                       widget->parentWidget() && qobject_cast<QMenu*>(widget->parentWidget()))
                    {
                        painter->save();
                        if(opts.menuStripe)
                        {
                            int stripeWidth(qMax(20, constMenuPixmapWidth));

                            drawBevelGradient(menuStripeCol(),
                                              painter, QRect(reverse ? r.right()-stripeWidth : r.x(), r.y(),
                                                             stripeWidth, r.height()), false,
                                              false, opts.menuStripeAppearance, WIDGET_OTHER);
                        }

#if 0
                        // For some reason the MenuTitle has a larger border on the left, so adjust the width by 1 pixel to make this look nicer.
                        //drawBorder(painter, r.adjusted(2, 2, -3, -2), option, ROUNDED_ALL, 0L, WIDGET_OTHER, BORDER_SUNKEN);
                        QStyleOptionToolButton opt(*toolbutton);
                        opt.rect = r.adjusted(2, 2, -3, -2);
                        opt.state=State_Raised|State_Enabled|State_Horizontal;
                        drawLightBevel(painter, opt.rect, &opt, widget, ROUNDED_ALL,
                                       getFill(&opt, m_backgroundCols), m_backgroundCols, true, WIDGET_NO_ETCH_BTN);
#else
                        if(!opts.menuStripe)
                            drawFadedLine(painter, QRect(r.x()+3, r.y()+r.height()-1, r.width()-7, 1),
                                          popupMenuCols(option)[MENU_SEP_SHADE], true, true, true);
#endif
                        QFont font(toolbutton->font);

                        font.setBold(true);
                        painter->setFont(font);
                        drawItemTextWithRole(painter, r, Qt::AlignHCenter | Qt::AlignVCenter,
                                             palette, state&State_Enabled, toolbutton->text, QPalette::Text);
                        painter->restore();
                        break;
                    }

                    // Amarok's toolbars (the one just above the collection list) are much thinner then normal,
                    // and QToolBarExtension does not seem to take this into account - so adjust the size here...
                    if(widget->inherits("QToolBarExtension") && widget->parentWidget())
                    {
                        if(r.height()>widget->parentWidget()->rect().height())
                            heightAdjust=(r.height()-widget->parentWidget()->rect().height())+2;
                        if(r.width()>widget->parentWidget()->rect().width())
                            widthAdjust=(r.width()-widget->parentWidget()->rect().width())+2;
                    }
                }
                QRect button(subControlRect(control, toolbutton, SC_ToolButton, widget)),
                      menuarea(subControlRect(control, toolbutton, SC_ToolButtonMenu, widget));
                State bflags(toolbutton->state);
                bool  etched = opts.buttonEffect != EFFECT_NONE,
                      raised=widget && (TBTN_RAISED==opts.tbarBtns || TBTN_JOINED==opts.tbarBtns),
                      horizTBar(true);
                int   round=ROUNDED_ALL,
                      leftAdjust(0), topAdjust(0), rightAdjust(0), bottomAdjust(0);

                if(raised)
                {
                    const QToolBar *toolbar=getToolBar(widget);

                    if(toolbar)
                    {
                        if(TBTN_JOINED==opts.tbarBtns)
                        {
                            horizTBar=Qt::Horizontal==toolbar->orientation();
                            adjustToolbarButtons(widget, toolbar, leftAdjust, topAdjust, rightAdjust, bottomAdjust, round);
                        }
                    }
                    else
                        raised=false;
                }

                if (!(bflags&State_Enabled))
                    bflags &= ~(State_MouseOver/* | State_Raised*/);

                if(bflags&State_MouseOver)
                    bflags |= State_Raised;
                else if(!raised && (bflags&State_AutoRaise))
                    bflags &= ~State_Raised;

                if(state&State_AutoRaise || toolbutton->subControls&SC_ToolButtonMenu)
                    bflags|=STATE_TBAR_BUTTON;

                State mflags(bflags);

                if (!isOOWidget(widget)) {
                    if (state & State_Sunken &&
                        !(toolbutton->activeSubControls & SC_ToolButton)) {
                        bflags &= ~State_Sunken;
                    }
                }

                bool         drawMenu=TBTN_JOINED==opts.tbarBtns
                                        ? mflags & (State_Sunken | State_On)
                                        : raised || (mflags & (State_Sunken | State_On | State_Raised)),
                             drawnBevel=false;
                QStyleOption tool(0);
                tool.palette = toolbutton->palette;

                if (raised ||
                     (toolbutton->subControls&SC_ToolButton && (bflags & (State_Sunken | State_On | State_Raised))) ||
                     (toolbutton->subControls&SC_ToolButtonMenu && drawMenu))
                {
                    const QColor *use(buttonColors(toolbutton));

                    tool.rect = (toolbutton->subControls&SC_ToolButtonMenu ? button.united(menuarea) : button)
                                .adjusted(leftAdjust, topAdjust, rightAdjust, bottomAdjust);
                    tool.state = bflags|State_Horizontal;

                    if(raised && TBTN_JOINED==opts.tbarBtns && !horizTBar)
                        tool.state &= ~State_Horizontal;

                    tool.rect.adjust(0, 0, -widthAdjust, -heightAdjust);
                    if(!(bflags&State_Sunken) && (mflags&State_Sunken))
                        tool.state &= ~State_MouseOver;
                    drawnBevel=true;
                    drawLightBevel(painter, tool.rect, &tool, widget, round, getFill(&tool, use), use, true, WIDGET_TOOLBAR_BUTTON);

                    if(raised && TBTN_JOINED==opts.tbarBtns)
                    {
                        const int constSpace=4;

                        QRect br(tool.rect.adjusted(-leftAdjust, -topAdjust, -rightAdjust, -bottomAdjust));

                        if(leftAdjust)
                            drawFadedLine(painter, QRect(br.x(), br.y()+constSpace, 1, br.height()-(constSpace*2)), use[0], true, true, false);
                        if(topAdjust)
                            drawFadedLine(painter, QRect(br.x()+constSpace, br.y(), br.width()-(constSpace*2), 1), use[0], true, true, true);
                        if(rightAdjust)
                            drawFadedLine(painter, QRect(br.x()+br.width()-1, br.y()+constSpace, 1, br.height()-(constSpace*2)),
                                          use[QTC_STD_BORDER], true, true, false);
                        if(bottomAdjust)
                            drawFadedLine(painter, QRect(br.x()+constSpace, br.y()+br.height()-1, br.width()-(constSpace*2), 1),
                                          use[QTC_STD_BORDER], true, true, true);
                    }
                }

                if (toolbutton->subControls&SC_ToolButtonMenu)
                {
                    if(etched)
                    {
                        if(reverse)
                            menuarea.adjust(1, 1, 0, -1);
                        else
                            menuarea.adjust(0, 1, -1, -1);
                    }

                    tool.state = mflags|State_Horizontal;

                    if(drawMenu)
                    {
                        const QColor *use(buttonColors(option));
                        int          mRound=reverse ? ROUNDED_LEFT : ROUNDED_RIGHT;

                        if(mflags&State_Sunken)
                            tool.state&=~State_MouseOver;

                        if(raised && TBTN_JOINED==opts.tbarBtns)
                        {
                            if(!horizTBar)
                                tool.state &= ~State_Horizontal;
                            painter->save();
                            painter->setClipRect(menuarea, Qt::IntersectClip);
                            if((reverse && leftAdjust) || (!reverse && rightAdjust))
                                mRound=ROUNDED_NONE;
                            if(reverse)
                                tool.rect.adjust(1, 0, 0, 0);
                            else
                                tool.rect.adjust(0, 0, -1, 0);
                        }
                        else
                            tool.rect = menuarea;

                        drawLightBevel(painter, tool.rect, &tool, widget, mRound, getFill(&tool, use), use, true,
                                       MO_GLOW==opts.coloredMouseOver ? WIDGET_MENU_BUTTON : WIDGET_NO_ETCH_BTN);
                        if(raised && TBTN_JOINED==opts.tbarBtns)
                            painter->restore();
                    }

                    tool.rect = menuarea;

                    if(mflags&State_Sunken)
                        tool.rect.adjust(1, 1, 1, 1);
                    drawArrow(painter, tool.rect, PE_IndicatorArrowDown,
                              MOArrow(state, palette,
                                      toolbutton->activeSubControls &
                                      SC_ToolButtonMenu,
                                      QPalette::ButtonText));
                }

                if ((FOCUS_GLOW!=opts.focus || !drawnBevel) && toolbutton->state&State_HasFocus)
                {
                    QStyleOptionFocusRect fr;

                    fr.QStyleOption::operator=(*toolbutton);
                    if (oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED)) {
                        if (etched) {
                            fr.rect.adjust(1, 1, -1, -1);
                        }
                    } else {
                        if (opts.focus == FOCUS_GLOW) {
                            fr.rect.adjust(1, 1, -1, -1);
                        } else if (etched) {
                            fr.rect.adjust(4, 4, -4, -4);
                        } else {
                            fr.rect.adjust(3, 3, -3, -3);
                        }
                        if (toolbutton->features &
                            QStyleOptionToolButton::MenuButtonPopup) {
                            fr.rect.adjust(
                                0, 0, -(pixelMetric(
                                            QStyle::PM_MenuButtonIndicator,
                                            toolbutton, widget) - 1), 0);
                        }
                    }
                    if (!(state & State_MouseOver &&
                          oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED) &&
                          opts.coloredMouseOver != MO_NONE)) {
                        drawPrimitive(PE_FrameFocusRect, &fr, painter, widget);
                    }
                }
                QStyleOptionToolButton label = *toolbutton;
                int fw = pixelMetric(PM_DefaultFrameWidth, option, widget);
                label.rect = button.adjusted(fw, fw, -(fw+widthAdjust), -(fw+heightAdjust));
                label.state = bflags;
                drawControl(CE_ToolButtonLabel, &label, painter, widget);

                if (!(toolbutton->subControls&SC_ToolButtonMenu) &&
                     (toolbutton->features&QStyleOptionToolButton::HasMenu))
                {
                    QRect arrow(r.right()-(LARGE_ARR_WIDTH+(etched ? 3 : 2)),
                                r.bottom()-(LARGE_ARR_HEIGHT+(etched ? 4 : 3)),
                                LARGE_ARR_WIDTH, LARGE_ARR_HEIGHT);

                    if(bflags&State_Sunken)
                        arrow.adjust(1, 1, 1, 1);

                    drawArrow(painter, arrow, PE_IndicatorArrowDown,
                              MOArrow(state, palette, QPalette::ButtonText));
                }
            }
            break;
        case CC_GroupBox:
            if (auto groupBox = styleOptCast<QStyleOptionGroupBox>(option)) {
                // Draw frame
                QRect textRect = subControlRect(CC_GroupBox, option,
                                                SC_GroupBoxLabel, widget);
                QRect checkBoxRect = subControlRect(CC_GroupBox, option,
                                                    SC_GroupBoxCheckBox, widget);
                if (groupBox->subControls & QStyle::SC_GroupBoxFrame) {
                    QStyleOptionFrameV2 frame;
                    frame.QStyleOption::operator=(*groupBox);
                    frame.features = groupBox->features;
                    frame.lineWidth = groupBox->lineWidth;
                    frame.midLineWidth = groupBox->midLineWidth;
                    frame.rect = subControlRect(CC_GroupBox, option,
                                                SC_GroupBoxFrame, widget);

                    if ((groupBox->features & QStyleOptionFrameV2::Flat) ||
                        !(opts.gbLabel & (GB_LBL_INSIDE | GB_LBL_OUTSIDE))) {
                        painter->save();
                        QRegion region(r);
                        if (!groupBox->text.isEmpty())
                            region -= QRect(groupBox->subControls &
                                            QStyle::SC_GroupBoxCheckBox ?
                                            checkBoxRect.united(textRect)
                                            .adjusted(reverse ? 0 : -2, 0,
                                                      reverse ? 2 : 0, 0) :
                                            textRect);
                        painter->setClipRegion(region);
                    }
                    drawPrimitive(PE_FrameGroupBox, &frame, painter, widget);
                    if ((groupBox->features & QStyleOptionFrameV2::Flat) ||
                        !(opts.gbLabel & (GB_LBL_INSIDE | GB_LBL_OUTSIDE))) {
                        painter->restore();
                    }
                }

                // Draw title
                if ((groupBox->subControls & QStyle::SC_GroupBoxLabel) &&
                    !groupBox->text.isEmpty()) {
                    QColor textColor = groupBox->textColor;
                    if (textColor.isValid())
                        painter->setPen(textColor);
                    int alignment = int(groupBox->textAlignment);
                    if (!styleHint(QStyle::SH_UnderlineShortcut, option, widget))
                        alignment |= Qt::TextHideMnemonic;

                    if (opts.gbLabel & GB_LBL_BOLD) {
                        QFont font(painter->font());

                        font.setBold(true);
                        painter->save();
                        painter->setFont(font);
                    }
                    drawItemText(painter, textRect,
                                 Qt::TextShowMnemonic | Qt::AlignHCenter |
                                 alignment, palette, state & State_Enabled,
                                 groupBox->text, textColor.isValid() ?
                                 QPalette::NoRole : QPalette::WindowText);

                    if (opts.gbLabel & GB_LBL_BOLD)
                        painter->restore();

                    if (state & State_HasFocus) {
                        QStyleOptionFocusRect fropt;
                        fropt.QStyleOption::operator=(*groupBox);
                        fropt.rect = textRect;
                        drawPrimitive(PE_FrameFocusRect, &fropt,
                                      painter, widget);
                    }
                }

                // Draw checkbox
                if (groupBox->subControls & SC_GroupBoxCheckBox) {
                    QStyleOptionButton box;
                    box.QStyleOption::operator=(*groupBox);
                    box.rect = checkBoxRect;
                    drawPrimitive(PE_IndicatorCheckBox, &box, painter, widget);
                }
            }
            break;
        case CC_Q3ListView:
            if (auto lv = styleOptCast<QStyleOptionQ3ListView>(option)) {
                int i;
                if (lv->subControls&SC_Q3ListView)
                    BaseStyle::drawComplexControl(control, lv, painter, widget);
                if (lv->subControls & (SC_Q3ListViewBranch | SC_Q3ListViewExpand))
                {
                    if (lv->items.isEmpty())
                        break;

                    QStyleOptionQ3ListViewItem item(lv->items.at(0));
                    int                        y(r.y()),
                                               c;
                    QPolygon                   lines;

                    painter->save();
                    painter->setRenderHint(QPainter::Antialiasing, false);
                    if ((lv->activeSubControls&SC_All) && (lv->subControls&SC_Q3ListViewExpand))
                    {
                        c = 2;
                        if(opts.lvLines)
                        {
                            lines.resize(2);
                            lines[0] = QPoint(r.right(), r.top());
                            lines[1] = QPoint(r.right(), r.bottom());
                        }
                    }
                    else
                    {
                        int linetop(0),
                            linebot(0);
                        // each branch needs at most two lines, ie. four end points
                        lines.resize(item.childCount * 4);
                        c = 0;

                        // skip the stuff above the exposed rectangle
                        for (i = 1;i < lv->items.size();++i) {
                            QStyleOptionQ3ListViewItem child = lv->items.at(i);
                            if (child.height + y > 0)
                                break;
                            y += child.totalHeight;
                        }
                        int bx(r.width() / 2);

                        // paint stuff in the magical area
                        while (i < lv->items.size() && y < r.height())
                        {
                            QStyleOptionQ3ListViewItem child = lv->items.at(i);
                            if (child.features & QStyleOptionQ3ListViewItem::Visible)
                            {
                                int lh(!(item.features & QStyleOptionQ3ListViewItem::MultiLine)
                                        ? child.height
                                        : painter->fontMetrics().height() + 2 * lv->itemMargin);

                                lh = qMax(lh, QApplication::globalStrut().height());
                                if (lh % 2 > 0)
                                    ++lh;
                                linebot = y + lh / 2;
                                if (child.features & QStyleOptionQ3ListViewItem::Expandable
                                    || (child.childCount > 0 && child.height > 0))
                                {

                                    QRect ar(bx-4, linebot-4, 11, 11);

#if 0
                                    if(LV_OLD==opts.lvLines)
                                    {
                                        int lo(opts.round != ROUND_NONE ? 2 : 0);

                                        painter->setPen(palette.mid().color());
                                        painter->drawLine(ar.x()+lo, ar.y(), (ar.x()+ar.width()-1)-lo, ar.y());
                                        painter->drawLine(ar.x()+lo, ar.y()+ar.height()-1, (ar.x()+ar.width()-1)-lo, ar.y()+ar.height()-1);
                                        painter->drawLine(ar.x(), ar.y()+lo, ar.x(), (ar.y()+ar.height()-1)-lo);
                                        painter->drawLine(ar.x()+ar.width()-1, ar.y()+lo, ar.x()+ar.width()-1, (ar.y()+ar.height()-1)-lo);

                                        if (opts.round != ROUND_NONE) {
                                            painter->drawPoint(ar.x()+1, ar.y()+1);
                                            painter->drawPoint(ar.x()+1, ar.y()+ar.height()-2);
                                            painter->drawPoint(ar.x()+ar.width()-2, ar.y()+1);
                                            painter->drawPoint(ar.x()+ar.width()-2, ar.y()+ar.height()-2);

                                            QColor col(palette.mid().color());

                                            col.setAlphaF(0.5);
                                            painter->setPen(col);
                                            painter->drawLine(ar.x()+1, ar.y()+1, ar.x()+2, ar.y());
                                            painter->drawLine(ar.x()+ar.width()-2, ar.y(), ar.x()+ar.width()-1, ar.y()+1);
                                            painter->drawLine(ar.x()+1, ar.y()+ar.height()-2, ar.x()+2, ar.y()+ar.height()-1);
                                            painter->drawLine(ar.x()+ar.width()-2, ar.y()+ar.height()-1, ar.x()+ar.width()-1, ar.y()+ar.height()-2);
                                        }
                                    }
#endif

                                    drawArrow(painter, ar,
                                              child.state&State_Open
                                                ? PE_IndicatorArrowDown
                                                : reverse
                                                    ? PE_IndicatorArrowLeft
                                                    : PE_IndicatorArrowRight,
                                              palette.text().color());

                                    if(opts.lvLines)
                                    {
                                        lines[c++] = QPoint(bx+1, linetop);
                                        lines[c++] = QPoint(bx+1, linebot - 4);
                                        lines[c++] = QPoint(bx + 6, linebot);
                                        lines[c++] = QPoint(r.width(), linebot);
                                        linetop = linebot + 6;
                                    }
                                }
                                else if(opts.lvLines)
                                {
                                    // just dotlinery
                                    lines[c++] = QPoint(bx+1, linebot -1);
                                    lines[c++] = QPoint(r.width(), linebot -1);
                                }
                                y += child.totalHeight;
                            }
                            ++i;
                        }

                        if(opts.lvLines)
                        {
                            // Expand line height to edge of rectangle if there's any
                            // visible child below
                            while (i < lv->items.size() && lv->items.at(i).height <= 0)
                                ++i;

                            if (i < lv->items.size())
                                linebot = r.height();

                            if (linetop < linebot)
                            {
                                lines[c++] = QPoint(bx+1, linetop);
                                lines[c++] = QPoint(bx+1, linebot-1);
                            }
                        }
                    }

                    if (opts.lvLines && (lv->subControls&SC_Q3ListViewBranch))
                    {
                        painter->setPen(palette.mid().color());

                        for(int line = 0; line < c; line += 2)
                            if (lines[line].y() == lines[line+1].y())
                                painter->drawLine(lines[line].x(), lines[line].y(), lines[line + 1].x(), lines[line].y());
                            else
                                painter->drawLine(lines[line].x(), lines[line].y(), lines[line].x(), lines[line + 1].y());
                    }
                    painter->restore();
                }
            }
            break;
        case CC_SpinBox:
            if (auto spinBox = styleOptCast<QStyleOptionSpinBox>(option)) {
                QRect frame(subControlRect(CC_SpinBox, option, SC_SpinBoxFrame, widget)),
                      up(subControlRect(CC_SpinBox, option, SC_SpinBoxUp, widget)),
                      down(subControlRect(CC_SpinBox, option, SC_SpinBoxDown, widget)),
                      all(frame.united(up).united(down));
                bool  doFrame(spinBox->frame && frame.isValid()),
                      sunken(state&State_Sunken),
                      enabled(state&State_Enabled),
                      mouseOver(state&State_MouseOver),
                      upIsActive(SC_SpinBoxUp==spinBox->activeSubControls),
                      downIsActive(SC_SpinBoxDown==spinBox->activeSubControls),
                      doEtch(opts.buttonEffect != EFFECT_NONE &&
                             opts.etchEntry),
                      isOO(isOOWidget(widget)),
                      oldUnify=opts.unifySpin; // See Krita note below...

                if(!doFrame && isOO && !opts.unifySpin)
                {
                    doFrame=true;
                    frame=all;
                }

                if(isOO)
                    painter->fillRect(r, palette.brush(QPalette::Window));

                if(up.isValid())
                {
                    if(reverse)
                        frame.adjust(up.width(), 0, 0, 0);
                    else
                        frame.adjust(0, 0, -up.width(), 0);
                }

                if(doEtch)
                {
                    drawEtch(painter, all, widget, WIDGET_SPIN, false,
                             opts.square&SQUARE_ENTRY
                                ? opts.unifySpin
                                    ? ROUNDED_NONE
                                    : reverse
                                        ? ROUNDED_LEFT
                                        : ROUNDED_RIGHT
                                : ROUNDED_ALL);
                    down.adjust(reverse ? 1 : 0, 0, reverse ? 0 : -1, -1);
                    up.adjust(reverse ? 1 : 0, 1, reverse ? 0 : -1, 0);
                    frame.adjust(reverse ? 0 : 1, 1, reverse ? -1 : 0, -1);
                    all.adjust(1, 1, -1, -1);
                }

                // Krita/KOffice uses a progressbar with spin buttons at the end
                // ...when drawn, the frame part is not set - so in this case dont draw the background behind the buttons!
                if(!isOO && !doFrame)
                    opts.unifySpin=true; // So, set this to true to fake the above scenario!
                else
                    if(opts.unifySpin)
                        drawEntryField(painter, all, widget, option, ROUNDED_ALL, true, false);
                    else
                    {
                        if(opts.unifySpinBtns)
                        {
                            QRect btns=up.united(down);
                            const QColor *use(buttonColors(option));
                            QStyleOption opt(*option);

                            opt.state&=~(State_Sunken|State_MouseOver);
                            opt.state|=State_Horizontal;

                            drawLightBevel(painter, btns, &opt, widget, reverse ?  ROUNDED_LEFT : ROUNDED_RIGHT,
                                        getFill(&opt, use), use, true, WIDGET_SPIN);

                            if(state&State_MouseOver && state&State_Enabled && !(state&State_Sunken))
                            {
                                opt.state|=State_MouseOver;
                                painter->save();
                                painter->setClipRect(upIsActive ? up : down);
                                drawLightBevel(painter, btns, &opt, widget, reverse ?  ROUNDED_LEFT : ROUNDED_RIGHT,
                                            getFill(&opt, use), use, true, WIDGET_SPIN);
                                painter->restore();
                            }
                            drawFadedLine(painter, down.adjusted(2, 0, -2, 0), use[BORDER_VAL(state&State_Enabled)], true, true, true);
                        }
                    }

                if(up.isValid())
                {
                    QStyleOption opt(*option);

                    up.setHeight(up.height()+1);
                    opt.rect=up;
                    opt.direction=option->direction;
                    opt.state=(enabled && (spinBox->stepEnabled&QAbstractSpinBox::StepUpEnabled ||
                                           (QAbstractSpinBox::StepNone==spinBox->stepEnabled && isOO))
                                    ? State_Enabled : State_None)|
                              (upIsActive && sunken ? State_Sunken : State_Raised)|
                              (upIsActive && !sunken && mouseOver ? State_MouseOver : State_None)|State_Horizontal;

                    drawPrimitive(QAbstractSpinBox::PlusMinus==spinBox->buttonSymbols ? PE_IndicatorSpinPlus : PE_IndicatorSpinUp,
                                  &opt, painter, widget);
                }

                if(down.isValid())
                {
                    QStyleOption opt(*option);

                    opt.rect=down;
                    opt.state=(enabled && (spinBox->stepEnabled&QAbstractSpinBox::StepDownEnabled ||
                                           (QAbstractSpinBox::StepNone==spinBox->stepEnabled && isOO))
                                    ? State_Enabled : State_None)|
                              (downIsActive && sunken ? State_Sunken : State_Raised)|
                              (downIsActive && !sunken && mouseOver ? State_MouseOver : State_None)|State_Horizontal;
                    opt.direction=option->direction;

                    drawPrimitive(QAbstractSpinBox::PlusMinus==spinBox->buttonSymbols ? PE_IndicatorSpinMinus : PE_IndicatorSpinDown,
                                  &opt, painter, widget);
                }
                if(doFrame && !opts.unifySpin)
                {
                    if(reverse)
                        frame.setX(frame.x()-1);
                    else
                        frame.setWidth(frame.width()+1);
                    drawEntryField(painter, frame, widget, option, reverse ? ROUNDED_RIGHT : ROUNDED_LEFT, true, false);
                }
                opts.unifySpin=oldUnify;
            }
            break;
        case CC_Slider:
            if (auto slider = styleOptCast<QStyleOptionSlider>(option)) {
                QRect groove(subControlRect(CC_Slider, option,
                                            SC_SliderGroove, widget));
                QRect handle(subControlRect(CC_Slider, option,
                                            SC_SliderHandle, widget));
                // QRect ticks(subControlRect(CC_Slider, option,
                //                            SC_SliderTickmarks, widget));
                bool  horizontal(slider->orientation == Qt::Horizontal),
                      ticksAbove(slider->tickPosition & QSlider::TicksAbove),
                      ticksBelow(slider->tickPosition & QSlider::TicksBelow);

                //The clickable region is 5 px wider than the visible groove for improved usability
//                 if (groove.isValid())
//                     groove = horizontal ? groove.adjusted(0, 5, 0, -5) : groove.adjusted(5, 0, -5, 0);

                if ((option->subControls&SC_SliderGroove) && groove.isValid())
                    drawSliderGroove(painter, groove, handle, slider, widget);

                if ((option->subControls&SC_SliderHandle) && handle.isValid())
                {
                    QStyleOptionSlider s(*slider);
                    if(!(s.activeSubControls & QStyle::SC_SliderHandle))
                    {
                        s.state &= ~QStyle::State_MouseOver;
                        s.state &= ~QStyle::State_Sunken;
                    }

                    drawSliderHandle(painter, handle, &s);

                    if (state&State_HasFocus && FOCUS_GLOW!=opts.focus)
                    {
                        QStyleOptionFocusRect fropt;
                        fropt.QStyleOption::operator=(*slider);
                        fropt.rect = slider->rect;

                        if(horizontal)
                            fropt.rect.adjust(0, 0, 0, -1);
                        else
                            fropt.rect.adjust(0, 0, -1, 0);

                        drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
                    }
                }

                if (option->subControls&SC_SliderTickmarks)
                {
                    QPen oldPen = painter->pen();
                    painter->setPen(backgroundColors(option)[QTC_STD_BORDER]);
                    int tickSize(pixelMetric(PM_SliderTickmarkOffset, option, widget)),
                        available(pixelMetric(PM_SliderSpaceAvailable, slider, widget)),
                        interval(slider->tickInterval);
                    if (interval <= 0)
                    {
                        interval = slider->singleStep;
                        if (QStyle::sliderPositionFromValue(slider->minimum, slider->maximum, interval,
                                                            available)
                            - QStyle::sliderPositionFromValue(slider->minimum, slider->maximum,
                                                            0, available) < 3)
                            interval = slider->pageStep;
                    }
                    if (interval <= 0)
                        interval = 1;

                    int sliderLength(slider->maximum - slider->minimum + 1),
                        nticks(sliderLength / interval); // add one to get the end tickmark
                    if (sliderLength % interval > 0)
                        nticks++; // round up the number of tick marks

                    int v(slider->minimum),
                        len(pixelMetric(PM_SliderLength, slider, widget));

                    while (v <= slider->maximum + 1)
                    {
                        if (v == slider->maximum + 1 && interval == 1)
                            break;

                        int pos(sliderPositionFromValue(slider->minimum, slider->maximum,
                                                        qMin(v, slider->maximum), (horizontal
                                                            ? slider->rect.width()
                                                            : slider->rect.height()) - len,
                                                        slider->upsideDown) + len / 2);

                        int extra(2); // - ((v == slider->minimum || v == slider->maximum) ? 1 : 0);

                        if (horizontal)
                        {
                            if (ticksAbove)
                                painter->drawLine(QLine(pos, slider->rect.top() + extra,
                                                pos, slider->rect.top() + tickSize));
                            if (ticksBelow)
                                painter->drawLine(QLine(pos, slider->rect.bottom() - extra,
                                                pos, slider->rect.bottom() - tickSize));
                        }
                        else
                        {
                            if (ticksAbove)
                                painter->drawLine(QLine(slider->rect.left() + extra, pos,
                                                slider->rect.left() + tickSize, pos));
                            if (ticksBelow)
                                painter->drawLine(QLine(slider->rect.right() - extra, pos,
                                                slider->rect.right() - tickSize, pos));
                        }

                        // in the case where maximum is max int
                        int nextInterval = v + interval;
                        if (nextInterval < v)
                            break;
                        v = nextInterval;
                    }
                    painter->setPen(oldPen);
                }
            }
            break;
        case CC_TitleBar:
            if (auto titleBar = styleOptCast<QStyleOptionTitleBar>(option)) {
                painter->save();

                EAppearance  app=qtcWidgetApp(WIDGET_MDI_WINDOW_TITLE, &opts, option->state&State_Active);
                bool         active(state&State_Active),
                             kwin(theThemedApp==APP_KWIN || titleBar->titleBarState&QtC_StateKWin);
                const QColor *bgndCols(APPEARANCE_NONE==app
                                        ? kwin ? backgroundColors(option) : backgroundColors(palette.color(QPalette::Active, QPalette::Window))
                                        : kwin ? buttonColors(option) : getMdiColors(titleBar, active)),
                             *btnCols(kwin || opts.titlebarButtons&TITLEBAR_BUTTON_STD_COLOR
                                        ? buttonColors(option)
                                        : getMdiColors(titleBar, active)),
                             *titleCols(APPEARANCE_NONE==app
                                        ? bgndCols
                                        : kwin || !(opts.titlebarButtons&TITLEBAR_BUTTON_STD_COLOR)
                                            ? btnCols : getMdiColors(titleBar, active));
                QColor       textColor(theThemedApp==APP_KWIN
                                        ? option->palette.color(QPalette::WindowText)
                                        : active
                                            ? m_activeMdiTextColor
                                            : m_mdiTextColor),
                             iconColor(textColor),
                             shadow(WINDOW_SHADOW_COLOR(opts.titlebarEffect));
                QStyleOption opt(*option);
                QRect        tr(r),
                             menuRect(subControlRect(CC_TitleBar, titleBar, SC_TitleBarSysMenu, widget));
                ERound       round=(opts.square&SQUARE_WINDOWS && opts.round>ROUND_SLIGHT) ? ROUND_SLIGHT : opts.round;
                QColor       borderCol(kwin && option->version==(TBAR_BORDER_VERSION_HACK+2)
                                        ? palette.color(QPalette::Active, QPalette::Shadow)
                                        : titleCols[kwin && option->version==TBAR_BORDER_VERSION_HACK ? 0 : QTC_STD_BORDER]);

                if (!kwin && widget && BLEND_TITLEBAR &&
                    qobject_cast<const QMdiSubWindow*>(widget)) {
                    const QWidget *w = nullptr;
                    if (qobject_cast<const QMainWindow*>(widget)) {
                        w = widget;
                    } else if (static_cast<const QMdiSubWindow*>(
                                   widget)->widget()) {
                        w = qobject_cast<const QMainWindow*>(
                            static_cast<const QMdiSubWindow*>(
                                widget)->widget());
                    }
                    if(w)
                    {
                        const QMenuBar *menuBar=static_cast<const QMainWindow*>(w)->menuBar();

                        if(menuBar)
                            tr.adjust(0, 0, 0, menuBar->rect().height());
                    }
                }

                opt.state=State_Horizontal|State_Enabled|State_Raised|(active ? State_Active : State_None);

#ifndef QTC_QT4_ENABLE_KDE
                QPainterPath path;
#else
                QPainterPath path(round < ROUND_SLIGHT ? QPainterPath() :
                                  buildPath(QRectF(state&QtC_StateKWinNoBorder ?
                                                   tr : tr.adjusted(1, 1, -1, 0)),
                                            WIDGET_MDI_WINDOW_TITLE,
                                            state & QtC_StateKWin &&
                                            state & QtC_StateKWinTabDrag ?
                                            ROUNDED_ALL : ROUNDED_TOP,
                                            (round > ROUND_SLIGHT /*&& kwin*/ ?
                                             6.0 : 2.0)));
#endif
                if (!kwin && !qtcIsCustomBgnd(opts))
                    painter->fillRect(tr, borderCol);

                painter->setRenderHint(QPainter::Antialiasing, true);

                if (kwin && (state & QtC_StateKWinFillBgnd))
                    drawBevelGradient(titleCols[ORIGINAL_SHADE], painter, tr,
                                      path, true, false, APPEARANCE_FLAT,
                                      WIDGET_MDI_WINDOW, false);
                if ((!kwin && !m_isPreview) ||
                    (APPEARANCE_NONE != app &&
                     (!qtcIsFlat(app) || (titleCols[ORIGINAL_SHADE] != QApplication::palette().background().color()))))
                    drawBevelGradient(titleCols[ORIGINAL_SHADE], painter, tr, path, true, false, app, WIDGET_MDI_WINDOW, false);

                if (!(state & QtC_StateKWinNoBorder)) {
                    QColor light(titleCols[0]),
                           dark(borderCol);
                    bool   addLight=opts.windowBorder&WINDOW_BORDER_ADD_LIGHT_BORDER && (!kwin || qtcGetWindowBorderSize(false).sides>1);

                    if(kwin)
                    {
                        light.setAlphaF(1.0);
                        dark.setAlphaF(1.0);
                    }

                    if(addLight)
                    {
                        painter->setPen(light);
                        painter->save();
                        painter->setClipRect(r.adjusted(0, 0, -1, -1));
                        painter->drawPath(buildPath(r.adjusted(1, 1, 0, 1), WIDGET_MDI_WINDOW_TITLE, ROUNDED_TOP,
                                                    round<ROUND_SLIGHT
                                                        ? 0
                                                        : round>ROUND_SLIGHT /*&& kwin*/
                                                            ? 5.0
                                                            : 1.0));
                        painter->restore();
                    }

                    painter->setPen(dark);
                    painter->drawPath(buildPath(r, WIDGET_MDI_WINDOW_TITLE, ROUNDED_TOP,
                                                round<ROUND_SLIGHT
                                                    ? 0
                                                    : round>ROUND_SLIGHT /*&& kwin*/
                                                        ? 6.0
                                                        : 2.0));

                    painter->setRenderHint(QPainter::Antialiasing, false);

                    if(addLight)
                    {
                        painter->setPen(light);
                        painter->drawPoint(r.x()+1, r.y()+r.height()-1);
                    }

                    if (round > ROUND_SLIGHT && opts.round >= ROUND_FULL) {
                        if (!(state & QtC_StateKWinCompositing)) {
                            painter->setPen(dark);

                            painter->drawLine(r.x()+1, r.y()+4, r.x()+1, r.y()+3);
                            painter->drawPoint(r.x()+2, r.y()+2);
                            painter->drawLine(r.x()+3, r.y()+1, r.x()+4, r.y()+1);
                            painter->drawLine(r.x()+r.width()-2, r.y()+4, r.x()+r.width()-2, r.y()+3);
                            painter->drawPoint(r.x()+r.width()-3, r.y()+2);
                            painter->drawLine(r.x()+r.width()-4, r.y()+1, r.x()+r.width()-5, r.y()+1);
                        }

                        if(addLight &&
                            (APPEARANCE_SHINY_GLASS!=(active ? opts.titlebarAppearance : opts.inactiveTitlebarAppearance)))
                        {
                            painter->setPen(light);
                            painter->drawLine(r.x()+2, r.y()+4, r.x()+2, r.y()+3);
                            painter->drawLine(r.x()+3, r.y()+2, r.x()+4, r.y()+2);
                            painter->drawLine(r.x()+r.width()-4, r.y()+2, r.x()+r.width()-5, r.y()+2);
                        }
                    }

                    if(opts.windowBorder&WINDOW_BORDER_BLEND_TITLEBAR && (!kwin || !(state&QtC_StateKWinNoBorder)))
                    {
                        static const int constFadeLen=8;
                        QPoint          start(0, r.y()+r.height()-(1+constFadeLen)),
                                        end(start.x(), start.y()+constFadeLen);
                        QLinearGradient grad(start, end);

                        grad.setColorAt(0, dark);
                        grad.setColorAt(1, m_backgroundCols[QTC_STD_BORDER]);
                        painter->setPen(QPen(QBrush(grad), 1));
                        painter->drawLine(r.x(), start.y(), r.x(), end.y());
                        painter->drawLine(r.x()+r.width()-1, start.y(), r.x()+r.width()-1, end.y());

                        if(addLight)
                        {
                            grad.setColorAt(0, light);
                            grad.setColorAt(1, m_backgroundCols[0]);
                            painter->setPen(QPen(QBrush(grad), 1));
                            painter->drawLine(r.x()+1, start.y(), r.x()+1, end.y());
                        }
                    }
                }
                else
                    painter->setRenderHint(QPainter::Antialiasing, false);

                if(kwin)
                {
                    painter->restore();
                    break;
                }

                int   adjust(0);
                QRect captionRect(subControlRect(CC_TitleBar, titleBar, SC_TitleBarLabel, widget));

                if(opts.titlebarButtons&TITLEBAR_BUTTON_SUNKEN_BACKGROUND && captionRect!=r)
                {
                    bool menuIcon=TITLEBAR_ICON_MENU_BUTTON==opts.titlebarIcon,
                         menuLeft=menuRect.isValid() && !titleBar->icon.isNull() && menuRect.left()<(r.left()+constWindowMargin+4);
                    int  height=r.height()-(1+(2*constWindowMargin));

                    adjust=1;
                    if(captionRect.left()>(r.left()+constWindowMargin))
                    {
                        int width=captionRect.left()-(r.left()+(2*constWindowMargin));

                        if(!(menuIcon && menuLeft) || width>(height+4))
                            drawSunkenBevel(painter, QRect(r.left()+constWindowMargin+1, r.top()+constWindowMargin+1, width, height), titleCols[ORIGINAL_SHADE]);
                    }
                    if(captionRect.right()<(r.right()-constWindowMargin))
                    {
                        int width=r.right()-(captionRect.right()+(2*constWindowMargin));

                        if(!(menuIcon && !menuLeft) || width>(height+4))
                            drawSunkenBevel(painter, QRect(captionRect.right()+constWindowMargin, r.top()+constWindowMargin+1, width, height), titleCols[ORIGINAL_SHADE]);
                    }
                }

                bool    showIcon=TITLEBAR_ICON_NEXT_TO_TITLE==opts.titlebarIcon && !titleBar->icon.isNull();
                int     iconSize=showIcon ? pixelMetric(QStyle::PM_SmallIconSize) : 0,
                        iconX=r.x();
                QPixmap pixmap;

                if(showIcon)
                    pixmap=getIconPixmap(titleBar->icon, iconSize, titleBar->state);

                if(!titleBar->text.isEmpty())
                {
                    static const int constPad=4;

                    Qt::Alignment alignment((Qt::Alignment)pixelMetric((QStyle::PixelMetric)QtC_TitleAlignment, 0L, 0L));
                    bool          alignFull(Qt::AlignHCenter==alignment),
                                  iconRight((!reverse && alignment&Qt::AlignRight) || (reverse && alignment&Qt::AlignLeft));
                    QRect         textRect(alignFull
                                            ? QRect(r.x(), captionRect.y(), r.width(), captionRect.height())
                                            : captionRect);

#ifndef QTC_QT4_ENABLE_KDE
                    QFont         font(painter->font());
                    font.setBold(true);
                    painter->setFont(font);
#else
                    painter->setFont(KGlobalSettings::windowTitleFont());
#endif

                    QFontMetrics fm(painter->fontMetrics());
                    QString str(fm.elidedText(titleBar->text, Qt::ElideRight, textRect.width(), QPalette::WindowText));

                    int           textWidth=alignFull || (showIcon && alignment&Qt::AlignHCenter)
                                                ? fm.boundingRect(str).width()+(showIcon ? iconSize+constPad : 0) : 0;

                    if(alignFull &&
                        ((captionRect.left()>((textRect.width()-textWidth)>>1)) ||
                          (captionRect.right()<((textRect.width()+textWidth)>>1)) ) )
                    {
                        alignment=Qt::AlignVCenter|Qt::AlignRight;
                        textRect=captionRect;
                    }

                    if(alignment&Qt::AlignLeft && constWindowMargin==textRect.x())
                        textRect.adjust(showIcon ? 4 : 6, 0, 0, 0);

                    if(showIcon)
                    {
                        if(alignment&Qt::AlignHCenter)
                        {
                            if(reverse)
                            {
                                iconX=((textRect.width()-textWidth)/2.0)+0.5+textWidth+iconSize;
                                textRect.setX(textRect.x()-(iconSize+constPad));
                            }
                            else
                            {
                                iconX=((textRect.width()-textWidth)/2.0)+0.5;
                                textRect.setX(iconX+iconSize+constPad);
                                alignment=Qt::AlignVCenter|Qt::AlignLeft;
                            }
                        }
                        else if((!reverse && alignment&Qt::AlignLeft) || (reverse && alignment&Qt::AlignRight))
                        {
                            iconX=textRect.x();
                            textRect.setX(textRect.x()+(iconSize+constPad));
                        }
                        else if((!reverse && alignment&Qt::AlignRight) || (reverse && alignment&Qt::AlignLeft))
                        {
                            if(iconRight)
                            {
                                iconX=textRect.x()+textRect.width()-iconSize;
                                textRect.setWidth(textRect.width()-(iconSize+constPad));
                            }
                            else
                            {
                                iconX=textRect.x()+textRect.width()-textWidth;
                                if(iconX<textRect.x())
                                    iconX=textRect.x();
                            }
                        }
                    }

                    QTextOption textOpt(alignment|Qt::AlignVCenter);
                    textOpt.setWrapMode(QTextOption::NoWrap);

                    if(EFFECT_NONE!=opts.titlebarEffect)
                    {
                        shadow.setAlphaF(WINDOW_TEXT_SHADOW_ALPHA(opts.titlebarEffect));
                        //painter->setPen(shadow);
                        painter->setPen(blendColors(WINDOW_SHADOW_COLOR(opts.titlebarEffect), titleCols[ORIGINAL_SHADE],
                                                    WINDOW_TEXT_SHADOW_ALPHA(opts.titlebarEffect)));
                        painter->drawText(EFFECT_SHADOW==opts.titlebarEffect
                                            ? textRect.adjusted(1, 1, 1, 1)
                                            : textRect.adjusted(0, 1, 0, 1),
                                          str, textOpt);

                        if (!active && DARK_WINDOW_TEXT(textColor))
                        {
                            //textColor.setAlpha((textColor.alpha() * 180) >> 8);
                            textColor=blendColors(textColor, titleCols[ORIGINAL_SHADE], ((255 * 180) >> 8)/256.0);
                        }
                    }
                    painter->setPen(textColor);
                    painter->drawText(textRect, str, textOpt);
                }

                if(showIcon && iconX>=0)
                    painter->drawPixmap(iconX, r.y()+((r.height()-iconSize)/2)+1, pixmap);

                if ((titleBar->subControls&SC_TitleBarMinButton) && (titleBar->titleBarFlags&Qt::WindowMinimizeButtonHint) &&
                    !(titleBar->titleBarState&Qt::WindowMinimized))
                    drawMdiControl(painter, titleBar, SC_TitleBarMinButton, widget, TITLEBAR_MIN, iconColor, btnCols, bgndCols,
                                   adjust, active);

                if ((titleBar->subControls&SC_TitleBarMaxButton) && (titleBar->titleBarFlags&Qt::WindowMaximizeButtonHint) &&
                    !(titleBar->titleBarState&Qt::WindowMaximized))
                    drawMdiControl(painter, titleBar, SC_TitleBarMaxButton, widget, TITLEBAR_MAX, iconColor, btnCols, bgndCols,
                                   adjust, active);

                if ((titleBar->subControls&SC_TitleBarCloseButton) && (titleBar->titleBarFlags&Qt::WindowSystemMenuHint))
                    drawMdiControl(painter, titleBar, SC_TitleBarCloseButton, widget, TITLEBAR_CLOSE, iconColor, btnCols, bgndCols,
                                   adjust, active);

                if ((titleBar->subControls&SC_TitleBarNormalButton) &&
                    (((titleBar->titleBarFlags&Qt::WindowMinimizeButtonHint) &&
                    (titleBar->titleBarState&Qt::WindowMinimized)) ||
                    ((titleBar->titleBarFlags&Qt::WindowMaximizeButtonHint) &&
                    (titleBar->titleBarState&Qt::WindowMaximized))))
                    drawMdiControl(painter, titleBar, SC_TitleBarNormalButton, widget, TITLEBAR_MAX, iconColor, btnCols, bgndCols,
                                   adjust, active);

                if (titleBar->subControls&SC_TitleBarContextHelpButton && (titleBar->titleBarFlags&Qt::WindowContextHelpButtonHint))
                    drawMdiControl(painter, titleBar, SC_TitleBarContextHelpButton, widget, TITLEBAR_HELP, iconColor, btnCols, bgndCols,
                                   adjust, active);

                if (titleBar->subControls&SC_TitleBarShadeButton && (titleBar->titleBarFlags&Qt::WindowShadeButtonHint))
                    drawMdiControl(painter, titleBar, SC_TitleBarShadeButton, widget, TITLEBAR_SHADE, iconColor, btnCols, bgndCols,
                                   adjust, active);

                if (titleBar->subControls&SC_TitleBarUnshadeButton && (titleBar->titleBarFlags&Qt::WindowShadeButtonHint))
                    drawMdiControl(painter, titleBar, SC_TitleBarUnshadeButton, widget, TITLEBAR_SHADE, iconColor, btnCols, bgndCols,
                                   adjust, active);

                if ((titleBar->subControls&SC_TitleBarSysMenu) && (titleBar->titleBarFlags&Qt::WindowSystemMenuHint))
                {
                    if(TITLEBAR_ICON_MENU_BUTTON==opts.titlebarIcon)
                    {
                        bool hover((titleBar->activeSubControls&SC_TitleBarSysMenu) && (titleBar->state&State_MouseOver));

                        if(active || hover || !(opts.titlebarButtons&TITLEBAR_BUTTOM_HIDE_ON_INACTIVE_WINDOW))
                        {
                            if (menuRect.isValid())
                            {
                                bool sunken((titleBar->activeSubControls&SC_TitleBarSysMenu) && (titleBar->state&State_Sunken));
                                int  offset(sunken ? 1 : 0);

//                                 if(!(opts.titlebarButtons&TITLEBAR_BUTTON_ROUND))
//                                     drawMdiButton(painter, menuRect, hover, sunken,
//                                                   coloredMdiButtons(state&State_Active, hover)
//                                                     ? m_titleBarButtonsCols[TITLEBAR_MENU] : btnCols);

                                if (!titleBar->icon.isNull())
                                    titleBar->icon.paint(painter, menuRect.adjusted(offset, offset, offset, offset));
                                else
                                {
                                    QStyleOption tool(0);

                                    tool.palette = palette;
                                    tool.rect = menuRect;
                                    painter->save();
                                    drawItemPixmap(painter, menuRect.adjusted(offset, offset, offset, offset), Qt::AlignCenter,
                                                   standardIcon(SP_TitleBarMenuButton, &tool, widget).pixmap(16, 16));
                                    painter->restore();
                                }
                            }
                        }
                    }
                    else
                        drawMdiControl(painter, titleBar, SC_TitleBarSysMenu, widget, TITLEBAR_MENU, iconColor, btnCols, bgndCols,
                                       adjust, active);

                    if(active && opts.windowBorder&WINDOW_BORDER_SEPARATOR)
                    {
                        QColor        color(active ? m_activeMdiTextColor : m_mdiTextColor);
                        Qt::Alignment align(pixelMetric((QStyle::PixelMetric)QtC_TitleAlignment, 0L, 0L));
                        QRect         lr(r.x(), captionRect.y(), r.width(), captionRect.height());

                        lr.adjust(16, lr.height()-2, -16, 0);
                        color.setAlphaF(0.5);
                        drawFadedLine(painter, lr, color, align&(Qt::AlignHCenter|Qt::AlignRight),
                                      align&(Qt::AlignHCenter|Qt::AlignLeft), true);
                    }
                }

                painter->restore();
            }
            break;
        case CC_ScrollBar:
            if (auto scrollbar = styleOptCast<QStyleOptionSlider>(option)) {
                bool useThreeButtonScrollBar(SCROLLBAR_KDE==opts.scrollbarType),
                                   horiz(Qt::Horizontal==scrollbar->orientation),
                                   maxed(scrollbar->minimum == scrollbar->maximum),
                                   atMin(maxed || scrollbar->sliderValue==scrollbar->minimum),
                                   atMax(maxed || scrollbar->sliderValue==scrollbar->maximum)/*,
                                   inStack(0!=opts.tabBgnd && inStackWidget(widget))*/;
                QRect subline(subControlRect(control, option,
                                             SC_ScrollBarSubLine, widget));
                QRect addline(subControlRect(control, option,
                                             SC_ScrollBarAddLine, widget));
                QRect subpage(subControlRect(control, option,
                                             SC_ScrollBarSubPage, widget));
                QRect addpage(subControlRect(control, option,
                                             SC_ScrollBarAddPage, widget));
                QRect slider(subControlRect(control, option,
                                            SC_ScrollBarSlider, widget));
                QRect first(subControlRect(control, option,
                                           SC_ScrollBarFirst, widget));
                QRect last(subControlRect(control, option,
                                          SC_ScrollBarLast, widget));
                QRect subline2(addline);
                // QRect sbRect(scrollbar->rect);
                QStyleOptionSlider opt(*scrollbar);

                // For OO.o 3.2 need to fill widget background!
                if(isOOWidget(widget))
                    painter->fillRect(r, palette.brush(QPalette::Window));

                if(reverse && horiz)
                {
                    bool tmp(atMin);

                    atMin=atMax;
                    atMax=tmp;
                }

                if (useThreeButtonScrollBar)
                {
                    int sbextent(pixelMetric(PM_ScrollBarExtent, scrollbar, widget));

                    if(horiz && reverse)
                        subline2=QRect((r.x()+r.width()-1)-sbextent, r.y(), sbextent, sbextent);
                    else if (horiz)
                        subline2.translate(-addline.width(), 0);
                    else
                        subline2.translate(0, -addline.height());

                    if (horiz)
                        subline.setWidth(sbextent);
                    else
                        subline.setHeight(sbextent);
                }

                // Draw trough...
                bool  noButtons(opts.round != ROUND_NONE &&
                                (SCROLLBAR_NONE==opts.scrollbarType || opts.flatSbarButtons));
                QRect s2(subpage), a2(addpage);

#ifndef SIMPLE_SCROLLBARS
                if(noButtons)
                {
                    // Increase clipping to allow trough to "bleed" into slider corners...
                    a2.adjust(-3, -3, 3, 3);
                    s2.adjust(-3, -3, 3, 3);
                }
#endif

                painter->save();

                if ((opts.thinSbarGroove || opts.flatSbarButtons) &&
                    qtcCheckType(getParent<2>(widget),
                                 "QComboBoxListView")) {
                    painter->fillRect(r, palette.brush(QPalette::Base));
                } else if (opts.thinSbarGroove && theThemedApp == APP_ARORA &&
                           qtcCheckType(widget, "WebView")) {
                    painter->fillRect(r, m_backgroundCols[ORIGINAL_SHADE]);
                }

                if(!opts.gtkScrollViews ||
                   (opts.flatSbarButtons && !qtcIsFlat(opts.sbarBgndAppearance)/* && SCROLLBAR_NONE!=opts.scrollbarType*/))
                    drawBevelGradientReal(palette.brush(QPalette::Background).color(), painter, r, horiz, false,
                                          opts.sbarBgndAppearance, WIDGET_SB_BGND);

                if(noButtons || opts.flatSbarButtons)
                {
                    int mod=THIN_SBAR_MOD;
                    // Draw complete groove here, as we want to round both ends...
                    opt.rect=subpage.united(addpage);
                    opt.state=scrollbar->state;
                    opt.state&=~(State_MouseOver|State_Sunken|State_On);

                    if(opts.thinSbarGroove && slider.isValid())
                    {
                        painter->save();
                        painter->setClipRegion(QRegion(opt.rect).subtract(slider.adjusted(1, 1, -1, -1)));
                    }
                    drawLightBevel(painter, opts.thinSbarGroove
                                                ? horiz
                                                    ? opt.rect.adjusted(0, mod, 0, -mod)
                                                    : opt.rect.adjusted(mod, 0, -mod, 0)
                                                : opt.rect, &opt, widget,
#ifndef SIMPLE_SCROLLBARS
                                   !(opts.square&SQUARE_SB_SLIDER) && (SCROLLBAR_NONE==opts.scrollbarType || opts.flatSbarButtons)
                                        ? ROUNDED_ALL :
#endif
                                   ROUNDED_NONE,
                                   m_backgroundCols[2], m_backgroundCols, true,
                                   opts.thinSbarGroove ? WIDGET_SLIDER_TROUGH : WIDGET_TROUGH);
                    if(opts.thinSbarGroove && slider.isValid())
                        painter->restore();
                }
                else
                {
                    if((option->subControls&SC_ScrollBarSubPage) && subpage.isValid())
                    {
                        opt.state=scrollbar->state;
                        opt.rect = subpage;
//                         if (!(scrollbar->activeSubControls&SC_ScrollBarSubPage))
                            opt.state &= ~(State_Sunken|State_MouseOver|State_On);
                        drawControl(CE_ScrollBarSubPage, &opt, painter, widget);
                    }

                    if((option->subControls&SC_ScrollBarAddPage) && addpage.isValid())
                    {
                        opt.state=scrollbar->state;
                        opt.rect = addpage;
//                         if (!(scrollbar->activeSubControls&SC_ScrollBarAddPage))
                            opt.state &= ~(State_Sunken|State_MouseOver|State_On);
                        drawControl(CE_ScrollBarAddPage, &opt, painter, widget);
                    }
                }

                if((option->subControls&SC_ScrollBarSubLine) && subline.isValid())
                {
                    opt.rect=subline;
                    opt.state=scrollbar->state/*|(inStack ? NO_BGND_BUTTON : State_None)*/;
                    if(maxed || atMin)
                        opt.state&=~State_Enabled;
                    if (!(scrollbar->activeSubControls&SC_ScrollBarSubLine) ||
                        (useThreeButtonScrollBar && m_sbWidget && m_sbWidget==widget))
                        opt.state &= ~(State_Sunken | State_MouseOver);

                    drawControl(CE_ScrollBarSubLine, &opt, painter, widget);

                    if (useThreeButtonScrollBar && subline2.isValid())
                    {
                        opt.rect=subline2;
                        opt.state=scrollbar->state/*|(inStack ? NO_BGND_BUTTON : State_None)*/;
                        if(maxed || atMin)
                            opt.state&=~State_Enabled;
                        if ((!(scrollbar->activeSubControls&SC_ScrollBarSubLine)) || (m_sbWidget && m_sbWidget!=widget))
                            opt.state &= ~(State_Sunken | State_MouseOver);

                        drawControl(CE_ScrollBarSubLine, &opt, painter, widget);
                    }
                }

                if((option->subControls&SC_ScrollBarAddLine) && addline.isValid())
                {
                    opt.rect=addline;
                    opt.state=scrollbar->state/*|(inStack ? NO_BGND_BUTTON : State_None)*/;
                    if(maxed || atMax)
                        opt.state&=~State_Enabled;
                    if (!(scrollbar->activeSubControls&SC_ScrollBarAddLine))
                        opt.state &= ~(State_Sunken | State_MouseOver);
                    drawControl(CE_ScrollBarAddLine, &opt, painter, widget);
                }

                if((option->subControls&SC_ScrollBarFirst) && first.isValid())
                {
                    opt.rect=first;
                    opt.state=scrollbar->state;
                    if (!(scrollbar->activeSubControls&SC_ScrollBarFirst))
                        opt.state &= ~(State_Sunken | State_MouseOver);
                    drawControl(CE_ScrollBarFirst, &opt, painter, widget);
                }

                if((option->subControls&SC_ScrollBarLast) && last.isValid())
                {
                    opt.rect=last;
                    opt.state=scrollbar->state;
                    if (!(scrollbar->activeSubControls&SC_ScrollBarLast))
                        opt.state &= ~(State_Sunken | State_MouseOver);
                    drawControl(CE_ScrollBarLast, &opt, painter, widget);
                }

                if(((option->subControls&SC_ScrollBarSlider) || noButtons) && slider.isValid())
                {
                    // If "SC_ScrollBarSlider" wasn't specified, then we only want to draw the portion
                    // of the slider that overlaps with the trough. So, once again set the clipping
                    // region...

                    // NO! Seeems to mess things up with Arora, su just dsiable all clipping when drawing
                    // the slider...
                    painter->setClipping(false);
#ifdef INCREASE_SB_SLIDER
                    if (!opts.flatSbarButtons) {
                        if(atMax)
                            switch(opts.scrollbarType)
                            {
                                case SCROLLBAR_KDE:
                                case SCROLLBAR_WINDOWS:
                                case SCROLLBAR_PLATINUM:
                                    if(horiz)
                                        slider.adjust(0, 0, 1, 0);
                                    else
                                        slider.adjust(0, 0, 0, 1);
                                default:
                                    break;
                            }
                        if(atMin)
                            switch(opts.scrollbarType)
                            {
                                case SCROLLBAR_KDE:
                                case SCROLLBAR_WINDOWS:
                                case SCROLLBAR_NEXT:
                                    if(horiz)
                                        slider.adjust(-1, 0, 0, 0);
                                    else
                                        slider.adjust(0, -1, 0, 0);
                                default:
                                    break;
                            }
                    }
#endif
                    opt.rect=slider;
                    opt.state=scrollbar->state;
                    if (!(scrollbar->activeSubControls&SC_ScrollBarSlider))
                        opt.state &= ~(State_Sunken | State_MouseOver);
                    drawControl(CE_ScrollBarSlider, &opt, painter, widget);

                    // ### perhaps this should not be able to accept focus if maxedOut?
                    if(state&State_HasFocus)
                    {
                        opt.state=scrollbar->state;
                        opt.rect=QRect(slider.x()+2, slider.y()+2, slider.width()-5, slider.height()-5);
                        drawPrimitive(PE_FrameFocusRect, &opt, painter, widget);
                    }
                }
                painter->restore();
            }
            break;
        case CC_ComboBox:
            if (auto comboBox = styleOptCast<QStyleOptionComboBox>(option)) {
                painter->save();

                QRect        frame(subControlRect(CC_ComboBox, option, SC_ComboBoxFrame, widget)),
                             arrow(subControlRect(CC_ComboBox, option, SC_ComboBoxArrow, widget)),
                             field(subControlRect(CC_ComboBox, option, SC_ComboBoxEditField, widget));
                const QColor *use(buttonColors(option));
                bool         sunken(state&State_On), // comboBox->listBox() ? comboBox->listBox()->isShown() : false),
                             glowOverFocus(state & State_MouseOver &&
                                           oneOf(opts.focus, FOCUS_FULL,
                                                 FOCUS_FILLED) &&
                                           MO_GLOW == opts.coloredMouseOver &&
                                           opts.buttonEffect != EFFECT_NONE &&
                                           !sunken && !comboBox->editable &&
                                           state & State_Enabled &&
                                           state & State_HasFocus),
                             doEffect(opts.buttonEffect != EFFECT_NONE &&
                                      (!comboBox->editable || opts.etchEntry)),
                             isOO(isOOWidget(widget)),
                             isOO31(isOO);

                if(isOO)
                {
                    // This (hopefull) checks is we're OO.o 3.2 - in which case no adjustment is required...
                    const QImage *img=getImage(painter);

                    isOO31=!img || img->rect()!=r;

                    if(isOO31)
                        frame.adjust(0, 0, 0, -2), arrow.adjust(0, 0, 0, -2), field.adjust(0, 0, 0, -2);
                    else
                        arrow.adjust(1, 0, 0, 0);
                }

//                 painter->fillRect(r, Qt::transparent);
                if(doEffect)
                {
                    bool glowFocus(state&State_HasFocus && state&State_Enabled && USE_GLOW_FOCUS(state&State_MouseOver));

                    if (!glowOverFocus && !(opts.thin & THIN_FRAMES) &&
                        !sunken && opts.coloredMouseOver == MO_GLOW &&
                        (((oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED) ||
                           glowFocus) && state & State_HasFocus) ||
                         state & State_MouseOver) &&
                        state & State_Enabled && !comboBox->editable) {
                        drawGlow(painter, r,
                                 oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED) &&
                                 state & State_HasFocus ? WIDGET_DEF_BUTTON :
                                 WIDGET_COMBO, glowFocus ? m_focusCols : 0L);
                    } else {
                        drawEtch(painter, r, widget, WIDGET_COMBO,
                                 !comboBox->editable &&
                                 opts.buttonEffect == EFFECT_SHADOW && !sunken,
                                 comboBox->editable && opts.square &
                                 SQUARE_ENTRY ? opts.unifyCombo ? ROUNDED_NONE :
                                 reverse ? ROUNDED_LEFT :
                                 ROUNDED_RIGHT : ROUNDED_ALL);
                    }
                    frame.adjust(1, 1, -1, -1);
                }

                if(/*comboBox->frame &&*/ frame.isValid() && (!comboBox->editable || !opts.unifyCombo))
                {
                    const QColor *cols=m_comboBtnCols && comboBox->editable && state&State_Enabled ? m_comboBtnCols : use;

                    QStyleOption frameOpt(*option);

                    if (comboBox->editable && !(comboBox->activeSubControls&SC_ComboBoxArrow))
                        frameOpt.state &= ~(State_Sunken | State_MouseOver);

                    if(!sunken)
                        frameOpt.state|=State_Raised;

                    //if(opts.coloredMouseOver && frameOpt.state&State_MouseOver && comboBox->editable && !sunken)
                    //    frame.adjust(reverse ? 0 : 1, 0, reverse ? 1 : 0, 0);

                    drawLightBevel(painter, frame, &frameOpt, widget,
                                   comboBox->editable ? (reverse ? ROUNDED_LEFT : ROUNDED_RIGHT) : ROUNDED_ALL,
                                   getFill(&frameOpt, cols, false,
                                           (SHADE_DARKEN==opts.comboBtn || (SHADE_NONE!=opts.comboBtn &&
                                                                            !(state&State_Enabled))) &&
                                           comboBox->editable),
                                   cols, true, comboBox->editable ? WIDGET_COMBO_BUTTON : WIDGET_COMBO);
                }

                if(/*controls&SC_ComboBoxEditField &&*/ field.isValid())
                {
                    if(comboBox->editable)
                    {
                        if(opts.unifyCombo)
                        {
                            field=r;
                            if(doEffect)
                                field.adjust(1, 1, -1, -1);
                            if(isOO31)
                                field.adjust(0, 0, 0, -2);
                        }
                        else if(doEffect)
                            field.adjust(reverse ? -4 : -3, -1, reverse ? 3 : 4, 1);
                        else
                            field.adjust(reverse ? -4 : -2, -1, reverse ? 2 : 4, 1);
                        drawEntryField(painter, field, widget, option, opts.unifyCombo ? ROUNDED_ALL : reverse ? ROUNDED_RIGHT : ROUNDED_LEFT,
                                       true, false);
                    }
                    else if(opts.comboSplitter && !(SHADE_DARKEN==opts.comboBtn || m_comboBtnCols))
                    {
                        drawFadedLine(painter, QRect(reverse ? arrow.right()+1 : arrow.x()-1, arrow.top()+2,
                                                     1, arrow.height()-4),
                                      use[BORDER_VAL(state&State_Enabled)], true, true, false);
                        if(!sunken)
                            drawFadedLine(painter, QRect(reverse ? arrow.right()+2 : arrow.x(), arrow.top()+2,
                                                         1, arrow.height()-4),
                                          use[0], true, true, false);
                    }
                }

                if(/*controls&SC_ComboBoxArrow && */arrow.isValid())
                {
                    bool mouseOver=comboBox->editable && !(comboBox->activeSubControls&SC_ComboBoxArrow)
                                    ? false : (state&State_MouseOver ? true : false);

                    if(!comboBox->editable && (SHADE_DARKEN==opts.comboBtn || m_comboBtnCols))
                    {
                        if(!comboBox->editable && isOO && !isOO31)
                            arrow.adjust(reverse ? 0 : 1, 0, reverse ? -1 : 0, 0);

                        QStyleOption frameOpt(*option);
                        QRect        btn(arrow.x(), frame.y(), arrow.width()+1, frame.height());
                        const QColor *cols=SHADE_DARKEN==opts.comboBtn || !(state&State_Enabled) ? use : m_comboBtnCols;
                        if(!sunken)
                            frameOpt.state|=State_Raised;
                        painter->save();
                        painter->setClipRect(btn, Qt::IntersectClip);
                        drawLightBevel(painter, opts.comboSplitter
                                                    ? btn.adjusted(reverse ? -2 : 0, 0, reverse ? 2 : 1, 0)
                                                    : btn.adjusted(reverse ? -3 : -2, 0, reverse ? 2 : 1, 0),
                                       &frameOpt, widget, reverse ? ROUNDED_LEFT : ROUNDED_RIGHT,
                                       getFill(&frameOpt, cols, false,
                                               SHADE_DARKEN==opts.comboBtn || (SHADE_NONE!=opts.comboBtn &&
                                                                               !(state&State_Enabled))),
                                       cols, true, WIDGET_COMBO);
                        painter->restore();
                    }

                    if(sunken && (!comboBox->editable || !opts.unifyCombo))
                        arrow.adjust(1, 1, 1, 1);

                    const QColor &arrowColor = MOArrow(state, palette, mouseOver,
                                                       QPalette::ButtonText);
                    if(comboBox->editable || !(opts.gtkComboMenus && opts.doubleGtkComboArrow)) {
                        drawArrow(painter, arrow, PE_IndicatorArrowDown, arrowColor, false);
                    } else {
                        int middle = arrow.y() + (arrow.height() >> 1);
                        int gap = (opts.vArrows ? 2 : 1);

                        QRect ar = QRect(arrow.x(), middle - (LARGE_ARR_HEIGHT + gap),
                                         arrow.width(), LARGE_ARR_HEIGHT);
                        drawArrow(painter, ar, PE_IndicatorArrowUp, arrowColor, false);
                        ar = QRect(arrow.x(), middle + gap, arrow.width(), LARGE_ARR_HEIGHT);
                        drawArrow(painter, ar, PE_IndicatorArrowDown, arrowColor, false);
                    }
                }

                if(state&State_Enabled && state&State_HasFocus &&
                    /*state&State_KeyboardFocusChange &&*/ !comboBox->editable && FOCUS_GLOW!=opts.focus)
                {
                    QStyleOptionFocusRect focus;
                    bool listViewCombo =
                        (comboBox->frame && widget &&
                         widget->rect().height() <
                         (opts.buttonEffect != EFFECT_NONE ? 22 : 20));

                    if (oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED)) {
                        focus.rect = frame;
                    } else if (opts.comboSplitter) {
                        focus.rect = (reverse ? field.adjusted(0, -1, 1, 1) :
                                      field.adjusted(-1, -1, 0, 1));
                        if (listViewCombo) {
                            focus.rect.adjust(0, -2, 0, 2);
                        }
                    } else if (listViewCombo) {
                        focus.rect = frame.adjusted(1, 1, -1, -1);
                    } else {
                        focus.rect = frame.adjusted(3, 3, -3, -3);
                    }
                    // Draw glow over top of filled focus
                    if(glowOverFocus && !(opts.thin&THIN_FRAMES))
                        drawGlow(painter, frame.adjusted(-1, -1, 1, 1), WIDGET_COMBO);
                    else
                        drawPrimitive(PE_FrameFocusRect, &focus, painter, widget);
                }
                painter->restore();
            }
            break;
        default:
            BaseStyle::drawComplexControl(control, option, painter, widget);
            break;
    }
}

// Use 'drawItemTextWithRole' when already know which role to use.
void
Style::drawItemTextWithRole(QPainter *painter, const QRect &rect, int flags,
                            const QPalette &pal, bool enabled,
                            const QString &text,
                            QPalette::ColorRole textRole) const
{
    BaseStyle::drawItemText(painter, rect, flags, pal, enabled,
                               text, textRole);
}

void
Style::drawItemText(QPainter *painter, const QRect &rect, int flags,
                    const QPalette &pal, bool enabled, const QString &text,
                    QPalette::ColorRole textRole) const
{
    if (textRole == QPalette::ButtonText && !opts.stdSidebarButtons) {
        const QAbstractButton *button = getButton(nullptr, painter);

        if (button && isMultiTabBarTab(button) && button->isChecked()) {
            QPalette p(pal);

            if (m_inactiveChangeSelectionColor &&
                p.currentColorGroup() == QPalette::Inactive) {
                p.setCurrentColorGroup(QPalette::Active);
            }
            BaseStyle::drawItemText(painter, rect, flags, p, enabled,
                                       text, QPalette::HighlightedText);
            return;
        }
    }

    BaseStyle::drawItemText(painter, rect, flags, pal, enabled,
                               text, textRole);
}

QSize Style::sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const
{
    prePolish(widget);
    QSize newSize(BaseStyle::sizeFromContents(type, option, size, widget));

    switch (type)
    {
        case CT_TabBarTab:
            newSize+=QSize(1, 1);
            break;
        case CT_Splitter:
        {
            int sw=pixelMetric(PM_SplitterWidth, 0L, 0L);
            return QSize(sw, sw);
        }
        case CT_PushButton:
        {
            newSize=size;
            newSize.setWidth(newSize.width()+(ROUND_MAX==opts.round ? 12 : 8));

            if (auto btn = styleOptCast<QStyleOptionButton>(option)) {
                if (!opts.stdBtnSizes) {
                    // Cant rely on AutoDefaultButton
                    //   - as VirtualBox does not set this!!!
                    if (qtcCheckType<QDialogButtonBox>(getParent(widget)) ||
                        qtcCheckType(getParent(widget), "KFileWidget")) {
                        int iconHeight = (btn->icon.isNull() ?
                                          btn->iconSize.height() : 16);
                        if (size.height() < iconHeight + 2) {
                            newSize.setHeight(iconHeight + 2);
                        }
                    }
                }

                int margin = (pixelMetric(PM_ButtonMargin, btn, widget)+
                              (pixelMetric(PM_DefaultFrameWidth, btn, widget) * 2))-MAX_ROUND_BTN_PAD;

                newSize+=QSize(margin, margin);

                if (btn->features&QStyleOptionButton::HasMenu)
                    newSize+=QSize(4, 0);

                if (!btn->text.isEmpty() && "..."!=btn->text && newSize.width() < 80)
                    newSize.setWidth(80);

                newSize.rheight() += ((1 - newSize.rheight()) & 1);
            }
            break;
        }
//         case CT_RadioButton:
//             ++newSize.rheight();
//             ++newSize.rwidth();
//             break;
        case CT_RadioButton:
        case CT_CheckBox:
            if (auto btn = styleOptCast<QStyleOptionButton>(option)) {
                bool isRadio = CT_RadioButton==type;
                int  w = /*proxy()->*/pixelMetric(isRadio ? PM_ExclusiveIndicatorWidth : PM_IndicatorWidth, btn, widget),
                     h = /*proxy()->*/pixelMetric(isRadio ? PM_ExclusiveIndicatorHeight : PM_IndicatorHeight, btn, widget),
                     margins = 0;

                newSize=size;
                // we add 4 pixels for label margins
                if (btn->icon.isNull() || !btn->text.isEmpty())
                    margins = 0+/*proxy()->*/pixelMetric(isRadio ? PM_RadioButtonLabelSpacing : PM_CheckBoxLabelSpacing, option, widget)+
                              (opts.crHighlight ? 4 : 0);

                newSize += QSize(w + margins, 4);
                newSize.setHeight(qMax(newSize.height(), h));
            }
            break;
        case CT_ScrollBar:
            if (auto scrollBar = styleOptCast<QStyleOptionSlider>(option)) {
                int scrollBarExtent =
                    pixelMetric(PM_ScrollBarExtent, option, widget);
                // See https://github.com/QtCurve/qtcurve-qt4/issues/7
                // and https://bugs.kde.org/show_bug.cgi?id=317690.
                int scrollBarLen =
                    (qtcMax(scrollBarExtent, 13) *
                     qtcScrollbarButtonNumSize(opts.scrollbarType) +
                     pixelMetric(PM_ScrollBarSliderMin, option, widget));

                if (scrollBar->orientation == Qt::Horizontal) {
                    newSize = QSize(scrollBarLen, scrollBarExtent);
                } else {
                    newSize = QSize(scrollBarExtent, scrollBarLen);
                }
            }
            break;
        case CT_LineEdit:
            if (auto f = styleOptCast<QStyleOptionFrame>(option)) {
                newSize = size + QSize(2 * f->lineWidth, 2 * f->lineWidth);
            }
            break;
        case CT_SpinBox:
            if(!opts.unifySpin)
                newSize.rheight() -= ((1 - newSize.rheight()) & 1);
            break;
        case CT_ToolButton:
        {
            newSize = QSize(size.width()+8, size.height()+8);
            // -- from kstyle & oxygen --
            // We want to avoid super-skiny buttons, for things like "up" when icons + text
            // For this, we would like to make width >= height.
            // However, once we get here, QToolButton may have already put in the menu area
            // (PM_MenuButtonIndicator) into the width. So we may have to take it out, fix things
            // up, and add it back in. So much for class-independent rendering...
            int menuAreaWidth(0);

            if (auto tbOpt = styleOptCast<QStyleOptionToolButton>(option)) {
                // Make Kate/KWrite's option toolbuton have the same size as the next/prev buttons...
                if (widget && !getToolBar(widget) && !tbOpt->text.isEmpty() &&
                   tbOpt->features & QStyleOptionToolButton::MenuButtonPopup) {
                    QStyleOptionButton btn;

                    btn.init(widget);
                    btn.text=tbOpt->text;
                    btn.icon=tbOpt->icon;
                    btn.iconSize=tbOpt->iconSize;
                    btn.features=tbOpt->features&QStyleOptionToolButton::MenuButtonPopup
                                    ? QStyleOptionButton::HasMenu : QStyleOptionButton::None;
                    return sizeFromContents(CT_PushButton, &btn, size, widget);
                }

                if (!tbOpt->icon.isNull() && !tbOpt->text.isEmpty() && Qt::ToolButtonTextUnderIcon==tbOpt->toolButtonStyle)
                    newSize.setHeight(newSize.height()-4);

                if (tbOpt->features & QStyleOptionToolButton::MenuButtonPopup)
                    menuAreaWidth = pixelMetric(QStyle::PM_MenuButtonIndicator,
                                                option, widget);
                else if (tbOpt->features & QStyleOptionToolButton::HasMenu)
                    switch(tbOpt->toolButtonStyle)
                    {
                        case Qt::ToolButtonIconOnly:
                            newSize.setWidth(newSize.width()+LARGE_ARR_WIDTH+2);
                            break;
                        case Qt::ToolButtonTextBesideIcon:
                            newSize.setWidth(newSize.width()+3);
                            break;
                        case Qt::ToolButtonTextOnly:
                            newSize.setWidth(newSize.width()+8);
                            break;
                        case Qt::ToolButtonTextUnderIcon:
                            newSize.setWidth(newSize.width()+8);
                            break;
                        default:
                            break;
                    }
            }

            newSize.setWidth(newSize.width() - menuAreaWidth);
            if (newSize.width() < newSize.height())
                newSize.setWidth(newSize.height());
            newSize.setWidth(newSize.width() + menuAreaWidth);

            break;
        }
        case CT_ComboBox:
        {
            newSize=size;
            newSize.setWidth(newSize.width()+4);

            auto combo = styleOptCast<QStyleOptionComboBox>(option);

            int  margin = (pixelMetric(PM_ButtonMargin, option, widget)+
                              (pixelMetric(PM_DefaultFrameWidth, option, widget) * 2))-MAX_ROUND_BTN_PAD,
                 textMargins = 2*(pixelMetric(PM_FocusFrameHMargin) + 1),
                 // QItemDelegate::sizeHint expands the textMargins two times, thus the 2*textMargins...
                 other = qMax(opts.buttonEffect != EFFECT_NONE ? 20 : 18, 2*textMargins + pixelMetric(QStyle::PM_ScrollBarExtent, option, widget));
            bool editable=combo ? combo->editable : false;
            newSize+=QSize(margin+other, margin-2);
            newSize.rheight() += ((1 - newSize.rheight()) & 1);

            if (!opts.etchEntry && opts.buttonEffect != EFFECT_NONE && editable)
                newSize.rheight()-=2;
            // KWord's zoom combo clips 'Fit Page Width' without the following...
            if(editable)
                newSize.rwidth()+=6;
            break;
        }
        case CT_MenuItem:
            if (auto mi = styleOptCast<QStyleOptionMenuItem>(option)) {
                // Taken from QWindowStyle...
                int w = size.width();

                if (QStyleOptionMenuItem::Separator==mi->menuItemType)
                    newSize = QSize(10, windowsSepHeight);
                else if (mi->icon.isNull())
                {
                    newSize.setHeight(newSize.height() - 2);
                    w -= 6;
                }

                if (QStyleOptionMenuItem::Separator!=mi->menuItemType && !mi->icon.isNull())
                {
                    int iconExtent = pixelMetric(PM_SmallIconSize, option, widget);
                    newSize.setHeight(qMax(newSize.height(),
                                  mi->icon.actualSize(QSize(iconExtent, iconExtent)).height()
                                  + 2 * windowsItemFrame));
                }
                int maxpmw = mi->maxIconWidth,
                    tabSpacing = 20;

                if (!opts.buttonStyleMenuSections && QStyleOptionMenuItem::Separator == mi->menuItemType && !mi->text.isEmpty())
                {
                    QFont fontBold = mi->font;
                    fontBold.setBold(true);
                    QFontMetrics fmBold(fontBold);
                    // _set_ w, it will have been initialised to something inappropriately small
                    w = fmBold.width(mi->text);
                }
                else if (mi->text.contains(QLatin1Char('\t')))
                    w += tabSpacing;
                else if (mi->menuItemType == QStyleOptionMenuItem::SubMenu)
                    w += 2 * windowsArrowHMargin;
                else if (mi->menuItemType == QStyleOptionMenuItem::DefaultItem)
                {
                    // adjust the font and add the difference in size.
                    // it would be better if the font could be adjusted in the initStyleOption qmenu func!!
                    QFontMetrics fm(mi->font);
                    QFont fontBold = mi->font;
                    fontBold.setBold(true);
                    QFontMetrics fmBold(fontBold);
                    w += fmBold.width(mi->text) - fm.width(mi->text);
                }

                if (QStyleOptionMenuItem::Separator != mi->menuItemType || opts.buttonStyleMenuSections)
                {
                    int checkcol = qMax<int>(maxpmw, windowsCheckMarkWidth); // Windows always shows a check column
                    w += checkcol + windowsRightBorder;
                }
                else
                {
                    w += 5;
                }
                w += 10;
                newSize.setWidth(w);
                // ....

                int h(newSize.height()-8); // Fix mainly for Qt4.4

                if (QStyleOptionMenuItem::Separator==mi->menuItemType && mi->text.isEmpty())
                    h = 7;
                else
                {
                    h = qMax(h, mi->fontMetrics.height());
                    if (!mi->icon.isNull())
                        h = qMax(h, mi->icon.pixmap(pixelMetric(PM_SmallIconSize), QIcon::Normal).height());

                    if (h < 18)
                        h = 18;
                    h+=((opts.thin&THIN_MENU_ITEMS) ? 2 : 4);

                    if(QStyleOptionMenuItem::Separator==mi->menuItemType)
                        h+=4;
                }

                newSize.setHeight(h);
                // Gtk2's icon->text spacing is 2 pixels smaller - so adjust here...
                newSize.setWidth(newSize.width()-2);
            }
            break;
        case CT_MenuBarItem:
            if (!size.isEmpty())
                newSize = size + QSize(windowsItemHMargin * 4 + 2,
                                       windowsItemVMargin + 1);
            break;
        default:
            break;
    }

    return newSize;
}

QRect Style::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    prePolish(widget);
    QRect rect;
    switch (element)
    {
        case SE_SliderFocusRect:
        case SE_ToolBoxTabContents:
            return visualRect(option->direction, option->rect, option->rect);
        case SE_DockWidgetTitleBarText: {
            auto v2= styleOptCast<QStyleOptionDockWidgetV2>(option);
            bool verticalTitleBar = v2 ? v2->verticalTitleBar : false;
            int m = pixelMetric(PM_DockWidgetTitleMargin, option, widget);

            rect = BaseStyle::subElementRect(element, option, widget);

            if (verticalTitleBar)
                rect.adjust(0, 0, 0, -m);
            else if (Qt::LeftToRight==option->direction )
                rect.adjust(m, 0, 0, 0);
            else
                rect.adjust(0, 0, -m, 0);
            return rect;
        }
        case SE_TabBarTabLeftButton:
            return BaseStyle::subElementRect(element, option, widget).translated(-2, -1);
        case SE_TabBarTabRightButton:
            return BaseStyle::subElementRect(element, option, widget).translated(2, -1);
        case SE_TabBarTabText:
            if (auto tab = styleOptCast<QStyleOptionTab>(option)) {
                QStyleOptionTabV3 tabV2(*tab);
                bool              verticalTabs=QTabBar::RoundedEast==tabV2.shape || QTabBar::RoundedWest==tabV2.shape ||
                                               QTabBar::TriangularEast==tabV2.shape || QTabBar::TriangularWest==tabV2.shape;

                rect=tabV2.rect;
                if (verticalTabs)
                    rect.setRect(0, 0, rect.height(), rect.width());
                int verticalShift = pixelMetric(QStyle::PM_TabBarTabShiftVertical, tab, widget),
                    horizontalShift = pixelMetric(QStyle::PM_TabBarTabShiftHorizontal, tab, widget);
                if (tabV2.shape == QTabBar::RoundedSouth || tabV2.shape == QTabBar::TriangularSouth)
                    verticalShift = -verticalShift;
                rect.adjust(0, 0, horizontalShift, verticalShift);
                bool selected = tabV2.state & State_Selected;
                if (selected)
                {
                    rect.setBottom(rect.bottom() - verticalShift);
                    rect.setRight(rect.right() - horizontalShift);
                }

                // left widget
                if(opts.centerTabText)
                {
                    if (!tabV2.leftButtonSize.isEmpty())  // left widget
                        rect.setLeft(rect.left() + constTabPad +
                                     (verticalTabs ? tabV2.leftButtonSize.height() : tabV2.leftButtonSize.width()));
                    if (!tabV2.rightButtonSize.isEmpty()) // right widget
                        rect.setRight(rect.right() - constTabPad -
                                     (verticalTabs ? tabV2.rightButtonSize.height() : tabV2.rightButtonSize.width()));
                }
                else
                {
                    if (tabV2.leftButtonSize.isNull())
                        rect.setLeft(rect.left()+constTabPad);
                    else if(tabV2.leftButtonSize.width()>0)
                        rect.setLeft(rect.left() + constTabPad + 2 +
                                    (verticalTabs ? tabV2.leftButtonSize.height() : tabV2.leftButtonSize.width()));
                    else if(tabV2.icon.isNull())
                        rect.setLeft(rect.left()+constTabPad);
                    else
                        rect.setLeft(rect.left() + 2);
                }

                // icon
                if (!tabV2.icon.isNull())
                {
                    QSize iconSize = tabV2.iconSize;
                    if (!iconSize.isValid())
                    {
                        int iconExtent = pixelMetric(PM_SmallIconSize);
                        iconSize = QSize(iconExtent, iconExtent);
                    }
                    QSize tabIconSize = tabV2.icon.actualSize(iconSize,
                                                            (tabV2.state & State_Enabled) ? QIcon::Normal
                                                            : QIcon::Disabled);
                    int offset = 4;

                    if (!opts.centerTabText && tabV2.leftButtonSize.isNull())
                        offset += 2;

                    QRect iconRect = QRect(rect.left() + offset, rect.center().y() - tabIconSize.height() / 2,
                                           tabIconSize.width(), tabIconSize .height());
                    if (!verticalTabs)
                        iconRect = visualRect(option->direction, option->rect, iconRect);
                    rect.setLeft(rect.left() + tabIconSize.width() + offset + 2);
                }

                // right widget
                if (!opts.centerTabText && !tabV2.rightButtonSize.isNull() && tabV2.rightButtonSize.width()>0)
                    rect.setRight(rect.right() - constTabPad - 2 -
                                  (verticalTabs ? tabV2.rightButtonSize.height() : tabV2.rightButtonSize.width()));
                else
                    rect.setRight(rect.right() - constTabPad);


                if (!verticalTabs)
                    rect = visualRect(option->direction, option->rect, rect);
                return rect;
            }
            break;
        case SE_RadioButtonIndicator:
            rect = visualRect(option->direction, option->rect,
                              BaseStyle::subElementRect(element, option, widget)).adjusted(0, 0, 1, 1);
            break;
        case SE_ProgressBarContents:
            return (opts.fillProgress ? opts.buttonEffect != EFFECT_NONE &&
                    opts.borderProgress ? option->rect.adjusted(1, 1, -1, -1) :
                    option->rect : opts.buttonEffect != EFFECT_NONE &&
                    opts.borderProgress ? option->rect.adjusted(3, 3, -3, -3) :
                    option->rect.adjusted(2, 2, -2, -2));
        case SE_ProgressBarGroove:
        case SE_ProgressBarLabel:
            return option->rect;
        case SE_GroupBoxLayoutItem:
            rect = option->rect;
//             if (auto groupBoxOpt = styleOptCast<QStyleOptionGroupBox>(option))
//                 if (groupBoxOpt->subControls & (SC_GroupBoxCheckBox | SC_GroupBoxLabel))
//                     rect.setTop(rect.top() + 2);    // eat the top margin a little bit
            break;
        case SE_PushButtonFocusRect:
            if (oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED)) {
                rect = subElementRect(SE_PushButtonContents, option, widget);
                if (opts.buttonEffect != EFFECT_NONE) {
                    rect.adjust(-1, -1, 1, 1);
                } else {
                    rect.adjust(-2, -2, 2, 2);
                }
            } else {
                rect = BaseStyle::subElementRect(element, option, widget);
                if (opts.buttonEffect != EFFECT_NONE) {
                    rect.adjust(1, 1, -1, -1);
                }
            }
            return rect;
        default:
            return BaseStyle::subElementRect(element, option, widget);
    }

    return visualRect(option->direction, option->rect, rect);
}

QRect Style::subControlRect(ComplexControl control, const QStyleOptionComplex *option, SubControl subControl, const QWidget *widget) const
{
    prePolish(widget);
    QRect r(option->rect);
    bool  reverse(Qt::RightToLeft==option->direction);

    switch (control)
    {
        case CC_ComboBox:
            if (auto comboBox = styleOptCast<QStyleOptionComboBox>(option))
            {
                bool ed(comboBox->editable),
                     doEtch((!ed || opts.etchEntry) &&
                            opts.buttonEffect != EFFECT_NONE);
                int  x(r.x()),
                     y(r.y()),
                     w(r.width()),
                     h(r.height());

                switch (subControl)
                {
                    case SC_ComboBoxFrame:
                        if(ed)
                        {
                            int btnWidth(doEtch ? 22 : 20);

                            r=QRect(x+w-btnWidth, y, btnWidth, h);
                        }
                        break;
                    case SC_ComboBoxArrow:
                    {
                        int bmarg(comboBox->frame ? 2 : 0);

                        r.setRect(x + w - bmarg - (doEtch ? 17 : 16), y + bmarg, 16, h - 2*bmarg);
                        if(ed && opts.unifyCombo)
                            r.adjust(-1, 0, 0, 0);
                        break;
                    }
                    case SC_ComboBoxEditField:
                    {
                        int margin(comboBox->frame ? 3 : 0);

                        r.setRect(x + margin+(opts.unifyCombo ? 0 : 2), y + margin,
                                  w - 2 * margin - (opts.unifyCombo ? 15 : 23), h - 2 * margin);
                        if(doEtch)
                            r.adjust(ed ? 0 : 1, 1, ed ? 0 : -1, -1);
                        if(ed)
                            r.adjust(-1, -2, 1, 2);
                        break;
                    }
                    case SC_ComboBoxListBoxPopup:
                    default:
                        break;
                }
                return visualRect(comboBox->direction, comboBox->rect, r);
            }
            break;
        case CC_SpinBox:
            if (auto spinbox = styleOptCast<QStyleOptionSpinBox>(option)) {
                int   fw(spinbox->frame ? pixelMetric(PM_SpinBoxFrameWidth, spinbox, widget) : 0);
                QSize bs;

                bs.setHeight(r.height()>>1);
                if(bs.height()< 8)
                    bs.setHeight(8);
                bs.setWidth(opts.buttonEffect != EFFECT_NONE && opts.etchEntry ?
                            16 : 15);
                bs=bs.expandedTo(QApplication::globalStrut());

                int y(0), x(reverse ? 0 : r.width()-bs.width());

                switch(subControl)
                {
                    case SC_SpinBoxUp:
                        return QAbstractSpinBox::NoButtons==spinbox->buttonSymbols
                                    ? QRect()
                                    : QRect(x, y, bs.width(), bs.height());
                    case SC_SpinBoxDown:
                        if(QAbstractSpinBox::NoButtons==spinbox->buttonSymbols)
                            return QRect();
                        else
                            return QRect(x, y+bs.height(), bs.width(), bs.height()+(bs.height()*2==r.height() ? 0 : 1));
                    case SC_SpinBoxEditField:
                    {
                        int pad=opts.round>ROUND_FULL ? 2 : 0;

                        if (QAbstractSpinBox::NoButtons==spinbox->buttonSymbols)
                            return QRect(fw, fw, (x-fw*2)-pad, r.height()-2*fw);
                        else
                            return QRect(fw+(reverse ? bs.width() : 0), fw, (x-fw*2)-pad, r.height()-2*fw);
                    }
                    case SC_SpinBoxFrame:
                    default:
                        return visualRect(spinbox->direction, spinbox->rect, spinbox->rect);
                }
            }
            break;
        case CC_ScrollBar:
            if (auto scrollBar = styleOptCast<QStyleOptionSlider>(option)) {
                // Taken from kstyle.cpp (KDE 3) , and modified so as to allow for no scrollbar butttons...
                bool  threeButtonScrollBar(SCROLLBAR_KDE==opts.scrollbarType),
                      platinumScrollBar(SCROLLBAR_PLATINUM==opts.scrollbarType),
                      nextScrollBar(SCROLLBAR_NEXT==opts.scrollbarType),
                      noButtons(SCROLLBAR_NONE==opts.scrollbarType);
                QRect ret;
                bool  horizontal(Qt::Horizontal==scrollBar->orientation);
                int   sbextent(pixelMetric(PM_ScrollBarExtent, scrollBar, widget)),
                      sliderMaxLength(((scrollBar->orientation == Qt::Horizontal) ?
                                      scrollBar->rect.width() : scrollBar->rect.height()) - (sbextent * qtcScrollbarButtonNum(opts.scrollbarType))),
                      sliderMinLength(pixelMetric(PM_ScrollBarSliderMin, scrollBar, widget)),
                      sliderLength;

                if (scrollBar->maximum != scrollBar->minimum)
                {
                    uint valueRange = scrollBar->maximum - scrollBar->minimum;
                    sliderLength = (scrollBar->pageStep * sliderMaxLength) / (valueRange + scrollBar->pageStep);

                    if (sliderLength < sliderMinLength || (!isOOWidget(widget) && valueRange > INT_MAX / 2))
                        sliderLength = sliderMinLength;
                    if (sliderLength > sliderMaxLength)
                        sliderLength = sliderMaxLength;
                }
                else
                    sliderLength = sliderMaxLength;

                int sliderstart(sliderPositionFromValue(scrollBar->minimum,
                                                        scrollBar->maximum,
                                                        scrollBar->sliderPosition,
                                                        sliderMaxLength - sliderLength,
                                                        scrollBar->upsideDown));

                switch(opts.scrollbarType)
                {
                    case SCROLLBAR_KDE:
                    case SCROLLBAR_WINDOWS:
                        sliderstart+=sbextent;
                        break;
                    case SCROLLBAR_NEXT:
                        sliderstart+=sbextent*2;
                    default:
                        break;
                }

                // Subcontrols
                switch(subControl)
                {
                    case SC_ScrollBarSubLine:
                        if(noButtons)
                            return QRect();

                        // top/left button
                        if (platinumScrollBar)
                            if (horizontal)
                                ret.setRect(scrollBar->rect.width() - 2 * sbextent, 0, sbextent, sbextent);
                            else
                                ret.setRect(0, scrollBar->rect.height() - 2 * sbextent, sbextent, sbextent);
                        else if(threeButtonScrollBar)
                            if (horizontal)
                                ret.setRect(0, 0, scrollBar->rect.width() - sbextent +1, sbextent);
                            else
                                ret.setRect(0, 0, sbextent, scrollBar->rect.height() - sbextent +1);
                        else
                            ret.setRect(0, 0, sbextent, sbextent);
                        break;
                    case SB_SUB2:
                        if(threeButtonScrollBar)
                            if (horizontal)
                                if(reverse)
                                    ret.setRect(sbextent, 0, sbextent, sbextent);
                                else
                                    ret.setRect(scrollBar->rect.width() - 2 * sbextent, 0, sbextent, sbextent);
                            else
                                ret.setRect(0, scrollBar->rect.height() - 2 * sbextent, sbextent, sbextent);
                        else
                            return QRect();
                        break;
                    case SC_ScrollBarAddLine:
                        if(noButtons)
                            return QRect();

                        // bottom/right button
                        if (nextScrollBar)
                            if (horizontal)
                                ret.setRect(sbextent, 0, sbextent, sbextent);
                            else
                                ret.setRect(0, sbextent, sbextent, sbextent);
                        else
                            if (horizontal)
                                ret.setRect(scrollBar->rect.width() - sbextent, 0, sbextent, sbextent);
                            else
                                ret.setRect(0, scrollBar->rect.height() - sbextent, sbextent, sbextent);
                        break;
                    case SC_ScrollBarSubPage:
                        // between top/left button and slider
                        if (platinumScrollBar)
                            if (horizontal)
                                ret.setRect(0, 0, sliderstart, sbextent);
                            else
                                ret.setRect(0, 0, sbextent, sliderstart);
                        else if (nextScrollBar)
                            if (horizontal)
                                ret.setRect(sbextent*2, 0, sliderstart-2*sbextent, sbextent);
                            else
                                ret.setRect(0, sbextent*2, sbextent, sliderstart-2*sbextent);
                        else
                            if (horizontal)
                                ret.setRect(noButtons ? 0 : sbextent, 0,
                                            noButtons ? sliderstart
                                                    : (sliderstart - sbextent), sbextent);
                            else
                                ret.setRect(0, noButtons ? 0 : sbextent, sbextent,
                                            noButtons ? sliderstart : (sliderstart - sbextent));
                        break;
                    case SC_ScrollBarAddPage:
                    {
                        // between bottom/right button and slider
                        int fudge;

                        if (platinumScrollBar)
                            fudge = 0;
                        else if (nextScrollBar)
                            fudge = 2*sbextent;
                        else if(noButtons)
                            fudge = 0;
                        else
                            fudge = sbextent;

                        if (horizontal)
                            ret.setRect(sliderstart + sliderLength, 0,
                                        sliderMaxLength - sliderstart - sliderLength + fudge, sbextent);
                        else
                            ret.setRect(0, sliderstart + sliderLength, sbextent,
                                        sliderMaxLength - sliderstart - sliderLength + fudge);
                        break;
                    }
                    case SC_ScrollBarGroove:
                        if(noButtons)
                        {
                            if (horizontal)
                                ret=QRect(0, 0, scrollBar->rect.width(), scrollBar->rect.height());
                            else
                                ret=QRect(0, 0, scrollBar->rect.width(), scrollBar->rect.height());
                        }
                        else
                        {
                            int multi = threeButtonScrollBar ? 3 : 2,
                                fudge;

                            if (platinumScrollBar)
                                fudge = 0;
                            else if (nextScrollBar)
                                fudge = 2*sbextent;
                            else
                                fudge = sbextent;

                            if (horizontal)
                                ret=QRect(fudge, 0, scrollBar->rect.width() - sbextent * multi, scrollBar->rect.height());
                            else
                                ret=QRect(0, fudge, scrollBar->rect.width(), scrollBar->rect.height() - sbextent * multi);
                        }
                        break;
                    case SC_ScrollBarSlider:
                        if (horizontal)
                            ret=QRect(sliderstart, 0, sliderLength, sbextent);
                        else
                            ret=QRect(0, sliderstart, sbextent, sliderLength);
                        break;
                    default:
                        ret = BaseStyle::subControlRect(control, option, subControl, widget);
                        break;
                }
                return visualRect(scrollBar->direction/*Qt::LeftToRight*/, scrollBar->rect, ret);
            }
            break;
        case CC_Slider:
            if (auto slider = styleOptCast<QStyleOptionSlider>(option)) {
                if(SLIDER_TRIANGULAR==opts.sliderStyle)
                {
                    int   tickSize(pixelMetric(PM_SliderTickmarkOffset, option, widget)),
                          mod=MO_GLOW==opts.coloredMouseOver && opts.buttonEffect != EFFECT_NONE ? 2 : 0;
                    QRect rect(BaseStyle::subControlRect(control, option, subControl, widget));

                    switch (subControl)
                    {
                        case SC_SliderHandle:
                            if (slider->orientation == Qt::Horizontal)
                            {
                                rect.setWidth(11+mod);
                                rect.setHeight(15+mod);
                                int centerY(r.center().y() - rect.height() / 2);
                                if (slider->tickPosition & QSlider::TicksAbove)
                                    centerY += tickSize;
                                if (slider->tickPosition & QSlider::TicksBelow)
                                    centerY -= (tickSize-1);
                                rect.moveTop(centerY);
                            }
                            else
                            {
                                rect.setWidth(15+mod);
                                rect.setHeight(11+mod);
                                int centerX(r.center().x() - rect.width() / 2);
                                if (slider->tickPosition & QSlider::TicksAbove)
                                    centerX += tickSize;
                                if (slider->tickPosition & QSlider::TicksBelow)
                                    centerX -= (tickSize-1);
                                rect.moveLeft(centerX);
                            }
                            break;
                        case SC_SliderGroove:
                        {
                            QPoint grooveCenter(r.center());

                            if (Qt::Horizontal==slider->orientation)
                            {
                                rect.setHeight(13);
                                --grooveCenter.ry();
                                if (slider->tickPosition & QSlider::TicksAbove)
                                    grooveCenter.ry() += (tickSize+2);
                                if (slider->tickPosition & QSlider::TicksBelow)
                                    grooveCenter.ry() -= (tickSize-1);
                            }
                            else
                            {
                                rect.setWidth(13);
                                --grooveCenter.rx();
                                if (slider->tickPosition & QSlider::TicksAbove)
                                    grooveCenter.rx() += (tickSize+2);
                                if (slider->tickPosition & QSlider::TicksBelow)
                                    grooveCenter.rx() -= (tickSize-1);
                            }
                            rect.moveCenter(grooveCenter);
                            break;
                        }
                        default:
                            break;
                    }
                    return rect;
                }
                else
                {
                    bool horizontal(Qt::Horizontal==slider->orientation);
                    int  thickness(pixelMetric(PM_SliderControlThickness, slider, widget)),
                         tickOffset(slider->tickPosition&QSlider::TicksAbove ||
                                    slider->tickPosition&QSlider::TicksBelow
                                        ? pixelMetric(PM_SliderTickmarkOffset, slider, widget)
                                        : ((horizontal ? r.height() : r.width()) - thickness)/2);

                    switch (subControl)
                    {
                        case SC_SliderHandle:
                        {
                            int len(pixelMetric(PM_SliderLength, slider, widget)),
                                sliderPos(sliderPositionFromValue(slider->minimum, slider->maximum,
                                                                slider->sliderPosition,
                                                                (horizontal ? r.width()
                                                                            : r.height()) - len,
                                                                slider->upsideDown));

                            if (horizontal)
                                r.setRect(r.x() + sliderPos, r.y() + tickOffset, len, thickness);
                            else
                                r.setRect(r.x() + tickOffset, r.y() + sliderPos, thickness, len);
                            break;
                        }
                        case SC_SliderGroove:
                            if (horizontal)
                                r.setRect(r.x(), r.y() + tickOffset, r.width(), thickness);
                            else
                                r.setRect(r.x() + tickOffset, r.y(), thickness, r.height());
                            break;
                        default:
                            break;
                    }
                    return visualRect(slider->direction, r, r);
                }
            }
            break;
        case CC_GroupBox:
            if (oneOf(subControl, SC_GroupBoxCheckBox, SC_GroupBoxLabel))
                if (auto groupBox = styleOptCast<QStyleOptionGroupBox>(option)) {
                    QFont font(widget ? widget->font() : QApplication::font());

                    font.setBold(opts.gbLabel&GB_LBL_BOLD);

                    QFontMetrics fontMetrics(font);
                    int          h(fontMetrics.height()),
                                 tw(fontMetrics.size(Qt::TextShowMnemonic, groupBox->text + QLatin1Char(' ')).width()),
                                 marg((groupBox->features & QStyleOptionFrameV2::Flat) ||
                                       qtcNoFrame(opts.groupBox) || opts.gbLabel&GB_LBL_OUTSIDE
                                            ? 0
                                            : opts.gbLabel&GB_LBL_INSIDE
                                                ? 2
                                                : 6),
                                 indicatorWidth(pixelMetric(PM_IndicatorWidth, option, widget)),
                                 indicatorSpace(pixelMetric(PM_CheckBoxLabelSpacing, option, widget) - 1);
                    bool         hasCheckBox(groupBox->subControls & QStyle::SC_GroupBoxCheckBox);
                    int          checkBoxSize(hasCheckBox ? (indicatorWidth + indicatorSpace) : 0),
                                 checkAdjust(qtcNoFrame(opts.groupBox) || opts.gbLabel&GB_LBL_OUTSIDE ? 0 : 2);

                    if(0==checkAdjust)
                        checkBoxSize-=2;

                    r.adjust(marg, 0, -marg, 0);
                    if(!qtcNoFrame(opts.groupBox) &&
                       opts.gbLabel & GB_LBL_INSIDE) {
                        r.adjust(0, 2, 0, 2);
                    }
                    r.setHeight(h);

                    // Adjusted rect for label + indicatorWidth + indicatorSpace
                    Qt::Alignment align(groupBox->textAlignment);
                    if(opts.gbLabel&GB_LBL_CENTRED)
                    {
                        align&=~(Qt::AlignLeft|Qt::AlignRight);
                        align|=Qt::AlignHCenter;
                    }
                    r=alignedRect(groupBox->direction, align, QSize(tw + checkBoxSize, h), r);

                    // Adjust totalRect if checkbox is set
                    if (hasCheckBox)
                    {
                        if (SC_GroupBoxCheckBox==subControl) // Adjust for check box
                        {
                            int indicatorHeight(pixelMetric(PM_IndicatorHeight, option, widget)),
                                top(r.top() + (fontMetrics.height() - indicatorHeight) / 2);

                            r.setRect(reverse ? (r.right() - indicatorWidth) : r.left()+checkAdjust, top, indicatorWidth, indicatorHeight);
                        }
                        else // Adjust for label
                            r.setRect(reverse ? r.left() : (r.left() + checkBoxSize), r.top(), r.width() - checkBoxSize, r.height());
                    }
                    return r;
                }
        break;
    case CC_TitleBar:
        if (auto tb = styleOptCast<QStyleOptionTitleBar>(option)) {
            bool isMinimized(tb->titleBarState&Qt::WindowMinimized),
                 isMaximized(tb->titleBarState&Qt::WindowMaximized);

            if((isMaximized && SC_TitleBarMaxButton==subControl) ||
                (isMinimized && SC_TitleBarMinButton==subControl) ||
                (isMinimized && SC_TitleBarShadeButton==subControl) ||
                (!isMinimized && SC_TitleBarUnshadeButton==subControl))
                return QRect();

            readMdiPositions();

            const int controlSize(tb->rect.height() - constWindowMargin *2);
            int sc = (SC_TitleBarUnshadeButton == subControl ?
                      SC_TitleBarShadeButton :
                      SC_TitleBarNormalButton == subControl ? isMaximized ?
                      SC_TitleBarMaxButton : SC_TitleBarMinButton :
                      subControl);
            int pos = 0;
            int totalLeft = 0;
            int totalRight = 0;
            bool rhs = false;
            bool found = false;

            foreach (int hint, m_mdiButtons[0]) {
                if (hint == SC_TitleBarCloseButton ||
                    hint == WINDOWTITLE_SPACER ||
                    tb->titleBarFlags & toHint(hint)) {
                    totalLeft += (WINDOWTITLE_SPACER == hint ?
                                  controlSize / 2 : controlSize);
                    if (hint == sc) {
                        found = true;
                    } else if (!found) {
                        pos += (WINDOWTITLE_SPACER == hint ? controlSize / 2 :
                                controlSize);
                    }
                }
            }
            if (!found) {
                pos = 0;
                rhs = true;
            }

            foreach (int hint, m_mdiButtons[1]) {
                if (hint == SC_TitleBarCloseButton ||
                    hint == WINDOWTITLE_SPACER ||
                    tb->titleBarFlags & toHint(hint)) {
                    if (hint != WINDOWTITLE_SPACER || totalRight)
                        totalRight += (hint == WINDOWTITLE_SPACER ?
                                       controlSize / 2 : controlSize);
                    if (rhs) {
                        if (hint == sc) {
                            pos += controlSize;
                            found = true;
                        } else if (found) {
                            pos += (hint == WINDOWTITLE_SPACER ?
                                    controlSize / 2 : controlSize);
                        }
                    }
                }
            }

            totalLeft += constWindowMargin * (totalLeft ? 2 : 1);
            totalRight += constWindowMargin * (totalRight ? 2 : 1);

            if (subControl == SC_TitleBarLabel) {
                r.adjust(totalLeft, 0, -totalRight, 0);
            } else if (!found) {
                return QRect();
            } else if (rhs) {
                r.setRect(r.right() - (pos + constWindowMargin),
                          r.top() + constWindowMargin,
                          controlSize, controlSize);
            } else {
                r.setRect(r.left() + constWindowMargin + pos,
                          r.top() + constWindowMargin,
                          controlSize, controlSize);
            }
            if (r.height() % 2 == 0)
                r.adjust(0, 0, 1, 1);
            return visualRect(tb->direction, tb->rect, r);
        }
    default:
        break;
    }
    return BaseStyle::subControlRect(control, option, subControl, widget);
}

QStyle::SubControl
Style::hitTestComplexControl(ComplexControl control,
                             const QStyleOptionComplex *option,
                             const QPoint &pos, const QWidget *widget) const
{
    prePolish(widget);
    m_sbWidget = 0L;
    switch (control) {
        case CC_ScrollBar:
            if (auto scrollBar = styleOptCast<QStyleOptionSlider>(option)) {
                if (subControlRect(control, scrollBar, SC_ScrollBarSlider, widget).contains(pos))
                    return SC_ScrollBarSlider;

                if (subControlRect(control, scrollBar, SC_ScrollBarAddLine, widget).contains(pos))
                    return SC_ScrollBarAddLine;

                if (subControlRect(control, scrollBar, SC_ScrollBarSubPage, widget).contains(pos))
                    return SC_ScrollBarSubPage;

                if (subControlRect(control, scrollBar, SC_ScrollBarAddPage, widget).contains(pos))
                    return SC_ScrollBarAddPage;

                if (subControlRect(control, scrollBar, SC_ScrollBarSubLine, widget).contains(pos))
                {
                    if (SCROLLBAR_KDE==opts.scrollbarType && subControlRect(control, scrollBar, SB_SUB2, widget).contains(pos))
                        m_sbWidget=widget;
                    return SC_ScrollBarSubLine;
                }
            }
        default:
            break;
    }

    return BaseStyle::hitTestComplexControl(control, option,  pos, widget);
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
        drawLightBevel(painter, r2, &opt, widget, ROUNDED_NONE, getFill(&opt, use), use, false, WIDGET_MENU_ITEM);
    }
    else
        painter->fillRect(r2, palette.background().color());

    if(opt.state&State_MouseOver && opts.coloredMouseOver)
    {
        r2=r;
        if(MO_PLASTIK==opts.coloredMouseOver)
            if(horiz)
                r2.adjust(0, 1, 0, -1);
            else
                r2.adjust(1, 0, -1, 0);
        else
            r2.adjust(1, 1, -1, -1);

        if(MO_GLOW==opts.coloredMouseOver)
        {
            QColor col(m_mouseOverCols[opt.state&State_On ? 0 : 1]);

            col.setAlphaF(GLOW_ALPHA(false));
            painter->setPen(col);
            drawRect(painter, r);
            col=m_mouseOverCols[opt.state&State_On ? 4 : 3];
            col.setAlphaF(0.8);
            painter->setPen(col);
            drawRect(painter, r2);
        }
        else
        {
            painter->setPen(m_mouseOverCols[opt.state&State_On ? 0 : 1]);

            if(horiz || MO_PLASTIK!=opts.coloredMouseOver)
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
    bool aa = p->testRenderHint(QPainter::Antialiasing);
    QPointF start(r.x() + (aa ? 0.5 : 0.0), r.y() + (aa ? 0.5 : 0.0));
    QPointF end(r.x() + (horiz ? r.width() - 1 : 0) + (aa ? 0.5 : 0.0),
                r.y() + (horiz ? 0 : r.height() - 1) + (aa ? 0.5 : 0.0));

    if (opts.fadeLines && (fadeStart || fadeEnd)) {
        QLinearGradient grad(start, end);
        QColor fade(col);

        fade.setAlphaF(0.0);
        grad.setColorAt(0, fadeStart && opts.fadeLines ? fade : col);
        if (fadeSizeStart >= 0 && fadeSizeStart <= 1.0)
            grad.setColorAt(fadeSizeStart, col);
        if (fadeSizeEnd >= 0 && fadeSizeEnd <= 1.0)
            grad.setColorAt(1.0 - fadeSizeEnd, col);
        grad.setColorAt(1, fadeEnd && opts.fadeLines ? fade : col);
        p->setPen(QPen(QBrush(grad), 1));
    } else {
        p->setPen(col);
    }
    p->drawLine(start, end);
}

void Style::drawLines(QPainter *p, const QRect &r, bool horiz, int nLines, int offset, const QColor *cols, int startOffset,
                      int dark, ELine type) const
{
    int  space((nLines*2)+(LINE_DASHES!=type ? (nLines-1) : 0)),
         step(LINE_DASHES!=type ? 3 : 2),
         etchedDisp(LINE_SUNKEN==type ? 1 : 0),
         x(horiz ? r.x() : r.x()+((r.width()-space)>>1)),
         y(horiz ? r.y()+((r.height()-space)>>1) : r.y()),
         x2(r.x()+r.width()-1),
         y2(r.y()+r.height()-1),
         i;
    QPen dp(cols[dark], 1),
         lp(cols[0], 1);

    if(opts.fadeLines && (horiz ? r.width() : r.height())>16)
    {
        QLinearGradient grad(r.topLeft(), horiz ? r.topRight() : r.bottomLeft());
        QColor          fade(cols[dark]);

        fade.setAlphaF(0.0);
        grad.setColorAt(0, fade);
        grad.setColorAt(0.4, cols[dark]);
        grad.setColorAt(0.6, cols[dark]);
        grad.setColorAt(1, fade);

        dp=QPen(QBrush(grad), 1);

        if(LINE_FLAT!=type)
        {
            fade=QColor(cols[0]);

            fade.setAlphaF(0.0);
            grad.setColorAt(0, fade);
            grad.setColorAt(0.4, cols[0]);
            grad.setColorAt(0.6, cols[0]);
            grad.setColorAt(1, fade);
            lp=QPen(QBrush(grad), 1);
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
    p->setRenderHint(QPainter::Antialiasing, false);
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
    if(STRIPE_FADE==opts.stripedProgress && fillRect.width()>4 && fillRect.height()>4)
        addStripes(p, QPainterPath(), fillRect, !vertical);
    p->restore();

    if(!inCache)
        delete pix;
}

void
Style::drawBevelGradient(const QColor &base, QPainter *p, const QRect &origRect,
                         const QPainterPath &path, bool horiz, bool sel,
                         EAppearance bevApp, EWidget w, bool useCache) const
{
    if (origRect.width() < 1 || origRect.height() < 1)
        return;

    if (qtcIsFlat(bevApp)) {
        if (noneOf(w, WIDGET_TAB_TOP, WIDGET_TAB_BOT) ||
            !qtcIsCustomBgnd(opts) || opts.tabBgnd || !sel) {
            if (path.isEmpty())
                p->fillRect(origRect, base);
            else
                p->fillPath(path, base);
        }
    } else {
        bool        tab(WIDGET_TAB_TOP==w || WIDGET_TAB_BOT==w),
                    selected(tab ? false : sel);
        EAppearance app(selected
                            ? opts.sunkenAppearance
                            : WIDGET_LISTVIEW_HEADER==w && APPEARANCE_BEVELLED==bevApp
                                ? APPEARANCE_LV_BEVELLED
                                : APPEARANCE_BEVELLED!=bevApp || WIDGET_BUTTON(w) || WIDGET_LISTVIEW_HEADER==w ||
                                  WIDGET_TROUGH==w || WIDGET_NO_ETCH_BTN==w || WIDGET_MENU_BUTTON==w
                                    ? bevApp
                                    : APPEARANCE_GRADIENT);

        if(WIDGET_PROGRESSBAR==w || !useCache)
            drawBevelGradientReal(base, p, origRect, path, horiz, sel, app, w);
        else
        {
            QRect   r(0, 0, horiz ? PIXMAP_DIMENSION : origRect.width(),
                            horiz ? origRect.height() : PIXMAP_DIMENSION);
            QtcKey  key(createKey(horiz ? r.height() : r.width(), base, horiz, app, w));
            QPixmap *pix(m_pixmapCache.object(key));
            bool    inCache(true);

            if(!pix)
            {
                pix=new QPixmap(r.width(), r.height());
                pix->fill(Qt::transparent);

                QPainter pixPainter(pix);

                drawBevelGradientReal(base, &pixPainter, r, horiz, sel, app, w);
                pixPainter.end();

                int cost(pix->width()*pix->height()*(pix->depth()/8));

                if(cost<m_pixmapCache.maxCost())
                    m_pixmapCache.insert(key, pix, cost);
                else
                    inCache=false;
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

void Style::drawBevelGradientReal(const QColor &base, QPainter *p, const QRect &r, const QPainterPath &path,
                                  bool horiz, bool sel, EAppearance app, EWidget w) const
{
    bool                             topTab(WIDGET_TAB_TOP==w),
                                     botTab(WIDGET_TAB_BOT==w),
                                     dwt(qtcIsCustomBgnd(opts) && WIDGET_DOCK_WIDGET_TITLE==w),
                                     titleBar(opts.windowBorder&WINDOW_BORDER_BLEND_TITLEBAR &&
                                                    (WIDGET_MDI_WINDOW==w || WIDGET_MDI_WINDOW_TITLE==w ||
                                                     (opts.dwtSettings&DWT_COLOR_AS_PER_TITLEBAR &&
                                                                     WIDGET_DOCK_WIDGET_TITLE==w && !dwt))),
                                     reverse(Qt::RightToLeft==QApplication::layoutDirection());
    const Gradient                   *grad=qtcGetGradient(app, &opts);
    QLinearGradient                  g(r.topLeft(), horiz ? r.bottomLeft() : r.topRight());
    GradientStopCont::const_iterator it(grad->stops.begin()),
                                     end(grad->stops.end());
    int                              numStops(grad->stops.size());

    for(int i=0; it!=end; ++it, ++i)
    {
        QColor col;

        if(/*sel && */(topTab || botTab || dwt || titleBar) && i==numStops-1)
        {
            if(titleBar)
            {
                col=m_backgroundCols[ORIGINAL_SHADE];
                //if(APPEARANCE_STRIPED==opts.bgndAppearance)
                    col.setAlphaF(0.0);
            }
            else
            {
                col=base;
                if((sel /*&& qtcIsCustomBgnd(opts)*/ && 0==opts.tabBgnd && !reverse) || dwt)
                    col.setAlphaF(0.0);
            }
        }
        else
            shade(base, &col, botTab && opts.invertBotTab ? qMax(INVERT_SHADE((*it).val), 0.9) : (*it).val);
        if(WIDGET_TOOLTIP!=w && (*it).alpha<1.0)
            col.setAlphaF(col.alphaF()*(*it).alpha);
        g.setColorAt(botTab ? 1.0-(*it).pos : (*it).pos, col);
    }

    if(APPEARANCE_AGUA==app && !(topTab || botTab || dwt) && (horiz ? r.height() : r.width())>AGUA_MAX)
    {
        QColor col;
        double pos=AGUA_MAX/((horiz ? r.height() : r.width())*2.0);
        shade(base, &col, AGUA_MID_SHADE);
        g.setColorAt(pos, col);
        g.setColorAt(1.0-pos, col);
    }

    //p->fillRect(r, base);
    if(path.isEmpty())
        p->fillRect(r, QBrush(g));
    else
        p->fillPath(path, QBrush(g));
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

void Style::drawLightBevel(QPainter *p, const QRect &r, const QStyleOption *option, const QWidget *widget, int round, const QColor &fill,
                           const QColor *custom, bool doBorder, EWidget w) const
{
    bool onToolbar=APPEARANCE_NONE!=opts.tbarBtnAppearance && (WIDGET_TOOLBAR_BUTTON==w || (WIDGET_BUTTON(w) && isOnToolbar(widget)));

    if(WIDGET_PROGRESSBAR==w || WIDGET_SB_BUTTON==w || (WIDGET_SPIN==w && !opts.unifySpin)/* || !m_usePixmapCache*/)
        drawLightBevelReal(p, r, option, widget, round, fill, custom, doBorder, w, true, opts.round, onToolbar);
    else
    {
        static const int constMaxCachePixmap = 128;

        int    endSize=0,
               middleSize=8;
        bool   horiz(CIRCULAR_SLIDER(w) || isHoriz(option, w, TBTN_JOINED==opts.tbarBtns)),
               circular((WIDGET_MDI_WINDOW_BUTTON==w && (opts.titlebarButtons&TITLEBAR_BUTTON_ROUND)) ||
                         WIDGET_RADIO_BUTTON==w || WIDGET_DIAL==w || CIRCULAR_SLIDER(w));
        double radius=0;
        ERound realRound=qtcGetWidgetRound(&opts, r.width(), r.height(), w);

        if(!circular)
        {
            switch(realRound)
            {
                case ROUND_SLIGHT:
                case ROUND_NONE:
                case ROUND_FULL:
                    endSize=SLIDER(w) && MO_PLASTIK==opts.coloredMouseOver && option->state&State_MouseOver ? 9 : 5;
                    break;
                case ROUND_EXTRA:
                    endSize=7;
                    break;
                case ROUND_MAX:
                {
                    radius=qtcGetRadius(&opts, r.width(), r.height(), w, RADIUS_ETCH);
                    endSize=SLIDER(w)
                                ? qMax((opts.sliderWidth/2)+1, (int)(radius+1.5))
                                : (int)(radius+2.5);
                    middleSize=(MIN_ROUND_MAX_WIDTH-(endSize*2))+4;
                    if(middleSize<4)
                        middleSize=4;
                    break;
                }
            }
        }

        int size((2*endSize)+middleSize);

        if(size>constMaxCachePixmap)
            drawLightBevelReal(p, r, option, widget, round, fill, custom, doBorder, w, true, realRound, onToolbar);
        else
        {
            QString key;
            bool    small(circular || (horiz ? r.width() : r.height())<(2*endSize));
            QPixmap pix;
            QSize   pixSize(small ? QSize(r.width(), r.height()) : QSize(horiz ? size : r.width(), horiz ? r.height() : size));
            uint    state(option->state&(State_Raised|State_Sunken|State_On|State_Horizontal|State_HasFocus|State_MouseOver|
                          (WIDGET_MDI_WINDOW_BUTTON==w ? State_Active : State_None)));

            key.sprintf("qtc-%x-%x-%x-%x-%x-%x-%x-%x-%x", w, onToolbar ? 1 : 0, round, (int)realRound, pixSize.width(), pixSize.height(),
                        state, fill.rgba(), (int)(radius*100));
            if(!m_usePixmapCache || !QPixmapCache::find(key, pix))
            {
                pix=QPixmap(pixSize);
                pix.fill(Qt::transparent);

                QPainter pixPainter(&pix);
                ERound   oldRound=opts.round;
                opts.round=realRound;
                drawLightBevelReal(&pixPainter, QRect(0, 0, pix.width(), pix.height()), option, widget, round, fill, custom,
                                   doBorder, w, false, realRound, onToolbar);
                opts.round=oldRound;
                pixPainter.end();

                if(m_usePixmapCache)
                    QPixmapCache::insert(key, pix);
            }

            if(small)
                p->drawPixmap(r.topLeft(), pix);
            else if(horiz)
            {
                int middle(qMin(r.width()-(2*endSize), middleSize));
                if(middle>0)
                    p->drawTiledPixmap(r.x()+endSize, r.y(), r.width()-(2*endSize), pix.height(), pix.copy(endSize, 0, middle, pix.height()));
                p->drawPixmap(r.x(), r.y(), pix.copy(0, 0, endSize, pix.height()));
                p->drawPixmap(r.x()+r.width()-endSize, r.y(), pix.copy(pix.width()-endSize, 0, endSize, pix.height()));
            }
            else
            {
                int middle(qMin(r.height()-(2*endSize), middleSize));
                if(middle>0)
                    p->drawTiledPixmap(r.x(), r.y()+endSize, pix.width(), r.height()-(2*endSize),
                                       pix.copy(0, endSize, pix.width(), middle));
                p->drawPixmap(r.x(), r.y(), pix.copy(0, 0, pix.width(), endSize));
                p->drawPixmap(r.x(), r.y()+r.height()-endSize, pix.copy(0, pix.height()-endSize, pix.width(), endSize));
            }

            if(WIDGET_SB_SLIDER==w && opts.stripedSbar)
            {
                QRect rx(r.adjusted(1, 1, -1, -1));
                addStripes(p, buildPath(rx, WIDGET_SB_SLIDER, realRound, qtcGetRadius(&opts, rx.width()-1, rx.height()-1, WIDGET_SB_SLIDER,
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
    EAppearance app = qtcWidgetApp(onToolbar ? WIDGET_TOOLBAR_BUTTON : w,
                                   &opts, option->state & State_Active);
    QRect r(rOrig);
    bool bevelledButton = ((WIDGET_BUTTON(w) || WIDGET_NO_ETCH_BTN==w || WIDGET_MENU_BUTTON==w) && APPEARANCE_BEVELLED==app),
                 sunken(option->state &(/*State_Down | */State_On | State_Sunken)),
                 flatWidget((WIDGET_MDI_WINDOW_BUTTON==w &&
                              (opts.round==ROUND_MAX || opts.titlebarButtons&TITLEBAR_BUTTON_ROUND)) ||
                              (WIDGET_PROGRESSBAR==w && !opts.borderProgress)),
                 lightBorder(!flatWidget && DRAW_LIGHT_BORDER(sunken, w, app)),
                 draw3dfull(!flatWidget && !lightBorder && DRAW_3D_FULL_BORDER(sunken, app)),
                 draw3d(!flatWidget && (draw3dfull || (
                            !lightBorder && DRAW_3D_BORDER(sunken, app)))),
                 drawShine(DRAW_SHINE(sunken, app)),
                 doColouredMouseOver(doBorder && option->state&State_Enabled &&
                                     WIDGET_MDI_WINDOW_BUTTON!=w &&
                                     WIDGET_SPIN!=w && WIDGET_COMBO_BUTTON!=w && WIDGET_SB_BUTTON!=w &&
                                     (!SLIDER(w) || !opts.colorSliderMouseOver) &&
                                     !(option->state&STATE_KWIN_BUTTON) &&
                                     (opts.coloredTbarMo || !(option->state&STATE_TBAR_BUTTON)) &&
                                     opts.coloredMouseOver && option->state&State_MouseOver &&
                                     WIDGET_PROGRESSBAR!=w &&
                                     (option->state&STATE_TOGGLE_BUTTON || !sunken)),
                 plastikMouseOver(doColouredMouseOver && MO_PLASTIK==opts.coloredMouseOver),
                 colouredMouseOver(doColouredMouseOver && WIDGET_MENU_BUTTON!=w &&
                                       (MO_COLORED==opts.coloredMouseOver ||
                                        MO_COLORED_THICK==opts.coloredMouseOver ||
                                        (MO_GLOW==opts.coloredMouseOver &&
                                         !(opts.buttonEffect != EFFECT_NONE)))),
                 doEtch(doBorder && ETCH_WIDGET(w) &&
                        opts.buttonEffect != EFFECT_NONE),
                 glowFocus(doEtch && USE_GLOW_FOCUS(option->state&State_MouseOver) && option->state&State_HasFocus &&
                           option->state&State_Enabled),
                 horiz(CIRCULAR_SLIDER(w) || isHoriz(option, w, TBTN_JOINED==opts.tbarBtns)),
                 sunkenToggleMo(sunken && !(option->state&State_Sunken) && option->state&(State_MouseOver|STATE_TOGGLE_BUTTON));
    const QColor *cols(custom ? custom : m_backgroundCols),
                 *border(colouredMouseOver ? borderColors(option, cols) : cols);

    p->save();

    if (doEtch)
        r.adjust(1, 1, -1, -1);

    if (w == WIDGET_TROUGH && !opts.borderSbarGroove)
        doBorder=false;

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

                        p->setPen(m_mouseOverCols[MO_PLASTIK_DARK(w)]);
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
                            p->setPen(m_mouseOverCols[MO_PLASTIK_LIGHT(w)]);
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

    if(plastikMouseOver && (!sunken  || sunkenToggleMo))
    {
        bool thin(WIDGET_SB_BUTTON==w || WIDGET_SPIN==w || ((horiz ? r.height() : r.width())<16)),
             horizontal(SLIDER(w) ? !horiz : (horiz && WIDGET_SB_BUTTON!=w)|| (!horiz && WIDGET_SB_BUTTON==w));
        int len = (SLIDER(w) ? sbSliderMOLen(opts, horiz ? r.width() :
                                             r.height()) : (thin ? 1 : 2));

        p->save();
        if(horizontal)
            p->setClipRect(r.x(), r.y()+len, r.width(), r.height()-(len*2));
        else
            p->setClipRect(r.x()+len, r.y(), r.width()-(len*2), r.height());
    }

    if(!colouredMouseOver && lightBorder)
    {
        p->setPen(cols[LIGHT_BORDER(app)]);
        p->drawPath(buildPath(r, w, round, qtcGetRadius(&opts, r.width(), r.height(), w, RADIUS_INTERNAL)));
    }
    else if(colouredMouseOver || (draw3d && option->state&State_Raised))
    {
        QPainterPath innerTlPath,
                     innerBrPath;
        int          dark(/*bevelledButton ? */2/* : 4*/);

        buildSplitPath(r, round, qtcGetRadius(&opts, r.width(), r.height(), w, RADIUS_INTERNAL),
                       innerTlPath, innerBrPath);

        p->setPen(border[colouredMouseOver ? MO_STD_LIGHT(w, sunken) : (sunken ? dark : 0)]);
        p->drawPath(innerTlPath);
        if(colouredMouseOver || bevelledButton || draw3dfull)
        {
            p->setPen(border[colouredMouseOver ? MO_STD_DARK(w) : (sunken ? 0 : dark)]);
            p->drawPath(innerBrPath);
        }
    }
    if(plastikMouseOver && (!sunken  || sunkenToggleMo))
        p->restore();
    p->setRenderHint(QPainter::Antialiasing, false);

    if(doEtch || glowFocus)
    {
        if(!(opts.thin&THIN_FRAMES) && (!sunken || sunkenToggleMo ||
               (sunken && glowFocus && widget && qobject_cast<const QAbstractButton*>(widget) &&
                static_cast<const QAbstractButton*>(widget)->isCheckable())) &&
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
        const QColor *borderCols=glowFocus || ((WIDGET_COMBO==w || WIDGET_MENU_BUTTON==w || (WIDGET_NO_ETCH_BTN==w && ROUNDED_ALL!=round)) &&
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
            ((((doEtch && WIDGET_OTHER!=w && WIDGET_SLIDER_TROUGH!=w) || SLIDER(w) || WIDGET_COMBO==w || WIDGET_MENU_BUTTON==w ) &&
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
    p->setPen(col);
    p->drawPath(buildPath(r, w, ROUNDED_ALL, qtcGetRadius(&opts, r.width(), r.height(), w, RADIUS_ETCH)));
    p->setRenderHint(QPainter::Antialiasing, false);
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
    p->setPen(col);

    if(!raised && WIDGET_SLIDER!=w)
    {
        p->drawPath(tl);
        if(WIDGET_SLIDER_TROUGH==w && opts.thinSbarGroove && widget && qobject_cast<const QScrollBar*>(widget))
        {
            QColor col(Qt::white);
            col.setAlphaF(USE_CUSTOM_ALPHAS(opts) ? opts.customAlphas[ALPHA_ETCH_LIGHT] : ETCH_BOTTOM_ALPHA); // 0.25);
            p->setPen(col);
        }
        else
            p->setPen(getLowerEtchCol(widget));
    }

    p->drawPath(br);
    p->setRenderHint(QPainter::Antialiasing, false);
}

void Style::drawBgndRing(QPainter &painter, int x, int y, int size, int size2, bool isWindow) const
{
    double width=(size-size2)/2.0,
           width2=width/2.0;
    QColor col(Qt::white);

    col.setAlphaF(RINGS_INNER_ALPHA(isWindow ? opts.bgndImage.type : opts.menuBgndImage.type));
    painter.setPen(QPen(col, width));
    painter.drawEllipse(QRectF(x+width2, y+width2, size-width, size-width));

    if(IMG_BORDERED_RINGS==(isWindow ? opts.bgndImage.type : opts.menuBgndImage.type))
    {
        col.setAlphaF(RINGS_OUTER_ALPHA);
        painter.setPen(QPen(col, 1));
        painter.drawEllipse(QRectF(x, y, size, size));
        if(size2)
            painter.drawEllipse(QRectF(x+width, y+width, size2, size2));
    }
}

QPixmap Style::drawStripes(const QColor &color, int opacity) const
{
    QPixmap pix;
    QString key;
    QColor  col(color);

    if(100!=opacity)
        col.setAlphaF(opacity/100.0);

    key.sprintf("qtc-stripes-%x", col.rgba());
    if(!m_usePixmapCache || !QPixmapCache::find(key, pix))
    {
        pix=QPixmap(QSize(64, 64));

        if(100!=opacity)
            pix.fill(Qt::transparent);

        QPainter pixPainter(&pix);
        QColor   col2(shade(col, BGND_STRIPE_SHADE));

        if(100!=opacity)
        {
            col2.setAlphaF(opacity/100.0);
            pixPainter.setPen(col);
            for(int i=0; i<pix.height(); i+=4)
                pixPainter.drawLine(0, i, pix.width()-1, i);
        }
        else
            pixPainter.fillRect(pix.rect(), col);
        pixPainter.setPen(QColor((3*col.red()+col2.red())/4,
                                 (3*col.green()+col2.green())/4,
                                 (3*col.blue()+col2.blue())/4,
                                 100!=opacity ? col2.alpha() : 255));

        for(int i=1; i<pix.height(); i+=4)
        {
            pixPainter.drawLine(0, i, pix.width()-1, i);
            pixPainter.drawLine(0, i+2, pix.width()-1, i+2);
        }
        pixPainter.setPen(col2);
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
    bool isWindow(BGND_MENU!=type);

    if(!qtcIsFlatBgnd(app))
    {
        static const int constPixmapWidth  = 16;
        static const int constPixmapHeight = 512;

        QColor    col(bgnd);
        QPixmap   pix;
        QSize     scaledSize;
        EGradType grad=isWindow ? opts.bgndGrad : opts.menuBgndGrad;

        if(APPEARANCE_STRIPED==app)
            pix=drawStripes(col, opacity);
        else if(APPEARANCE_FILE==app)
            pix=isWindow ? opts.bgndPixmap.img : opts.menuBgndPixmap.img;
        else
        {
            QString key;

            scaledSize=QSize(GT_HORIZ==grad ? constPixmapWidth : r.width(), GT_HORIZ==grad ? r.height() : constPixmapWidth);

            if(100!=opacity)
                col.setAlphaF(opacity/100.0);

            key.sprintf("qtc-bgnd-%x-%d-%d", col.rgba(), grad, app);
            if(!m_usePixmapCache || !QPixmapCache::find(key, pix))
            {
                pix=QPixmap(QSize(GT_HORIZ==grad ? constPixmapWidth : constPixmapHeight, GT_HORIZ==grad ? constPixmapHeight : constPixmapWidth));
                pix.fill(Qt::transparent);

                QPainter pixPainter(&pix);

                drawBevelGradientReal(col, &pixPainter, QRect(0, 0, pix.width(), pix.height()), GT_HORIZ==grad, false, app, WIDGET_OTHER);
                pixPainter.end();
                if(m_usePixmapCache)
                    QPixmapCache::insert(key, pix);
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
            p->fillPath(path, QBrush(oneOf(app, APPEARANCE_STRIPED,
                                           APPEARANCE_FILE) ||
                                     scaledSize == pix.size() ? pix :
                                     pix.scaled(scaledSize,
                                                Qt::IgnoreAspectRatio)));
            p->save();
        }

        if (isWindow && noneOf(app, APPEARANCE_STRIPED, APPEARANCE_FILE) &&
            grad == GT_HORIZ &&
            qtcGetGradient(app, &opts)->border == GB_SHINE) {
            int size = qMin(BGND_SHINE_SIZE, qMin(r.height() * 2, r.width()));
            QString key;
            key.sprintf("qtc-radial-%x", size/BGND_SHINE_STEPS);
            if (!m_usePixmapCache || !QPixmapCache::find(key, pix)) {
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
            p->drawPixmap(r.x() + (r.width() - pix.width()) / 2, r.y(), pix);
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
            if(!img.pixmap.img.isNull())
            {
                switch(img.pos)
                {
                    case PP_TL:
                        p->drawPixmap(r.x(), r.y(), img.pixmap.img);
                        break;
                    case PP_TM:
                        p->drawPixmap(r.x()+((r.width()-img.pixmap.img.width())/2), r.y(), img.pixmap.img);
                        break;
                    default:
                    case PP_TR:
                        p->drawPixmap(r.right()-img.pixmap.img.width(), r.y(), img.pixmap.img);
                        break;
                    case PP_BL:
                        p->drawPixmap(r.x(), r.bottom()-img.pixmap.img.height(), img.pixmap.img);
                        break;
                    case PP_BM:
                        p->drawPixmap(r.x()+((r.width()-img.pixmap.img.width())/2), r.bottom()-img.pixmap.img.height(), img.pixmap.img);
                        break;
                    case PP_BR:
                        p->drawPixmap(r.right()-img.pixmap.img.width(), r.bottom()-img.pixmap.img.height(), img.pixmap.img);
                        break;
                    case PP_LM:
                        p->drawPixmap(r.left(), r.y()+((r.height()-img.pixmap.img.height())/2), img.pixmap.img);
                        break;
                    case PP_RM:
                        p->drawPixmap(r.right()-img.pixmap.img.width(), r.y()+((r.height()-img.pixmap.img.height())/2), img.pixmap.img);
                        break;
                    case PP_CENTRED:
                        p->drawPixmap(r.x()+((r.width()-img.pixmap.img.width())/2),
                                      r.y()+((r.height()-img.pixmap.img.height())/2),
                                      img.pixmap.img);
                }
            }
            break;
        case IMG_PLAIN_RINGS:
        case IMG_BORDERED_RINGS:
            if(img.pixmap.img.isNull())
            {
                img.pixmap.img=QPixmap(imgWidth, imgHeight);
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
            p->drawPixmap(r.right()-img.pixmap.img.width(), r.y()+1, img.pixmap.img);
            break;
        case IMG_SQUARE_RINGS:
            if(img.pixmap.img.isNull())
            {
                img.pixmap.img=QPixmap(imgWidth, imgHeight);
                img.pixmap.img.fill(Qt::transparent);
                QPainter pixPainter(&img.pixmap.img);
                QColor   col(Qt::white);
                double   halfWidth=RINGS_SQUARE_LINE_WIDTH/2.0;

                col.setAlphaF(RINGS_SQUARE_SMALL_ALPHA);
                pixPainter.setRenderHint(QPainter::Antialiasing);
                pixPainter.setPen(QPen(col, RINGS_SQUARE_LINE_WIDTH, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin));
                pixPainter.drawPath(buildPath(QRectF(halfWidth+0.5, halfWidth+0.5,
                                                     RINGS_SQUARE_SMALL_SIZE, RINGS_SQUARE_SMALL_SIZE),
                                              WIDGET_OTHER, ROUNDED_ALL, RINGS_SQUARE_RADIUS));
                pixPainter.drawPath(buildPath(QRectF(halfWidth+0.5+(imgWidth-(RINGS_SQUARE_SMALL_SIZE+RINGS_SQUARE_LINE_WIDTH)),
                                                     halfWidth+0.5+(imgHeight-(RINGS_SQUARE_SMALL_SIZE+RINGS_SQUARE_LINE_WIDTH)),
                                                     RINGS_SQUARE_SMALL_SIZE, RINGS_SQUARE_SMALL_SIZE),
                                              WIDGET_OTHER, ROUNDED_ALL, RINGS_SQUARE_RADIUS));
                col.setAlphaF(RINGS_SQUARE_LARGE_ALPHA);
                pixPainter.setPen(QPen(col, RINGS_SQUARE_LINE_WIDTH, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin));
                pixPainter.drawPath(buildPath(QRectF(halfWidth+0.5+((imgWidth-RINGS_SQUARE_LARGE_SIZE-RINGS_SQUARE_LINE_WIDTH)/2.0),
                                                     halfWidth+0.5+((imgHeight-RINGS_SQUARE_LARGE_SIZE-RINGS_SQUARE_LINE_WIDTH)/2.0),
                                                     RINGS_SQUARE_LARGE_SIZE, RINGS_SQUARE_LARGE_SIZE),
                                              WIDGET_OTHER, ROUNDED_ALL, RINGS_SQUARE_RADIUS));
                pixPainter.end();
            }
            p->drawPixmap(r.right()-img.pixmap.img.width(), r.y()+1, img.pixmap.img);
            break;
    }
}

void
Style::drawBackground(QPainter *p, const QWidget *widget,
                      BackgroundType type) const
{
    bool isWindow(BGND_MENU != type);
    bool previewMdi(isWindow && m_isPreview &&
                    qobject_cast<const QMdiSubWindow*>(widget));
    const QWidget *window = m_isPreview ? widget : widget->window();
    int opacity = (BGND_MENU == type ? opts.menuBgndOpacity :
                   BGND_DIALOG == type ? opts.dlgOpacity : opts.bgndOpacity);
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

    drawBackground(p, isWindow ? window->palette().window().color() :
                   popupMenuCols()[ORIGINAL_SHADE], bgndRect, opacity, type,
                   type != BGND_MENU ? opts.bgndAppearance :
                   opts.menuBgndAppearance);
    // FIXME, workaround only, the non transparent part of the image will have
    // a different overall opacity.
    p->save();
    p->setCompositionMode(QPainter::CompositionMode_SourceOver);
    drawBackgroundImage(p, isWindow, imgRect);
    p->restore();
}

QPainterPath Style::buildPath(const QRectF &r, EWidget w, int round, double radius) const
{
    QPainterPath path;

    if(WIDGET_RADIO_BUTTON==w || WIDGET_DIAL==w ||
       (WIDGET_MDI_WINDOW_BUTTON==w && opts.titlebarButtons&TITLEBAR_BUTTON_ROUND) ||
       CIRCULAR_SLIDER(w))
    {
        path.addEllipse(r);
        return path;
    }

    if(ROUND_NONE==opts.round || (radius<0.01))
        round=ROUNDED_NONE;

    double       diameter(radius*2);

    if (WIDGET_MDI_WINDOW_TITLE!=w && round&CORNER_BR)
        path.moveTo(r.x()+r.width(), r.y()+r.height()-radius);
    else
        path.moveTo(r.x()+r.width(), r.y()+r.height());

    if (round&CORNER_TR)
        path.arcTo(r.x()+r.width()-diameter, r.y(), diameter, diameter, 0, 90);
    else
        path.lineTo(r.x()+r.width(), r.y());

    if (round&CORNER_TL)
        path.arcTo(r.x(), r.y(), diameter, diameter, 90, 90);
    else
        path.lineTo(r.x(), r.y());

    if (WIDGET_MDI_WINDOW_TITLE!=w && round&CORNER_BL)
        path.arcTo(r.x(), r.y()+r.height()-diameter, diameter, diameter, 180, 90);
    else
        path.lineTo(r.x(), r.y()+r.height());

    if(WIDGET_MDI_WINDOW_TITLE!=w)
    {
        if (round&CORNER_BR)
            path.arcTo(r.x()+r.width()-diameter, r.y()+r.height()-diameter, diameter, diameter, 270, 90);
        else
            path.lineTo(r.x()+r.width(), r.y()+r.height());
    }

    return path;
}

QPainterPath Style::buildPath(const QRect &r, EWidget w, int round, double radius) const
{
    return buildPath(QRectF(r.x()+0.5, r.y()+0.5, r.width()-1, r.height()-1), w, round, radius);
}

void Style::buildSplitPath(const QRect &r, int round, double radius, QPainterPath &tl, QPainterPath &br) const
{
    double xd(r.x()+0.5),
           yd(r.y()+0.5),
           diameter(radius*2);
    bool   rounded=diameter>0.0;
    int    width(r.width()-1),
           height(r.height()-1);

    if (rounded && round&CORNER_TR)
    {
        tl.arcMoveTo(xd+width-diameter, yd, diameter, diameter, 45);
        tl.arcTo(xd+width-diameter, yd, diameter, diameter, 45, 45);
        if(width>diameter)
            tl.lineTo(xd+width-diameter, yd);
    }
    else
        tl.moveTo(xd+width, yd);

    if (rounded && round&CORNER_TL)
        tl.arcTo(xd, yd, diameter, diameter, 90, 90);
    else
        tl.lineTo(xd, yd);

    if (rounded && round&CORNER_BL)
    {
        tl.arcTo(xd, yd+height-diameter, diameter, diameter, 180, 45);
        br.arcMoveTo(xd, yd+height-diameter, diameter, diameter, 180+45);
        br.arcTo(xd, yd+height-diameter, diameter, diameter, 180+45, 45);
    }
    else
    {
        tl.lineTo(xd, yd+height);
        br.moveTo(xd, yd+height);
    }

    if (rounded && round&CORNER_BR)
        br.arcTo(xd+width-diameter, yd+height-diameter, diameter, diameter, 270, 90);
    else
        br.lineTo(xd+width, yd+height);

    if (rounded && round&CORNER_TR)
        br.arcTo(xd+width-diameter, yd, diameter, diameter, 0, 45);
    else
        br.lineTo(xd+width, yd);
}

void Style::drawBorder(QPainter *p, const QRect &r, const QStyleOption *option, int round, const QColor *custom, EWidget w,
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
    const QColor *cols(enabled && hasMouseOver && opts.coloredMouseOver &&
                       entry ? m_mouseOverCols : enabled &&
                       // && m_focusCols // (always true)
                       hasFocus && entry ?
                       m_focusCols : custom ? custom :
                       APP_KRUNNER==theThemedApp ? m_backgroundCols : backgroundColors(option));
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

                if(((hasMouseOver || hasFocus) && WIDGET_ENTRY==w) ||
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

                p->setPen((enabled || BORDER_SUNKEN==borderProfile) /*&&
                            (BORDER_RAISED==borderProfile || BORDER_LIGHT==borderProfile || hasFocus || APPEARANCE_FLAT!=app)*/
                                ? tl
                                : option->palette.background().color());
                p->drawPath(topPath);
                if(WIDGET_SCROLLVIEW==w || // Because of list view headers, need to draw dark line on right!
                    (! ((WIDGET_ENTRY==w && !hasFocus && !hasMouseOver) ||
                        (WIDGET_ENTRY!=w && doBlend && BORDER_SUNKEN==borderProfile) ) ) )
                {
                    if(!hasFocus && !hasMouseOver && BORDER_LIGHT!=borderProfile && WIDGET_SCROLLVIEW!=w)
                        p->setPen(/*WIDGET_SCROLLVIEW==w && !hasFocus
                                    ? checkColour(option, QPalette::Window)
                                    : WIDGET_ENTRY==w && !hasFocus
                                        ? checkColour(option, QPalette::Base)
                                        : */enabled && (BORDER_SUNKEN==borderProfile || hasFocus || /*APPEARANCE_FLAT!=app ||*/
                                            WIDGET_TAB_TOP==w || WIDGET_TAB_BOT==w)
                                            ? br
                                            : checkColour(option, QPalette::Window));
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
        p->setPen(/*enabled ? */border/* : col*/);
        p->drawPath(topPath);
//         if(enabled)
            p->setPen(col);
        p->drawPath(botPath);
    }
    else
    {
        p->setPen(border);
        p->drawPath(buildPath(r, w, round, qtcGetRadius(&opts, r.width(), r.height(), w, RADIUS_EXTERNAL)));
    }

    p->setRenderHint(QPainter::Antialiasing, false);
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

void Style::drawDwtControl(QPainter *p, const QFlags<State> &state, const QRect &rect, ETitleBarButtons btn, Icon icon,
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
    p->setRenderHint(QPainter::Antialiasing, false);

    if(doEtch && opts.etchEntry)
        drawEtch(p, rx, widget, WIDGET_SCROLLVIEW==w ? w : WIDGET_ENTRY, false);

    drawBorder(p, r, option, round, 0L, w, BORDER_SUNKEN);
}

void Style::drawMenuItem(QPainter *p, const QRect &r, const QStyleOption *option, MenuItemType type, int round, const QColor *cols) const
{
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
        double fadePercent=((double)MENUITEM_FADE_SIZE)/rf.width();
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
        }
        else
            p->fillRect(r2, QBrush(grad));
    }
    else if(MENU_BAR==type || opts.borderMenuitems)
    {
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
        if(/*MENU_COMBO==type || */POPUP_MENUS_SQUARE(opts))
            drawBevelGradient(cols[fill], p, r, true, false, opts.menuitemAppearance, WIDGET_MENU_ITEM);
        else
        {
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
    QMatrix matrix;
    matrix.rotate(angle);
    return matrix.map(p);
}

void Style::drawArrow(QPainter *p, const QRect &rx, PrimitiveElement pe,
                      QColor col, bool small, bool kwin) const
{
    QPolygon a;
    QRect r(rx);
    int m = !small && kwin ? ((r.height() - 7) / 2) : 0;

    if (small) {
        a.setPoints(opts.vArrows ? 6 : 3,
                    2, 0, 0, -2,
                    -2, 0, -2, 1,
                    0, -1, 2, 1);
    } else {
        a.setPoints(opts.vArrows ? 8 : 3,
                    3 + m, 1 + m, 0, -2,
                    -(3 + m), 1 + m, -(3 + m), 2 + m,
                    -(2 + m), 2 + m, 0, 0,
                    2 + m, 2 + m, 3 + m, 2 + m);
    }

    switch (pe) {
    case PE_IndicatorArrowUp:
        if (m)
            r.adjust(0, -m, 0, -m);
        break;
    case PE_IndicatorArrowDown:
        if (m)
            r.adjust(0, m, 0, m);
        a = rotate(a, 180);
        break;
    case PE_IndicatorArrowRight:
        a = rotate(a, 90);
        break;
    case PE_IndicatorArrowLeft:
        a = rotate(a, 270);
        break;
    default:
        return;
    }
    a.translate(r.x() + r.width() / 2, r.y() + r.height() / 2);
    col.setAlpha(255);
    p->save();
    p->setPen(col);
    p->setBrush(col);
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 5))
    // Qt >= 4.8.5 has problem drawing polygons correctly. Enabling
    // antialiasing can work around the problem although it will also make
    // the arrow blurry.
    // QtCurve issue:
    // https://github.com/QtCurve/qtcurve-qt4/issues/3.
    // Upstream bug report:
    // https://bugreports.qt-project.org/browse/QTBUG-33512
    p->setRenderHint(QPainter::Antialiasing, opts.vArrows);
#else
    p->setRenderHint(QPainter::Antialiasing, false);
#endif
    p->drawPolygon(a);
    p->restore();
}

void
Style::drawSbSliderHandle(QPainter *p, const QRect &rOrig,
                          const QStyleOption *option, bool slider) const
{
    QStyleOption opt(*option);
    QRect r = rOrig;

    if(opt.state&(State_Sunken|State_On))
        opt.state|=State_MouseOver;

    if(r.width()>r.height())
        opt.state|=State_Horizontal;

    opt.state&=~(State_Sunken|State_On);
    opt.state|=State_Raised;

    if (auto slider = styleOptCast<QStyleOptionSlider>(option))
        if(slider->minimum==slider->maximum)
            opt.state&=~(State_MouseOver|State_Enabled);

    int min = MIN_SLIDER_SIZE(opts.sliderThumbs);
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

    if(SLIDER_TRIANGULAR==opts.sliderStyle)
    {
        if(r.width()>r.height())
            opt.state|=State_Horizontal;
        opt.state&=~(State_Sunken|State_On);

        opt.state|=State_Raised;

        const QColor     *use(sliderColors(&opt)),
                         *border(opt.state&State_MouseOver && (MO_GLOW==opts.coloredMouseOver ||
                                                               MO_COLORED==opts.coloredMouseOver)
                                    ? m_mouseOverCols : use);
        const QColor     &fill(getFill(&opt, use, false, SHADE_DARKEN==opts.shadeSliders));
        int              x(r.x()),
                         y(r.y());
        PrimitiveElement direction(horiz ? PE_IndicatorArrowDown : PE_IndicatorArrowRight);
        QPolygon         clipRegion;
        bool             drawLight(MO_PLASTIK!=opts.coloredMouseOver || !(opt.state&State_MouseOver));
        int              size(SLIDER_TRIANGULAR==opts.sliderStyle ? 15 : 13),
                         borderVal(m_mouseOverCols==border ? SLIDER_MO_BORDER_VAL : BORDER_VAL(opt.state&State_Enabled));

        if(option->tickPosition & QSlider::TicksBelow)
            direction=horiz ? PE_IndicatorArrowDown : PE_IndicatorArrowRight;
        else if(option->tickPosition & QSlider::TicksAbove)
            direction=horiz ? PE_IndicatorArrowUp : PE_IndicatorArrowLeft;

        if(MO_GLOW==opts.coloredMouseOver && opts.buttonEffect != EFFECT_NONE)
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
                p->setRenderHint(QPainter::Antialiasing, false);
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
                p->setRenderHint(QPainter::Antialiasing, false);
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
                p->setRenderHint(QPainter::Antialiasing, false);
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
                p->setRenderHint(QPainter::Antialiasing, false);
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

    if (horiz) {
        int dh = (grv.height() - 5) / 2;
        grv.adjust(0, dh, 0, -dh);
        opt.state |= State_Horizontal;

        if (opts.buttonEffect != EFFECT_NONE) {
            grv.adjust(0, -1, 0, 1);
        }
    } else {
        int dw = (grv.width() - 5) / 2;
        grv.adjust(dw, 0, -dw, 0);
        opt.state &= ~State_Horizontal;

        if (opts.buttonEffect != EFFECT_NONE) {
            grv.adjust(-1, 0, 1, 0);
        }
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
    // LibreOffice - when drawMenuOrToolBarBackground is
    // called with menuRect, this is empty!
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
        QColor col(menu && (option->state & State_Enabled ||
                            opts.shadeMenubars != SHADE_NONE) ?
                   menuColors(option, m_active)[ORIGINAL_SHADE] :
                   option->palette.background().color());
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

void
Style::drawHandleMarkers(QPainter *p, const QRect &rx,
                         const QStyleOption *option,
                         bool tb, ELine handles) const
{
    if (rx.width() < 2 || rx.height() < 2)
        return;

    QRect r(rx);

    if (theThemedApp == APP_OPENOFFICE) {
        r.setX(r.x() + 2);
        r.setWidth(10);
    }

    // CPD: Mouse over of toolbar handles not working -
    // the whole toolbar seems to be active :-(
    QStyleOption opt(*option);

    opt.state &= ~State_MouseOver;

    const QColor *border(borderColors(&opt, m_backgroundCols));

    switch (handles) {
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
    QColor col(invertedSel ? option->palette.background().color() : fill);

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
    double hl = TO_FACTOR(opts.highlightFactor);

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

const QColor*
Style::popupMenuCols(const QStyleOption *option) const
{
    return (opts.lighterPopupMenuBgnd || opts.shadePopupMenu || !option ?
            m_popupMenuCols : backgroundColors(option));
}

const QColor * Style::checkRadioColors(const QStyleOption *option) const
{
    return opts.crColor && option && option->state&State_Enabled && (option->state&State_On || option->state&State_NoChange)
        ? m_checkRadioSelCols
        : buttonColors(option);
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
    switch(opts.shadeMenubars)
    {
        case SHADE_NONE:
            memcpy(m_menubarCols, m_backgroundCols, sizeof(QColor)*(TOTAL_SHADES+1));
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
                        ? (QColor*)getMdiColors(0L, true) // TODO: option!!!
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
    }
    else if(opts.customMenuTextColor || SHADE_BLEND_SELECTED==opts.shadeMenubars ||
            SHADE_SELECTED==opts.shadeMenubars ||
            (SHADE_CUSTOM==opts.shadeMenubars && TOO_DARK(m_menubarCols[ORIGINAL_SHADE])))
    {
        QPalette pal(widget->palette());

        pal.setBrush(QPalette::Active, QPalette::Foreground, opts.customMenuTextColor
                                                ? opts.customMenuNormTextColor
                                                : pal.highlightedText().color());
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
    return SHADE_WINDOW_BORDER==opts.shadeMenubars
            ? getMdiColors(option, active)
            : SHADE_NONE==opts.shadeMenubars || (opts.shadeMenubarOnlyWhenActive && !active)
                ? backgroundColors(option)
                : m_menubarCols;
}

bool Style::coloredMdiButtons(bool active, bool mouseOver) const
{
    return opts.titlebarButtons&TITLEBAR_BUTTON_COLOR &&
            (active
                ? (mouseOver || !(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR_MOUSE_OVER))
                : ((opts.titlebarButtons&TITLEBAR_BUTTON_COLOR_MOUSE_OVER && mouseOver) ||
                    (!(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR_MOUSE_OVER) &&
                       opts.titlebarButtons&TITLEBAR_BUTTON_COLOR_INACTIVE)) );
}

const QColor * Style::getMdiColors(const QStyleOption *option, bool active) const
{
    if(!m_activeMdiColors)
    {
#ifndef QTC_QT4_ENABLE_KDE
        m_activeMdiTextColor=option ? option->palette.text().color() : QApplication::palette().text().color();
        m_mdiTextColor=option ? option->palette.text().color() : QApplication::palette().text().color();

        QFile f(Utils::kdeHome()+"/share/config/kdeglobals");

        if(f.open(QIODevice::ReadOnly))
        {
            QTextStream in(&f);
            bool        inPal(false);

            while (!in.atEnd())
            {
                QString line(in.readLine());

                if(inPal)
                {
                    if(!m_activeMdiColors && 0==line.indexOf("activeBackground="))
                    {
                        QColor col;

                        setRgb(&col, line.mid(17).split(","));

                        if(col!=m_highlightCols[ORIGINAL_SHADE])
                        {
                            m_activeMdiColors=new QColor [TOTAL_SHADES+1];
                            shadeColors(col, m_activeMdiColors);
                        }
                    }
                    else if(!m_mdiColors && 0==line.indexOf("inactiveBackground="))
                    {
                        QColor col;

                        setRgb(&col, line.mid(19).split(","));
                        if(col!=m_buttonCols[ORIGINAL_SHADE])
                        {
                            m_mdiColors=new QColor [TOTAL_SHADES+1];
                            shadeColors(col, m_mdiColors);
                        }
                    }
                    else if(0==line.indexOf("activeForeground="))
                        setRgb(&m_activeMdiTextColor, line.mid(17).split(","));
                    else if(0==line.indexOf("inactiveForeground="))
                        setRgb(&m_mdiTextColor, line.mid(19).split(","));
                    else if (-1!=line.indexOf('['))
                        break;
                }
                else if(0==line.indexOf("[WM]"))
                    inPal=true;
            }
            f.close();
        }
#else
        Q_UNUSED(option)

        QColor col=KGlobalSettings::activeTitleColor();

        if(col!=m_backgroundCols[ORIGINAL_SHADE])
        {
            m_activeMdiColors=new QColor [TOTAL_SHADES+1];
            shadeColors(col, m_activeMdiColors);
        }

        col=KGlobalSettings::inactiveTitleColor();
        if(col!=m_backgroundCols[ORIGINAL_SHADE])
        {
            m_mdiColors=new QColor [TOTAL_SHADES+1];
            shadeColors(col, m_mdiColors);
        }

        m_activeMdiTextColor=KGlobalSettings::activeTextColor();
        m_mdiTextColor=KGlobalSettings::inactiveTextColor();
#endif

        if(!m_activeMdiColors)
            m_activeMdiColors=(QColor*)m_backgroundCols;
        if(!m_mdiColors)
            m_mdiColors=(QColor*)m_backgroundCols;

        if(opts.shadeMenubarOnlyWhenActive && SHADE_WINDOW_BORDER==opts.shadeMenubars &&
           m_activeMdiColors[ORIGINAL_SHADE]==m_mdiColors[ORIGINAL_SHADE])
            opts.shadeMenubarOnlyWhenActive=false;
    }

    return active ? m_activeMdiColors : m_mdiColors;
}

void Style::readMdiPositions() const
{
    if(0==m_mdiButtons[0].size() && 0==m_mdiButtons[1].size())
    {
#ifdef Q_OS_MAC
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

#ifdef QTC_QT4_ENABLE_KDE
        KSharedConfigPtr cfg = KSharedConfig::openConfig("kwinrc");
        KConfigGroup grp = cfg->group("Style");

        if(grp.readEntry("CustomButtonPositions", false))
        {
            QString left=grp.readEntry("ButtonsOnLeft"),
                    right=grp.readEntry("ButtonsOnRight");

            if(!left.isEmpty() || !right.isEmpty())
                m_mdiButtons[0].clear(), m_mdiButtons[1].clear();

            if(!left.isEmpty())
                parseWindowLine(left, m_mdiButtons[0]);

            if(!right.isEmpty())
                parseWindowLine(right, m_mdiButtons[1]);

            // Designer uses shade buttons, not min/max - so if we don't have shade in our kwin config.
            // then add this button near the max button...
            if(-1==m_mdiButtons[0].indexOf(SC_TitleBarShadeButton) && -1==m_mdiButtons[1].indexOf(SC_TitleBarShadeButton))
            {
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
    QtcKey key(createKey(col, p));
    QPixmap *pix = m_pixmapCache.object(key);

    if (!pix) {
        if(PIX_DOT==p)
        {
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
        } else {
            pix = new QPixmap();
            QImage img;
            switch (p) {
            case PIX_CHECK:
                if(opts.xCheck)
                    img = qtc_check_x_on;
                else
                    img = qtc_check_on;
                break;
            default:
                break;
            }

            qtcAdjustPix(img.bits(), 4, img.width(), img.height(),
                         img.bytesPerLine(), col.red(), col.green(),
                         col.blue(), shade, QTC_PIXEL_QT);
            *pix = QPixmap::fromImage(img);
        }
        m_pixmapCache.insert(key, pix, pix->depth()/8);
    }

    return pix;
}

const QColor & Style::getTabFill(bool current, bool highlight, const QColor *use) const
{
    return current
            ? use[ORIGINAL_SHADE]
            : highlight
                ? use[SHADE_2_HIGHLIGHT]
                : use[2];
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
    if (USE_CUSTOM_ALPHAS(opts)) {
        QColor col(Qt::white);
        col.setAlphaF(opts.customAlphas[ALPHA_ETCH_LIGHT]);
        return col;
    }

    QtcQWidgetProps props(widget);
    if (qtcIsFlatBgnd(opts.bgndAppearance)) {
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
    col.setAlphaF(0.1);
    // qtcIsFlatBgnd(opts.bgndAppearance) ? 0.25 : 0.4);
    return col;
}

int Style::getFrameRound(const QWidget *widget) const
{
    if(opts.square&SQUARE_FRAME)
        return ROUNDED_NONE;

    const QWidget *window=widget ? widget->window() : 0L;

    if(window)
    {
        QRect widgetRect(widget->rect()),
              windowRect(window->rect());

        if(widgetRect==windowRect)
            return ROUNDED_NONE;
    }

    if((opts.square&SQUARE_ENTRY) && widget && qobject_cast<const QLabel*>(widget))
        return ROUNDED_NONE;

    return ROUNDED_ALL;
}

void Style::widgetDestroyed(QObject *o)
{
    QWidget *w = static_cast<QWidget*>(o);
    if (APP_KONTACT == theThemedApp) {
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
        foreach (QWidget *widget, rem) {
            m_sViewContainers.remove(widget);
        }
    }
}

#ifdef QTC_QT4_ENABLE_KDE
void Style::setupKde4()
{
    if(kapp)
        setDecorationColors();
    else
    {
        applyKdeSettings(true);
        applyKdeSettings(false);
    }
}

void Style::setDecorationColors()
{
    KColorScheme kcs(QPalette::Active);
    if(opts.coloredMouseOver)
        shadeColors(kcs.decoration(KColorScheme::HoverColor).color(), m_mouseOverCols);
    shadeColors(kcs.decoration(KColorScheme::FocusColor).color(), m_focusCols);
}

void Style::applyKdeSettings(bool pal)
{
    if(pal)
    {
        if(!kapp)
            QApplication::setPalette(standardPalette());
        setDecorationColors();
    }
    else
    {
        KConfigGroup g(KGlobal::config(), "General");
        QFont        mnu=g.readEntry("menuFont", QApplication::font());

        QApplication::setFont(g.readEntry("font", QApplication::font()));
        QApplication::setFont(mnu, "QMenuBar");
        QApplication::setFont(mnu, "QMenu");
        QApplication::setFont(mnu, "KPopupTitle");
        QApplication::setFont(KGlobalSettings::toolBarFont(), "QToolBar");
    }
}
#endif

void Style::kdeGlobalSettingsChange(int type, int)
{
#ifndef QTC_QT4_ENABLE_KDE
    Q_UNUSED(type)
#else
    switch(type) {
    case KGlobalSettings::StyleChanged: {
        KGlobal::config()->reparseConfiguration();
        if (m_usePixmapCache)
            QPixmapCache::clear();
        init(false);
        foreach (QWidget *widget, QApplication::topLevelWidgets()) {
            widget->update();
        }
        break;
    }
    case KGlobalSettings::PaletteChanged:
        KGlobal::config()->reparseConfiguration();
        applyKdeSettings(true);
        if (m_usePixmapCache)
            QPixmapCache::clear();
        break;
    case KGlobalSettings::FontChanged:
        KGlobal::config()->reparseConfiguration();
        applyKdeSettings(false);
        break;
    }
#endif

    m_blurHelper->setEnabled(Utils::compositingActive());
    m_windowManager->initialize(opts.windowDrag);
}

void Style::borderSizesChanged()
{
#ifdef QTC_QT4_ENABLE_KDE
    int old = qtcGetWindowBorderSize(false).titleHeight;

    if (old != qtcGetWindowBorderSize(true).titleHeight) {
        foreach (QWidget *widget, QApplication::topLevelWidgets()) {
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
    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
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
    static unsigned int lastXid  = 0;
    static struct timeval lastTime = {0, 0};

    if (diffTime(&lastTime) || lastXid != xid) {
        QMainWindow *win = getWindow(xid);
        if (win) {
            toggleStatusBar(win);
        }
    }
    lastXid = xid;
}

void
Style::compositingToggled()
{
    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        widget->update();
    }
}

void
Style::toggleMenuBar(QMainWindow *window)
{
    bool triggeredAction(false);

#ifdef QTC_QT4_ENABLE_KDE
    if (qobject_cast<KXmlGuiWindow*>(window)) {
        KActionCollection *collection =
            static_cast<KXmlGuiWindow*>(window)->actionCollection();
        QAction *act = collection ? collection->action(KStandardAction::name(KStandardAction::ShowMenubar)) : 0L;
        if (act) {
            act->trigger();
            triggeredAction = true;
        }
    }
#endif
    if (!triggeredAction) {
        QWidget *menubar = window->menuWidget();
        if (m_saveMenuBarStatus) {
            qtcSetMenuBarHidden(appName, menubar->isVisible());
        }
        window->menuWidget()->setHidden(menubar->isVisible());
    }
}

void Style::toggleStatusBar(QMainWindow *window)
{
    bool triggeredAction(false);

#ifdef QTC_QT4_ENABLE_KDE
    if (qobject_cast<KXmlGuiWindow*>(window)) {
        KActionCollection *collection = static_cast<KXmlGuiWindow*>(window)->actionCollection();
        QAction *act = collection ? collection->action(KStandardAction::name(KStandardAction::ShowStatusbar)) : 0L;
        if (act) {
            act->trigger();
            triggeredAction = true;
            // emitStatusBarState(true); // TODO: ???
        }
    }
#endif
    if (!triggeredAction) {
        QList<QStatusBar*> sb = getStatusBars(window);

        if (sb.count()) {
            if (m_saveStatusBarStatus)
                qtcSetStatusBarHidden(appName, sb.first()->isVisible());
            foreach (QStatusBar *statusBar, sb) {
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
        static const char *constMenuSizeProperty="qtcMenuSize";
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
            getKWinDBus()->call(QDBus::NoBlock, "menuBarSize",
                                (unsigned int)wid, (int)size);
        }
    }
}

void Style::emitStatusBarState(QStatusBar *sb)
{
    if (opts.statusbarHiding & HIDE_KWIN) {
        getKWinDBus()->call(QDBus::NoBlock, "statusBarState",
                            (unsigned int)qtcGetWid(sb->window()),
                            sb->isVisible());
    }
}
}
