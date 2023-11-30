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

#include <qtcurve-utils/qtprops.h>

#include "qtcurve_p.h"
#include "qtcurve_fonthelper.h"
#include "argbhelper.h"
#include "utils.h"
#include "shortcuthandler.h"
#include "windowmanager.h"
#include "blurhelper.h"
#include <common/config_file.h>

#include "shadowhelper.h"

#include <QFormLayout>
#include <QProgressBar>
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
#include <QFontMetrics>
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
#include <QDir>
#include <QSettings>
#include <QPixmapCache>
#include <QTextStream>
#include <QFileDialog>
#include <QToolBox>
#include <QFontDatabase>

#include <QDebug>

#ifdef QTC_QT6_ENABLE_KDE
#include <QPrintDialog>
#include <KIconThemes/KIconLoader>
#include <KWidgetsAddons/KTitleWidget>
#include <KXmlGui/KAboutApplicationDialog>
#endif

#include <qtcurve-utils/color.h>

namespace QtCurve {

template<typename T>
static inline int
horizontalAdvance(const QFontMetrics &fm, T &&text)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    return fm.horizontalAdvance(std::forward<T>(text));
#else
    return fm.width(std::forward<T>(text));
#endif
}

static const char *constBoldProperty = "qtc-set-bold";

Style::FontHelper::FontHelper()
    : m_fntDB(new QFontDatabase())
{}

Style::FontHelper::~FontHelper()
{
    delete m_fntDB;
}

QFont Style::FontHelper::fontStripStyleName(const QFont &f) const
{
    const QString &styleName = f.styleName();
    if (styleName.isEmpty()) {
        // we can simply return the input font
        return f;
    } else {
        // Check for a mismatch between styleString and styleName; when
        // found the font probably had a style name set directly instead of
        // receiving it e.g. via the FontDialog. This means its attributes
        // may still correspond to the original font, not to the current style.
        // Do a database lookup to get a consistent QFont instance to work with,
        // so that methods like setWeight(), setStyle() will work as expected.
        QFont g = (m_fntDB->styleString(f) != styleName) ?
            m_fntDB->font(f.family(), styleName, f.pointSize())
            : QFont(f.family(), f.pointSize(), f.weight());
        if (auto s = f.pixelSize() > 0) {
            g.setPixelSize(s);
        }
        g.setStyleHint(f.styleHint(), f.styleStrategy());
        g.setStyle(f.style());
        if (f.underline()) {
            g.setUnderline(true);
        }
        if (f.strikeOut()) {
            g.setStrikeOut(true);
        }
        if (f.fixedPitch()) {
            g.setFixedPitch(true);
        }
        return g;
    }
}

void Style::FontHelper::setBold(QWidget *widget)
{
    QVariant prop(widget->property(constBoldProperty));
    if (!prop.isValid() || !prop.toBool()) {
        QFont font = fontStripStyleName(widget->font());
        if (!font.bold()) {
            font.setBold(true);
            widget->setFont(font);
            widget->setProperty(constBoldProperty, true);
        }
    }
}

void Style::FontHelper::unSetBold(QWidget *widget)
{
    QVariant prop(widget->property(constBoldProperty));
    if (prop.isValid() && prop.toBool()) {
        QFont font = fontStripStyleName(widget->font());
        font.setBold(false);
        widget->setFont(font);
        widget->setProperty(constBoldProperty, false);
    }
}

void
Style::polish(QApplication *app)
{
    // appName = getFile(app->arguments()[0]);

    if (appName == "kwin" || appName == "kwin_x11" || appName == "kwin_wayland") {
        theThemedApp = APP_KWIN;
    } else if (appName == "systemsettings" || appName == "systemsettings5") {
        theThemedApp = APP_SYSTEMSETTINGS;
    } else if ("plasma" == appName || appName.startsWith("plasma-")) {
        theThemedApp = APP_PLASMA;
    } else if ("krunner" == appName || "krunner_lock" == appName ||
               "kscreenlocker" == appName) {
        theThemedApp = APP_KRUNNER;
    } else if ("kontact" == appName) {
        theThemedApp = APP_KONTACT;
    } else if ("k3b" == appName) {
        theThemedApp = APP_K3B;
    } else if("arora" == appName) {
        theThemedApp = APP_ARORA;
    } else if("rekonq" == appName) {
        theThemedApp = APP_REKONQ;
    } else if("QtCreator" == QCoreApplication::applicationName()) {
        theThemedApp = APP_QTCREATOR;
    } else if("kdevelop" == appName || "kdevelop.bin" == appName) {
        theThemedApp = APP_KDEVELOP;
    } else if("soffice.bin" == appName) {
        theThemedApp = APP_OPENOFFICE;
    } else if("kdmgreet" == appName) {
        opts.forceAlternateLvCols = false;
    }

    qtcInfo("QtCurve: Application name: \"%s\"\n",
            appName.toLatin1().constData());

    if (theThemedApp == APP_REKONQ)
        opts.statusbarHiding=0;
    if(opts.menubarHiding)
        m_saveMenuBarStatus=opts.menubarApps.contains("kde") || opts.menubarApps.contains(appName);
    if(opts.statusbarHiding)
        m_saveStatusBarStatus=opts.statusbarApps.contains("kde") || opts.statusbarApps.contains(appName);

    if(!qtcIsFlatBgnd(opts.bgndAppearance) && opts.noBgndGradientApps.contains(appName))
        opts.bgndAppearance = APPEARANCE_FLAT;
    if(IMG_NONE!=opts.bgndImage.type && opts.noBgndImageApps.contains(appName))
        opts.bgndImage.type=IMG_NONE;
    if(SHADE_NONE!=opts.menuStripe && opts.noMenuStripeApps.contains(appName))
        opts.menuStripe=SHADE_NONE;

    if((100!=opts.bgndOpacity || 100!=opts.dlgOpacity) && (opts.noBgndOpacityApps.contains(appName) || appName.endsWith(".kss")))
        opts.bgndOpacity=opts.dlgOpacity=100;
    if (100 != opts.menuBgndOpacity &&
        opts.noMenuBgndOpacityApps.contains(appName))
        opts.menuBgndOpacity = 100;

    if (APP_KWIN == theThemedApp) {
        opts.bgndAppearance = APPEARANCE_FLAT;
    } else if(APP_OPENOFFICE == theThemedApp) {
        opts.scrollbarType=SCROLLBAR_WINDOWS;
        if(APPEARANCE_FADE == opts.menuitemAppearance)
            opts.menuitemAppearance = APPEARANCE_FLAT;
        opts.borderMenuitems=opts.etchEntry=false;

        if(opts.useHighlightForMenu && blendOOMenuHighlight(QApplication::palette(), m_highlightCols[ORIGINAL_SHADE]))
        {
            m_ooMenuCols=new QColor [TOTAL_SHADES+1];
            shadeColors(tint(popupMenuCols()[ORIGINAL_SHADE], m_highlightCols[ORIGINAL_SHADE], 0.5), m_ooMenuCols);
        }
        opts.menubarHiding=opts.statusbarHiding=HIDE_NONE;
        opts.square|=SQUARE_POPUP_MENUS|SQUARE_TOOLTIPS;
        if(!qtcIsFlatBgnd(opts.menuBgndAppearance) && 0 == opts.lighterPopupMenuBgnd)
            opts.lighterPopupMenuBgnd=1; // shade so that we dont have 3d-ish borders...
        opts.menuBgndAppearance = APPEARANCE_FLAT;
    }

    ParentStyleClass::polish(app);
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

    if (contrast != opts.contrast) {
        opts.contrast = contrast;
        newContrast = true;
    }

    bool newHighlight(newContrast ||
                      m_highlightCols[ORIGINAL_SHADE]!=palette.color(QPalette::Active, QPalette::Highlight)),
        newGray(newContrast ||
                m_backgroundCols[ORIGINAL_SHADE]!=palette.color(QPalette::Active, QPalette::Base)),
        newButton(newContrast ||
                  m_buttonCols[ORIGINAL_SHADE]!=palette.color(QPalette::Active, QPalette::Button)),
        newSlider(m_sliderCols && m_highlightCols!=m_sliderCols && SHADE_BLEND_SELECTED == opts.shadeSliders &&
                  (newButton || newHighlight)),
        newDefBtn(m_defBtnCols &&
                  (opts.defBtnIndicator != IND_COLORED ||
                   opts.shadeSliders != SHADE_BLEND_SELECTED) &&
                  noneOf(opts.defBtnIndicator, IND_SELECTED, IND_GLOW) &&
                  (newContrast || newButton || newHighlight)),
        newComboBtn(m_comboBtnCols &&
                    noneOf(m_comboBtnCols, m_highlightCols, m_sliderCols) &&
                    opts.comboBtn == SHADE_BLEND_SELECTED &&
                    (newButton || newHighlight)),
        newSortedLv(m_sortedLvColors && ((SHADE_BLEND_SELECTED == opts.sortedLv && m_defBtnCols!=m_sortedLvColors &&
                                            m_sliderCols!=m_sortedLvColors && m_comboBtnCols!=m_sortedLvColors) ||
                                           SHADE_DARKEN == opts.sortedLv) &&
                    (newContrast || (opts.lvButton ? newButton : newGray))),
        newCheckRadioSelCols(m_checkRadioSelCols && ((SHADE_BLEND_SELECTED == opts.crColor && m_defBtnCols!=m_checkRadioSelCols &&
                                                       m_sliderCols!=m_checkRadioSelCols && m_comboBtnCols!=m_checkRadioSelCols &&
                                                       m_sortedLvColors!=m_checkRadioSelCols) ||
                                                      SHADE_DARKEN == opts.crColor) &&
                             (newContrast || newButton)),
        newProgressCols(m_progressCols && SHADE_BLEND_SELECTED == opts.progressColor &&
                        m_sliderCols!=m_progressCols && m_comboBtnCols!=m_progressCols &&
                        m_sortedLvColors!=m_progressCols && m_checkRadioSelCols!=m_progressCols && (newContrast || newButton));

    if (newGray) {
        shadeColors(palette.color(QPalette::Active, QPalette::Base), m_backgroundCols);
        if (oneOf(opts.bgndImage.type, IMG_PLAIN_RINGS, IMG_BORDERED_RINGS,
                  IMG_SQUARE_RINGS) ||
            oneOf(opts.menuBgndImage.type, IMG_PLAIN_RINGS,
                  IMG_BORDERED_RINGS, IMG_SQUARE_RINGS)) {
            qtcCalcRingAlphas(&m_backgroundCols[ORIGINAL_SHADE]);
            if (m_usePixmapCache) {
                QPixmapCache::clear();
            }
        }
    }

    if (newButton)
        shadeColors(palette.color(QPalette::Active, QPalette::Button),
                    m_buttonCols);

    if (newHighlight)
        shadeColors(palette.color(QPalette::Active, QPalette::Highlight),
                    m_highlightCols);

// Dont set these here, they will be updated in setDecorationColors()...
//     shadeColors(QApplication::palette().color(QPalette::Active, QPalette::Highlight), m_focusCols);
//     if(opts.coloredMouseOver)
//         shadeColors(QApplication::palette().color(QPalette::Active, QPalette::Highlight), m_mouseOverCols);

    setMenuColors(palette.color(QPalette::Active, QPalette::Base));

    if (newSlider) {
        shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE],
                             m_buttonCols[ORIGINAL_SHADE]), m_sliderCols);
    }

    if (newDefBtn) {
        if (opts.defBtnIndicator == IND_TINT) {
            shadeColors(tint(m_buttonCols[ORIGINAL_SHADE],
                             m_highlightCols[ORIGINAL_SHADE],
                             DEF_BNT_TINT), m_defBtnCols);
        } else if (opts.defBtnIndicator != IND_GLOW) {
            shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE],
                                 m_buttonCols[ORIGINAL_SHADE]), m_defBtnCols);
        }
    }

    if (newComboBtn) {
        shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE],
                             m_buttonCols[ORIGINAL_SHADE]), m_comboBtnCols);
    }
    if (newSortedLv) {
        if (opts.sortedLv == SHADE_BLEND_SELECTED) {
            shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE],
                                 opts.lvButton ? m_buttonCols[ORIGINAL_SHADE] :
                                 m_backgroundCols[ORIGINAL_SHADE]),
                        m_sortedLvColors);
        } else {
            shadeColors(shade(opts.lvButton ? m_buttonCols[ORIGINAL_SHADE] :
                              m_backgroundCols[ORIGINAL_SHADE],
                              LV_HEADER_DARK_FACTOR), m_sortedLvColors);
        }
    }

    if (m_sidebarButtonsCols && opts.shadeSliders != SHADE_BLEND_SELECTED &&
        opts.defBtnIndicator != IND_COLORED) {
        shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE],
                             m_buttonCols[ORIGINAL_SHADE]),
                    m_sidebarButtonsCols);
    }

    switch (opts.shadeCheckRadio) {
    default:
        m_checkRadioCol = palette.color(QPalette::Active,
                                         opts.crButton ? QPalette::ButtonText :
                                         QPalette::Text);
        break;
    case SHADE_BLEND_SELECTED:
    case SHADE_SELECTED:
        m_checkRadioCol = palette.color(QPalette::Active, QPalette::Highlight);
        break;
    case SHADE_CUSTOM:
        m_checkRadioCol = opts.customCheckRadioColor;
    }

    if (newCheckRadioSelCols) {
        if (opts.crColor == SHADE_BLEND_SELECTED) {
            shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE],
                                 m_buttonCols[ORIGINAL_SHADE]),
                        m_checkRadioSelCols);
        } else {
            shadeColors(shade(m_buttonCols[ORIGINAL_SHADE],
                              LV_HEADER_DARK_FACTOR), m_checkRadioSelCols);
        }
    }
    if (newProgressCols) {
        shadeColors(midColor(m_highlightCols[ORIGINAL_SHADE],
                             m_backgroundCols[ORIGINAL_SHADE]),
                    m_progressCols);
    }
    if (theThemedApp == APP_OPENOFFICE && opts.useHighlightForMenu &&
        (newGray || newHighlight)) {
        if (blendOOMenuHighlight(palette, m_highlightCols[ORIGINAL_SHADE])) {
            if (!m_ooMenuCols) {
                m_ooMenuCols = new QColor[TOTAL_SHADES + 1];
            }
            shadeColors(tint(popupMenuCols()[ORIGINAL_SHADE],
                             m_highlightCols[ORIGINAL_SHADE], 0.5),
                        m_ooMenuCols);
        } else if (m_ooMenuCols) {
            delete []m_ooMenuCols;
            m_ooMenuCols = 0L;
        }
    }

    palette.setColor(QPalette::Active, QPalette::Light, m_backgroundCols[0]);
    palette.setColor(QPalette::Active, QPalette::Dark,
                     m_backgroundCols[QTC_STD_BORDER]);
    palette.setColor(QPalette::Inactive, QPalette::Light, m_backgroundCols[0]);
    palette.setColor(QPalette::Inactive, QPalette::Dark,
                     m_backgroundCols[QTC_STD_BORDER]);
    palette.setColor(QPalette::Inactive, QPalette::WindowText,
                     palette.color(QPalette::Active, QPalette::WindowText));
    palette.setColor(QPalette::Disabled, QPalette::Light, m_backgroundCols[0]);
    palette.setColor(QPalette::Disabled, QPalette::Dark,
                     m_backgroundCols[QTC_STD_BORDER]);

    palette.setColor(QPalette::Disabled, QPalette::Base,
                     palette.color(QPalette::Active, QPalette::Base));
    palette.setColor(QPalette::Disabled, QPalette::Base,
                     palette.color(QPalette::Active, QPalette::Base));

    // Fix KDE4's palette...
    if (palette.color(QPalette::Active, QPalette::Highlight) !=
        palette.color(QPalette::Inactive, QPalette::Highlight)) {
        m_inactiveChangeSelectionColor = true;
    }
    for (int i = QPalette::WindowText;i < QPalette::NColorRoles;i++) {
        // if (i != QPalette::Highlight && i != QPalette::HighlightedText)
        palette.setColor(QPalette::Inactive, (QPalette::ColorRole)i,
                         palette.color(QPalette::Active,
                                       (QPalette::ColorRole)i));
    }

    // Force this to be re-generated!
    if (opts.menuStripe == SHADE_BLEND_SELECTED) {
        opts.customMenuStripeColor = Qt::black;
    }
#ifdef QTC_QT6_ENABLE_KDE
    // Only set palette here...
    if (qApp) {
        setDecorationColors();
    }
#endif
}

void Style::polish(QWidget *widget)
{
    // TODO:
    //      Reorganize this polish function
    if (!widget)
        return;

    prePolish(widget);
    QtcQWidgetProps qtcProps(widget);
    bool enableMouseOver(opts.highlightFactor || opts.coloredMouseOver);

    if (opts.buttonEffect != EFFECT_NONE && !USE_CUSTOM_ALPHAS(opts) &&
        isNoEtchWidget(widget)) {
        qtcProps->noEtch = true;
    }

    m_windowManager->registerWidget(widget);
    m_shadowHelper->registerWidget(widget);

    // Need to register all widgets to blur helper, in order to have proper
    // blur_behind region set have proper regions removed for opaque widgets.
    // Note: that the helper does nothing as long as compositing and ARGB are
    // not enabled
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
    if (qobject_cast<QMdiSubWindow*>(widget)) {
        widget->setAttribute(Qt::WA_StyledBackground);
    }
    if (opts.menubarHiding && qobject_cast<QMainWindow*>(widget) &&
        static_cast<QMainWindow*>(widget)->menuWidget()) {
        widget->installEventFilter(this);
        if (m_saveMenuBarStatus)
            static_cast<QMainWindow*>(widget)->menuWidget()
                ->installEventFilter(this);
        if (m_saveMenuBarStatus && qtcMenuBarHidden(appName)) {
            static_cast<QMainWindow*>(widget)->menuWidget()->setHidden(true);
            if (BLEND_TITLEBAR || opts.menubarHiding & HIDE_KWIN ||
                opts.windowBorder & WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR)
                emitMenuSize(static_cast<QMainWindow*>(widget)->menuWidget(), 0);
        }
    }

    if (opts.statusbarHiding && qobject_cast<QMainWindow*>(widget)) {
        QList<QStatusBar*> sb = getStatusBars(widget);

        if (sb.count()) {
            widget->installEventFilter(this);
            for (QStatusBar *statusBar: const_(sb)) {
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
    if (QAbstractItemView *itemView = qobject_cast<QAbstractItemView*>(widget))
    {
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
             ((QListView*)widget)->viewMode() != QListView::IconMode)))
            itemView->setAlternatingRowColors(true);
    }

    if(APP_KONTACT==theThemedApp && qobject_cast<QToolButton *>(widget))
        ((QToolButton *)widget)->setAutoRaise(true);

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
        widget->inherits("QDockWidgetSeparator")))
        widget->setAttribute(Qt::WA_Hover, true);

    if (qobject_cast<QSplitterHandle*>(widget)) {
        widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
    } else if (qobject_cast<QScrollBar*>(widget)) {
        if(enableMouseOver)
            widget->setAttribute(Qt::WA_Hover, true);
        widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
        if (!opts.gtkScrollViews) {
            widget->installEventFilter(this);
        }
    } else if (qobject_cast<QAbstractScrollArea*>(widget) &&
               widget->inherits("KFilePlacesView")) {
        if (qtcIsCustomBgnd(opts))
            polishScrollArea(static_cast<QAbstractScrollArea*>(widget), true);
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

        if (opts.boldProgress)
            m_fntHelper->setBold(widget);
        widget->installEventFilter(this);
    } else if (qobject_cast<QMenuBar*>(widget)) {
        if (BLEND_TITLEBAR || opts.menubarHiding & HIDE_KWIN ||
            opts.windowBorder &
            WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR) {
            emitMenuSize(widget, PREVIEW_MDI == m_isPreview ||
                         !widget->isVisible() ? 0 : widget->rect().height());
        }
        if (qtcIsCustomBgnd(opts))
            widget->setBackgroundRole(QPalette::NoRole);

        widget->setAttribute(Qt::WA_Hover, true);

        // if(opts.shadeMenubarOnlyWhenActive && SHADE_NONE!=opts.shadeMenubars)
        widget->installEventFilter(this);

        setMenuTextColors(widget, true);
    } else if(qobject_cast<QLabel*>(widget)) {
        widget->installEventFilter(this);
        if (opts.windowDrag == WM_DRAG_ALL &&
            ((QLabel*)widget)->textInteractionFlags()
            .testFlag(Qt::TextSelectableByMouse) &&
            qtcCheckType<QFrame>(widget->parentWidget()) &&
            qtcCheckKDEType(getParent<2>(widget), KTitleWidget))
            ((QLabel*)widget)->setTextInteractionFlags(
                ((QLabel*)widget)->textInteractionFlags() &
                ~Qt::TextSelectableByMouse);
    } else if (qobject_cast<QAbstractScrollArea*>(widget)) {
        if (qtcIsCustomBgnd(opts))
            polishScrollArea(static_cast<QAbstractScrollArea *>(widget));
        if (!opts.gtkScrollViews && (((QFrame*)widget)->frameWidth() > 0)) {
            widget->installEventFilter(this);
        }
        if (APP_KONTACT == theThemedApp && widget->parentWidget()) {
            QWidget *frame = scrollViewFrame(widget->parentWidget());

            if (frame) {
                frame->installEventFilter(this);
                m_sViewContainers[frame].insert(widget);
                connect(qtcSlot(widget, destroyed),
                        qtcSlot(this, widgetDestroyed));
                connect(qtcSlot(frame, destroyed),
                        qtcSlot(this, widgetDestroyed));
            }
        }
    } else if (qobject_cast<QDialog*>(widget) &&
               widget->inherits("QPrintPropertiesDialog") &&
               widget->parentWidget() && widget->parentWidget()->window() &&
               widget->window() && widget->window()->windowTitle().isEmpty() &&
               !widget->parentWidget()->window()->windowTitle().isEmpty()) {
        widget->window()->setWindowTitle(widget->parentWidget()->window()
                                         ->windowTitle());
    } else if (widget->inherits("QWhatsThat")) {
        QPalette pal(widget->palette());
        QColor   shadow(pal.shadow().color());

        shadow.setAlpha(32);
        pal.setColor(QPalette::Shadow, shadow);
        widget->setPalette(pal);
        widget->setMask(QRegion(widget->rect().adjusted(0, 0, -6, -6))+QRegion(widget->rect().adjusted(6, 6, 0, 0)));
    } else if (qobject_cast<QDockWidget*>(widget) &&
               qtcCheckType<QSplitter>(widget->parentWidget()) &&
               qtcCheckType(getParent<2>(widget), "KFileWidget"))
        ((QDockWidget*)widget)->setTitleBarWidget(new QtCurveDockWidgetTitleBar(widget));

    if (widget->inherits("QTipLabel") && !qtcIsFlat(opts.tooltipAppearance)) {
        widget->setBackgroundRole(QPalette::NoRole);
        // TODO: turn this into addAlphaChannel
        widget->setAttribute(Qt::WA_TranslucentBackground);
    }

    if (!widget->isWindow())
        if (QFrame *frame = qobject_cast<QFrame*>(widget)) {
            // kill ugly frames...
            if (QFrame::Box == frame->frameShape() ||
                QFrame::Panel == frame->frameShape() ||
                QFrame::WinPanel == frame->frameShape()) {
                frame->setFrameShape(QFrame::StyledPanel);
            }
            //else if (QFrame::HLine==frame->frameShape() || QFrame::VLine==frame->frameShape())
            widget->installEventFilter(this);

            if (qtcCheckKDEType(widget->parent(), KTitleWidget)) {
                if (qtcIsCustomBgnd(opts)) {
                    frame->setAutoFillBackground(false);
                } else {
                    frame->setBackgroundRole(QPalette::Window);
                }
                QLayout *layout(frame->layout());
                if (layout) {
                    layout->setContentsMargins(0, 0, 0, 0);
                }
            }

            QComboBox *p = nullptr;
            if (opts.gtkComboMenus &&
                (p = qtcObjCast<QComboBox>(getParent<2>(widget))) &&
                !p->isEditable()) {
                QPalette pal(widget->palette());
                QColor   col(popupMenuCols()[ORIGINAL_SHADE]);

                if (!qtcIsFlatBgnd(opts.menuBgndAppearance) ||
                   100 != opts.menuBgndOpacity ||
                    !(opts.square & SQUARE_POPUP_MENUS))
                    col.setAlphaF(0);

                pal.setBrush(QPalette::Active, QPalette::Base, col);
                pal.setBrush(QPalette::Active, QPalette::Window, col);
                widget->setPalette(pal);
                if(opts.shadePopupMenu)
                    setMenuTextColors(widget, false);
            }
        }

    if (qobject_cast<QMenu*>(widget)) {
        if (opts.lighterPopupMenuBgnd || opts.shadePopupMenu) {
            QPalette pal(widget->palette());
            pal.setBrush(QPalette::Active, QPalette::Window,
                         popupMenuCols()[ORIGINAL_SHADE]);
            widget->setPalette(pal);
            if (opts.shadePopupMenu) {
                setMenuTextColors(widget, false);
            }
        }
        if (opts.menuBgndOpacity != 100 ||
            !(opts.square & SQUARE_POPUP_MENUS)) {
            widget->setAttribute(Qt::WA_NoSystemBackground);
            addAlphaChannel(widget);
        }
    }

    if ((!qtcIsFlatBgnd(opts.menuBgndAppearance) ||
         opts.menuBgndOpacity != 100 ||
         !(opts.square & SQUARE_POPUP_MENUS)) &&
        widget->inherits("QComboBoxPrivateContainer")) {
        widget->installEventFilter(this);
        widget->setAttribute(Qt::WA_NoSystemBackground);
        addAlphaChannel(widget);
    }

    bool parentIsToolbar(false);

    // Using dark menubars - konqueror's combo box texts get messed up. Seems
    // to be when a plain QWidget has widget->setBackgroundRole(QPalette::Window);
    // and widget->setAutoFillBackground(false); set (below). These only happen
    // if 'parentIsToolbar' - so dont bather detecting this if the widget
    // is a plain QWidget
    //
    // QWidget QComboBoxListView QComboBoxPrivateContainer SearchBarCombo KToolBar KonqMainWindow
    // QWidget KCompletionBox KLineEdit SearchBarCombo KToolBar KonqMainWindow
    if(strcmp(widget->metaObject()->className(), "QWidget"))
    {
        QWidget *wid=widget ? widget->parentWidget() : 0L;

        while(wid && !parentIsToolbar) {
            parentIsToolbar=qobject_cast<QToolBar*>(wid);
            wid=wid->parentWidget();
        }
    }

    if (APP_QTCREATOR == theThemedApp &&
        qobject_cast<QMainWindow*>(widget) &&
        static_cast<QMainWindow*>(widget)->menuWidget()) {
        // As of 2.8.1, QtCreator still uses it's own style by default.
        // Have no idea what the **** they are thinking.
        static_cast<QMainWindow*>(widget)->menuWidget()->setStyle(this);
    }

    if (APP_QTCREATOR == theThemedApp && qobject_cast<QDialog*>(widget) &&
        qtcCheckKDEType(widget, QFileDialog)) {

        QToolBar *tb = getToolBarChild(widget);
        if (tb) {
            int size = pixelMetric(PM_ToolBarIconSize);
            tb->setIconSize(QSize(size, size));
            tb->setMinimumSize(QSize(size + 14, size + 14));
            setStyleRecursive(tb, this, size + 4);
        }
    }

    if(parentIsToolbar && (qobject_cast<QComboBox *>(widget) ||
                           qobject_cast<QLineEdit *>(widget)))
        widget->setFont(QApplication::font());

    if (qobject_cast<QMenuBar*>(widget) || qobject_cast<QToolBar*>(widget) ||
        parentIsToolbar)
        widget->setBackgroundRole(QPalette::Window);

    if (!qtcIsFlat(opts.toolbarAppearance) && parentIsToolbar) {
        widget->setAutoFillBackground(false);
    }

    if (theThemedApp == APP_SYSTEMSETTINGS &&
        qobject_cast<QTabWidget*>(getParent<2>(widget)) &&
        qobject_cast<QFrame*>(widget) &&
        ((QFrame*)widget)->frameShape() != QFrame::NoFrame &&
        qobject_cast<QFrame*>(widget->parentWidget())) {
        ((QFrame*)widget)->setFrameShape(QFrame::NoFrame);
    }

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

    if ((theThemedApp == APP_K3B &&
         widget->inherits("K3b::ThemedHeader") &&
         qobject_cast<QFrame*>(widget)) ||
        widget->inherits("KColorPatch")) {
        ((QFrame*)widget)->setLineWidth(0);
        ((QFrame*)widget)->setFrameShape(QFrame::NoFrame);
    }

    if (theThemedApp == APP_KDEVELOP && !opts.stdSidebarButtons &&
        widget->inherits("Sublime::IdealButtonBarWidget") && widget->layout()) {
        widget->layout()->setSpacing(0);
        widget->layout()->setContentsMargins(0, 0, 0, 0);;
    }

    QWidget *window=widget->window();

    if ((100 != opts.bgndOpacity && qtcIsWindow(window)) ||
        (100 != opts.dlgOpacity && qtcIsDialog(window))) {
        widget->installEventFilter(this);
        if (widget->inherits("KFilePlacesView")) {
            widget->setAutoFillBackground(false);
            widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
        }
    }

#ifdef QTC_QT6_ENABLE_KDE
    // Make file selection button in QPrintDialog appear more KUrlRequester like...
    if (qtcCheckType<QPrintDialog>(getParent<3>(widget)) &&
        qobject_cast<QToolButton*>(widget) &&
        qobject_cast<QGroupBox*>(widget->parentWidget()) &&
        static_cast<QToolButton*>(widget)->text() == QLatin1String("...")) {
        static_cast<QToolButton*>(widget)->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
        static_cast<QToolButton*>(widget)->setAutoRaise(false);
    }
#endif
}

void
Style::unpolish(QApplication *app)
{
    if (opts.hideShortcutUnderline)
        app->removeEventFilter(m_shortcutHandler);
    ParentStyleClass::unpolish(app);
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
    if (qtcIsCustomBgnd(opts) || opts.groupBox == FRAME_SHADED ||
        opts.groupBox == FRAME_FADED) {
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

    if (qobject_cast<QMdiSubWindow*>(widget))
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
            for (QStatusBar *statusBar: getStatusBars(widget)) {
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
       // qobject_cast<QDockWidget *>(widget) ||
       widget->inherits("QWorkspaceTitleBar") ||
       widget->inherits("QDockSeparator") ||
       widget->inherits("QDockWidgetSeparator"))
        widget->setAttribute(Qt::WA_Hover, false);
    if (qobject_cast<QScrollBar*>(widget)) {
        widget->setAttribute(Qt::WA_Hover, false);
        if (opts.round != ROUND_NONE && !opts.flatSbarButtons)
            widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
    } else if (qobject_cast<QProgressBar*>(widget)) {
        if(opts.boldProgress)
            m_fntHelper->unSetBold(widget);
        m_progressBars.remove((QProgressBar *)widget);
    } else if (qobject_cast<QMenuBar*>(widget)) {
        widget->setAttribute(Qt::WA_Hover, false);

        if(qtcIsCustomBgnd(opts))
            widget->setBackgroundRole(QPalette::Base);

        if(SHADE_WINDOW_BORDER==opts.shadeMenubars || opts.customMenuTextColor || SHADE_BLEND_SELECTED==opts.shadeMenubars ||
           SHADE_SELECTED==opts.shadeMenubars || (SHADE_CUSTOM==opts.shadeMenubars &&TOO_DARK(m_menubarCols[ORIGINAL_SHADE])))
            widget->setPalette(QApplication::palette());
    } else if (/*!opts.gtkScrollViews && */
        qobject_cast<QAbstractScrollArea*>(widget)) {
        if (APP_KONTACT==theThemedApp && widget->parentWidget()) {
            QWidget *frame = scrollViewFrame(widget->parentWidget());
            if (frame) {
                if (m_sViewContainers.contains(frame)) {
                    m_sViewContainers[frame].remove(widget);
                    if (0 == m_sViewContainers[frame].count()) {
                        frame->removeEventFilter(this);
                        m_sViewContainers.remove(frame);
                        disconnect(frame, &QWidget::destroyed,
                                   this, &Style::widgetDestroyed);
                    }
                }
            }
        }
    } else if (qobject_cast<QDockWidget *>(widget) &&
               ((QDockWidget*)widget)->titleBarWidget() &&
               qobject_cast<QtCurveDockWidgetTitleBar*>(
                   ((QDockWidget*)widget)->titleBarWidget()) &&
               qtcCheckType<QSplitter>(widget->parentWidget()) &&
               getParent<3>(widget) &&
               qtcCheckType(getParent<2>(widget), "KFileWidget")) {
        delete ((QDockWidget *)widget)->titleBarWidget();
        ((QDockWidget*)widget)->setTitleBarWidget(0L);
    } else if (opts.boldProgress && "CE_CapacityBar"==widget->objectName()) {
        m_fntHelper->unSetBold(widget);
    }

    if (widget->inherits("QTipLabel") && !qtcIsFlat(opts.tooltipAppearance)) {
        widget->setAttribute(Qt::WA_NoSystemBackground, false);
        widget->clearMask();
    }

    if (!widget->isWindow())
        if (QFrame *frame = qobject_cast<QFrame *>(widget)) {
            if (qtcCheckKDEType(widget->parent(), KTitleWidget)) {
                if(qtcIsCustomBgnd(opts)) {
                    frame->setAutoFillBackground(true);
                } else {
                    frame->setBackgroundRole(QPalette::Base);
                }
                QLayout *layout(frame->layout());

                if(layout) {
                    layout->setContentsMargins(6, 6, 6, 6);
                }
            }

            QComboBox *p = nullptr;
            if (opts.gtkComboMenus &&
                (p = qtcObjCast<QComboBox>(getParent<2>(widget))) &&
                !p->isEditable()) {
                widget->setPalette(QApplication::palette());
            }
        }

    if (qobject_cast<QMenu*>(widget)) {
        // TODO remove these
        widget->setAttribute(Qt::WA_NoSystemBackground, false);
        widget->clearMask();

        if (opts.lighterPopupMenuBgnd || opts.shadePopupMenu) {
            widget->setPalette(QApplication::palette());
        }
    }

    if((!qtcIsFlatBgnd(opts.menuBgndAppearance) || 100!=opts.menuBgndOpacity || !(opts.square&SQUARE_POPUP_MENUS)) &&
       widget->inherits("QComboBoxPrivateContainer")) {
        widget->setAttribute(Qt::WA_NoSystemBackground, false);
        widget->clearMask();
    }

    if (widget && (qobject_cast<QMenuBar*>(widget) ||
                   qobject_cast<QToolBar*>(widget) ||
                   qobject_cast<QToolBar*>(widget->parent()))) {
        widget->setBackgroundRole(QPalette::Button);
    }
}

bool Style::eventFilter(QObject *object, QEvent *event)
{
    if (qobject_cast<QMenuBar*>(object) &&
        dynamic_cast<QMouseEvent*>(event)) {
        if (updateMenuBarEvent((QMouseEvent*)event, (QMenuBar*)object)) {
            return true;
        }
    }

    if (event->type() == QEvent::Show &&
        qobject_cast<QAbstractScrollArea*>(object) &&
        object->inherits("KFilePlacesView")) {
        QWidget *view = ((QAbstractScrollArea*)object)->viewport();
        QPalette palette = view->palette();
        QColor color = ((QWidget*)object)->palette().window().color();

        if (qtcIsCustomBgnd(opts))
            color.setAlphaF(0.0);

        palette.setColor(view->backgroundRole(), color);
        view->setPalette(palette);
        object->removeEventFilter(this);
    }

    bool isSViewCont = (APP_KONTACT == theThemedApp &&
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
            pos=((QWheelEvent*)event)->pos();
        default:
            break;
        }

        if (!pos.isNull()) {
            QAbstractScrollArea *area = 0L;
            QPoint mapped(pos);

            if (isSViewCont) {
                QSet<QWidget*>::ConstIterator it =
                    m_sViewContainers[(QWidget*)object].begin();
                QSet<QWidget*>::ConstIterator end =
                    m_sViewContainers[(QWidget*)object].end();

                for (;it != end && !area;++it) {
                    if ((*it)->isVisible()) {
                        mapped = (*it)->mapFrom((QWidget*)object, pos);
                        if ((*it)->rect().adjusted(0, 0, 4, 4).contains(mapped)) {
                            area = (QAbstractScrollArea*)(*it);
                        }
                    }
                }
            } else {
                area = (QAbstractScrollArea*)object;
            }

            if (area) {
                QScrollBar *sbars[2] = {
                    area->verticalScrollBar(),
                    area->horizontalScrollBar()
                };

                for (int i = 0;i < 2;++i) {
                    if (sbars[i]) {
                        QRect r(i ? 0 : area->rect().right()-3,
                                i ? area->rect().bottom()-3 : 0,
                                sbars[i]->rect().width(),
                                sbars[i]->rect().height());

                        if (r.contains(pos) ||
                            (sbars[i] == m_sViewSBar &&
                             (QEvent::MouseMove == event->type() ||
                              QEvent::MouseButtonRelease == event->type()))) {
                            if (QEvent::Wheel != event->type()) {
                                struct HackEvent : public QMouseEvent {
                                    void set(const QPoint &mapped, bool vert)
                                        {
                                            l = QPointF(vert ? 0 : mapped.x(),
                                                        vert ? mapped.y() : 0);
                                            s = QPointF(s.x() + (vert ? 0 : -3),
                                                        s.y() + (vert ? -3 : 0));
                                        }
                                };
                                ((HackEvent*)event)->set(mapped, 0 == i);
                            }
                            sbars[i]->event(event);
                            if (QEvent::MouseButtonPress == event->type()) {
                                m_sViewSBar = sbars[i];
                            } else if (QEvent::MouseButtonRelease ==
                                       event->type()) {
                                m_sViewSBar = 0L;
                            }
                            return true;
                        }
                    }
                }
            }
        }
    }

    switch((int)(event->type())) {
    case QEvent::Timer:
    case QEvent::Move:
        return false; // just for performance - they can occur really often
    case QEvent::Resize:
        if(!(opts.square & SQUARE_POPUP_MENUS) &&
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

            if (re->size().height() != re->oldSize().height()) {
                emitMenuSize((QMenuBar*)object,
                             PREVIEW_MDI == m_isPreview ||
                             !((QMenuBar*)object)->isVisible() ? 0 :
                             re->size().height());
            }
        }
        break;
    case QEvent::ShortcutOverride:
        if ((opts.menubarHiding || opts.statusbarHiding) &&
            qobject_cast<QMainWindow*>(object)) {
            QMainWindow *window = static_cast<QMainWindow*>(object);

            if (window->isVisible()) {
                if (opts.menubarHiding & HIDE_KEYBOARD &&
                    window->menuWidget()) {
                    QKeyEvent *k = static_cast<QKeyEvent*>(event);

                    if (k->modifiers() & Qt::ControlModifier &&
                        k->modifiers() & Qt::AltModifier &&
                        Qt::Key_M == k->key()) {
                        toggleMenuBar(window);
                    }
                }
                if (opts.statusbarHiding & HIDE_KEYBOARD) {
                    QKeyEvent *k = static_cast<QKeyEvent*>(event);

                    if(k->modifiers()&Qt::ControlModifier && k->modifiers()&Qt::AltModifier && Qt::Key_S==k->key())
                        toggleStatusBar(window);
                }
            }
        }
        break;
    case QEvent::ShowToParent:
        if (qobject_cast<QMenuBar *>(object)) {
            if(opts.menubarHiding && m_saveMenuBarStatus && qtcMenuBarHidden(appName))
                static_cast<QMenuBar *>(object)->setHidden(true);
#ifdef Q_OS_MACOS
                if (opts.nonnativeMenubarApps.contains(appName)) {
                    QMenuBar *mnb = static_cast<QMenuBar*>(object);
                    if (mnb->isNativeMenuBar()) {
                        mnb->setNativeMenuBar(false);
                        mnb->setHidden(false);
                        mnb->setVisible(true);
                        if (!opts.currentNonnativeMenubarApps.contains(appName)) {
                            opts.currentNonnativeMenubarApps << appName;
                        }
                    }
                } else if ((QGuiApplication::platformName() == QLatin1String("cocoa"))
                        && opts.currentNonnativeMenubarApps.contains(appName)) {
                    // only force the native menubar on applications that were forced
                    // to use something else before!
                    static_cast<QMenuBar*>(object)->setNativeMenuBar(true);
                    opts.currentNonnativeMenubarApps.remove(appName);
                }
#endif
        }
        if(opts.statusbarHiding && m_saveStatusBarStatus && qobject_cast<QStatusBar *>(object) &&
           qtcStatusBarHidden(appName))
            static_cast<QStatusBar *>(object)->setHidden(true);
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
             !(opts.square & SQUARE_POPUP_MENUS)) &&
            object->inherits("QComboBoxPrivateContainer")) {
            QWidget *widget = qtcToWidget(object);
            QPainter p(widget);
            QRect r(widget->rect());
            double radius = opts.round >= ROUND_FULL ? 5.0 : 2.5;
            QStyleOption opt;
            opt.initFrom(widget);
            const QColor *use(popupMenuCols(&opt));

            p.setClipRegion(static_cast<QPaintEvent*>(event)->region());
            if (!opts.popupBorder) {
                p.setRenderHint(QPainter::Antialiasing, true);
                p.setPen(use[ORIGINAL_SHADE]);
                p.drawPath(buildPath(r, WIDGET_OTHER, ROUNDED_ALL, radius));
                QPAINTER_RENDERHINT_AA_MAYBE_OFF(&p);
            }
            if (!(opts.square&SQUARE_POPUP_MENUS))
                p.setClipRegion(windowMask(r, opts.round>ROUND_SLIGHT),
                                Qt::IntersectClip);

            // In case the gradient uses alpha, we need to fill with the background colour - this makes it consistent with Gtk.
            if(100==opts.menuBgndOpacity)
                p.fillRect(r, opt.palette.brush(QPalette::Base));
            drawBackground(&p, widget, BGND_MENU);
            if (opts.popupBorder) {
                EGradientBorder border=qtcGetGradient(opts.menuBgndAppearance, &opts)->border;

                p.setClipping(false);
                p.setPen(use[QTC_STD_BORDER]);
                // For now dont round combos - getting weird effects with shadow/clipping in Gtk2 style :-(
                if(opts.square&SQUARE_POPUP_MENUS) // || isCombo)
                    drawRect(&p, r);
                else
                {
                    p.setRenderHint(QPainter::Antialiasing, true);
                    p.drawPath(buildPath(r, WIDGET_OTHER, ROUNDED_ALL, radius));
                }

                if(qtcUseBorder(border) && APPEARANCE_FLAT!=opts.menuBgndAppearance)
                {
                    QRect ri(r.adjusted(1, 1, -1, -1));

                    p.setPen(use[0]);
                    if(GB_LIGHT==border)
                    {
                        if(opts.square&SQUARE_POPUP_MENUS) // || isCombo)
                            drawRect(&p, ri);
                        else
                            p.drawPath(buildPath(ri, WIDGET_OTHER, ROUNDED_ALL, radius-1.0));
                    }
                    else if(opts.square&SQUARE_POPUP_MENUS) // || isCombo)
                    {
                        if(GB_3D!=border)
                        {
                            p.drawLine(ri.x(), ri.y(), ri.x()+ri.width()-1,  ri.y());
                            p.drawLine(ri.x(), ri.y(), ri.x(), ri.y()+ri.height()-1);
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
        } else if (m_clickedLabel == object &&
                   qobject_cast<QLabel*>(object) &&
                   ((QLabel*)object)->buddy() &&
                   ((QLabel*)object)->buddy()->isEnabled()) {
            // paint focus rect
            QLabel                *lbl = (QLabel *)object;
            QPainter              painter(lbl);
            QStyleOptionFocusRect opts;

            opts.palette = lbl->palette();
            opts.rect    = QRect(0, 0, lbl->width(), lbl->height());
            drawPrimitive(PE_FrameFocusRect, &opts, &painter, lbl);
        } else {
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
        if(dynamic_cast<QMouseEvent*>(event) && qobject_cast<QLabel*>(object) && ((QLabel *)object)->buddy())
        {
            QLabel      *lbl = (QLabel *)object;
            QMouseEvent *mev = (QMouseEvent *)event;

            if (lbl->rect().contains(mev->pos()))
            {
                m_clickedLabel=lbl;
                lbl->repaint();
            }
        }
        break;
    case QEvent::MouseButtonRelease:
        if(dynamic_cast<QMouseEvent*>(event) && qobject_cast<QLabel*>(object) && ((QLabel *)object)->buddy())
        {
            QLabel      *lbl = (QLabel *)object;
            QMouseEvent *mev = (QMouseEvent *)event;

            if(m_clickedLabel)
            {
                m_clickedLabel=0;
                lbl->update();
            }

            // set focus to the buddy...
            if (lbl->rect().contains(mev->pos()))
                ((QLabel *)object)->buddy()->setFocus(Qt::ShortcutFocusReason);
        }
        break;
    case QEvent::StyleChange:
    case QEvent::Show:
    {
        QProgressBar *bar = qobject_cast<QProgressBar *>(object);

        if(bar)
        {
            m_progressBars.insert(bar);
            if (!m_progressBarAnimateTimer) {
                if (opts.animatedProgress || (0 == bar->minimum() && 0 == bar->maximum())) {
                    // we know we'll need a timer, start it at once
                    if (!m_timer.isValid())
                        m_timer.start();
                    m_progressBarAnimateFps = constProgressBarFps;
                    m_progressBarAnimateTimer = startTimer(1000/m_progressBarAnimateFps);
                }
            }
        } else if (!(opts.square & SQUARE_POPUP_MENUS) &&
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
            QMenuBar *mb=(QMenuBar*)object;
            emitMenuSize((QMenuBar*)mb, PREVIEW_MDI==m_isPreview ||
                         !((QMenuBar *)mb)->isVisible() ? 0 :
                         mb->size().height(), true);
        } else if (QEvent::Show==event->type()) {
            QWidget *widget = qtcToWidget(object);

            if(widget && widget->isWindow() &&
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
        if ((BLEND_TITLEBAR ||
             opts.windowBorder &
             WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR ||
             opts.menubarHiding & HIDE_KWIN) &&
           qobject_cast<QMenuBar*>(object)) {
            QMenuBar *mb = (QMenuBar*)object;
            emitMenuSize((QMenuBar*)mb, 0);
        }
        // if(m_hoverWidget && object==m_hoverWidget) {
        //     // m_pos.setX(-1);
        //     // m_pos.setY(-1);
        //     m_hoverWidget=0L;
        // }

        // The Destroy event is sent from ~QWidget, which happens after ~QProgressBar - therefore, we can't cast to a QProgressBar.
        // So we have to check on object.
        if (object && !m_progressBars.isEmpty()) {
            m_progressBars.remove(reinterpret_cast<QProgressBar*>(object));
            if (m_progressBars.isEmpty() && m_progressBarAnimateTimer) {
                killTimer(m_progressBarAnimateTimer);
                m_progressBarAnimateTimer = 0;
            }
        }
        break;
    }
    case QEvent::Enter:
        break;
    case QEvent::Leave:
        // if(m_hoverWidget && object==m_hoverWidget)
        // {
        //     // m_pos.setX(-1);
        //     // m_pos.setY(-1);
        //     m_hoverWidget=0L;
        //     ((QWidget *)object)->repaint();
        // }
        break;
    case QEvent::MouseMove:  // Only occurs for widgets with mouse tracking enabled
        break;
    case QEvent::FocusIn:
    case QEvent::FocusOut:
        break;
    case QEvent::WindowActivate:
        if(opts.shadeMenubarOnlyWhenActive && SHADE_NONE!=opts.shadeMenubars && qobject_cast<QMenuBar *>(object))
        {
            m_active=true;
            ((QWidget *)object)->repaint();
            return false;
        }
        break;
    case QEvent::WindowDeactivate:
        if(opts.shadeMenubarOnlyWhenActive && SHADE_NONE!=opts.shadeMenubars && qobject_cast<QMenuBar *>(object))
        {
            m_active=false;
            ((QWidget *)object)->repaint();
            return false;
        }
        break;
    default:
        break;
    }

    return ParentStyleClass::eventFilter(object, event);
}

void Style::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_progressBarAnimateTimer) {
        bool hasAnimation = false;
        m_animateStep = m_timer.elapsed() / (1000 / constProgressBarFps);
        for (QProgressBar *bar: const_(m_progressBars)) {
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

int
Style::pixelMetric(PixelMetric metric, const QStyleOption *option,
                   const QWidget *widget) const
{
    prePolish(widget);
    switch((unsigned)metric) {
    case PM_ToolTipLabelFrameWidth:
        if (opts.round != ROUND_NONE && !(opts.square & SQUARE_TOOLTIPS))
            return 3;
        return ParentStyleClass::pixelMetric(metric, option, widget);
    case PM_MdiSubWindowFrameWidth:
        return 3;
    case PM_DockWidgetTitleMargin:
        if (!(opts.dwtSettings & DWT_TEXT_ALIGN_AS_PER_TITLEBAR) ||
            opts.titlebarAlignment == ALIGN_LEFT)
            return 4;
        return 0;
    case PM_DockWidgetTitleBarButtonMargin:
        return 4;
    case PM_DockWidgetFrameWidth:
        return 2;
    case PM_ToolBarExtensionExtent:
        return 15;
#ifndef QTC_QT6_ENABLE_KDE
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
        return (opts.popupBorder ?
                pixelMetric(PM_DefaultFrameWidth, option, widget) : 0);
    case PM_SizeGripSize:
        return SIZE_GRIP_SIZE;
    case PM_TabBarScrollButtonWidth:
        return 18;
    case PM_HeaderMargin:
        return 3;
    case PM_DefaultChildMargin:
        return isOOWidget(widget) ? 2 : 6;
    case PM_DefaultTopLevelMargin:
        return 9;
    case PM_LayoutHorizontalSpacing:
    case PM_LayoutVerticalSpacing:
        return -1; // use layoutSpacing
    case PM_LayoutLeftMargin:
    case PM_LayoutTopMargin:
    case PM_LayoutRightMargin:
    case PM_LayoutBottomMargin:
        return pixelMetric((option && (option->state & State_Window)) ||
                           (widget && widget->isWindow()) ?
                           PM_DefaultTopLevelMargin : PM_DefaultChildMargin,
                           option, widget);
    case PM_MenuBarItemSpacing:
        return 0;
    case PM_ToolBarItemMargin:
        // with two locked toolbars together the last/first items are too close when there is no frame,
        // so add a margin instead.
        return opts.toolbarBorders == TB_NONE ? 1 : 0;
    case PM_ToolBarItemSpacing:
        return opts.tbarBtns == TBTN_JOINED ? 0 : 1;
    case PM_ToolBarFrameWidth:
        return opts.toolbarBorders == TB_NONE ? 0 : 1;
    case PM_FocusFrameVMargin:
    case PM_FocusFrameHMargin:
        return 2;
    case PM_MenuBarVMargin:
    case PM_MenuBarHMargin:
        // Bangarang (media player) has a 4 pixel high menubar at the top -
        // when it doesn't actually have a menubar! Seems to be because of
        // the return 2 below (which was previously always returned unless
        // XBar support and size was 0). So, if we are askes for these metrics
        // for a widet whose size<6, then return 0.
        return widget && widget->size().height() < 6 ? 0 : 2;
    case PM_MenuHMargin:
    case PM_MenuVMargin:
        return 0;
    case PM_MenuButtonIndicator:
        return ((opts.buttonEffect != EFFECT_NONE ? 10 : 9) +
                (!widget || qobject_cast<const QToolButton*>(widget) ? 6 : 0));
    case PM_ButtonMargin:
        return (opts.buttonEffect != EFFECT_NONE ? (opts.thin & THIN_BUTTONS) ?
                4 : 6 : (opts.thin & THIN_BUTTONS) ? 2 : 4) + MAX_ROUND_BTN_PAD;
    case PM_TabBarTabShiftVertical:
        return 2;
    case PM_TabBarTabShiftHorizontal:
        return 0;
    case PM_ButtonShiftHorizontal:
        // return Qt::RightToLeft==QApplication::layoutDirection() ? -1 : 1;
    case PM_ButtonShiftVertical:
        return (theThemedApp == APP_KDEVELOP && !opts.stdSidebarButtons &&
                widget && isMultiTabBarTab(getButton(widget, 0L)) ? 0 : 1);
    case PM_ButtonDefaultIndicator:
        return 0;
    case PM_DefaultFrameWidth:
        if (opts.gtkComboMenus &&
            qtcCheckType(widget,"QComboBoxPrivateContainer")) {
            return (opts.gtkComboMenus ?
                    (opts.borderMenuitems ||
                     !(opts.square & SQUARE_POPUP_MENUS) ? 2 : 1) : 0);
        }
        if ((!opts.gtkScrollViews || (opts.square & SQUARE_SCROLLVIEW)) &&
            isKateView(widget))
            return (opts.square&SQUARE_SCROLLVIEW) ? 1 : 0;

        if ((opts.square & SQUARE_SCROLLVIEW) && widget && !opts.etchEntry &&
            (qobject_cast<const QAbstractScrollArea*>(widget) ||
             isKontactPreviewPane(widget)))
            return ((opts.gtkScrollViews || opts.thinSbarGroove ||
                     !opts.borderSbarGroove) && (!opts.highlightScrollViews) ?
                    1 : 2);

        if  (!qtcDrawMenuBorder(opts) && !opts.borderMenuitems &&
             opts.square & SQUARE_POPUP_MENUS &&
             qobject_cast<const QMenu*>(widget))
            return 1;

        if (opts.buttonEffect != EFFECT_NONE && opts.etchEntry &&
            (!widget || // !isFormWidget(widget) &&
            qobject_cast<const QLineEdit*>(widget) ||
             qobject_cast<const QAbstractScrollArea*>(widget)
             /*|| isKontactPreviewPane(widget)*/)) {
            return 3;
        } else {
            return 2;
        }
    case PM_SpinBoxFrameWidth:
        return opts.buttonEffect != EFFECT_NONE && opts.etchEntry ? 3 : 2;
    case PM_IndicatorWidth:
    case PM_IndicatorHeight:
    case PM_ExclusiveIndicatorWidth:
    case PM_ExclusiveIndicatorHeight:
        // TODO??
        // case PM_CheckListControllerSize:
        // case PM_CheckListButtonSize:
        return opts.buttonEffect != EFFECT_NONE ? opts.crSize+2 : opts.crSize;
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
        return opts.sliderStyle == SLIDER_TRIANGULAR ? 5 : 4;
    case PM_SliderSpaceAvailable:
        if (auto slider = styleOptCast<QStyleOptionSlider>(option)) {
            int size = pixelMetric(PM_SliderControlThickness, slider, widget);

            if (slider->tickPosition & QSlider::TicksBelow)
                ++size;
            if (slider->tickPosition & QSlider::TicksAbove)
                ++size;
            return size;
        }
        return ParentStyleClass::pixelMetric(metric, option, widget);
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
        return qMax(widget ? widget->fontMetrics().lineSpacing() :
                    option ? option->fontMetrics.lineSpacing() : 0, 24);
    case PM_MenuBarPanelWidth:
        return 0;
    case QtC_Round:
        return (int)((opts.square & SQUARE_WINDOWS && opts.round>ROUND_SLIGHT) ?
                     ROUND_SLIGHT : opts.round);
    case QtC_WindowBorder:
        return opts.windowBorder;
    case QtC_CustomBgnd:
        return qtcIsCustomBgnd(opts);
    case QtC_TitleAlignment:
        switch (opts.titlebarAlignment) {
        default:
        case ALIGN_LEFT:
            return Qt::AlignLeft;
        case ALIGN_CENTER:
            return Qt::AlignHCenter | Qt::AlignVCenter;
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
        return ((opts.menubarHiding & HIDE_KWIN   ? 0x1 : 0) +
                (opts.statusbarHiding & HIDE_KWIN ? 0x2 : 0));
    case QtC_MenubarColor:
        return m_menubarCols[ORIGINAL_SHADE].rgb();
    case QtC_TitleBarApp:
        return (!option || option->state & State_Active ?
                opts.titlebarAppearance : opts.inactiveTitlebarAppearance);
        // The following was a somewhat hackyish fix for konqueror's show close
        // button on tab setting...... its hackish in the way that I'm assuming
        // when KTabBar is positioning the close button and it asks for these
        // options, it only passes in a QStyleOption  not a QStyleOptionTab
        // Now that KTabBar is deprecated and (KF5) code should use QTabBar instead
        // it isn't certain these hacks ought to be preserved.
    case PM_TabBarBaseHeight:
        if (qtcCheckKDEType(widget, QTabBar) &&
            !styleOptCast<QStyleOptionTab>(option)) {
            return 10;
        }
        return ParentStyleClass::pixelMetric(metric, option, widget);
    case PM_TabBarBaseOverlap:
        if (qtcCheckKDEType(widget, QTabBar) &&
            !styleOptCast<QStyleOptionTab>(option)) {
            return 0;
        }
        // Fall through!
    default:
        return ParentStyleClass::pixelMetric(metric, option, widget);
    }
}

int
Style::styleHint(StyleHint hint, const QStyleOption *option,
                 const QWidget *widget, QStyleHintReturn *returnData) const
{
    prePolish(widget);
    switch (hint) {
    case SH_ToolTip_Mask:
    case SH_Menu_Mask:
        if ((SH_ToolTip_Mask == hint && (opts.square & SQUARE_TOOLTIPS)) ||
            (SH_Menu_Mask == hint && (opts.square & SQUARE_POPUP_MENUS))) {
            return ParentStyleClass::styleHint(hint, option, widget, returnData);
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
        return ((widget && opts.hideShortcutUnderline) ?
                m_shortcutHandler->showShortcut(widget) : true);
    case SH_GroupBox_TextLabelVerticalAlignment:
        if (auto frame = styleOptCast<QStyleOptionGroupBox>(option)) {
            if (frame->features & QStyleOptionFrame::Flat) {
                return Qt::AlignVCenter;
            }
        }
        if (opts.gbLabel & GB_LBL_INSIDE) {
            return Qt::AlignBottom;
        } else if (opts.gbLabel & GB_LBL_OUTSIDE) {
            return Qt::AlignTop;
        } else {
            return Qt::AlignVCenter;
        }
    case SH_MessageBox_CenterButtons:
    case SH_ProgressDialog_CenterCancelButton:
    case SH_DitherDisabledText:
    case SH_EtchDisabledText:
    case SH_Menu_AllowActiveAndDisabled:
    case SH_ItemView_ShowDecorationSelected:
        // Controls whether the highlighting of listview/treeview
        // items highlights whole line.
    case SH_MenuBar_AltKeyNavigation:
        return false;
    case SH_ItemView_ChangeHighlightOnFocus:
        // gray out selected items when losing focus.
        return false;
    case SH_WizardStyle:
        return QWizard::ClassicStyle;
    case SH_RubberBand_Mask: {
        auto opt = styleOptCast<QStyleOptionRubberBand>(option);
        if (!opt) {
            return true;
        }
        if (auto mask = styleOptCast<QStyleHintReturnMask>(returnData)) {
            mask->region = option->rect;
            mask->region -= option->rect.adjusted(1,1,-1,-1);
        }
        return true;
    }
    case SH_Menu_SubMenuPopupDelay:
        return opts.menuDelay;
#if QT_VERSION >= QT_VERSION_CHECK(5,5,0)
    case SH_Menu_SloppySubMenus:
        return opts.menuCloseDelay > 0;
    case SH_Menu_SubMenuSloppyCloseTimeout:
        return opts.menuCloseDelay;
#endif
    case SH_ToolButton_PopupDelay:
        return 250;
    case SH_ComboBox_PopupFrameStyle:
        return opts.popupBorder || !(opts.square&SQUARE_POPUP_MENUS) ? QFrame::StyledPanel|QFrame::Plain : QFrame::NoFrame;
    case SH_TabBar_Alignment:
        return Qt::AlignLeft;
    case SH_Header_ArrowAlignment:
        return Qt::AlignLeft;
    case SH_WindowFrame_Mask:
        if (auto mask = styleOptCast<QStyleHintReturnMask>(returnData)) {
            const QRect &r = option->rect;
            switch ((opts.square & SQUARE_WINDOWS &&
                     opts.round > ROUND_SLIGHT) ? ROUND_SLIGHT : opts.round) {
            case ROUND_NONE:
                mask->region = r;
                break;
            case ROUND_SLIGHT:
                mask->region = QRegion(r.x() + 1, r.y(),
                                       r.width() - 2, r.height());
                mask->region += QRegion(r.x() + 0, r.y() + 1,
                                        1, r.height() - 2);
                mask->region += QRegion(r.x() + r.width() - 1, r.y() + 1,
                                        1, r.height() - 2);
                break;
            default: // ROUND_FULL
                mask->region = QRegion(r.x() + 5, r.y(),
                                       r.width() - 10, r.height());
                mask->region += QRegion(r.x() + 0, r.y() + 5,
                                        1, r.height() - 5);
                mask->region += QRegion(r.x() + 1, r.y() + 3,
                                        1, r.height() - 2);
                mask->region += QRegion(r.x() + 2, r.y() + 2,
                                        1, r.height() - 1);
                mask->region += QRegion(r.x() + 3, r.y() + 1,
                                        2, r.height());
                mask->region += QRegion(r.x() + r.width() - 1, r.y() + 5,
                                        1, r.height() - 5);
                mask->region += QRegion(r.x() + r.width() - 2, r.y() + 3,
                                        1, r.height() - 2);
                mask->region += QRegion(r.x() + r.width() - 3, r.y() + 2,
                                        1, r.height() - 1);
                mask->region += QRegion(r.x() + r.width() - 5, r.y() + 1,
                                        2, r.height() - 0);
            }
        }
        return 1;
    case SH_TitleBar_NoBorder:
    case SH_TitleBar_AutoRaise:
        return 1;
    case SH_MainWindow_SpaceBelowMenuBar:
        return 0;
    case SH_DialogButtonLayout:
        if (opts.gtkButtonOrder)
            return QDialogButtonBox::GnomeLayout;
        return QDialogButtonBox::KdeLayout;
    case SH_MessageBox_TextInteractionFlags:
        return Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse;
    case SH_LineEdit_PasswordCharacter:
        if (opts.passwordChar) {
            int chars[4] = {opts.passwordChar, 0x25CF, 0x2022, 0};
            const QFontMetrics &fm(option ? option->fontMetrics :
                                   (widget ? widget->fontMetrics() :
                                    QFontMetrics(QFont())));
            for (int i = 0;chars[i];i++) {
                if (fm.inFont(QChar(chars[i]))) {
                    return chars[i];
                }
            }
            return '*';
        } else {
            return '\0';
        }
    case SH_MenuBar_MouseTracking:
        // Always return 1, as setting to 0 dissables the effect when
        // a menu is shown.
        return 1; // opts.menubarMouseOver ? 1 : 0;
    case SH_ScrollView_FrameOnlyAroundContents:
        return (widget && widget->isWindow() ? false :
                opts.gtkScrollViews &&
                !qtcCheckType(widget, "QComboBoxListView"));
    case SH_ComboBox_Popup:
        if (opts.gtkComboMenus) {
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
#ifdef QTC_QT6_ENABLE_KDE
    case SH_DialogButtonBox_ButtonsHaveIcons:{
        KConfigGroup cg(m_kdeGlobals, "KDE");
        return cg.readEntry("ShowIconsOnPushButtons", false);
    }
    case SH_ItemView_ActivateItemOnSingleClick:{
        KConfigGroup cg(m_kdeGlobals, "KDE");
        return cg.readEntry("SingleClick", false);
    }
#endif
    case SH_Widget_Animate:
        return false;
    case SH_Menu_SupportsSections:
        return true;
    default:
#ifdef QTC_QT6_ENABLE_KDE
        // Tell the calling app that we can handle certain custom widgets...
        if (hint >= SH_CustomBase && widget) {
            if (widget->objectName() == "CE_CapacityBar") {
                if (opts.boldProgress) {
                    m_fntHelper->setBold(const_cast<QWidget*>(widget));
                }
                return CE_QtC_KCapacityBar;
            }
        }
#endif
        return ParentStyleClass::styleHint(hint, option, widget, returnData);
    }
}

QPalette Style::standardPalette() const
{
    return ParentStyleClass::standardPalette();
}

void Style::initFontTickData(const QFont &font, const QWidget*) const
{
    if (opts.onlyTicksInMenu && opts.fontTickWidth <= 0) {
        opts.tickFont = font;
#ifndef Q_OS_MACOS
        opts.tickFont.setBold(true);
#else
        opts.tickFont.setFamily(QStringLiteral("Apple Symbols"));
        opts.tickFont.setBold(false);
#endif
        // adjust the size so the tickmark looks just about right
        opts.tickFont.setPointSizeF(opts.tickFont.pointSizeF() * 1.3);
        opts.fontTickWidth = horizontalAdvance(QFontMetrics(opts.tickFont), opts.menuTick);
    }
}

static bool menuTickCompensation(const Options &opts, int refWidth, int &dx)
{
    if (opts.onlyTicksInMenu && opts.fontTickWidth > refWidth) {
        dx = opts.fontTickWidth - refWidth;
        return true;
    }
    return false;
}

void
Style::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const
{
    prePolish(widget);
    bool (Style::*drawFunc)(PrimitiveElement, const QStyleOption*,
                            QPainter*, const QWidget*) const = nullptr;
    switch ((unsigned)element) {
    case PE_IndicatorTabClose:
        drawFunc = &Style::drawPrimitiveIndicatorTabClose;
        break;
    case PE_Widget:
        drawFunc = &Style::drawPrimitiveWidget;
        break;
    case PE_PanelScrollAreaCorner:
        drawFunc = &Style::drawPrimitivePanelScrollAreaCorner;
        break;
    case PE_IndicatorBranch:
        drawFunc = &Style::drawPrimitiveIndicatorBranch;
        break;
    case PE_IndicatorViewItemCheck:
        drawFunc = &Style::drawPrimitiveIndicatorViewItemCheck;
        break;
    case PE_IndicatorHeaderArrow:
        drawFunc = &Style::drawPrimitiveIndicatorHeaderArrow;
        break;
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowLeft:
    case PE_IndicatorArrowRight:
        drawFunc = &Style::drawPrimitiveIndicatorArrow;
        break;
    case PE_IndicatorSpinMinus:
    case PE_IndicatorSpinPlus:
    case PE_IndicatorSpinUp:
    case PE_IndicatorSpinDown:
        drawFunc = &Style::drawPrimitiveIndicatorSpin;
        break;
    case PE_IndicatorToolBarSeparator:
        drawFunc = &Style::drawPrimitiveIndicatorToolBarSeparator;
        break;
    case PE_FrameGroupBox:
        drawFunc = &Style::drawPrimitiveFrameGroupBox;
        break;
    case PE_Frame:
        drawFunc = &Style::drawPrimitiveFrame;
        break;
    case PE_PanelMenuBar:
        drawFunc = &Style::drawPrimitivePanelMenuBar;
        break;
    case PE_FrameTabBarBase:
        drawFunc = &Style::drawPrimitiveFrameTabBarBase;
        break;
    case PE_FrameStatusBar:
    case PE_FrameMenu:
        initFontTickData(widget ? widget->font() : QApplication::font("QMenu"), widget);
        drawFunc = &Style::drawPrimitiveFrameStatusBarOrMenu;
        break;
    case PE_FrameDockWidget:
        drawFunc = &Style::drawPrimitiveFrameDockWidget;
        break;
    case PE_FrameButtonTool:
    case PE_PanelButtonTool:
    case PE_IndicatorButtonDropDown:
        drawFunc = &Style::drawPrimitiveButtonTool;
        break;
    case PE_IndicatorDockWidgetResizeHandle:
        drawFunc = &Style::drawPrimitiveIndicatorDockWidgetResizeHandle;
        break;
    case PE_PanelLineEdit:
        drawFunc = &Style::drawPrimitivePanelLineEdit;
        break;
    case PE_FrameLineEdit:
        drawFunc = &Style::drawPrimitiveFrameLineEdit;
        break;
    case PE_IndicatorMenuCheckMark:
    case PE_IndicatorCheckBox:
        drawFunc = &Style::drawPrimitiveIndicatorCheckBox;
        break;
    case PE_IndicatorRadioButton:
        drawFunc = &Style::drawPrimitiveIndicatorRadioButton;
        break;
    case PE_IndicatorToolBarHandle:
        drawFunc = &Style::drawPrimitiveIndicatorToolBarHandle;
        break;
    case PE_FrameFocusRect:
        drawFunc = &Style::drawPrimitiveFrameFocusRect;
        break;
    case PE_FrameButtonBevel:
    case PE_PanelButtonBevel:
    case PE_PanelButtonCommand:
        drawFunc = &Style::drawPrimitiveButton;
        break;
    case PE_FrameDefaultButton:
        drawFunc = &Style::drawPrimitiveNone;
        break;
    case PE_FrameWindow:
        drawFunc = &Style::drawPrimitiveFrameWindow;
        break;
    case PE_FrameTabWidget:
        drawFunc = &Style::drawPrimitiveFrameTabWidget;
        break;
    case PE_PanelItemViewItem:
        drawFunc = &Style::drawPrimitivePanelItemViewItem;
        break;
    case QtC_PE_DrawBackground:
        drawFunc = &Style::drawPrimitiveQtcBackground;
        break;
    case PE_PanelTipLabel:
        drawFunc = &Style::drawPrimitivePanelTipLabel;
        break;
    case PE_PanelMenu:
        initFontTickData(widget ? widget->font() : QApplication::font("QMenu"), widget);
        drawFunc = &Style::drawPrimitivePanelMenu;
        break;
    default:
        break;
    }
    painter->save();
    if (!drawFunc ||
        qtcUnlikely(!(this->*drawFunc)(element, option, painter, widget))) {
        ParentStyleClass::drawPrimitive(element, option, painter, widget);
    }
    painter->restore();
}

void
Style::drawControl(ControlElement element, const QStyleOption *option,
                   QPainter *painter, const QWidget *widget) const
{
    prePolish(widget);
    QRect r = option->rect;
    const State &state = option->state;
    const QPalette &palette = option->palette;
    bool reverse = option->direction == Qt::RightToLeft;

    switch ((unsigned)element) {
    case CE_QtC_SetOptions:
        if (auto preview = styleOptCast<PreviewOption>(option)) {
            if (!painter && widget &&
                widget->objectName() == QLatin1String("QtCurveConfigDialog")) {
                Style *that = (Style*)this;
                opts = preview->opts;
                qtcCheckConfig(&opts);
                that->init(true);
            }
        }
        break;
    case CE_QtC_Preview:
        if (auto preview = styleOptCast<PreviewOption>(option)) {
            if (widget &&
                widget->objectName() ==
                QLatin1String("QtCurveConfigDialog-GradientPreview")) {
                Options old = opts;
                const QColor *use(buttonColors(option));
                opts = preview->opts;

                drawLightBevelReal(painter, r, option, widget, ROUNDED_ALL,
                                   getFill(option, use, false, false), use,
                                   true, WIDGET_STD_BUTTON, false, opts.round,
                                   false);
                opts = old;
            }
        }
        break;
    case CE_QtC_KCapacityBar:
        if (auto bar = styleOptCast<QStyleOptionProgressBar>(option)) {
            QStyleOptionProgressBar mod = *bar;

            if (mod.rect.height() > 16 &&
                (qtcCheckType<QStatusBar>(widget->parentWidget()) ||
                 qtcCheckType(widget->parentWidget(), "DolphinStatusBar"))) {
                int m = (mod.rect.height() - 16) / 2;
                mod.rect.adjust(0, m, 0, -m);
            }
            drawControl(CE_ProgressBarGroove, &mod, painter, widget);
            if (opts.buttonEffect != EFFECT_NONE && opts.borderProgress)
                mod.rect.adjust(1, 1, -1, -1);
            drawControl(CE_ProgressBarContents, &mod, painter, widget);
            drawControl(CE_ProgressBarLabel, &mod, painter, widget);
        }
        break;
    case CE_ToolBoxTabShape: {
        auto tb = styleOptCast<QStyleOptionToolBox>(option);
        if (!(tb && widget))
            break;
        const QColor *use =
            backgroundColors(widget->palette().color(QPalette::Window));
        QPainterPath path;
        int y = r.height() * 15 / 100;

        painter->save();
        if (reverse) {
            path.moveTo(r.left() + 52, r.top());
            path.cubicTo(QPointF(r.left() + 50 - 8, r.top()),
                         QPointF(r.left() + 50 - 10, r.top() + y),
                         QPointF(r.left() + 50 - 10, r.top() + y));
            path.lineTo(r.left() + 18 + 9, r.bottom() - y);
            path.cubicTo(QPointF(r.left() + 18 + 9, r.bottom() - y),
                         QPointF(r.left() + 19 + 6, r.bottom() - 1 - 0.3),
                         QPointF(r.left() + 19, r.bottom() - 1 - 0.3));
        } else {
            path.moveTo(r.right() - 52, r.top());
            path.cubicTo(QPointF(r.right() - 50 + 8, r.top()),
                         QPointF(r.right() - 50 + 10, r.top() + y),
                         QPointF(r.right() - 50 + 10, r.top() + y));
            path.lineTo(r.right() - 18 - 9, r.bottom() - y);
            path.cubicTo(QPointF(r.right() - 18 - 9, r.bottom() - y),
                         QPointF(r.right() - 19 - 6, r.bottom() - 1 - 0.3),
                         QPointF(r.right() - 19, r.bottom() - 1 - 0.3));
        }

        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->translate(0, 1);
        painter->setPen(use[0]);
        painter->drawPath(path);
        painter->translate(0, -1);
        painter->setPen(use[4]);
        painter->drawPath(path);
        QPAINTER_RENDERHINT_AA_MAYBE_OFF(painter);
        if (reverse) {
            painter->drawLine(r.left() + 50 - 1, r.top(), r.right(), r.top());
            painter->drawLine(r.left() + 20, r.bottom() - 2,
                              r.left(), r.bottom() - 2);
            painter->setPen(use[0]);
            painter->drawLine(r.left() + 50, r.top() + 1,
                              r.right(), r.top() + 1);
            painter->drawLine(r.left() + 20, r.bottom() - 1,
                              r.left(), r.bottom() - 1);
        } else {
            painter->drawLine(r.left(), r.top(), r.right() - 50 + 1, r.top());
            painter->drawLine(r.right() - 20, r.bottom() - 2,
                              r.right(), r.bottom() - 2);
            painter->setPen(use[0]);
            painter->drawLine(r.left(), r.top() + 1,
                              r.right() - 50, r.top() + 1);
            painter->drawLine(r.right() - 20, r.bottom() - 1,
                              r.right(), r.bottom() - 1);
        }
        painter->restore();
        break;
    }
    case CE_MenuScroller: {
        const QColor *use = popupMenuCols();
        painter->fillRect(r, use[ORIGINAL_SHADE]);
        painter->setPen(use[QTC_STD_BORDER]);
        drawRect(painter, r);
        drawPrimitive(((state & State_DownArrow) ? PE_IndicatorArrowDown :
                       PE_IndicatorArrowUp), option, painter, widget);
        break;
    }
    case CE_RubberBand: {
        // Rubber band used in such things as iconview.
        if (r.width() > 0 && r.height() > 0) {
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
    case CE_Splitter: {
        const QColor *use = buttonColors(option);
        const QColor *border = borderColors(option, use);
        // In Amarok nightly (2.2) State_Horizontal doesn't
        // seem to always be set...
        bool horiz = (state & State_Horizontal ||
                      (r.height() > 6 && r.height() > r.width()));

        painter->save();
        if (/*qtcIsFlatBgnd(opts.bgndAppearance) || */
            state & State_MouseOver && state & State_Enabled) {
            QColor color(palette.color(QPalette::Active, QPalette::Window));

            if (state & State_MouseOver && state & State_Enabled &&
                opts.splitterHighlight) {
                if (ROUND_NONE != opts.round) {
                    painter->save();
                    painter->setRenderHint(QPainter::Antialiasing, true);
                    double radius =
                        qtcGetRadius(&opts, r.width(), r.height(),
                                     WIDGET_OTHER, RADIUS_SELECTION);

                    drawBevelGradient(shade(palette.window().color(),
                                            TO_FACTOR(opts.splitterHighlight)),
                                      painter, r,
                                      buildPath(QRectF(r), WIDGET_OTHER,
                                                ROUNDED_ALL, radius),
                                      !(state & State_Horizontal), false,
                                      opts.selectionAppearance,
                                      WIDGET_SELECTION, false);
                    painter->restore();
                } else {
                    drawBevelGradient(shade(palette.window().color(),
                                            TO_FACTOR(opts.splitterHighlight)),
                                      painter, r, !(state & State_Horizontal),
                                      false, opts.selectionAppearance,
                                      WIDGET_SELECTION);
                }
            } else {
                painter->fillRect(r, color);
            }
        }

        switch (opts.splitters) {
        case LINE_NONE:
            break;
        case LINE_1DOT:
            painter->drawPixmap(r.x() + ((r.width() - 5) / 2),
                                r.y() + ((r.height() - 5) / 2),
                                *getPixmap(border[QTC_STD_BORDER],
                                           PIX_DOT, 1.0));
            break;
        default:
        case LINE_DOTS:
            drawDots(painter, r, horiz, NUM_SPLITTER_DASHES, 1, border, 0, 5);
            break;
        case LINE_FLAT:
        case LINE_SUNKEN:
        case LINE_DASHES:
            drawLines(painter, r, horiz, NUM_SPLITTER_DASHES, 3,
                      border, 0, 3, opts.splitters);
        }
        painter->restore();
        break;
    }
    case CE_SizeGrip: {
        QPolygon triangle(3);
        Qt::Corner corner;
        int size = SIZE_GRIP_SIZE - 2;

        if (auto sgrp = styleOptCast<QStyleOptionSizeGrip>(option)) {
            corner = sgrp->corner;
        } else if (option->direction == Qt::RightToLeft) {
            corner = Qt::BottomLeftCorner;
        } else {
            corner = Qt::BottomRightCorner;
        }

        switch (corner) {
        case Qt::BottomLeftCorner:
            triangle.putPoints(0, 3, 0, 0, size, size, 0, size);
            triangle.translate(r.x(), r.y() + (r.height() -
                                               (SIZE_GRIP_SIZE - 1)));
            break;
        case Qt::BottomRightCorner:
            triangle.putPoints(0, 3, size, 0, size, size, 0, size);
            triangle.translate(r.x() + (r.width() - (SIZE_GRIP_SIZE - 1)),
                               r.y() + (r.height() - (SIZE_GRIP_SIZE - 1)));
            break;
        case Qt::TopRightCorner:
            triangle.putPoints(0, 3, 0, 0, size, 0, size, size);
            triangle.translate(r.x() + (r.width() - (SIZE_GRIP_SIZE - 1)),
                               r.y());
            break;
        case Qt::TopLeftCorner:
            triangle.putPoints(0, 3, 0, 0, size, 0, 0, size);
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
            if (!widget || !widget->parent() ||
                qobject_cast<QMainWindow*>(widget->parent())) {
                painter->save();
                drawMenuOrToolBarBackground(
                    widget, painter, r, option, false,
                    oneOf(toolbar->toolBarArea, Qt::NoToolBarArea,
                          Qt::BottomToolBarArea, Qt::TopToolBarArea));
                if (opts.toolbarBorders != TB_NONE) {
                    const QColor *use = backgroundColors(option);
                    bool dark = oneOf(opts.toolbarBorders,
                                      TB_DARK, TB_DARK_ALL);

                    if (oneOf(opts.toolbarBorders, TB_DARK_ALL, TB_LIGHT_ALL)) {
                        painter->setPen(use[0]);
                        painter->drawLine(r.x(), r.y(),
                                          r.x() + r.width() - 1, r.y());
                        painter->drawLine(r.x(), r.y(),
                                          r.x(), r.y() + r.height() - 1);
                        painter->setPen(use[dark ? 3 : 4]);
                        painter->drawLine(r.x(), r.y() + r.height() - 1,
                                          r.x() + r.width() - 1,
                                          r.y() + r.height() - 1);
                        painter->drawLine(r.x() + r.width() - 1, r.y(),
                                          r.x() + r.width() - 1,
                                          r.y() + r.height() - 1);
                    } else {
                        bool paintH = true;
                        bool paintV = true;

                        switch (toolbar->toolBarArea) {
                        case Qt::BottomToolBarArea:
                        case Qt::TopToolBarArea:
                            paintV = false;
                            break;
                        case Qt::RightToolBarArea:
                        case Qt::LeftToolBarArea:
                            paintH = false;
                        default:
                            break;
                        }

                        painter->setPen(use[0]);
                        if (paintH) {
                            painter->drawLine(r.x(), r.y(),
                                              r.x() + r.width() - 1, r.y());
                        }
                        if (paintV) {
                            painter->drawLine(r.x(), r.y(), r.x(),
                                              r.y() + r.height()-1);
                        }
                        painter->setPen(use[dark ? 3 : 4]);
                        if (paintH) {
                            painter->drawLine(r.x(), r.y() + r.height() - 1,
                                              r.x() + r.width() - 1,
                                              r.y() + r.height() - 1);
                        }
                        if (paintV) {
                            painter->drawLine(r.x() + r.width() - 1,
                                              r.y(), r.x() + r.width() - 1,
                                              r.y() + r.height() - 1);
                        }
                    }
                }
                painter->restore();
            }
        }
        break;
    case CE_DockWidgetTitle:
        if (auto dwOpt = styleOptCast<QStyleOptionDockWidget>(option)) {
            bool verticalTitleBar = dwOpt->verticalTitleBar;
            bool isKOffice = qtcCheckType(widget, "KoDockWidgetTitleBar");
            QRect fillRect = r;

            // This fixes the look of KOffice's dock widget titlebars...
            if (isKOffice)
                fillRect.adjust(-r.x(), -r.y(), 0, 0);

            if (!qtcIsFlat(opts.dwtAppearance)) {
                painter->save();

                QColor col((opts.dwtSettings & DWT_COLOR_AS_PER_TITLEBAR) ?
                           getMdiColors(option,
                                        state&State_Active)[ORIGINAL_SHADE] :
                           palette.window().color());
                if (opts.round < ROUND_FULL) {
                    drawBevelGradient(col, painter, fillRect, !verticalTitleBar,
                                      false, opts.dwtAppearance,
                                      WIDGET_DOCK_WIDGET_TITLE);
                } else {
                    double radius = qtcGetRadius(&opts, fillRect.width(),
                                                 fillRect.height(),
                                                 WIDGET_OTHER, RADIUS_EXTERNAL);
                    int round = ROUNDED_ALL;

                    if (opts.dwtSettings & DWT_ROUND_TOP_ONLY) {
                        round = verticalTitleBar ? ROUNDED_LEFT : ROUNDED_TOP;
                    }
                    painter->setRenderHint(QPainter::Antialiasing, true);
                    drawBevelGradient(col, painter, fillRect,
                                      buildPath(QRectF(fillRect), WIDGET_OTHER,
                                                round, radius),
                                      !verticalTitleBar, false,
                                      opts.dwtAppearance,
                                      WIDGET_DOCK_WIDGET_TITLE, false);
                }
                painter->restore();
            }
            if (!dwOpt->title.isEmpty()) {
                QRect titleRect(subElementRect(SE_DockWidgetTitleBarText,
                                               option, widget));

                if (verticalTitleBar) {
                    QRect rVert(r);
                    QSize s(rVert.size());

                    s.transpose();
                    rVert.setSize(s);

                    titleRect = QRect(rVert.left() + r.bottom() -
                                      titleRect.bottom(),
                                      rVert.top() + titleRect.left() - r.left(),
                                      titleRect.height(), titleRect.width());

                    painter->translate(rVert.left(),
                                       rVert.top() + rVert.width());
                    painter->rotate(-90);
                    painter->translate(-rVert.left(), -rVert.top());
                }
#ifdef QTC_QT6_ENABLE_KDE
                if (opts.dwtSettings & DWT_FONT_AS_PER_TITLEBAR) {
                    painter->setFont(QFontDatabase::systemFont(QFontDatabase::TitleFont));
                }
#endif
                QFontMetrics fm(painter->fontMetrics());
                QString title(fm.elidedText(dwOpt->title, Qt::ElideRight,
                                            titleRect.width(),
                                            QPalette::WindowText));
                painter->save();
                getMdiColors(option, state & State_Active);

                QColor textColor((opts.dwtSettings &
                                  DWT_COLOR_AS_PER_TITLEBAR) ?
                                 state&State_Active ? m_activeMdiTextColor :
                                 m_mdiTextColor :
                                 palette.color(QPalette::WindowText));
                QColor shadow(WINDOW_SHADOW_COLOR(opts.titlebarEffect));
                int textOpt(Qt::AlignVCenter); // TODO: dwtPosAsPerTitleBar ?

                if (opts.dwtSettings & DWT_TEXT_ALIGN_AS_PER_TITLEBAR) {
                    switch (opts.titlebarAlignment) {
                    case ALIGN_FULL_CENTER:
                        if (!verticalTitleBar && !reverse) {
                            QFontMetrics fm(painter->fontMetrics());
                            int width = fm.boundingRect(title).width();

                            if (((fillRect.width() + width) / 2) <=
                                titleRect.width() + (isKOffice ? r.x() : 0)) {
                                titleRect = fillRect;
                                textOpt |= Qt::AlignHCenter;
                            } else {
                                textOpt |= Qt::AlignRight;
                            }
                            break;
                        }
                        QTC_FALLTHROUGH();
                    case ALIGN_CENTER:
                        textOpt |= Qt::AlignHCenter;
                        break;
                    case ALIGN_RIGHT:
                        textOpt |= Qt::AlignRight;
                        break;
                    default:
                    case ALIGN_LEFT:
                        textOpt|=Qt::AlignLeft;
                        break;
                    }
                } else {
                    textOpt |= Qt::AlignLeft;
                }

                if (!styleHint(SH_UnderlineShortcut, dwOpt, widget)) {
                    textOpt |= Qt::TextHideMnemonic;
                } else {
                    textOpt |= Qt::TextShowMnemonic;
                }

                if ((opts.dwtSettings & DWT_EFFECT_AS_PER_TITLEBAR) &&
                    EFFECT_NONE != opts.titlebarEffect) {
                    shadow.setAlphaF(
                        WINDOW_TEXT_SHADOW_ALPHA(opts.titlebarEffect));
                    painter->setPen(shadow);
                    painter->drawText(titleRect.adjusted(1, 1, 1, 1),
                                      textOpt, title);

                    if (!(state & State_Active) &&
                        DARK_WINDOW_TEXT(textColor)) {
                        textColor.setAlpha((textColor.alpha() * 180) >> 8);
                    }
                }
                painter->setPen(textColor);
                painter->drawText(titleRect, textOpt, title);
                painter->restore();
            }
        }
        break;
    case CE_HeaderEmptyArea: {
        auto ho = styleOptCast<QStyleOptionHeader>(option);
        bool horiz = (ho ? ho->orientation == Qt::Horizontal :
                      state & State_Horizontal);
        QStyleOption opt(*option);
        const QColor *use = (opts.lvButton ? buttonColors(option) :
                             backgroundColors(option));

        opt.state &= ~State_MouseOver;
        painter->save();

        drawBevelGradient(getFill(&opt, use), painter, r, horiz, false,
                          opts.lvAppearance, WIDGET_LISTVIEW_HEADER);

        painter->setRenderHint(QPainter::Antialiasing, true);
        if(opts.lvAppearance == APPEARANCE_RAISED) {
            painter->setPen(use[4]);
            if (horiz) {
                drawAaLine(painter, r.x(), r.y() + r.height() - 2,
                           r.x() + r.width() - 1, r.y() + r.height() - 2);
            } else {
                drawAaLine(painter, r.x() + r.width() - 2, r.y(),
                           r.x() + r.width() - 2, r.y() + r.height() - 1);
            }
        }

        painter->setPen(use[QTC_STD_BORDER]);
        if (horiz) {
            drawAaLine(painter, r.x(), r.y() + r.height() - 1,
                       r.x() + r.width() - 1, r.y() + r.height() - 1);
        } else if (reverse) {
            drawAaLine(painter, r.x(), r.y(), r.x(), r.y() + r.height() - 1);
        } else {
            drawAaLine(painter, r.x() + r.width() - 1, r.y(),
                       r.x() + r.width() - 1, r.y() + r.height() - 1);
        }
        QPAINTER_RENDERHINT_AA_MAYBE_OFF(painter);
        painter->restore();
        break;
    }
    case CE_HeaderSection:
        if (auto ho = styleOptCast<QStyleOptionHeader>(option)) {
            const QColor *use = (state & State_Enabled && m_sortedLvColors &&
                                 QStyleOptionHeader::None != ho->sortIndicator ?
                                 m_sortedLvColors : opts.lvButton ?
                                 buttonColors(option) :
                                 backgroundColors(option));
            painter->save();
            if (state & (State_Raised | State_Sunken)) {
                bool sunken = (state & (/*State_Down |*/ /*State_On | */
                                   State_Sunken));
                QStyleOption opt(*option);
                opt.state &= ~State_On;

                if (ho->section == -1 && !(state & State_Enabled) && widget &&
                    widget->isEnabled()) {
                    opt.state |= State_Enabled;
                }
                drawBevelGradient(getFill(&opt, use), painter, r,
                                  ho->orientation == Qt::Horizontal, sunken,
                                  opts.lvAppearance, WIDGET_LISTVIEW_HEADER);
                painter->setRenderHint(QPainter::Antialiasing, true);
                if(opts.lvAppearance == APPEARANCE_RAISED) {
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
                               r.x() + r.width() - 1, r.y() + r.height() - 1);
                    if (opts.coloredMouseOver && state & State_MouseOver &&
                        state & State_Enabled) {
                        drawHighlight(painter, QRect(r.x(),
                                                     r.y() + r.height() - 2,
                                                     r.width(), 2), true, true);
                    }

                    if (noneOf(ho->position, QStyleOptionHeader::End,
                               QStyleOptionHeader::OnlyOneSection)) {
                        // Draw header separator
                        int sep_x = reverse ? r.x() : r.x() + r.width() - 2;
                        drawFadedLine(painter, QRect(sep_x, r.y() + 5, 1,
                                                     r.height() - 10),
                                      use[QTC_STD_BORDER], true, true, false);
                        drawFadedLine(painter, QRect(sep_x + 1, r.y() + 5, 1,
                                                     r.height() - 10),
                                      use[0], true, true, false);
                    }
                } else {
                    painter->setPen(use[QTC_STD_BORDER]);
                    if (reverse) {
                        drawAaLine(painter, r.x(), r.y(), r.x(),
                                   r.y() + r.height() - 1);
                    } else {
                        drawAaLine(painter, r.x() + r.width() - 1,
                                   r.y(), r.x() + r.width() - 1,
                                   r.y() + r.height() - 1);
                    }

                    if (noneOf(ho->position, QStyleOptionHeader::End,
                               QStyleOptionHeader::OnlyOneSection)) {
                        // Draw header separator
                        drawFadedLine(painter, QRect(r.x() + 5,
                                                     r.y() + r.height() - 2,
                                                     r.width() - 10, 1),
                                      use[QTC_STD_BORDER], true, true, true);
                        drawFadedLine(painter, QRect(r.x() + 5,
                                                     r.y() + r.height() - 1,
                                                     r.width() - 10, 1),
                                      use[0], true, true, true);
                    }
                    if (opts.coloredMouseOver && state & State_MouseOver &&
                        state & State_Enabled) {
                        drawHighlight(painter, QRect(r.x(),
                                                     r.y() + r.height() - 3,
                                                     r.width(), 2), true, true);
                    }
                }
                QPAINTER_RENDERHINT_AA_MAYBE_OFF(painter);
            } else if (!qtcIsFlat(opts.lvAppearance) && !reverse &&
                       ((State_Enabled | State_Active) == state ||
                        State_Enabled == state)) {
                QPolygon top;
                const QColor &col = getFill(option, use);

                top.setPoints(3, r.x(), r.y(), r.x() + r.width(), r.y(),
                              r.x() + r.width(), r.y() + r.height());
                painter->setClipRegion(QRegion(top));
                drawBevelGradient(col, painter, r, true, false,
                                  opts.lvAppearance, WIDGET_LISTVIEW_HEADER);
                painter->setClipRegion(QRegion(r).xored(QRegion(top)));
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
            if (!header->icon.isNull()) {
                QPixmap pixmap(getIconPixmap(header->icon,
                                             pixelMetric(PM_SmallIconSize),
                                             header->state));
                int pixw = pixmap.width();
                QRect aligned(alignedRect(header->direction,
                                          QFlag(header->iconAlignment),
                                          pixmap.size(), r));
                QRect inter(aligned.intersected(r));

                painter->drawPixmap(inter.x(), inter.y(), pixmap,
                                    inter.x() - aligned.x(),
                                    inter.y() - aligned.y(), inter.width(),
                                    inter.height());

                if (Qt::LeftToRight == header->direction) {
                    r.setLeft(r.left() + pixw + 2);
                } else {
                    r.setRight(r.right() - pixw - 2);
                }
            }
            drawItemTextWithRole(painter, r, header->textAlignment, palette,
                                 state & State_Enabled, header->text,
                                 QPalette::ButtonText);
        }
        break;
    case CE_ProgressBarGroove: {
        bool doEtch = opts.buttonEffect != EFFECT_NONE && opts.borderProgress;
        bool horiz = true;
        QColor col;

        if (auto bar = styleOptCast<QStyleOptionProgressBar>(option)) {
            horiz = bar->orientation == Qt::Horizontal;
        }

        painter->save();

        if (doEtch) {
            r.adjust(1, 1, -1, -1);
        }

        switch (opts.progressGrooveColor) {
        default:
        case ECOLOR_BASE:
            col = palette.base().color();
            break;
        case ECOLOR_BACKGROUND:
            col = palette.window().color();
            break;
        case ECOLOR_DARK:
            col = m_backgroundCols[2];
        }

        drawBevelGradient(col, painter, r, opts.borderProgress ?
                          buildPath(r, WIDGET_PBAR_TROUGH, ROUNDED_ALL,
                                    qtcGetRadius(&opts, r.width(), r.height(),
                                                 WIDGET_PBAR_TROUGH,
                                                 RADIUS_EXTERNAL)) :
                          QPainterPath(), horiz, false,
                          opts.progressGrooveAppearance, WIDGET_PBAR_TROUGH);

        if (doEtch) {
            drawEtch(painter, r.adjusted(-1, -1, 1, 1), widget,
                     WIDGET_PBAR_TROUGH);
        } else if (!opts.borderProgress) {
            painter->setPen(m_backgroundCols[QTC_STD_BORDER]);
            if (horiz) {
                painter->drawLine(r.topLeft(), r.topRight());
                painter->drawLine(r.bottomLeft(), r.bottomRight());
            } else {
                painter->drawLine(r.topLeft(), r.bottomLeft());
                painter->drawLine(r.topRight(), r.bottomRight());
            }
        }

        if (opts.borderProgress)
            drawBorder(painter, r, option, ROUNDED_ALL,
                       backgroundColors(option), WIDGET_PBAR_TROUGH,
                       qtcIsFlat(opts.progressGrooveAppearance) &&
                       ECOLOR_DARK != opts.progressGrooveColor ?
                       BORDER_SUNKEN : BORDER_FLAT);
        painter->restore();
        break;
    }
    case CE_ProgressBarContents:
        if (auto bar = styleOptCast<QStyleOptionProgressBar>(option)) {
            bool vertical(false);
            bool inverted(false);
            bool indeterminate = bar->minimum == 0 && bar->maximum == 0;

            // Get extra style options if version 2
            if (auto bar = styleOptCast<QStyleOptionProgressBar>(option)) {
                vertical = bar->orientation == Qt::Vertical;
                inverted = bar->invertedAppearance;
            }

            if (!indeterminate && bar->progress == -1)
                break;

            bool reverse = ((!vertical && (bar->direction == Qt::RightToLeft)) ||
                            vertical);

            if (inverted)
                reverse = !reverse;

            painter->save();

            if (!m_progressBarAnimateTimer && (opts.animatedProgress || indeterminate)) {
                if (!m_timer.isValid())
                    m_timer.start();
                // now we'll need a timer, start it at the regular frequency
                m_progressBarAnimateFps = constProgressBarFps;
                m_progressBarAnimateTimer = const_cast<Style*>(this)->startTimer(1000/m_progressBarAnimateFps);
            }

            if (indeterminate) {
                //Busy indicator
                int chunkSize = PROGRESS_CHUNK_WIDTH * 3.4;
                int measure = vertical ? r.height() : r.width();

                if (chunkSize > measure / 2)
                    chunkSize = measure / 2;

                int step = m_animateStep % ((measure - chunkSize) * 2);
                QStyleOption opt(*option);

                if (step > (measure - chunkSize))
                    step = 2 * (measure - chunkSize) - step;

                opt.state |= State_Raised | State_Horizontal;
                drawProgress(painter, vertical ? QRect(r.x(), r.y() + step,
                                                       r.width(), chunkSize) :
                             QRect(r.x() + step, r.y(), chunkSize, r.height()),
                             option, vertical);
            } else if (r.isValid() && bar->progress > 0) {
                // workaround for bug in QProgressBar
                qint64 progress = qMax(bar->progress, bar->minimum);
                double pg = ((progress - bar->minimum) /
                             qtcMax(1.0, double(bar->maximum - bar->minimum)));

                if (vertical) {
                    int height = qtcMin(r.height(), pg * r.height());

                    if (inverted) {
                        drawProgress(painter, QRect(r.x(), r.y(), r.width(),
                                                    height), option, true);
                    } else {
                        drawProgress(painter,
                                     QRect(r.x(), r.y() + (r.height() - height),
                                           r.width(), height), option, true);
                    }
                } else {
                    int width = qtcMin(r.width(), pg * r.width());

                    if (reverse || inverted) {
                        drawProgress(painter, QRect(r.x() + r.width() - width,
                                                    r.y(), width, r.height()),
                                     option, false, true);
                    } else {
                        drawProgress(painter, QRect(r.x(), r.y(), width,
                                                    r.height()), option);
                    }
                }
            }
            painter->restore();
        }
        break;
    case CE_ProgressBarLabel:
        if (auto bar = styleOptCast<QStyleOptionProgressBar>(option)) {
            // The busy indicator doesn't draw a label
            if (bar->minimum == 0 && bar->maximum == 0)
                return;

            bool vertical = false;
            bool inverted = false;
            bool bottomToTop = false;

            // Get extra style options if version 2
            if (auto bar = styleOptCast<QStyleOptionProgressBar>(option)) {
                vertical = bar->orientation == Qt::Vertical;
                inverted = bar->invertedAppearance;
                bottomToTop = bar->bottomToTop;
            }

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);

            if (vertical) {
                // flip width and height
                r = QRect(r.left(), r.top(), r.height(), r.width());

                QTransform m;
                if (bottomToTop) {
                    m.translate(0.0, r.width());
                    m.rotate(-90);
                } else {
                    m.translate(r.height(), 0.0);
                    m.rotate(90);
                }
                painter->setTransform(m);
            }

            int progressIndicatorPos =
                ((bar->progress - bar->minimum) /
                 qMax(1.0, double(bar->maximum - bar->minimum)) * r.width());
            bool flip((!vertical &&
                       (((bar->direction == Qt::RightToLeft) && !inverted) ||
                        ((Qt::LeftToRight==bar->direction) && inverted))) ||
                      (vertical && ((!inverted && !bottomToTop) ||
                                    (inverted && bottomToTop))));
            QRect leftRect;
            QRegion rightRect(r);
            QPalette::ColorGroup cg =
                (state & State_Enabled || state == State_None ?
                 QPalette::Active : QPalette::Current);

            if (flip) {
                int indicatorPos = r.width() - progressIndicatorPos;

                if (indicatorPos >= 0 && indicatorPos <= r.width()) {
                    painter->setPen(palette.brush(cg, QPalette::Base).color());
                    leftRect = QRect(r.left(), r.top(), indicatorPos, r.height());
                    // rightRect = QRect(r.left() + indicatorPos, r.top(),
                    //                   r.width() - indicatorPos, r.height());
                } else if (indicatorPos > r.width()) {
                    painter->setPen(palette.brush(cg, QPalette::Text).color());
                } else {
                    painter->setPen(
                        palette.brush(cg, QPalette::HighlightedText).color());
                }
            } else {
                if (progressIndicatorPos >= 0 &&
                    progressIndicatorPos <= r.width()) {
                    leftRect = QRect(r.left(), r.top(), progressIndicatorPos,
                                     r.height());
                    // rightRect = QRect(r.left() + progressIndicatorPos, r.top(),
                    //                   r.width() - progressIndicatorPos,
                    //                   r.height());
                } else if (progressIndicatorPos > r.width()) {
                    painter->setPen(
                        palette.brush(cg, QPalette::HighlightedText).color());
                } else {
                    painter->setPen(palette.brush(cg, QPalette::Text).color());
                }
            }

            QString text = bar->fontMetrics.elidedText(bar->text,
                                                       Qt::ElideRight,
                                                       r.width());
            rightRect = rightRect.subtracted(leftRect);
            painter->setClipRegion(rightRect);
            painter->drawText(r, text,
                              QTextOption(Qt::AlignAbsolute | Qt::AlignHCenter |
                                          Qt::AlignVCenter));
            if (!leftRect.isNull()) {
                painter->setPen(
                    palette.brush(cg, flip ? QPalette::Text :
                                  QPalette::HighlightedText).color());
                painter->setClipRect(leftRect);
                painter->drawText(
                    r, text, QTextOption(Qt::AlignAbsolute | Qt::AlignHCenter |
                                         Qt::AlignVCenter));
            }
            painter->restore();
        }
        break;
    case CE_MenuBarItem:
        if (auto mbi = styleOptCast<QStyleOptionMenuItem>(option)) {
            bool down = state & (State_On | State_Sunken);
            bool active = (state & State_Enabled &&
                           (down || (state & State_Selected &&
                                     opts.menubarMouseOver)));
            uint alignment = (Qt::AlignCenter | Qt::TextShowMnemonic |
                              Qt::TextDontClip | Qt::TextSingleLine);
            QPixmap pix = getIconPixmap(mbi->icon,
                                        pixelMetric(PM_SmallIconSize),
                                        mbi->state);
            if (!styleHint(SH_UnderlineShortcut, mbi, widget))
                alignment |= Qt::TextHideMnemonic;

            painter->save();
            drawMenuOrToolBarBackground(widget, painter, mbi->menuRect, option);
            if (active)
                drawMenuItem(painter, !opts.roundMbTopOnly &&
                             !(opts.square & SQUARE_POPUP_MENUS) ?
                             r.adjusted(1, 1, -1, -1) : r, option, MENU_BAR,
                             (down || theThemedApp == APP_OPENOFFICE) &&
                             opts.roundMbTopOnly ? ROUNDED_TOP : ROUNDED_ALL,
                             opts.useHighlightForMenu &&
                             (opts.colorMenubarMouseOver || down ||
                              theThemedApp == APP_OPENOFFICE) ?
                             (m_ooMenuCols ? m_ooMenuCols :
                              m_highlightCols) : m_backgroundCols);
            if (!pix.isNull()) {
                drawItemPixmap(painter, mbi->rect, alignment, pix);
            } else {
                const QColor &col =
                    (state & State_Enabled ?
                     ((opts.colorMenubarMouseOver && active) ||
                      (!opts.colorMenubarMouseOver && down)) ?
                     opts.customMenuTextColor ?
                     opts.customMenuSelTextColor :
                     opts.useHighlightForMenu ?
                     palette.highlightedText().color() :
                     palette.windowText().color() :
                     palette.windowText().color() :
                     palette.windowText().color());

                painter->setPen(col);
                painter->drawText(r, alignment, mbi->text);
            }
            painter->restore();
        }
        break;
    case CE_MenuItem:
        if (auto menuItem = styleOptCast<QStyleOptionMenuItem>(option)) {
            // TODO check qtquick
            bool comboMenu(qobject_cast<const QComboBox*>(widget)),
                reverse(Qt::RightToLeft==menuItem->direction),
                isOO(isOOWidget(widget));
            int checkcol(qMax(menuItem->maxIconWidth, 20)),
                stripeWidth(qMax(checkcol, constMenuPixmapWidth)-2);
            const QColor *use = popupMenuCols(option);

            QRect rx(r);

            if (isOO) {
                if (opts.borderMenuitems) {
                    r.adjust(2, 0, -2, 0);
                } else if (opts.menuitemAppearance == APPEARANCE_FADE) {
                    r.adjust(1, 0, -1, 0);
                }
            }

            painter->save();

            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                bool isMenu = !widget || qobject_cast<const QMenu*>(widget);
                bool doStripe = isMenu && opts.menuStripe && !comboMenu;

                if (doStripe)
                    drawBevelGradient(menuStripeCol(), painter,
                                      QRect(reverse ? r.right() - stripeWidth :
                                            r.x(), r.y(), stripeWidth,
                                            r.height()), false, false,
                                      opts.menuStripeAppearance, WIDGET_OTHER);

                if (!menuItem->text.isEmpty()) {
                    int textAlignment;
                    QFont font = m_fntHelper->fontStripStyleName(menuItem->font);
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
                } else {
                    QRect miRect(menuItem->rect.left() + 3 +
                                 (!reverse && doStripe ? stripeWidth : 0),
                                 menuItem->rect.center().y(),
                                 menuItem->rect.width() -
                                 (7 + (doStripe ? stripeWidth : 0)), 1);
                    drawFadedLine(painter, miRect, use[MENU_SEP_SHADE], true,
                                  true, true);
                }

                if (isOO) {
                    painter->setPen(use[QTC_STD_BORDER]);
                    painter->drawLine(rx.topLeft(), rx.bottomLeft());
                    painter->drawLine(rx.topRight(), rx.bottomRight());
                }
                painter->restore();
                break;
            }

            bool selected = state & State_Selected;
            bool checkable =
                menuItem->checkType != QStyleOptionMenuItem::NotCheckable;
            bool checked = menuItem->checked;
            bool enabled = state & State_Enabled;

            // check if the font tick is wider than the current margin
            // and adjust if necessary
            initFontTickData(menuItem->font, widget);
            int dx = 0;
            if (menuTickCompensation(opts, checkcol, dx)) {
                r.adjust(-dx, 0, dx, 0);
                checkcol += dx;
            }
            if (opts.menuStripe && !comboMenu)
                drawBevelGradient(menuStripeCol(), painter,
                                  QRect(reverse ? r.right()-stripeWidth : r.x(),
                                        r.y(), stripeWidth, r.height()),
                                  false, false, opts.menuStripeAppearance,
                                  WIDGET_OTHER);

            if (selected && enabled) {
                drawMenuItem(painter, r, option,
                             /*comboMenu ? MENU_COMBO : */MENU_POPUP,
                             ROUNDED_ALL,
                             opts.useHighlightForMenu ?
                             (m_ooMenuCols ? m_ooMenuCols :
                              m_highlightCols) : use);
            }

            if (comboMenu) {
                if (menuItem->icon.isNull()) {
                    checkcol = 0;
                } else {
                    checkcol = menuItem->maxIconWidth;
                }
            } else {
                // Check
                QRect checkRect(r.left() + 3, r.center().y() - 6,
                                opts.crSize, opts.crSize);
                checkRect = visualRect(menuItem->direction, menuItem->rect,
                                       checkRect);
                if (checkable) {
                    if (!opts.onlyTicksInMenu && (menuItem->checkType &
                         QStyleOptionMenuItem::Exclusive) &&
                        menuItem->icon.isNull()) {
                        QStyleOptionButton button;
                        button.rect = checkRect;
                        button.state = menuItem->state|STATE_MENU;
                        if (checked)
                            button.state |= State_On;
                        button.palette = palette;
                        drawPrimitive(PE_IndicatorRadioButton, &button,
                                      painter, widget);
                    } else {
                        if (menuItem->icon.isNull() || !opts.menuIcons) {
                            QStyleOptionButton button;
                            button.rect = checkRect;
                            button.state = menuItem->state|STATE_MENU;
                            if (checked)
                                button.state |= State_On;
                            button.palette = palette;
                            drawPrimitive(PE_IndicatorMenuCheckMark, &button,
                                          painter, widget);
                        } else if (checked) {
                            int iconSize = qMax(menuItem->maxIconWidth, 20);
                            QRect sunkenRect(r.left() + 1,
                                             r.top() + (r.height() -
                                                        iconSize) / 2,
                                             iconSize, iconSize);
                            QStyleOption opt(*option);

                            sunkenRect = visualRect(menuItem->direction,
                                                    menuItem->rect, sunkenRect);
                            opt.state = menuItem->state;
                            opt.state |= State_Raised | State_Horizontal | State_On;
                            drawLightBevel(painter, sunkenRect, &opt, widget,
                                           ROUNDED_ALL,
                                           getFill(&opt, m_buttonCols),
                                           m_buttonCols);
                        }
                    }
                }
            }

            // Text and icon, ripped from windows style
            bool dis = !(state & State_Enabled);
            bool act = state & State_Selected;

            if (opts.menuIcons && !menuItem->icon.isNull()) {
                QIcon::Mode mode = dis ? QIcon::Disabled : QIcon::Normal;
                if (act && !dis)
                    mode = QIcon::Active;
                QIcon::State state = checked ? QIcon::On : QIcon::Off;

                int size = pixelMetric(PM_SmallIconSize, option, widget);
                QRect iconRect(menuItem->rect.x(),
                               menuItem->rect.y(), checkcol,
                               menuItem->rect.height());
                iconRect = centerRect(iconRect, size, size);
                iconRect = visualRect(option->direction, menuItem->rect, iconRect);

                QPixmap pixmap(getIconPixmap(menuItem->icon, iconRect.size(), mode, state));
                painter->setPen(palette.text().color());
                if (checkable && checked)
                    iconRect.adjust(1, 1, 1, 1);
                painter->drawPixmap(iconRect.topLeft(), pixmap);
            }

            painter->setPen(dis ? palette.text().color() :
                            selected && opts.useHighlightForMenu &&
                            !m_ooMenuCols ? palette.highlightedText().color() :
                            palette.windowText().color());

            int x;
            int y;
            int w;
            int h;
            int tab = menuItem->tabWidth;
            menuItem->rect.getRect(&x, &y, &w, &h);
            int xm = windowsItemFrame + checkcol + windowsItemHMargin - 2;
            int xpos = menuItem->rect.x() + xm;
            QRect textRect(xpos, y + windowsItemVMargin,
                           opts.menuIcons ?
                           (w - xm - windowsRightBorder - tab + 1) :
                           (w - ((xm*2) + tab)), h - 2 * windowsItemVMargin);
            QRect vTextRect = visualRect(option->direction, menuItem->rect,
                                         textRect);
            QString s = menuItem->text;

            if (!s.isEmpty()) {
                // draw text
                int t(s.indexOf(QLatin1Char('\t'))),
                    textFlags(Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine);

                if (!styleHint(SH_UnderlineShortcut, menuItem, widget))
                    textFlags |= Qt::TextHideMnemonic;
                textFlags |= Qt::AlignLeft;

                if (t >= 0)
                {
                    QRect vShortcutRect(visualRect(option->direction, menuItem->rect,
                                                   QRect(textRect.topRight(), QPoint(menuItem->rect.right(), textRect.bottom()))));
                    vShortcutRect.adjust(dx, 0, dx, 0);
                    painter->drawText(vShortcutRect, textFlags, s.mid(t + 1));
                    s = s.left(t);
                }

                QFont font = m_fntHelper->fontStripStyleName(menuItem->font);

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
            if (!btn->icon.isNull()
                && !(styleHint(SH_DialogButtonBox_ButtonsHaveIcons, btn, widget) || btn->text.isEmpty())) {
                // remove the icon from our local QStyleOptionButton copy
                subopt.icon = QIcon();
            }

            subopt.rect = subElementRect(SE_PushButtonContents, &subopt, widget);
            drawControl(CE_PushButtonLabel, &subopt, painter, widget);

            if (state & State_HasFocus &&
                !(state & State_MouseOver && opts.coloredMouseOver != MO_NONE &&
                  oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED))) {
                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=(*btn);
                fropt.rect = subElementRect(SE_PushButtonFocusRect, btn, widget);
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
            if (!(btn->features & (QStyleOptionButton::Flat |
                                   QStyleOptionButton::CommandLinkButton)) ||
                state&(State_Sunken | State_On | State_MouseOver)) {
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

            if (!button->icon.isNull() &&
                (styleHint(SH_DialogButtonBox_ButtonsHaveIcons, button, widget) ||
                 button->text.isEmpty())) {
                QIcon::Mode mode(button->state & State_Enabled ? QIcon::Normal : QIcon::Disabled);

                if (QIcon::Normal == mode && button->state & State_HasFocus)
                    mode = QIcon::Active;

                QIcon::State state((button->state & State_On) || (button->state & State_Sunken) ?
                                   QIcon::On : QIcon::Off);

                QSize iconSize = button->iconSize;
                int width = iconSize.width();
                int height = iconSize.height();
                int iconSpacing = 4; //### 4 is currently hardcoded in QPushButton::sizeHint()

                // Center both icon and text
                int xoffset = width;
                if (!button->text.isEmpty())
                    xoffset += (button->fontMetrics.boundingRect(r, tf, button->text).width() +
                                iconSpacing);
                QRect iconRect(r.x() + (r.width() - xoffset) / 2,
                               r.y() + (r.height() - height) / 2, width, height);

                tf |= Qt::AlignLeft; // left align, we adjust the text-rect instead

                if (button->direction == Qt::RightToLeft)
                    r.setRight(iconRect.left() - iconSpacing);
                else
                    r.setLeft(iconRect.right() + iconSpacing);

                if (button->state & (State_On|State_Sunken))
                    iconRect.translate(pixelMetric(PM_ButtonShiftHorizontal, option, widget),
                                       pixelMetric(PM_ButtonShiftVertical, option, widget));
                iconRect = visualRect(option->direction, button->rect, iconRect);

                QPixmap pixmap(getIconPixmap(button->icon, iconRect.size(), mode, state));
                painter->drawPixmap(iconRect, pixmap);
            } else {
                tf |= Qt::AlignHCenter;
            }

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
    case CE_MenuBarEmptyArea:
    {
        painter->save();

        drawMenuOrToolBarBackground(widget, painter, r, option);
        if (TB_NONE!=opts.toolbarBorders && widget && widget->parentWidget() &&
            (qobject_cast<const QMainWindow *>(widget->parentWidget())))
        {
            const QColor *use=menuColors(option, m_active);
            bool         dark(TB_DARK==opts.toolbarBorders || TB_DARK_ALL==opts.toolbarBorders);

            if(TB_DARK_ALL==opts.toolbarBorders || TB_LIGHT_ALL==opts.toolbarBorders)
            {
                painter->setPen(use[0]);
                painter->drawLine(r.x(), r.y(), r.x()+r.width()-1, r.y());
                painter->drawLine(r.x(), r.y(), r.x(), r.y()+r.width()-1);
                painter->setPen(use[dark ? 3 : 4]);
                painter->drawLine(r.x(), r.y()+r.height()-1, r.x()+r.width()-1, r.y()+r.height()-1);
                painter->drawLine(r.x()+r.width()-1, r.y(), r.x()+r.width()-1, r.y()+r.height()-1);
            }
            else
            {
                painter->setPen(use[dark ? 3 : 4]);
                painter->drawLine(r.x(), r.y()+r.height()-1, r.x()+r.width()-1, r.y()+r.height()-1);
            }
        }
        painter->restore();
    }
    break;
    case CE_TabBarTabLabel:
        if (auto _tab = styleOptCast<QStyleOptionTab>(option)) {
            QStyleOptionTab tab(*_tab);
            bool verticalTabs(QTabBar::RoundedEast == tab.shape ||
                              QTabBar::RoundedWest == tab.shape ||
                              QTabBar::TriangularEast == tab.shape ||
                              QTabBar::TriangularWest == tab.shape);
            bool toolbarTab = (!opts.toolbarTabs &&
                               widget && widget->parentWidget() &&
                               qobject_cast<QToolBar*>(widget->parentWidget()));

            if (verticalTabs) {
                painter->save();
                int newX, newY, newRot;
                if (QTabBar::RoundedEast == tab.shape ||
                    QTabBar::TriangularEast == tab.shape) {
                    newX = r.width();
                    newY = r.y();
                    newRot = 90;
                } else {
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

            int alignment(Qt::AlignVCenter | Qt::TextShowMnemonic |
                          (opts.centerTabText ? Qt::AlignHCenter : Qt::AlignLeft));

            if (!styleHint(SH_UnderlineShortcut, option, widget))
                alignment |= Qt::TextHideMnemonic;

            if(toolbarTab)
                tab.state &= ~State_Selected;
            r = subElementRect(SE_TabBarTabText, &tab, widget);
            if (!tab.icon.isNull()) {
                QSize iconSize(tab.iconSize);
                if (!iconSize.isValid()) {
                    int iconExtent(pixelMetric(PM_SmallIconSize));
                    iconSize = QSize(iconExtent, iconExtent);
                }

                QPixmap tabIcon(getIconPixmap(tab.icon, iconSize,
                                              state & State_Enabled));
                QSize tabIconSize = tab.icon.actualSize(iconSize,
                                                        tab.state & State_Enabled
                                                        ? QIcon::Normal
                                                        : QIcon::Disabled);

                int offset = 4,
                    left = option->rect.left();
                if (tab.leftButtonSize.isNull() || tab.leftButtonSize.width()<=0) {
                    offset += 2;
                } else {
                    left += tab.leftButtonSize.width() + 2;
                }
                QRect iconRect = QRect(left + offset,
                                       r.center().y() - tabIcon.height() / 2,
                                       tabIconSize.width(),
                                       tabIconSize.height());
                if (!verticalTabs)
                    iconRect = visualRect(option->direction, option->rect,
                                          iconRect);
                painter->drawPixmap(iconRect.x(), iconRect.y(), tabIcon);
            }

            if(!_tab->text.isEmpty()) {
                drawItemTextWithRole(painter, r, alignment, _tab->palette,
                                     _tab->state & State_Enabled, _tab->text,
                                     !opts.stdSidebarButtons && toolbarTab &&
                                     state&State_Selected ?
                                     QPalette::HighlightedText :
                                     QPalette::WindowText);
            }

            if (verticalTabs)
                painter->restore();

            if (tab.state & State_HasFocus) {
                const int constOffset = 1 + pixelMetric(PM_DefaultFrameWidth);

                int x1 = tab.rect.left();
                int x2 = tab.rect.right() - 1;
                QStyleOptionFocusRect fropt;
                fropt.QStyleOption::operator=(*_tab);
                fropt.rect.setRect(x1 + 1 + constOffset,
                                   tab.rect.y() + constOffset,
                                   x2 - x1 - 2 * constOffset,
                                   tab.rect.height() - 2 * constOffset);

                fropt.state|=State_Horizontal;
                if(FOCUS_LINE != opts.focus) {
                    if(QTabBar::RoundedNorth == tab.shape ||
                       QTabBar::TriangularNorth == tab.shape) {
                        fropt.rect.adjust(0, 1, 0, 0);
                    }
                } else if(TAB_MO_BOTTOM == opts.tabMouseOver &&
                          FOCUS_LINE==opts.focus)
                    switch(tab.shape) {
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
        if(!opts.toolbarTabs && widget && widget->parentWidget() && qobject_cast<QToolBar *>(widget->parentWidget()))
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
        }
        else if (auto tab = styleOptCast<QStyleOptionTab>(option)) {
            bool onlyTab(widget && widget->parentWidget() ?
                         qobject_cast<const QTabWidget*>(widget->parentWidget()) ?
                         false : true : false),
                selected(state&State_Selected),
                horiz(QTabBar::RoundedNorth==tab->shape || QTabBar::RoundedSouth==tab->shape);
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
                moOffset(opts.round == ROUND_NONE || TAB_MO_TOP!=opts.tabMouseOver ? 1 : opts.round),
                highlightOffset(opts.highlightTab && opts.round>ROUND_SLIGHT ? 2 : 1),
                highlightBorder(opts.round>ROUND_FULL ? 4 : 3),
                sizeAdjust(!selected && TAB_MO_GLOW==opts.tabMouseOver ? 1 : 0);
            bool leftAligned((!rtlHorTabs && Qt::AlignLeft==tabBarAlignment) ||
                             (rtlHorTabs && Qt::AlignRight==tabBarAlignment)),
                rightAligned((!rtlHorTabs && Qt::AlignRight==tabBarAlignment) ||
                             (rtlHorTabs && Qt::AlignLeft==tabBarAlignment)),
                docMode(tab->documentMode),
                docFixLeft(!leftCornerWidget && leftAligned && firstTab && (docMode || onlyTab)),
                fixLeft(!onlyTab && !leftCornerWidget && leftAligned && firstTab && !docMode),
                fixRight(!onlyTab && !rightCornerWidget && rightAligned && lastTab && !docMode),
                mouseOver(state&State_Enabled && state&State_MouseOver),
                glowMo(!selected && mouseOver && opts.coloredMouseOver &&
                       TAB_MO_GLOW==opts.tabMouseOver),
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
                QWidget *top=widget ? widget->window() : 0L;
                bool isDialog = top && qtcIsDialog(top);

                // Note: opacity is divided by 150 to make dark
                // inactive tabs more translucent
                if (isDialog && 100 != opts.dlgOpacity) {
                    fill.setAlphaF(opts.dlgOpacity / 150.0);
                } else if (!isDialog && 100 != opts.bgndOpacity) {
                    fill.setAlphaF(opts.bgndOpacity / 150.0);
                }
            }

            switch(tab->shape) {
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
                        QPAINTER_RENDERHINT_AA_MAYBE_OFF(painter);
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
                        QPAINTER_RENDERHINT_AA_MAYBE_OFF(painter);
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
                        QPAINTER_RENDERHINT_AA_MAYBE_OFF(painter);
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
                        QPAINTER_RENDERHINT_AA_MAYBE_OFF(painter);
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
            if(!reverse && PE_IndicatorArrowLeft==pe && r.x()>3)
            {
                round=ROUNDED_NONE;
                br.adjust(0, 0, 1, 0);
                if(opts.flatSbarButtons || !opts.vArrows)
                    ar.adjust(1, 0, 1, 0);
            }
            else if(reverse && PE_IndicatorArrowRight==pe && r.x()>3)
            {
                if(SCROLLBAR_PLATINUM==opts.scrollbarType)
                {
                    round=ROUNDED_NONE;
                    br.adjust(-1, 0, 0, 0);
                    if(opts.flatSbarButtons || !opts.vArrows)
                        ar.adjust(-1, 0, -1, 0);
                }
                else
                {
                    if(r.x()<pixelMetric(PM_ScrollBarExtent, option, widget)+2)
                        round=ROUNDED_NONE;
                    br.adjust(0, 0, 1, 0);
                    if(opts.flatSbarButtons || !opts.vArrows)
                        ar.adjust(1, 0, 1, 0);
                }
            }
            else if(PE_IndicatorArrowUp==pe && r.y()>3)
            {
                round=ROUNDED_NONE;
                br.adjust(0, 0, 0, 1);
                if(opts.flatSbarButtons || !opts.vArrows)
                    ar.adjust(0, 1, 0, 1);
            }
            break;
        case SCROLLBAR_NEXT:
            if(!reverse && PE_IndicatorArrowRight==pe)
            {
                round=ROUNDED_NONE;
                br.adjust(-1, 0, 0, 0);
                if(opts.flatSbarButtons || !opts.vArrows)
                    ar.adjust(-1, 0, 0, -1);
            }
            else if(reverse && PE_IndicatorArrowLeft==pe)
            {
                round=ROUNDED_NONE;
                br.adjust(0, 0, 1, 0);
                if(opts.flatSbarButtons || !opts.vArrows)
                    ar.adjust(-1, 0, 0, 1);
            }
            else if(PE_IndicatorArrowDown==pe)
            {
                round=ROUNDED_NONE;
                br.adjust(0, -1, 0, 0);
                if(opts.flatSbarButtons || !opts.vArrows)
                    ar.adjust(0, -1, 0, -1);
            }
            break;
        }

        painter->save();
        if(opts.flatSbarButtons && !qtcIsFlat(opts.sbarBgndAppearance) /*&& SCROLLBAR_NONE!=opts.scrollbarType*/)
            drawBevelGradientReal(palette.brush(QPalette::Base).color(), painter, r, state&State_Horizontal, false,
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
            painter->fillRect(r, palette.window().color());
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
            int shiftX = 0;
            int shiftY = 0;
            if (state & (State_Sunken | State_On)) {
                shiftX = pixelMetric(PM_ButtonShiftHorizontal, tb, widget);
                shiftY = pixelMetric(PM_ButtonShiftVertical, tb, widget);
            }

            // Arrow type always overrules and is always shown
            bool hasArrow = tb->features & QStyleOptionToolButton::Arrow;

            if (((!hasArrow && tb->icon.isNull()) && !tb->text.isEmpty()) ||
                tb->toolButtonStyle == Qt::ToolButtonTextOnly) {
                int alignment = Qt::AlignCenter|Qt::TextShowMnemonic;

                if (!styleHint(SH_UnderlineShortcut, option, widget))
                    alignment |= Qt::TextHideMnemonic;

                r.translate(shiftX, shiftY);

                drawItemTextWithRole(painter, r, alignment, palette, state&State_Enabled,
                                     tb->text, QPalette::ButtonText);
            } else {
                QPixmap pm;
                QSize   iconSize = tb->iconSize;
                QRect   pr = r;

                if (!tb->icon.isNull()) {
                    QIcon::State state = tb->state & State_On ? QIcon::On : QIcon::Off;
                    QIcon::Mode mode;
                    if (!(tb->state & State_Enabled)) {
                        mode = QIcon::Disabled;
                    } else if ((state & State_MouseOver) && (state & State_AutoRaise)) {
                        mode = QIcon::Active;
                    } else {
                        mode = QIcon::Normal;
                    }

                    if (!iconSize.isValid()) {
                        int iconExtent = pixelMetric(PM_ToolBarIconSize);
                        iconSize = QSize(iconExtent, iconExtent);
                    }

                    if (iconSize.width() > tb->rect.size().width())
                        iconSize = QSize(tb->rect.size().width(), tb->rect.size().width());
                    if (iconSize.height() > tb->rect.size().height())
                        iconSize = QSize(tb->rect.size().height(), tb->rect.size().height());

                    pm = getIconPixmap(tb->icon, iconSize, mode, state);
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
                        pr.setHeight(iconSize.height() + 6);

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
                        pr.setWidth(iconSize.width() + 8);
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
                QFont f = m_fntHelper->fontStripStyleName(painter->font());
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

                        drawBevelGradient(shade(palette.window().color(), TO_FACTOR(opts.crHighlight)),
                                          painter, highlightRect,
                                          buildPath(QRectF(highlightRect), WIDGET_OTHER, ROUNDED_ALL, radius), true,
                                          false, opts.selectionAppearance, WIDGET_SELECTION, false);
                        painter->restore();
                    }
                    else
                        drawBevelGradient(shade(palette.window().color(), TO_FACTOR(opts.crHighlight)), painter,
                                          highlightRect, true, false, opts.selectionAppearance, WIDGET_SELECTION);
                }
                ParentStyleClass::drawControl(element, &copy, painter, widget);
                break;
            }
        // Fall through!
    default:
        ParentStyleClass::drawControl(element, option, painter, widget);
    }
}

void Style::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
    prePolish(widget);
    QRect               r(option->rect);
    const State &state(option->state);
    const QPalette      &palette(option->palette);
    bool                reverse(Qt::RightToLeft==option->direction);

    switch (control)
    {
    case CC_Dial:
        if (auto slider = styleOptCast<QStyleOptionSlider>(option)) {
            r.adjust(1, 1, -1, -1);

            QStyleOptionComplex opt(*option);
            bool                mo(state&State_Enabled && state&State_MouseOver);
            QRect               outer(r);
            int                 sliderWidth = /*qMin(2*r.width()/5, */CIRCULAR_SLIDER_SIZE/*)*/;
#ifdef DIAL_DOT_ON_RING
            int                 halfWidth=sliderWidth/2;
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

            // angle calculation from qcommonstyle.cpp (c) Trolltech 1992-2007, ASA.
            qreal               angle(0);
            if(slider->maximum == slider->minimum)
                angle = M_PI / 2;
            else
            {
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
        if(isOOWidget(widget))
            painter->fillRect(r, palette.brush(QPalette::Window));
        if (auto toolbutton = styleOptCast<QStyleOptionToolButton>(option)) {
            int widthAdjust(0),
                heightAdjust(0);

            if (widget) {
                if ((opts.dwtSettings & DWT_BUTTONS_AS_PER_TITLEBAR) &&
                    (widget->inherits("QDockWidgetTitleButton") ||
                     qtcCheckType(getParent(widget),
                                  "KoDockWidgetTitleBar"))) {
                    ETitleBarButtons btn = TITLEBAR_CLOSE;
                    Icon icon = ICN_CLOSE;

                    if (constDwtFloat == widget->objectName()) {
                        btn = TITLEBAR_MAX;
                        icon = ICN_RESTORE;
                    } else if (constDwtClose != widget->objectName() &&
                               qtcCheckType(getParent(widget),
                                            "KoDockWidgetTitleBar") &&
                               qtcCheckType<QDockWidget>(
                                   getParent<2>(widget))) {
                        QDockWidget *dw = (QDockWidget*)getParent<2>(widget);
                        QWidget *koDw = widget->parentWidget();
                        int fw = dw->isFloating()
                            ? pixelMetric(QStyle::PM_DockWidgetFrameWidth, 0, dw)
                            : 0;
                        QRect geom(widget->geometry());
                        QStyleOptionDockWidget dwOpt;
                        dwOpt.initFrom(dw);
                        dwOpt.rect = QRect(QPoint(fw, fw),
                                           QSize(koDw->geometry().width() -
                                                 (fw * 2),
                                                 koDw->geometry().height() -
                                                 (fw * 2)));
                        dwOpt.title = dw->windowTitle();
                        dwOpt.closable = (dw->features() &
                                          QDockWidget::DockWidgetClosable) == QDockWidget::DockWidgetClosable;
                        dwOpt.floatable = (dw->features() &
                                           QDockWidget::DockWidgetFloatable) ==
                            QDockWidget::DockWidgetFloatable;

                        if(dwOpt.closable && subElementRect(QStyle::SE_DockWidgetCloseButton, &dwOpt,
                                                            widget->parentWidget()->parentWidget())==geom)
                            btn=TITLEBAR_CLOSE, icon=ICN_CLOSE;
                        else if(dwOpt.floatable && subElementRect(QStyle::SE_DockWidgetFloatButton, &dwOpt,
                                                                  widget->parentWidget()->parentWidget())==geom)
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

                    drawDwtControl(painter, state, r.adjusted(-1, -1, 1, 1), btn,
                                   icon, option->palette.color(QPalette::WindowText), btnCols,
                                   bgndCols);
                    break;
                }
                if(qobject_cast<QTabBar *>(widget->parentWidget()))
                {
                    QStyleOptionToolButton btn(*toolbutton);

                    if(Qt::LeftArrow==toolbutton->arrowType || Qt::RightArrow==toolbutton->arrowType)
                        btn.rect.adjust(0, 4, 0, -4);
                    else
                        btn.rect.adjust(4, 0, -4, 0);
                    if(!(btn.state&State_Enabled))
                        btn.state&=~State_MouseOver;
                    drawPrimitive(PE_PanelButtonTool, &btn, painter, widget);
                    if(opts.vArrows)
                        switch(toolbutton->arrowType)
                        {
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
                    drawTbArrow(this, &btn, btn.rect, painter, widget);
                    break;
                }

                const QToolButton *btn = qobject_cast<const QToolButton *>(widget);

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
                    QFont font = m_fntHelper->fontStripStyleName(toolbutton->font);

                    font.setBold(true);
                    painter->setFont(font);
                    drawItemTextWithRole(painter, r, Qt::AlignHCenter | Qt::AlignVCenter,
                                         palette, state&State_Enabled, toolbutton->text, QPalette::Text);
                    painter->restore();
                    break;
                }

                // Amarok's toolbars (the one just above the collection list)
                // are much thinner then normal, and QToolBarExtension does not
                // seem to take this into account - so adjust the size here...
                QWidget *parent = getParent(widget);
                if (widget->inherits("QToolBarExtension") && parent) {
                    if (r.height() > parent->rect().height()) {
                        heightAdjust = (r.height() -
                                        parent->rect().height()) + 2;
                    }
                    if (r.width() > parent->rect().width()) {
                        widthAdjust = (r.width() -
                                       parent->rect().width()) + 2;
                    }
                }
            }
            QRect button(subControlRect(control, toolbutton, SC_ToolButton, widget)),
                menuarea(subControlRect(control, toolbutton, SC_ToolButtonMenu, widget));
            State bflags(toolbutton->state);
            bool  etched(opts.buttonEffect != EFFECT_NONE),
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
                if (state&State_Sunken &&
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

            if ( raised ||
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

            if ((opts.focus != FOCUS_GLOW || !drawnBevel) &&
                toolbutton->state & State_HasFocus) {
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
                            0, 0, -(pixelMetric(QStyle::PM_MenuButtonIndicator,
                                                toolbutton, widget)-1), 0);
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
            QRect textRect = /*proxy()->*/subControlRect(CC_GroupBox, option, SC_GroupBoxLabel, widget);
            QRect checkBoxRect = /*proxy()->*/subControlRect(CC_GroupBox, option, SC_GroupBoxCheckBox, widget);
            if (groupBox->subControls & QStyle::SC_GroupBoxFrame)
            {
                QStyleOptionFrame frame;
                frame.QStyleOption::operator=(*groupBox);
                frame.features = groupBox->features;
                frame.lineWidth = groupBox->lineWidth;
                frame.midLineWidth = groupBox->midLineWidth;
                frame.rect = /*proxy()->*/subControlRect(CC_GroupBox, option, SC_GroupBoxFrame, widget);

                if((groupBox->features & QStyleOptionFrame::Flat) || !(opts.gbLabel&(GB_LBL_INSIDE|GB_LBL_OUTSIDE)))
                {
                    painter->save();
                    QRegion region(r);
                    if (!groupBox->text.isEmpty())
                        region -= QRect(groupBox->subControls&QStyle::SC_GroupBoxCheckBox
                                        ? checkBoxRect.united(textRect).adjusted(reverse ? 0 : -2, 0, reverse ? 2 : 0, 0)
                                        : textRect);
                    painter->setClipRegion(region);
                }
                /*proxy()->*/drawPrimitive(PE_FrameGroupBox, &frame, painter, widget);
                if((groupBox->features&QStyleOptionFrame::Flat) || !(opts.gbLabel&(GB_LBL_INSIDE|GB_LBL_OUTSIDE)))
                    painter->restore();
            }

            // Draw title
            if ((groupBox->subControls & QStyle::SC_GroupBoxLabel) && !groupBox->text.isEmpty())
            {
                QColor textColor = groupBox->textColor;
                if (textColor.isValid())
                    painter->setPen(textColor);
                int alignment = int(groupBox->textAlignment);
                if (!/*proxy()->*/styleHint(QStyle::SH_UnderlineShortcut, option, widget))
                    alignment |= Qt::TextHideMnemonic;

                if(opts.gbLabel&GB_LBL_BOLD)
                {
                    QFont font = m_fntHelper->fontStripStyleName(painter->font());

                    font.setBold(true);
                    painter->save();
                    painter->setFont(font);
                }
                /*proxy()->*/drawItemText(painter, textRect,  Qt::TextShowMnemonic | Qt::AlignHCenter | alignment,
                                          palette, state & State_Enabled, groupBox->text,
                                          textColor.isValid() ? QPalette::NoRole : QPalette::WindowText);

                if(opts.gbLabel&GB_LBL_BOLD)
                    painter->restore();

                if (state & State_HasFocus)
                {
                    QStyleOptionFocusRect fropt;
                    fropt.QStyleOption::operator=(*groupBox);
                    fropt.rect = textRect;
                    /*proxy()->*/drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
                }
            }

            // Draw checkbox
            if (groupBox->subControls & SC_GroupBoxCheckBox)
            {
                QStyleOptionButton box;
                box.QStyleOption::operator=(*groupBox);
                box.rect = checkBoxRect;
                /*proxy()->*/drawPrimitive(PE_IndicatorCheckBox, &box, painter, widget);
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
                doEtch(opts.buttonEffect != EFFECT_NONE && opts.etchEntry),
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

            if(!kwin && widget && BLEND_TITLEBAR && qobject_cast<const QMdiSubWindow *>(widget))
            {
                const QWidget *w=nullptr;
                if(qobject_cast<const QMainWindow *>(widget))
                    w=widget;
                else if (static_cast<const QMdiSubWindow *>(widget)->widget())
                    w=qobject_cast<const QMainWindow *>(static_cast<const QMdiSubWindow *>(widget)->widget());
                if(w)
                {
                    const QMenuBar *menuBar=static_cast<const QMainWindow *>(w)->menuBar();

                    if(menuBar)
                        tr.adjust(0, 0, 0, menuBar->rect().height());
                }
            }

            opt.state=State_Horizontal|State_Enabled|State_Raised|(active ? State_Active : State_None);

#ifndef QTC_QT6_ENABLE_KDE
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

            if(kwin && (state&QtC_StateKWinFillBgnd))
                drawBevelGradient(titleCols[ORIGINAL_SHADE], painter, tr, path, true, false, APPEARANCE_FLAT, WIDGET_MDI_WINDOW, false);
            if((!kwin && !m_isPreview) ||
               (APPEARANCE_NONE!=app && (!qtcIsFlat(app) || (titleCols[ORIGINAL_SHADE]!=QApplication::palette().window().color()))))
                drawBevelGradient(titleCols[ORIGINAL_SHADE], painter, tr, path, true, false, app, WIDGET_MDI_WINDOW, false);

            if(!(state&QtC_StateKWinNoBorder))
            {
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

                QPAINTER_RENDERHINT_AA_MAYBE_OFF(painter);

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
                    painter->setPen(QPen(QBrush(grad), QPENWIDTH1));
                    painter->drawLine(r.x(), start.y(), r.x(), end.y());
                    painter->drawLine(r.x()+r.width()-1, start.y(), r.x()+r.width()-1, end.y());

                    if(addLight)
                    {
                        grad.setColorAt(0, light);
                        grad.setColorAt(1, m_backgroundCols[0]);
                        painter->setPen(QPen(QBrush(grad), QPENWIDTH1));
                        painter->drawLine(r.x()+1, start.y(), r.x()+1, end.y());
                    }
                }
            }
            else
                QPAINTER_RENDERHINT_AA_MAYBE_OFF(painter);

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

#ifndef QTC_QT6_ENABLE_KDE
                QFont         font = m_fntHelper->fontStripStyleName(painter->font());
                font.setBold(true);
                painter->setFont(font);
#else
                painter->setFont(QFontDatabase::systemFont(QFontDatabase::TitleFont));
#endif

                QFontMetrics fm(painter->fontMetrics());
                QString str(fm.elidedText(titleBar->text, Qt::ElideRight, textRect.width(), QPalette::WindowText));

                int           textWidth=alignFull || (showIcon && alignment&Qt::AlignHCenter)
                    ? fm.boundingRect(str).width()+(showIcon ? iconSize+constPad : 0) : 0;

                if(alignFull &&
                   ( (captionRect.left()>((textRect.width()-textWidth)>>1)) ||
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
                            (opts.scrollbarType == SCROLLBAR_NONE ||
                             opts.flatSbarButtons));
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
                qtcCheckType(getParent<2>(widget), "QComboBoxListView")) {
                painter->fillRect(r, palette.brush(QPalette::Base));
            } else if (opts.thinSbarGroove && theThemedApp == APP_ARORA &&
                       qtcCheckType(widget, "WebView")) {
                painter->fillRect(r, m_backgroundCols[ORIGINAL_SHADE]);
            }
            if (!opts.gtkScrollViews ||
                (opts.flatSbarButtons && !qtcIsFlat(opts.sbarBgndAppearance)))
                drawBevelGradientReal(palette.brush(QPalette::Base).color(), painter, r, horiz, false,
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
                    painter->setClipRegion(QRegion(opt.rect).subtracted(slider.adjusted(1, 1, -1, -1)));
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
                if(!opts.flatSbarButtons)
                {
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

            QRect frame(subControlRect(CC_ComboBox, option,
                                       SC_ComboBoxFrame, widget));
            QRect arrow(subControlRect(CC_ComboBox, option,
                                       SC_ComboBoxArrow, widget));
            QRect field(subControlRect(CC_ComboBox, option,
                                       SC_ComboBoxEditField, widget));
            const QColor *use = buttonColors(option);
            bool sunken = state & State_On;
            bool glowOverFocus =
                (state & State_MouseOver &&
                 oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED) &&
                 opts.coloredMouseOver == MO_GLOW &&
                 opts.buttonEffect != EFFECT_NONE && !sunken &&
                 !comboBox->editable && state & State_Enabled &&
                 state & State_HasFocus);
            bool doEffect = (opts.buttonEffect != EFFECT_NONE &&
                             (!comboBox->editable || opts.etchEntry));
            bool isOO = isOOWidget(widget);
            bool isOO31 = isOO;

            if (isOO) {
                // This (hopefull) checks is we're OO.o 3.2 -
                // in which case no adjustment is required...
                const QImage *img = getImage(painter);

                isOO31 = !img || img->rect() != r;

                if (isOO31) {
                    frame.adjust(0, 0, 0, -2);
                    arrow.adjust(0, 0, 0, -2);
                    field.adjust(0, 0, 0, -2);
                } else {
                    arrow.adjust(1, 0, 0, 0);
                }
            }

            // painter->fillRect(r, Qt::transparent);
            if (doEffect) {
                bool glowFocus(state&State_HasFocus && state&State_Enabled && USE_GLOW_FOCUS(state&State_MouseOver));

                if (!glowOverFocus && !(opts.thin & THIN_FRAMES) && !sunken &&
                    opts.coloredMouseOver == MO_GLOW &&
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
                             comboBox->editable && opts.square & SQUARE_ENTRY ?
                             opts.unifyCombo ? ROUNDED_NONE : reverse ?
                             ROUNDED_LEFT : ROUNDED_RIGHT : ROUNDED_ALL);
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
                if(comboBox->editable || !(opts.gtkComboMenus && opts.doubleGtkComboArrow))
                    drawArrow(painter, arrow, PE_IndicatorArrowDown, arrowColor, false);
                else
                {
                    int middle=arrow.y()+(arrow.height()>>1),
                        gap=(opts.vArrows ? 2 : 1);

                    QRect ar=QRect(arrow.x(), middle-(LARGE_ARR_HEIGHT+gap), arrow.width(), LARGE_ARR_HEIGHT);
                    drawArrow(painter, ar, PE_IndicatorArrowUp, arrowColor, false);
                    ar=QRect(arrow.x(), middle+gap, arrow.width(), LARGE_ARR_HEIGHT);
                    drawArrow(painter, ar, PE_IndicatorArrowDown, arrowColor, false);
                }
            }

            if(state&State_Enabled && state&State_HasFocus &&
               /*state&State_KeyboardFocusChange &&*/ !comboBox->editable && FOCUS_GLOW!=opts.focus)
            {
                QStyleOptionFocusRect focus;
                bool listViewCombo = (comboBox->frame && widget &&
                                      widget->rect().height() <
                                      (opts.buttonEffect != EFFECT_NONE ?
                                       22 : 20));

                if (oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED)) {
                    focus.rect = frame;
                } else if (opts.comboSplitter) {
                    focus.rect = (reverse ? field.adjusted(0, -1, 1, 1) :
                                  field.adjusted(-1, -1, 0, 1));

                    if(listViewCombo)
                        focus.rect.adjust(0, -2, 0, 2);
                }
                else if(listViewCombo)
                    focus.rect=frame.adjusted(1, 1, -1, -1);
                else
                    focus.rect=frame.adjusted(3, 3, -3, -3);

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
        ParentStyleClass::drawComplexControl(control, option, painter, widget);
        break;
    }
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
            ParentStyleClass::drawItemText(painter, rect, flags, p, enabled,
                                       text, QPalette::HighlightedText);
            return;
        }
    }

    ParentStyleClass::drawItemText(painter, rect, flags, pal,
                               enabled, text, textRole);
}

QSize Style::sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const
{
    prePolish(widget);
    QSize newSize(ParentStyleClass::sizeFromContents(type, option, size, widget));

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
                bool allowIcon = styleHint(SH_DialogButtonBox_ButtonsHaveIcons, btn, widget) || btn->text.isEmpty();
                if (allowIcon && (qtcCheckType<QDialogButtonBox>(getParent(widget)) ||
                    qtcCheckType(getParent(widget), "KFileWidget"))) {
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

                btn.initFrom(widget);
                btn.text = tbOpt->text;
                btn.icon = tbOpt->icon;
                btn.iconSize = tbOpt->iconSize;
                btn.features = ((tbOpt->features &
                                QStyleOptionToolButton::MenuButtonPopup) ?
                                QStyleOptionButton::HasMenu :
                                QStyleOptionButton::None);
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
    case CT_ComboBox: {
        newSize = size;
        newSize.setWidth(newSize.width() + 4);

        auto combo = styleOptCast<QStyleOptionComboBox>(option);
        int  margin = (pixelMetric(PM_ButtonMargin, option, widget)+
                       (pixelMetric(PM_DefaultFrameWidth, option, widget) * 2))-MAX_ROUND_BTN_PAD,
            textMargins = 2*(pixelMetric(PM_FocusFrameHMargin) + 1),
            // QItemDelegate::sizeHint expands the textMargins two times, thus the 2*textMargins...
            other = qMax(opts.buttonEffect != EFFECT_NONE ? 20 : 18,
                         2 * textMargins +
                         pixelMetric(QStyle::PM_ScrollBarExtent,
                                     option, widget));
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
            initFontTickData(mi->font, widget);
            // Taken from QWindowStyle...
            int w = size.width();

            if (QStyleOptionMenuItem::Separator==mi->menuItemType) {
                newSize = QSize(10, windowsSepHeight);
            } else {
                if (mi->icon.isNull()) {
                    newSize.setHeight(newSize.height() - 2);
                    w -= 6;
                } else {
                    int iconExtent = pixelMetric(PM_SmallIconSize, option, widget);
                    newSize.setHeight(qMax(newSize.height(),
                                        mi->icon.actualSize(QSize(iconExtent, iconExtent)).height()
                                        + 2 * windowsItemFrame));
                }
                int dx;
                if (menuTickCompensation(opts, 20, dx)) {
                    w += dx;
                }
            }
            int maxpmw = mi->maxIconWidth,
                tabSpacing = 20;

            if (!opts.buttonStyleMenuSections && QStyleOptionMenuItem::Separator == mi->menuItemType && !mi->text.isEmpty())
            {
                QFont fontBold = m_fntHelper->fontStripStyleName(mi->font);
                fontBold.setBold(true);
                QFontMetrics fmBold(fontBold);
                // _set_ w, it will have been initialised to something inappropriately small
                w = horizontalAdvance(fmBold, mi->text);
            }
            else if (mi->text.contains(QLatin1Char('\t')))
                w += tabSpacing;
            else if (mi->menuItemType == QStyleOptionMenuItem::SubMenu)
                w += 2 * windowsArrowHMargin;
            else if (mi->menuItemType == QStyleOptionMenuItem::DefaultItem)
            {
                // adjust the font and add the difference in size.
                // it would be better if the font could be adjusted in the initStyleOption qmenu func!!
                QFont fontBold = m_fntHelper->fontStripStyleName(mi->font);
                QFontMetrics fm(fontBold);
                fontBold.setBold(true);
                QFontMetrics fmBold(fontBold);
                w += horizontalAdvance(fmBold, mi->text) - horizontalAdvance(fm, mi->text);
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
            newSize = size + QSize((windowsItemHMargin * 4) + 2,
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
    switch (element) {
    case SE_SliderFocusRect:
    case SE_ToolBoxTabContents:
        return visualRect(option->direction, option->rect, option->rect);
    case SE_DockWidgetTitleBarText: {
        auto dwopt = styleOptCast<QStyleOptionDockWidget>(option);
        bool verticalTitleBar = dwopt ? dwopt->verticalTitleBar : false;
        int m = pixelMetric(PM_DockWidgetTitleMargin, option, widget);

        rect = ParentStyleClass::subElementRect(element, option, widget);

        if (verticalTitleBar)
            rect.adjust(0, 0, 0, -m);
        else if (Qt::LeftToRight==option->direction )
            rect.adjust(m, 0, 0, 0);
        else
            rect.adjust(0, 0, -m, 0);
        return rect;
    }
    case SE_TabBarTabLeftButton:
        return ParentStyleClass::subElementRect(element, option, widget).translated(-2, -1);
    case SE_TabBarTabRightButton:
        return ParentStyleClass::subElementRect(element, option, widget).translated(2, -1);
    case SE_TabBarTabText:
        if (auto _tab = styleOptCast<QStyleOptionTab>(option)) {
            QStyleOptionTab tab(*_tab);
            bool verticalTabs = QTabBar::RoundedEast == tab.shape ||
                QTabBar::RoundedWest == tab.shape ||
                QTabBar::TriangularEast == tab.shape ||
                QTabBar::TriangularWest == tab.shape;

            rect=tab.rect;
            if (verticalTabs)
                rect.setRect(0, 0, rect.height(), rect.width());
            int verticalShift = pixelMetric(QStyle::PM_TabBarTabShiftVertical,
                                            _tab, widget);
            int horizontalShift = pixelMetric(QStyle::PM_TabBarTabShiftHorizontal,
                                              _tab, widget);
            if (tab.shape == QTabBar::RoundedSouth ||
                tab.shape == QTabBar::TriangularSouth)
                verticalShift = -verticalShift;
            rect.adjust(0, 0, horizontalShift, verticalShift);
            bool selected = tab.state & State_Selected;
            if (selected) {
                rect.setBottom(rect.bottom() - verticalShift);
                rect.setRight(rect.right() - horizontalShift);
            }

            // left widget
            if(opts.centerTabText) {
                if (!tab.leftButtonSize.isEmpty())  // left widget
                    rect.setLeft(rect.left() + constTabPad +
                                 (verticalTabs ? tab.leftButtonSize.height() :
                                  tab.leftButtonSize.width()));
                if (!tab.rightButtonSize.isEmpty()) // right widget
                    rect.setRight(rect.right() - constTabPad -
                                  (verticalTabs ? tab.rightButtonSize.height() :
                                   tab.rightButtonSize.width()));
            } else {
                if (tab.leftButtonSize.isNull()) {
                    rect.setLeft(rect.left()+constTabPad);
                } else if(tab.leftButtonSize.width()>0) {
                    rect.setLeft(rect.left() + constTabPad + 2 +
                                 (verticalTabs ? tab.leftButtonSize.height() :
                                  tab.leftButtonSize.width()));
                } else if(tab.icon.isNull()) {
                    rect.setLeft(rect.left() + constTabPad);
                } else {
                    rect.setLeft(rect.left() + 2);
                }
            }

            // icon
            if (!tab.icon.isNull()) {
                QSize iconSize = tab.iconSize;
                if (!iconSize.isValid()) {
                    int iconExtent = pixelMetric(PM_SmallIconSize);
                    iconSize = QSize(iconExtent, iconExtent);
                }
                QSize tabIconSize = tab.icon.actualSize(iconSize,
                                                        (tab.state &
                                                         State_Enabled) ?
                                                        QIcon::Normal :
                                                        QIcon::Disabled);
                int offset = 4;

                if (!opts.centerTabText && tab.leftButtonSize.isNull())
                    offset += 2;

                QRect iconRect = QRect(rect.left() + offset, rect.center().y() - tabIconSize.height() / 2,
                                       tabIconSize.width(), tabIconSize .height());
                if (!verticalTabs)
                    iconRect = visualRect(option->direction, option->rect, iconRect);
                rect.setLeft(rect.left() + tabIconSize.width() + offset + 2);
            }

            // right widget
            if (!opts.centerTabText && !tab.rightButtonSize.isNull() &&
                tab.rightButtonSize.width() > 0) {
                rect.setRight(rect.right() - constTabPad - 2 -
                              (verticalTabs ? tab.rightButtonSize.height() :
                               tab.rightButtonSize.width()));
            } else {
                rect.setRight(rect.right() - constTabPad);
            }

            if (!verticalTabs)
                rect = visualRect(option->direction, option->rect, rect);
            return rect;
        }
        break;
    case SE_RadioButtonIndicator:
        rect = visualRect(option->direction, option->rect,
                          ParentStyleClass::subElementRect(element, option, widget)).adjusted(0, 0, 1, 1);
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
            rect = ParentStyleClass::subElementRect(element, option, widget);
            if (opts.buttonEffect != EFFECT_NONE) {
                rect.adjust(1, 1, -1, -1);
            }
        }
        return rect;
    default:
        return ParentStyleClass::subElementRect(element, option, widget);
    }

    return visualRect(option->direction, option->rect, rect);
}

QRect Style::subControlRect(ComplexControl control, const QStyleOptionComplex *option, SubControl subControl, const QWidget *widget) const
{
    prePolish(widget);
    QRect r(option->rect);
    bool  reverse(Qt::RightToLeft==option->direction);

    switch (control) {
    case CC_ComboBox:
        if (auto comboBox = styleOptCast<QStyleOptionComboBox>(option)) {
            bool ed = comboBox->editable;
            bool doEtch = ((!ed || opts.etchEntry) &&
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
            QSize bs;
            int fw = spinbox->frame ? pixelMetric(PM_SpinBoxFrameWidth, spinbox, widget) : 0;

            bs.setHeight(r.height() >> 1);
            if (bs.height() < 8)
                bs.setHeight(8);
            bs.setWidth(opts.buttonEffect != EFFECT_NONE && opts.etchEntry ?
                        16 : 15);
            bs = bs.expandedTo(QApplication::globalStrut());

            int y = 0;
            int x = reverse ? (0 + fw) : (r.width() - bs.width() - fw + r.x());

            switch (subControl) {
            case SC_SpinBoxUp:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                r = QRect(x, y, bs.width(), bs.height());
                break;
            case SC_SpinBoxDown:
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons)
                    return QRect();
                r = QRect(x, y + bs.height(), bs.width(),
                          bs.height() + (bs.height() * 2 == r.height() ? 0 : 1));
                break;
            case SC_SpinBoxEditField: {
                int pad = opts.round > ROUND_FULL ? 2 : 0;
                if (spinbox->buttonSymbols == QAbstractSpinBox::NoButtons) {
                    r = QRect(fw, fw, (x - fw * 2) - pad, r.height() - 2 * fw);
                }
                else {
                    r = QRect(fw + (reverse ? bs.width() : 0), fw,
                              (x - fw * 2) - pad, r.height() - 2 * fw);
                }
                break;
            }
            case SC_SpinBoxFrame:
            default:
                break;
            }
            return visualRect(spinbox->direction, spinbox->rect, r);
        }
        break;
    case CC_ScrollBar:
        if (auto scrollBar = styleOptCast<QStyleOptionSlider>(option)) {
            // Taken from kstyle.cpp (KDE 3) , and modified so as to allow
            // for no scrollbar butttons...
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
                ret = ParentStyleClass::subControlRect(control, option, subControl, widget);
                break;
            }
            return visualRect(scrollBar->direction/*Qt::LeftToRight*/, scrollBar->rect, ret);
        }
        break;
    case CC_Slider:
        if (auto slider = styleOptCast<QStyleOptionSlider>(option)) {
            if (SLIDER_TRIANGULAR == opts.sliderStyle) {
                int tickSize(pixelMetric(PM_SliderTickmarkOffset, option, widget)),
                    mod=MO_GLOW==opts.coloredMouseOver && opts.buttonEffect != EFFECT_NONE ? 2 : 0;
                QRect rect(ParentStyleClass::subControlRect(control, option, subControl, widget));

                switch (subControl) {
                case SC_SliderHandle:
                    if (slider->orientation == Qt::Horizontal) {
                        rect.setWidth(11 + mod);
                        rect.setHeight(15 + mod);
                        int centerY = r.center().y() - rect.height() / 2;
                        if (slider->tickPosition & QSlider::TicksAbove) {
                            centerY += tickSize;
                        }
                        if (slider->tickPosition & QSlider::TicksBelow) {
                            centerY -= tickSize - 1;
                        }
                        rect.moveTop(centerY);
                    } else {
                        rect.setWidth(15 + mod);
                        rect.setHeight(11 + mod);
                        int centerX = r.center().x() - rect.width() / 2;
                        if (slider->tickPosition & QSlider::TicksAbove) {
                            centerX += tickSize;
                        }
                        if (slider->tickPosition & QSlider::TicksBelow) {
                            centerX -= tickSize - 1;
                        }
                        rect.moveLeft(centerX);
                    }
                    break;
                case SC_SliderGroove: {
                    QPoint grooveCenter = r.center();

                    if (Qt::Horizontal == slider->orientation) {
                        rect.setHeight(13);
                        --grooveCenter.ry();
                        if (slider->tickPosition & QSlider::TicksAbove) {
                            grooveCenter.ry() += tickSize + 2;
                        }
                        if (slider->tickPosition & QSlider::TicksBelow) {
                            grooveCenter.ry() -= tickSize - 1;
                        }
                    } else {
                        rect.setWidth(13);
                        --grooveCenter.rx();
                        if (slider->tickPosition & QSlider::TicksAbove) {
                            grooveCenter.rx() += tickSize + 2;
                        }
                        if (slider->tickPosition & QSlider::TicksBelow) {
                            grooveCenter.rx() -= tickSize - 1;
                        }
                    }
                    rect.moveCenter(grooveCenter);
                    break;
                }
                default:
                    break;
                }
                return rect;
            } else {
                bool horizontal = Qt::Horizontal == slider->orientation;
                int thickness = pixelMetric(PM_SliderControlThickness,
                                            slider, widget);
                int tickOffset = (slider->tickPosition & QSlider::TicksAbove ||
                                  slider->tickPosition & QSlider::TicksBelow ?
                                  pixelMetric(PM_SliderTickmarkOffset, slider,
                                              widget) :
                                  ((horizontal ? r.height() :
                                    r.width()) - thickness) / 2);

                switch (subControl) {
                case SC_SliderHandle: {
                    int len = pixelMetric(PM_SliderLength, slider, widget);
                    int sliderPos =
                        sliderPositionFromValue(slider->minimum, slider->maximum,
                                                slider->sliderPosition,
                                                (horizontal ? r.width() :
                                                 r.height()) - len,
                                                slider->upsideDown);

                    if (horizontal) {
                        r.setRect(r.x() + sliderPos, r.y() + tickOffset,
                                  len, thickness);
                    } else {
                        r.setRect(r.x() + tickOffset, r.y() + sliderPos,
                                  thickness, len);
                    }
                    break;
                }
                case SC_SliderGroove:
                    if (horizontal) {
                        r.setRect(r.x(), r.y() + tickOffset,
                                  r.width(), thickness);
                    } else {
                        r.setRect(r.x() + tickOffset, r.y(),
                                  thickness, r.height());
                    }
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
                QFont font = m_fntHelper->fontStripStyleName(widget ? widget->font() : QApplication::font());
                font.setBold(opts.gbLabel & GB_LBL_BOLD);
                QFontMetrics fontMetrics(font);
                int h = fontMetrics.height();
                int tw = fontMetrics.size(Qt::TextShowMnemonic,
                                          groupBox->text +
                                          QLatin1Char(' ')).width();
                int marg = ((groupBox->features & QStyleOptionFrame::Flat) ||
                            qtcNoFrame(opts.groupBox) ||
                            opts.gbLabel & GB_LBL_OUTSIDE ? 0 :
                            opts.gbLabel & GB_LBL_INSIDE ? 2 : 6);
                int indicatorWidth = pixelMetric(PM_IndicatorWidth,
                                                 option, widget);
                int indicatorSpace = pixelMetric(PM_CheckBoxLabelSpacing,
                                                 option, widget) - 1;
                bool hasCheckBox(groupBox->subControls & QStyle::SC_GroupBoxCheckBox);
                int checkBoxSize(hasCheckBox ? (indicatorWidth + indicatorSpace) : 0),
                    checkAdjust(qtcNoFrame(opts.groupBox) ||
                                opts.gbLabel & GB_LBL_OUTSIDE ? 0 : 2);

                if(0==checkAdjust)
                    checkBoxSize-=2;

                r.adjust(marg, 0, -marg, 0);
                if(!qtcNoFrame(opts.groupBox) && opts.gbLabel & GB_LBL_INSIDE)
                    r.adjust(0, 2, 0, 2);
                r.setHeight(h);

                // Adjusted rect for label + indicatorWidth + indicatorSpace
                Qt::Alignment align = groupBox->textAlignment;
                if (opts.gbLabel & GB_LBL_CENTRED) {
                    align &= ~(Qt::AlignLeft | Qt::AlignRight);
                    align |= Qt::AlignHCenter;
                }
                r = alignedRect(groupBox->direction, align,
                                QSize(tw + checkBoxSize, h), r);

                // Adjust totalRect if checkbox is set
                if (hasCheckBox) {
                    if (SC_GroupBoxCheckBox == subControl) {
                        // Adjust for check box
                        int indicatorHeight(pixelMetric(PM_IndicatorHeight, option, widget)),
                            top(r.top() + (fontMetrics.height() - indicatorHeight) / 2);

                        r.setRect(reverse ? (r.right() - indicatorWidth) : r.left()+checkAdjust, top, indicatorWidth, indicatorHeight);
                    } else {
                        // Adjust for label
                        r.setRect(reverse ? r.left() :
                                  (r.left() + checkBoxSize), r.top(),
                                  r.width() - checkBoxSize, r.height());
                    }
                }
                return r;
            }
        break;
    case CC_TitleBar:
        if (auto tb = styleOptCast<QStyleOptionTitleBar>(option)) {
            bool isMinimized(tb->titleBarState&Qt::WindowMinimized),
                isMaximized(tb->titleBarState&Qt::WindowMaximized);

            if( (isMaximized && SC_TitleBarMaxButton==subControl) ||
                (isMinimized && SC_TitleBarMinButton==subControl) ||
                (isMinimized && SC_TitleBarShadeButton==subControl) ||
                (!isMinimized && SC_TitleBarUnshadeButton==subControl))
                return QRect();

            readMdiPositions();

            const int controlSize(tb->rect.height() - constWindowMargin *2);
            int sc = (subControl == SC_TitleBarUnshadeButton ?
                      SC_TitleBarShadeButton :
                      subControl == SC_TitleBarNormalButton ?
                      isMaximized ? SC_TitleBarMaxButton :
                      SC_TitleBarMinButton : subControl);
            int pos = 0;
            int totalLeft = 0;
            int totalRight = 0;
            bool rhs = false;
            bool found = false;

            for (int hint: const_(m_mdiButtons[0])) {
                if (hint == SC_TitleBarCloseButton ||
                    hint == WINDOWTITLE_SPACER ||
                    tb->titleBarFlags & toHint(hint)) {
                    totalLeft += (hint == WINDOWTITLE_SPACER ?
                                  controlSize / 2 : controlSize);
                    if (hint == sc) {
                        found = true;
                    } else if (!found) {
                        pos += (hint == WINDOWTITLE_SPACER ?
                                controlSize / 2 : controlSize);
                    }
                }
            }
            if (!found) {
                pos = 0;
                rhs = true;
            }

            for (int hint: const_(m_mdiButtons[1])) {
                if (hint == SC_TitleBarCloseButton ||
                    hint == WINDOWTITLE_SPACER ||
                    tb->titleBarFlags & toHint(hint)) {
                    if (hint != WINDOWTITLE_SPACER || totalRight) {
                        totalRight += (hint == WINDOWTITLE_SPACER ?
                                       controlSize / 2 : controlSize);
                    }
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
    return ParentStyleClass::subControlRect(control, option, subControl, widget);
}

QStyle::SubControl
Style::hitTestComplexControl(ComplexControl control,
                             const QStyleOptionComplex *option,
                             const QPoint &pos, const QWidget *widget) const
{
    prePolish(widget);
    m_sbWidget = nullptr;
    switch (control) {
    case CC_ScrollBar:
        if (auto scrollBar = styleOptCast<QStyleOptionSlider>(option)) {
            if (subControlRect(control, scrollBar,
                               SC_ScrollBarSlider, widget).contains(pos)) {
                return SC_ScrollBarSlider;
            }
            if (subControlRect(control, scrollBar,
                               SC_ScrollBarAddLine, widget).contains(pos)) {
                return SC_ScrollBarAddLine;
            }
            if (subControlRect(control, scrollBar,
                               SC_ScrollBarSubPage, widget).contains(pos)) {
                return SC_ScrollBarSubPage;
            }
            if (subControlRect(control, scrollBar,
                               SC_ScrollBarAddPage, widget).contains(pos)) {
                return SC_ScrollBarAddPage;
            }
            if (subControlRect(control, scrollBar,
                               SC_ScrollBarSubLine, widget).contains(pos)) {
                if (opts.scrollbarType == SCROLLBAR_KDE &&
                    subControlRect(control, scrollBar,
                                   SB_SUB2, widget).contains(pos)) {
                    m_sbWidget = widget;
                }
                return SC_ScrollBarSubLine;
            }
        }
    default:
        break;
    }
    return ParentStyleClass::hitTestComplexControl(control, option,  pos, widget);
}

}
