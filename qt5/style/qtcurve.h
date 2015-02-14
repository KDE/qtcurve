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

#ifndef __QTCURVE_H__
#define __QTCURVE_H__

#include <QTime>
#include <QPalette>
#include <QMap>
#include <QList>
#include <QSet>
#include <QCache>
#include <QColor>
#include <QStyleOption>
#include <QtGlobal>
#include <QCommonStyle>
typedef qulonglong QtcKey;
#include <common/common.h>

#ifdef QTC_QT5_ENABLE_KDE
#include <KDE/KComponentData>
#endif

class QStyleOptionSlider;
class QLabel;
class QMenuBar;
class QScrollBar;
class QDBusInterface;
class QMainWindow;
class QStatusBar;
class QAbstractScrollArea;
class QProgressBar;
class QFormLayout;

namespace QtCurve {
class WindowManager;
class BlurHelper;
class ShortcutHandler;
class ShadowHelper;

class Style: public QCommonStyle {
    Q_OBJECT
    Q_CLASSINFO("X-KDE-CustomElements", "true")
public:
    enum BackgroundType {
        BGND_WINDOW,
        BGND_DIALOG,
        BGND_MENU
    };

    enum MenuItemType {
        MENU_POPUP,
        MENU_BAR,
        MENU_COMBO
    };

    enum CustomElements {
        CE_QtC_KCapacityBar = CE_CustomBase + 0x00FFFF00,
        CE_QtC_Preview,
        CE_QtC_SetOptions
    };

    enum PreviewType {
        PREVIEW_FALSE,
        PREVIEW_MDI,
        PREVIEW_WINDOW
    };

    class PreviewOption: public QStyleOption {
    public:
        Options opts;
    };

    class BgndOption: public QStyleOption {
    public:
        EAppearance app;
        QPainterPath path;
        QRect widgetRect;
    };

    enum Icon {
        ICN_MIN,
        ICN_MAX,
        ICN_MENU,
        ICN_RESTORE,
        ICN_CLOSE,
        ICN_UP,
        ICN_DOWN,
        ICN_RIGHT,
        ICN_SHADE,
        ICN_UNSHADE
    };

    Style();
    ~Style();

    void polish(QApplication *app) override;
    void polish(QPalette &palette) override;
    void polish(QWidget *widget) override;
    void unpolish(QApplication *app) override;
    void unpolish(QWidget *widget) override;

    bool eventFilter(QObject *object, QEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

    int pixelMetric(PixelMetric metric, const QStyleOption *option=0,
                    const QWidget *widget=0) const override;
    int styleHint(StyleHint hint, const QStyleOption *option,
                  const QWidget *widget,
                  QStyleHintReturn *returnData=0) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                           const QSize &size,
                           const QWidget *widget) const override;
    QPalette standardPalette() const override;
    QIcon standardIcon(StandardPixmap pix, const QStyleOption *option=0,
                       const QWidget *widget=0) const override;
    int layoutSpacing(QSizePolicy::ControlType control1,
                      QSizePolicy::ControlType control2,
                      Qt::Orientation orientation,
                      const QStyleOption *option=0,
                      const QWidget *widget=0) const override;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter,
                       const QWidget *widget) const override;
    void drawControl(ControlElement control, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const override;
    void drawComplexControl(ComplexControl control,
                            const QStyleOptionComplex *option,
                            QPainter *painter,
                            const QWidget *widget) const override;
    void drawItemText(QPainter *painter, const QRect &rect, int flags,
                      const QPalette &pal, bool enabled, const QString &text,
                      QPalette::ColorRole=QPalette::NoRole) const override;

    QRect subElementRect(SubElement element, const QStyleOption *option,
                         const QWidget *widget) const override;
    QRect subControlRect(ComplexControl control,
                         const QStyleOptionComplex *option,
                         SubControl subControl,
                         const QWidget *widget) const override;
    SubControl hitTestComplexControl(ComplexControl control,
                                     const QStyleOptionComplex *option,
                                     const QPoint &pos,
                                     const QWidget *widget) const override;

    Options&
    options()
    {
        return opts;
    }
    void prePolish(QWidget *w) const;
    void
    prePolish(const QWidget *w) const
    {
        prePolish(const_cast<QWidget*>(w));
    }

private:
    void init(bool initial);
    void freeColor(QSet<QColor*> &freedColors, QColor **cols);
    void freeColors();
    void polishFormLayout(QFormLayout *layout);
    void polishLayout(QLayout *layout);
    void polishScrollArea(QAbstractScrollArea *scrollArea,
                          bool isKFilePlacesView=false) const;

    void drawItemTextWithRole(QPainter *painter, const QRect &rect, int flags,
                              const QPalette &pal, bool enabled,
                              const QString &text,
                              QPalette::ColorRole textRole) const;
    void drawSideBarButton(QPainter *painter, const QRect &r,
                           const QStyleOption *option,
                           const QWidget *widget) const;
    void drawHighlight(QPainter *p, const QRect &r, bool horiz, bool inc) const;
    void drawFadedLine(QPainter *p, const QRect &r, const QColor &col,
                       bool fadeStart, bool fadeEnd, bool horiz,
                       double fadeSizeStart=FADE_SIZE,
                       double fadeSizeEnd=FADE_SIZE) const;
    void drawLines(QPainter *p, const QRect &r, bool horiz, int nLines,
                   int offset, const QColor *cols, int startOffset,
                   int dark, ELine type) const;
    void drawProgressBevelGradient(QPainter *p, const QRect &origRect,
                                   const QStyleOption *option, bool horiz,
                                   EAppearance bevApp,
                                   const QColor *cols) const;
    void drawBevelGradient(const QColor &base, QPainter *p, QRect const &r,
                           const QPainterPath &path, bool horiz, bool sel,
                           EAppearance bevApp, EWidget w=WIDGET_OTHER,
                           bool useCache=true) const;
    void drawBevelGradientReal(const QColor &base, QPainter *p, const QRect &r,
                               const QPainterPath &path, bool horiz, bool sel,
                               EAppearance bevApp, EWidget w) const;

    void
    drawBevelGradient(const QColor &base, QPainter *p, QRect const &r,
                      bool horiz, bool sel, EAppearance bevApp,
                      EWidget w=WIDGET_OTHER, bool useCache=true) const
    {
        drawBevelGradient(base, p, r, QPainterPath(), horiz, sel, bevApp, w,
                          useCache);
    }
    void
    drawBevelGradientReal(const QColor &base, QPainter *p, const QRect &r,
                          bool horiz, bool sel,
                          EAppearance bevApp, EWidget w) const
    {
        drawBevelGradientReal(base, p, r, QPainterPath(), horiz,
                              sel, bevApp, w);
    }

    void drawSunkenBevel(QPainter *p, const QRect &r, const QColor &col) const;
    void drawLightBevel(QPainter *p, const QRect &r, const QStyleOption *option,
                        const QWidget *widget, int round, const QColor &fill,
                        const QColor *custom=0, bool doBorder=true,
                        EWidget w=WIDGET_OTHER) const;
    void drawLightBevelReal(QPainter *p, const QRect &r,
                            const QStyleOption *option, const QWidget *widget,
                            int round, const QColor &fill,
                            const QColor *custom, bool doBorder, EWidget w,
                            bool useCache, ERound realRound,
                            bool onToolbar) const;
    void drawGlow(QPainter *p, const QRect &r, EWidget w,
                  const QColor *cols=0L) const;
    void drawEtch(QPainter *p, const QRect &r, const QWidget *widget,
                  EWidget w, bool raised=false, int round=ROUNDED_ALL) const;
    void drawBgndRing(QPainter &painter, int x, int y, int size,
                      int size2, bool isWindow) const;
    QPixmap drawStripes(const QColor &color, int opacity) const;
    void drawBackground(QPainter *p, const QColor &bgnd, const QRect &r,
                        int opacity, BackgroundType type, EAppearance app,
                        const QPainterPath &path=QPainterPath()) const;
    void drawBackgroundImage(QPainter *p, bool isWindow, const QRect &r) const;
    void drawBackground(QPainter *p, const QWidget *widget,
                        BackgroundType type) const;
    QPainterPath buildPath(const QRectF &r, EWidget w,
                           int round, double radius) const;
    QPainterPath buildPath(const QRect &r, EWidget w,
                           int round, double radius) const;
    void buildSplitPath(const QRect &r, int round, double radius,
                        QPainterPath &tl, QPainterPath &br) const;
    void drawBorder(QPainter *p, const QRect &r, const QStyleOption *option,
                    int round, const QColor *custom=0, EWidget w=WIDGET_OTHER,
                    EBorder borderProfile=BORDER_FLAT, bool doBlend=true,
                    int borderVal=QTC_STD_BORDER) const;
    void drawMdiControl(QPainter *p, const QStyleOptionTitleBar *titleBar,
                        SubControl sc, const QWidget *widget,
                        ETitleBarButtons btn, const QColor &iconColor,
                        const QColor *btnCols, const QColor *bgndCols,
                        int adjust, bool activeWindow) const;
    void drawDwtControl(QPainter *p, const State &state,
                        const QRect &rect, ETitleBarButtons btn, Icon icon,
                        const QColor &iconColor, const QColor *btnCols,
                        const QColor *bgndCols) const;
    bool drawMdiButton(QPainter *painter, const QRect &r, bool hover,
                       bool sunken, const QColor *cols) const;
    void drawMdiIcon(QPainter *painter, const QColor &color, const QColor &bgnd,
                     const QRect &r, bool hover, bool sunken, Icon iclearcon,
                     bool stdSize, bool drewFrame) const;
    void drawIcon(QPainter *painter, const QColor &color, const QRect &r,
                  bool sunken, Icon icon, bool stdSize=true) const;
    void drawEntryField(QPainter *p, const QRect &rx, const QWidget *widget,
                        const QStyleOption *option, int round,
                        bool fill, bool doEtch, EWidget w=WIDGET_ENTRY) const;
    void drawMenuItem(QPainter *p, const QRect &r, const QStyleOption *option,
                      MenuItemType type, int round, const QColor *cols) const;
    void drawProgress(QPainter *p, const QRect &r, const QStyleOption *option,
                      bool vertical=false, bool reverse=false) const;
    void drawArrow(QPainter *p, const QRect &rx, PrimitiveElement pe,
                   QColor col, bool small=false, bool kwin=false) const;
    void drawSbSliderHandle(QPainter *p, const QRect &r,
                            const QStyleOption *option,
                            bool slider=false) const;
    void drawSliderHandle(QPainter *p, const QRect &r,
                          const QStyleOptionSlider *option) const;
    void drawSliderGroove(QPainter *p, const QRect &groove, const QRect &handle,
                          const QStyleOptionSlider *slider,
                          const QWidget *widget) const;
    void drawMenuOrToolBarBackground(const QWidget *widget, QPainter *p,
                                     const QRect &r, const QStyleOption *option,
                                     bool menu=true, bool horiz=true) const;
    void drawHandleMarkers(QPainter *p, const QRect &r,
                           const QStyleOption *option, bool tb,
                           ELine handles) const;
    void fillTab(QPainter *p, const QRect &r, const QStyleOption *option,
                 const QColor &fill, bool horiz, EWidget tab,
                 bool tabOnly) const;
    void colorTab(QPainter *p, const QRect &r, bool horiz,
                  EWidget tab, int round) const;
    void shadeColors(const QColor &base, QColor *vals) const;
    const QColor *buttonColors(const QStyleOption *option) const;
    QColor titlebarIconColor(const QStyleOption *option) const;
    const QColor *popupMenuCols(const QStyleOption *option=0L) const;
    const QColor *checkRadioColors(const QStyleOption *option) const;
    const QColor *sliderColors(const QStyleOption *option) const;
    const QColor *backgroundColors(const QColor &col) const;
    const QColor*
    backgroundColors(const QStyleOption *option) const
    {
        return (option ?
                backgroundColors(option->palette.background().color()) :
                m_backgroundCols);
    }
    const QColor *highlightColors(const QColor &col) const;
    const QColor*
    highlightColors(const QStyleOption *option, bool useActive) const
    {
        return highlightColors(
            option->palette.brush(useActive ? QPalette::Active :
                                  QPalette::Current,
                                  QPalette::Highlight).color());
    }
    const QColor *borderColors(const QStyleOption *option,
                               const QColor *use) const;
    const QColor *getSidebarButtons() const;
    void setMenuColors(const QColor &bgnd);
    void setMenuTextColors(QWidget *widget, bool isMenuBar) const;
    const QColor *menuColors(const QStyleOption *option, bool active) const;
    bool coloredMdiButtons(bool active, bool mouseOver) const;
    const QColor *getMdiColors(const QStyleOption *option, bool active) const;
    void readMdiPositions() const;
    const QColor &getFill(const QStyleOption *option, const QColor *use,
                          bool cr=false, bool darker=false) const;
    const QColor &getTabFill(bool current, bool highlight,
                             const QColor *use) const;
    QColor menuStripeCol() const;
    QPixmap *getPixmap(const QColor col, EPixmap p, double shade=1.0) const;
    const QColor &checkRadioCol(const QStyleOption *opt) const;
    QColor shade(const QColor &a, double k) const;
    void shade(const QColor &ca, QColor *cb, double k) const;
    QColor getLowerEtchCol(const QWidget *widget) const;
    int getFrameRound(const QWidget *widget) const;

private Q_SLOTS:
    void kdeGlobalSettingsChange(int type, int);
    void borderSizesChanged();
    void toggleMenuBar(unsigned int xid);
    void toggleStatusBar(unsigned int xid);
    void compositingToggled();

private:
    void widgetDestroyed(QObject *o);
    void toggleMenuBar(QMainWindow *window);
    void toggleStatusBar(QMainWindow *window);

#ifdef QTC_QT5_ENABLE_KDE
    void setupKde4();
    void setDecorationColors();
    void applyKdeSettings(bool pal);
#endif
    bool isWindowDragWidget(QObject *o);
    void emitMenuSize(QWidget *w, unsigned short size, bool force=false);
    void emitStatusBarState(QStatusBar *sb);
    const QColor&
    MOArrow(QFlags<State> state, const QPalette &palette, bool mo,
            QPalette::ColorRole rol) const
    {
        if (!(state & State_Enabled)) {
            return palette.color(QPalette::Disabled, rol);
        } else if (opts.coloredMouseOver != MO_NONE && mo) {
            return m_mouseOverCols[ARROW_MO_SHADE];
        } else {
            return palette.color(rol);
        }
    }
    const QColor&
    MOArrow(QFlags<State> state, const QPalette &palette,
            QPalette::ColorRole rol) const
    {
        return MOArrow(state, palette, state & State_MouseOver, rol);
    }

    bool
    drawPrimitiveNone(PrimitiveElement, const QStyleOption*,
                      QPainter*, const QWidget*) const
    {
        return true;
    }
    bool drawPrimitivePanelMenu(PrimitiveElement element,
                                const QStyleOption *option,
                                QPainter *painter,
                                const QWidget *widget) const;
    bool drawPrimitiveIndicatorTabClose(PrimitiveElement element,
                                        const QStyleOption *option,
                                        QPainter *painter,
                                        const QWidget *widget) const;
    bool drawPrimitiveWidget(PrimitiveElement element,
                             const QStyleOption *option,
                             QPainter *painter,
                             const QWidget *widget) const;
    bool drawPrimitivePanelScrollAreaCorner(PrimitiveElement element,
                                            const QStyleOption *option,
                                            QPainter *painter,
                                            const QWidget *widget) const;
    bool drawPrimitiveIndicatorBranch(PrimitiveElement element,
                                      const QStyleOption *option,
                                      QPainter *painter,
                                      const QWidget *widget) const;
    bool drawPrimitiveIndicatorViewItemCheck(PrimitiveElement element,
                                             const QStyleOption *option,
                                             QPainter *painter,
                                             const QWidget *widget) const;
    bool drawPrimitiveIndicatorHeaderArrow(PrimitiveElement element,
                                           const QStyleOption *option,
                                           QPainter *painter,
                                           const QWidget *widget) const;
    bool drawPrimitiveIndicatorArrow(PrimitiveElement element,
                                     const QStyleOption *option,
                                     QPainter *painter,
                                     const QWidget *widget) const;
    bool drawPrimitiveIndicatorSpin(PrimitiveElement element,
                                    const QStyleOption *option,
                                    QPainter *painter,
                                    const QWidget *widget) const;
    bool drawPrimitiveIndicatorToolBarSeparator(PrimitiveElement element,
                                                const QStyleOption *option,
                                                QPainter *painter,
                                                const QWidget *widget) const;
    bool drawPrimitiveFrameGroupBox(PrimitiveElement element,
                                    const QStyleOption *option,
                                    QPainter *painter,
                                    const QWidget *widget) const;
    bool drawPrimitiveFrame(PrimitiveElement element,
                            const QStyleOption *option, QPainter *painter,
                            const QWidget *widget) const;
    bool drawPrimitivePanelMenuBar(PrimitiveElement element,
                                   const QStyleOption *option,
                                   QPainter *painter,
                                   const QWidget *widget) const;
    bool drawPrimitivePanelTipLabel(PrimitiveElement element,
                                    const QStyleOption *option,
                                    QPainter *painter,
                                    const QWidget *widget) const;
    bool drawPrimitiveQtcBackground(PrimitiveElement element,
                                    const QStyleOption *option,
                                    QPainter *painter,
                                    const QWidget *widget) const;
    bool drawPrimitivePanelItemViewItem(PrimitiveElement element,
                                        const QStyleOption *option,
                                        QPainter *painter,
                                        const QWidget *widget) const;
    bool drawPrimitiveFrameTabWidget(PrimitiveElement element,
                                     const QStyleOption *option,
                                     QPainter *painter,
                                     const QWidget *widget) const;
    bool drawPrimitiveFrameWindow(PrimitiveElement element,
                                  const QStyleOption *option,
                                  QPainter *painter,
                                  const QWidget *widget) const;
    bool drawPrimitiveButton(PrimitiveElement element,
                             const QStyleOption *option, QPainter *painter,
                             const QWidget *widget) const;
    bool drawPrimitiveFrameFocusRect(PrimitiveElement element,
                                     const QStyleOption *option,
                                     QPainter *painter,
                                     const QWidget *widget) const;
    bool drawPrimitiveIndicatorToolBarHandle(PrimitiveElement element,
                                             const QStyleOption *option,
                                             QPainter *painter,
                                             const QWidget *widget) const;
    bool drawPrimitiveIndicatorRadioButton(PrimitiveElement element,
                                           const QStyleOption *option,
                                           QPainter *painter,
                                           const QWidget *widget) const;
    bool drawPrimitiveIndicatorCheckBox(PrimitiveElement element,
                                        const QStyleOption *option,
                                        QPainter *painter,
                                        const QWidget *widget) const;
    bool drawPrimitiveFrameLineEdit(PrimitiveElement element,
                                    const QStyleOption *option,
                                    QPainter *painter,
                                    const QWidget *widget) const;
    bool drawPrimitivePanelLineEdit(PrimitiveElement element,
                                    const QStyleOption *option,
                                    QPainter *painter,
                                    const QWidget *widget) const;
    bool drawPrimitiveIndicatorDockWidgetResizeHandle(
        PrimitiveElement element, const QStyleOption *option,
        QPainter *painter, const QWidget *widget) const;
    bool drawPrimitiveButtonTool(PrimitiveElement element,
                                 const QStyleOption *option, QPainter *painter,
                                 const QWidget *widget) const;
    bool drawPrimitiveFrameDockWidget(PrimitiveElement element,
                                      const QStyleOption *option,
                                      QPainter *painter,
                                      const QWidget *widget) const;
    bool drawPrimitiveFrameStatusBarOrMenu(PrimitiveElement element,
                                           const QStyleOption *option,
                                           QPainter *painter,
                                           const QWidget *widget) const;
    bool drawPrimitiveFrameTabBarBase(PrimitiveElement element,
                                      const QStyleOption *option,
                                      QPainter *painter,
                                      const QWidget *widget) const;

private:
    mutable Options opts;
    QColor m_highlightCols[TOTAL_SHADES + 1],
        m_backgroundCols[TOTAL_SHADES + 1],
        m_menubarCols[TOTAL_SHADES + 1],
        m_focusCols[TOTAL_SHADES + 1],
        m_mouseOverCols[TOTAL_SHADES + 1],
        *m_popupMenuCols,
        *m_sliderCols,
        *m_defBtnCols,
        *m_comboBtnCols,
        *m_checkRadioSelCols,
        *m_sortedLvColors,
        *m_ooMenuCols,
        *m_progressCols,
        m_buttonCols[TOTAL_SHADES + 1],
        m_checkRadioCol;
    bool m_saveMenuBarStatus,
        m_saveStatusBarStatus,
        m_usePixmapCache,
        m_inactiveChangeSelectionColor;
    PreviewType m_isPreview;
    mutable QColor *m_sidebarButtonsCols;
    mutable QColor *m_activeMdiColors;
    mutable QColor *m_mdiColors;
    mutable QColor m_activeMdiTextColor;
    mutable QColor m_mdiTextColor;
    mutable QColor m_coloredButtonCols[TOTAL_SHADES + 1];
    mutable QColor m_coloredBackgroundCols[TOTAL_SHADES + 1];
    mutable QColor m_coloredHighlightCols[TOTAL_SHADES + 1];
    mutable QCache<QtcKey, QPixmap> m_pixmapCache;
    mutable bool m_active;
    mutable const QWidget *m_sbWidget;
    mutable QLabel *m_clickedLabel;
    QSet<QProgressBar*> m_progressBars;
    int m_progressBarAnimateTimer,
        m_animateStep;
    QTime m_timer;
    mutable QMap<int, QColor*> m_titleBarButtonsCols;
    mutable QList<int> m_mdiButtons[2]; // 0=left, 1=right
    mutable int m_titlebarHeight;

    QDBusInterface *m_dBus;
    ShadowHelper *m_shadowHelper;
    mutable QScrollBar *m_sViewSBar;
    mutable QMap<QWidget*, QSet<QWidget*> > m_sViewContainers;
#ifdef QTC_QT5_ENABLE_KDE
    KComponentData m_componentData;
#endif
    WindowManager *m_windowManager;
    BlurHelper *m_blurHelper;
    ShortcutHandler *m_shortcutHandler;
};
}

#endif
