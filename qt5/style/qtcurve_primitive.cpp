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
#include <qtcurve-utils/qtprops.h>

#include <QMdiSubWindow>
#include <QTreeView>
#include <QSpinBox>
#include <QComboBox>
#include <QMainWindow>
#include <QListView>
#include <QPixmapCache>
#include <QDockWidget>
#include <QGroupBox>
#include <QDial>
#include <QCheckBox>
#include <QRadioButton>
#include <QToolBar>
#include <QMenuBar>

#include "shadowhelper.h"
#include "utils.h"
#include <common/config_file.h>

#ifdef QTC_QT5_ENABLE_KDE
#include <KDE/KIcon>
#include <KDE/KTitleWidget>
#endif

namespace QtCurve {

static inline void setPainterPen(QPainter *p, const QColor &col, const qreal width=1.0)
{
    p->setPen(QPen(col, width));
}

bool
Style::drawPrimitiveIndicatorTabClose(PrimitiveElement,
                                      const QStyleOption*option,
                                      QPainter *painter,
                                      const QWidget*) const
{
#ifdef QTC_QT5_ENABLE_KDE
    int size = pixelMetric(QStyle::PM_SmallIconSize);
    State state = option->state;
    QIcon::Mode mode = (state & State_Enabled ? state & State_Raised ?
                        QIcon::Active : QIcon::Normal : QIcon::Disabled);
    if (!(state & State_Raised) && !(state & State_Sunken) &&
        !(state & State_Selected)) {
        mode = QIcon::Disabled;
    }
    drawItemPixmap(painter, option->rect, Qt::AlignCenter,
                   KIcon("dialog-close").pixmap(size, mode,
                                                state & State_Sunken ?
                                                QIcon::On : QIcon::Off));
    return true;
#else
    QTC_UNUSED(option);
    QTC_UNUSED(painter);
    return false;
#endif
}

bool
Style::drawPrimitiveWidget(PrimitiveElement,
                           const QStyleOption *option,
                           QPainter *painter,
                           const QWidget *widget) const
{
    bool isDialog = qtcIsDialog(widget);
    bool isSubWindow = (widget && qobject_cast<const QMdiSubWindow*>(widget));
    if (widget && (widget->testAttribute(Qt::WA_NoSystemBackground) ||
                   !widget->testAttribute(Qt::WA_StyledBackground) ||
                   !(isDialog || qtcIsWindow(widget) || isSubWindow))) {
        // A few widgets (e.g. QDesignerWidget and Gammaray::OverlayWidget)
        // calls this function but we don't want to draw background for them.
        // Filter the ones we don't want to draw here.
        // TODO?: QtQuick2
        return true;
    }
    // TODO handle QtQuick2 better once there is update from upstream
    // The following is copied from Style::drawBackground in order to handle
    // widget == NULL, it should probably be merged back at some point.
    if (!isSubWindow) {
        painter->setCompositionMode(QPainter::CompositionMode_Source);
    }
    bool previewMdi = m_isPreview && isSubWindow;
    const QWidget *window = widget;
    if (!m_isPreview && widget) {
        window = widget->window();
    }
    int opacity = isDialog ? opts.dlgOpacity : opts.bgndOpacity;
    // Check if we are drawing on a 32-bits window, Note that we don't need
    // this check for QMdiSubWindow since we don't draw through the window.
    if (opacity != 100 &&
        !(isSubWindow || (widget && Utils::hasAlphaChannel(window)))) {
        opacity = 100;
    }
    if (widget) {
        QtcQWidgetProps(widget)->opacity = opacity;
    }
    QRect bgndRect = option->rect;
    painter->setClipRegion(option->rect, Qt::IntersectClip);

    if (!previewMdi) {
        WindowBorders borders = qtcGetWindowBorderSize(false);
        bgndRect.adjust(-borders.sides, -borders.titleHeight,
                        borders.sides, borders.bottom);
    } else {
        bgndRect.adjust(0, -pixelMetric(PM_TitleBarHeight, 0, widget), 0, 0);
    }
    drawBackground(painter, option->palette.window().color(), bgndRect,
                   opacity, isDialog ? BGND_DIALOG : BGND_WINDOW,
                   opts.bgndAppearance);
    // FIXME, workaround only, the non transparent part of the image will have
    // a different overall opacity.
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    drawBackgroundImage(painter, true, opts.bgndImage.type == IMG_FILE &&
                        opts.bgndImage.onBorder ? bgndRect : option->rect);
    return true;
}

bool
Style::drawPrimitivePanelScrollAreaCorner(PrimitiveElement,
                                          const QStyleOption *option,
                                          QPainter *painter,
                                          const QWidget *widget) const
{
    // disable painting of PE_PanelScrollAreaCorner
    // the default implementation fills the rect with the window background
    // color which does not work for windows that have gradients.
    // ...but need to for WebView!!!
    if (!opts.gtkScrollViews || !qtcIsCustomBgnd(opts) ||
        (widget && widget->inherits("WebView"))) {
        painter->fillRect(option->rect,
                          option->palette.brush(QPalette::Window));
    }
    return true;
}

bool
Style::drawPrimitiveIndicatorBranch(PrimitiveElement,
                                    const QStyleOption *option,
                                    QPainter *painter,
                                    const QWidget *widget) const
{
    bool reverse = option->direction == Qt::RightToLeft;
    const QRect &r = option->rect;
    const QPalette &palette = option->palette;
    State state = option->state;
    int middleH = (r.x() + r.width() / 2) - 1;
    int middleV = r.y() + r.height() / 2;
    int beforeV = middleV;
    int afterH = middleH;
#if 0
    int beforeH = middleH;
#endif
    int afterV = middleV;
    if (state & State_Children) {
        QRect ar(r.x() + (r.width() - LV_SIZE - 4) / 2,
                 r.y() + (r.height() - LV_SIZE - 4) / 2,
                 LV_SIZE + 4, LV_SIZE + 4);
        if (opts.lvLines) {
            beforeV = ar.y() - 1;
            afterH = ar.x() + LV_SIZE + 4;
            afterV = ar.y() + LV_SIZE + 4;
#if 0
            beforeH = ar.x();
            int lo = opts.round != ROUND_NONE ? 2 : 0;

            painter->setPen(palette.mid().color());
            painter->drawLine(ar.x() + lo, ar.y(), ar.x() + ar.width() - 1 - lo,
                              ar.y());
            painter->drawLine(ar.x() + lo, ar.y() + ar.height() - 1,
                              ar.x() + ar.width() - 1 - lo,
                              ar.y() + ar.height() - 1);
            painter->drawLine(ar.x(), ar.y() + lo, ar.x(),
                              ar.y() + ar.height() - 1 - lo);
            painter->drawLine(ar.x() + ar.width() - 1, ar.y() + lo,
                              ar.x() + ar.width() - 1,
                              ar.y() + ar.height() - 1 - lo);
            if (opts.round != ROUND_NONE) {
                painter->drawPoint(ar.x() + 1, ar.y() + 1);
                painter->drawPoint(ar.x() + 1, ar.y() + ar.height() - 2);
                painter->drawPoint(ar.x() + ar.width() - 2, ar.y() + 1);
                painter->drawPoint(ar.x() + ar.width() - 2,
                                   ar.y() + ar.height() - 2);

                QColor col(palette.mid().color());
                col.setAlphaF(0.5);
                painter->setPen(col);
                painter->drawLine(ar.x() + 1, ar.y() + 1, ar.x() + 2, ar.y());
                painter->drawLine(ar.x() + ar.width() - 2, ar.y(),
                                  ar.x() + ar.width() - 1, ar.y() + 1);
                painter->drawLine(ar.x() + 1, ar.y() + ar.height() - 2,
                                  ar.x() + 2, ar.y() + ar.height() - 1);
                painter->drawLine(ar.x() + ar.width() - 2,
                                  ar.y() + ar.height() - 1,
                                  ar.x() + ar.width() - 1,
                                  ar.y() + ar.height() - 2);
            }
#endif
        }
        drawArrow(painter, ar, state & State_Open ? PE_IndicatorArrowDown :
                  reverse ? PE_IndicatorArrowLeft :
                  PE_IndicatorArrowRight,
                  MOArrow(state, palette, QPalette::ButtonText));
    }
    const int constStep = (opts.lvLines ? 0 :
                           widget && qobject_cast<const QTreeView*>(widget) ?
                           ((QTreeView*)widget)->indentation() : 20);

    if (opts.lvLines) {
        painter->setPen(palette.mid().color());
        if (state & State_Item) {
            if (reverse) {
                painter->drawLine(r.left(), middleV, afterH, middleV);
            } else {
#if 0
                if (opts.lvLines == LV_NEW) {
                    if (state & State_Children) {
                        painter->drawLine(middleH - constStep, middleV,
                                          r.right() - constStep, middleV);
                    } else {
                        drawFadedLine(painter,
                                      QRect(middleH - constStep, middleV,
                                            r.right() - (middleH - constStep),
                                            middleV), palette.mid().color(),
                                      false, true, true);
                    }
                } else {
#endif
                    painter->drawLine(afterH, middleV, r.right(), middleV);
#if 0
                }
#endif
            }
        }
        if (state & State_Sibling && afterV < r.bottom()) {
            painter->drawLine(middleH - constStep, afterV,
                              middleH - constStep, r.bottom());
        }
        if (state & (State_Open | State_Children | State_Item |
                     State_Sibling) && beforeV > r.y()) {
            painter->drawLine(middleH - constStep, r.y(),
                              middleH - constStep, beforeV);
        }
    }
    return true;
}

bool
Style::drawPrimitiveIndicatorViewItemCheck(PrimitiveElement,
                                           const QStyleOption *option,
                                           QPainter *painter,
                                           const QWidget *widget) const
{
    QStyleOption opt = *option;
    opt.state &= ~State_MouseOver;
    opt.state |= STATE_VIEW;
    drawPrimitive(PE_IndicatorCheckBox, &opt, painter, widget);
    return true;
}

bool
Style::drawPrimitiveIndicatorHeaderArrow(PrimitiveElement,
                                         const QStyleOption *option,
                                         QPainter *painter,
                                         const QWidget*) const
{
    State state = option->state;
    const QPalette &palette = option->palette;
    if (auto header = styleOptCast<QStyleOptionHeader>(option)) {
        drawArrow(painter, option->rect,
                  header->sortIndicator & QStyleOptionHeader::SortUp ?
                  PE_IndicatorArrowUp : PE_IndicatorArrowDown,
                  MOArrow(state, palette, QPalette::ButtonText));
    }
    return true;
}

bool
Style::drawPrimitiveIndicatorArrow(PrimitiveElement element,
                                   const QStyleOption *option,
                                   QPainter *painter,
                                   const QWidget *widget) const
{
    State state = option->state;
    const QPalette &palette = option->palette;
    if (state == State_None) {
        state |= State_Enabled;
    }
    if (state == (State_Enabled | QtC_StateKWin)) {
        drawArrow(painter, option->rect, element, Qt::color1, false, true);
    } else {
        QRect r = option->rect;
        QColor col = MOArrow(state, palette, QPalette::Text);
        if (state & (State_Sunken | State_On) &&
            !(widget &&
              ((opts.unifySpin && qobject_cast<const QSpinBox*>(widget)) ||
               (opts.unifyCombo && qobject_cast<const QComboBox*>(widget) &&
                static_cast<const QComboBox*>(widget)->isEditable())))) {
            r.adjust(1, 1, 1, 1);
        }
        if (col.alpha() < 255 && element == PE_IndicatorArrowRight &&
            widget && widget->inherits("KUrlButton")) {
            col = blendColors(col, palette.background().color(), col.alphaF());
        }
        drawArrow(painter, r, element, col, false, false);
    }
    return true;
}

bool
Style::drawPrimitiveIndicatorSpin(PrimitiveElement element,
                                  const QStyleOption *option,
                                  QPainter *painter,
                                  const QWidget *widget) const
{
    const QRect &r = option->rect;
    const QPalette &palette = option->palette;
    State state = option->state;
    QRect sr = r;
    const QColor *use = buttonColors(option);
    const QColor &col = MOArrow(state, palette, QPalette::ButtonText);
    bool down = oneOf(element, PE_IndicatorSpinDown, PE_IndicatorSpinMinus);
    bool reverse = option->direction == Qt::RightToLeft;

    if ((!opts.unifySpinBtns || state & State_Sunken) && !opts.unifySpin) {
        drawLightBevel(painter, sr, option, widget, down ? reverse ?
                       ROUNDED_BOTTOMLEFT : ROUNDED_BOTTOMRIGHT : reverse ?
                       ROUNDED_TOPLEFT : ROUNDED_TOPRIGHT,
                       getFill(option, use), use, true, WIDGET_SPIN);
    }
    if (oneOf(element, PE_IndicatorSpinUp, PE_IndicatorSpinDown)) {
        sr.setY(sr.y() + (down ? -2 : 1));
        if (opts.unifySpin) {
            sr.adjust(reverse ? 1 : -1, 0, reverse ? 1 : -1, 0);
            if (!opts.vArrows) {
                sr.setY(sr.y() + (down ? -2 : 2));
            }
        } else if (state & State_Sunken) {
            sr.adjust(1, 1, 1, 1);
        }
        drawArrow(painter, sr, element == PE_IndicatorSpinUp ?
                  PE_IndicatorArrowUp : PE_IndicatorArrowDown,
                  col, !opts.unifySpin);
    } else {
        int l = qMin(r.width() - 6, r.height() - 6);
        QPoint c(r.x() + r.width() / 2, r.y() + r.height() / 2);
        l /= 2;
        if (l % 2 != 0) {
            --l;
        }
        if (state & State_Sunken && !opts.unifySpin) {
            c += QPoint(1, 1);
        }
        painter->setPen(col);
        painter->drawLine(c.x() - l, c.y(), c.x() + l, c.y());
        if (!down) {
            painter->drawLine(c.x(), c.y() - l, c.x(), c.y() + l);
        }
    }
    return true;
}

bool
Style::drawPrimitiveIndicatorToolBarSeparator(PrimitiveElement,
                                              const QStyleOption *option,
                                              QPainter *painter,
                                              const QWidget*) const
{
    const QRect &r = option->rect;
    State state = option->state;
    switch (opts.toolbarSeparators) {
    case LINE_NONE:
        break;
    case LINE_FLAT:
    case LINE_SUNKEN:
        if (r.width() < r.height()) {
            int x = r.x() + (r.width() - 2) / 2;
            drawFadedLine(painter, QRect(x, r.y() + TOOLBAR_SEP_GAP, 1,
                                         r.height() - TOOLBAR_SEP_GAP * 2),
                          m_backgroundCols[opts.toolbarSeparators ==
                                            LINE_SUNKEN ? 3 : 4],
                          true, true, false);

            if (opts.toolbarSeparators == LINE_SUNKEN) {
                drawFadedLine(painter, QRect(x + 1, r.y() + 6, 1,
                                             r.height() - 12),
                              m_backgroundCols[0], true, true, false);
            }
        } else {
            int y = r.y() + (r.height() - 2) / 2;
            drawFadedLine(painter, QRect(r.x() + TOOLBAR_SEP_GAP, y,
                                         r.width() - TOOLBAR_SEP_GAP * 2, 1),
                          m_backgroundCols[opts.toolbarSeparators ==
                                            LINE_SUNKEN ? 3 : 4],
                          true, true, true);
            if (opts.toolbarSeparators == LINE_SUNKEN) {
                drawFadedLine(painter,
                              QRect(r.x() + TOOLBAR_SEP_GAP, y + 1,
                                    r.width() - TOOLBAR_SEP_GAP * 2, 1),
                              m_backgroundCols[0], true, true, true);
            }
        }
        break;
    default:
    case LINE_DOTS:
        drawDots(painter, r, !(state & State_Horizontal), 1, 5,
                 m_backgroundCols, 0, 5);
    }
    return true;
}

bool
Style::drawPrimitiveFrameGroupBox(PrimitiveElement,
                                  const QStyleOption *option,
                                  QPainter *painter,
                                  const QWidget *widget) const
{
    if (opts.groupBox == FRAME_NONE) {
        return true;
    }
    if (auto _frame = styleOptCast<QStyleOptionFrame>(option)) {
        bool reverse = option->direction == Qt::RightToLeft;
        QStyleOptionFrame frame(*_frame);
        if (frame.features & QStyleOptionFrame::Flat ||
            opts.groupBox == FRAME_LINE) {
            const QRect &r = option->rect;
            drawFadedLine(painter, QRect(r.x(), r.y(), r.width(), 1),
                          backgroundColors(option)[QTC_STD_BORDER],
                          opts.gbLabel & GB_LBL_CENTRED || reverse,
                          opts.gbLabel & GB_LBL_CENTRED || !reverse, true);
        } else {
            QRect r = option->rect;
            if (opts.gbLabel & GB_LBL_OUTSIDE) {
                r.adjust(0, 2, 0, 0);
            }
            if (oneOf(opts.groupBox, FRAME_SHADED, FRAME_FADED)) {
                int round = (opts.square & SQUARE_FRAME ?
                             ROUNDED_NONE : ROUNDED_ALL);
                QPainterPath path =
                    buildPath(r, WIDGET_FRAME, round,
                              round == ROUNDED_ALL ?
                              qtcGetRadius(&opts, r.width(), r.height(),
                                           WIDGET_FRAME,
                                           RADIUS_EXTERNAL) : 0.0);
                painter->save();
                painter->setClipping(false);
                if (opts.gbFactor != 0) {
                    QColor col = opts.gbFactor < 0 ? Qt::black : Qt::white;

                    col.setAlphaF(TO_ALPHA(opts.gbFactor));
                    if (opts.groupBox == FRAME_SHADED) {
                        painter->fillPath(path, col);
                    } else {
                        QLinearGradient grad(r.topLeft(), r.bottomLeft());
                        grad.setColorAt(0, col);
                        col.setAlphaF(0.0);
                        grad.setColorAt(1, col);
                        painter->fillPath(path, grad);
                    }
                }

                if (!(opts.gbLabel & (GB_LBL_INSIDE | GB_LBL_OUTSIDE))) {
                    painter->restore();
                }
                if (opts.groupBox == FRAME_SHADED) {
                    drawBorder(painter, r, option, round,
                               backgroundColors(option), WIDGET_FRAME,
                               /* state & State_Raised && opts.gbFactor < 0 ?
                                  BORDER_RAISED : */BORDER_SUNKEN);
                } else {
                    QColor col = backgroundColors(option)[QTC_STD_BORDER];
                    QLinearGradient grad(r.topLeft(), r.bottomLeft());
                    col.setAlphaF(1.0);
                    grad.setColorAt(0, col);
                    col.setAlphaF(0.0);
                    grad.setColorAt(1, col);
                    painter->setRenderHint(QPainter::Antialiasing, true);
                    painter->setPen(QPen(QBrush(grad), QPENWIDTH1));
                    painter->drawPath(path);
                }
                if (opts.gbLabel & (GB_LBL_INSIDE | GB_LBL_OUTSIDE)) {
                    painter->restore();
                }
            } else {
                frame.state &= ~(State_Sunken | State_HasFocus);
                frame.rect = r;
                drawPrimitive(PE_Frame, &frame, painter, widget);
            }
        }
    }
    return true;
}

bool
Style::drawPrimitiveFrame(PrimitiveElement,
                          const QStyleOption *option, QPainter *painter,
                          const QWidget *widget) const
{
    QRect r = option->rect;
    const QPalette &palette(option->palette);
    State state = option->state;

    // Dont draw OO.o status bar frames...
    if (isOOWidget(widget) && r.height() < 22) {
        return true;
    }
    if (widget && qtcCheckKDEType(widget->parent(), KTitleWidget)) {
        return true;
    } else if (widget && qtcCheckType<QComboBox>(widget->parent())) {
        if (opts.gtkComboMenus &&
            !((QComboBox*)(widget->parent()))->isEditable()) {
            drawPrimitive(PE_FrameMenu, option, painter, widget);
        } else if (opts.square & SQUARE_POPUP_MENUS) {
            const QColor *use = (theThemedApp == APP_KRUNNER ?
                                 m_backgroundCols : backgroundColors(option));
            painter->setPen(use[QTC_STD_BORDER]);
            drawRect(painter, r);
            painter->setPen(palette.base().color());
            drawRect(painter, r.adjusted(1, 1, -1, -1));
        }
    } else {
        auto fo = styleOptCast<QStyleOptionFrame>(option);
        if (theThemedApp == APP_K3B &&
            !(state & (State_Sunken | State_Raised)) &&
            fo && fo->lineWidth == 1) {
            painter->setPen(backgroundColors(option)[QTC_STD_BORDER]);
            drawRect(painter, r);
        } else if (oneOf(state, QtC_StateKWin,
                         QtC_StateKWin | State_Active) && fo &&
                   fo->lineWidth == 1 && fo->midLineWidth == 1) {
            QColor border;
            if (fo->version == TBAR_BORDER_VERSION_HACK + 2) {
                border = palette.color(QPalette::Active, QPalette::Shadow);
            } else {
                const QColor *borderCols =
                    (opts.windowBorder & WINDOW_BORDER_COLOR_TITLEBAR_ONLY ?
                     backgroundColors(palette.color(QPalette::Active,
                                                    QPalette::Window)) :
                     theThemedApp == APP_KWIN ? buttonColors(option) :
                     getMdiColors(option, state & State_Active));
                border = borderCols[fo->version == TBAR_BORDER_VERSION_HACK ?
                                    0 : QTC_STD_BORDER];
            }
            border.setAlphaF(1.0);
            QPAINTER_RENDERHIT_AA_MAYBE_OFF(painter);
            painter->setPen(border);
            drawRect(painter, r);
        } else {
            bool kateView = isKateView(widget);
            bool kontactPreview = !kateView && isKontactPreviewPane(widget);
            bool sv = (isOOWidget(widget) ||
                       qobject_cast<const QAbstractScrollArea*>(widget) ||
                       ((opts.square & SQUARE_SCROLLVIEW) &&
                        (kateView || kontactPreview)));
            bool squareSv = (sv && ((opts.square & SQUARE_SCROLLVIEW) ||
                                    (widget && widget->isWindow())));
            bool inQAbstractItemView =
                (widget && widget->parentWidget() &&
                 isInQAbstractItemView(widget->parentWidget()));

            if (sv && (opts.etchEntry || squareSv || isOOWidget(widget))) {
                QtcQWidgetProps props(widget);
                // For some reason, in KPackageKit, the KTextBrower when
                // polished is not in the scrollview, but is when painted.
                // So check here if it should not be etched.
                // Also, see not in getLowerEtchCol()
                if (opts.buttonEffect != EFFECT_NONE &&
                    !USE_CUSTOM_ALPHAS(opts) && widget &&
                    widget->parentWidget() && !props->noEtch &&
                    inQAbstractItemView) {
                    props->noEtch = true;
                }
                // If we are set to have sunken scrollviews, then the frame
                // width is set to 3. ...but it we are a scrollview within
                // a scrollview, then we dont draw sunken, therefore
                // need to draw inner border...
                bool doEtch = (opts.buttonEffect != EFFECT_NONE &&
                               opts.etchEntry);
                bool noEtchW = (doEtch && !USE_CUSTOM_ALPHAS(opts) &&
                                props->noEtch);
                if (doEtch && noEtchW) {
                    painter->setPen(palette.brush(QPalette::Base).color());
                    drawRect(painter, r.adjusted(2, 2, -2, -2));
                }
                if (!opts.highlightScrollViews && fo) {
                    QStyleOptionFrame opt(*fo);
                    opt.state &= ~State_HasFocus;
                    drawEntryField(painter, r, widget, &opt,
                                   squareSv ? ROUNDED_NONE : ROUNDED_ALL,
                                   false, doEtch && !noEtchW,
                                   WIDGET_SCROLLVIEW);
                } else {
                    drawEntryField(painter, r, widget, option,
                                   squareSv ? ROUNDED_NONE : ROUNDED_ALL, false,
                                   doEtch && !noEtchW, WIDGET_SCROLLVIEW);
                }
            }
            // K3b's Disk usage status bar, etc...
            // else if (APP_K3B == theThemedApp && widget &&
            //          widget->inherits("K3b::FillStatusDisplay"))
            else if (fo && fo->lineWidth > 0) {
                bool kwinTab = (theThemedApp == APP_KWIN && widget &&
                                !widget->parentWidget() &&
                                !strcmp(widget->metaObject()->className(),
                                        "KWin::TabBox"));
                QStyleOption opt = *option;
                if (kwinTab) {
                    r.adjust(-1, -1, 1, 1);
                }
                if (!opts.highlightScrollViews) {
                    opt.state &= ~State_HasFocus;
                }
                if (opts.round && qtcIsFlatBgnd(opts.bgndAppearance) &&
                    opts.bgndOpacity == 100 && widget &&
                    widget->parentWidget() && !inQAbstractItemView // &&
                    // widget->palette().background().color() !=
                    // widget->parentWidget()->palette().background().color()
                    ) {
                    painter->setPen(widget->parentWidget()->palette()
                                    .background().color());
                    painter->drawRect(r);
                    painter->drawRect(r.adjusted(1, 1, -1, -1));
                }
                if (sv || kateView || kontactPreview) {
                    painter->setRenderHint(QPainter::Antialiasing, true);
                    painter->setPen(
                        option->palette.brush(
                            opts.thin & THIN_FRAMES &&
                            !(opts.square & SQUARE_SCROLLVIEW) ?
                            QPalette::Window : QPalette::Base).color());
                    painter->drawPath(
                        buildPath(r.adjusted(1, 1, -1, -1),
                                  WIDGET_SCROLLVIEW, ROUNDED_ALL,
                                  qtcGetRadius(&opts, r.width() - 2,
                                               r.height() - 2,
                                               WIDGET_SCROLLVIEW,
                                               RADIUS_INTERNAL)));
                    QPAINTER_RENDERHIT_AA_MAYBE_OFF(painter);
                }
                drawBorder(painter, r, &opt,
                           opts.round ? getFrameRound(widget) : ROUND_NONE,
                           backgroundColors(option),
                           sv || kateView || kontactPreview ?
                           WIDGET_SCROLLVIEW : WIDGET_FRAME,
                           state & State_Sunken || state & State_HasFocus ?
                           BORDER_SUNKEN : state & State_Raised ? BORDER_RAISED :
                           BORDER_FLAT);
            }
        }
    }
    return true;
}

bool
Style::drawPrimitivePanelMenuBar(PrimitiveElement,
                                 const QStyleOption *option,
                                 QPainter *painter,
                                 const QWidget *widget) const
{
    const QRect &r = option->rect;
    if (widget && widget->parentWidget() &&
        qobject_cast<const QMainWindow*>(widget->parentWidget())) {
        drawMenuOrToolBarBackground(widget, painter, r, option);
        if (opts.toolbarBorders != TB_NONE) {
            const QColor *use = (m_active ? m_menubarCols :
                                 backgroundColors(option));
            bool dark = oneOf(opts.toolbarBorders, TB_DARK, TB_DARK_ALL);
            if (oneOf(opts.toolbarBorders, TB_DARK_ALL, TB_LIGHT_ALL)) {
                painter->setPen(use[0]);
                painter->drawLine(r.x(), r.y(), r.x() + r.width() - 1, r.y());
                painter->drawLine(r.x(), r.y(), r.x(), r.y() + r.height() - 1);
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
    }
    return true;
}

bool
Style::drawPrimitivePanelTipLabel(PrimitiveElement,
                                  const QStyleOption *option,
                                  QPainter *painter,
                                  const QWidget *widget) const
{
    const QRect &r = option->rect;
    const QPalette &palette(option->palette);
    bool haveAlpha = Utils::hasAlphaChannel(widget);
    bool rounded = !(opts.square & SQUARE_TOOLTIPS);
    QPainterPath path =
        (rounded ? buildPath(QRectF(r), WIDGET_OTHER, ROUNDED_ALL,
                             opts.round >= ROUND_FULL ? 5.0 : 2.5) :
         QPainterPath());
    QColor col = palette.toolTipBase().color();

    if (widget && widget->window()) {
        m_shadowHelper->registerWidget(widget->window());
    }
    if (rounded) {
        painter->setRenderHint(QPainter::Antialiasing, true);
    }
    if (haveAlpha) {
        col.setAlphaF(0.875);
    }
    drawBevelGradient(col, painter, r, path, true, false,
                      opts.tooltipAppearance, WIDGET_TOOLTIP, !haveAlpha);
    if (qtcIsFlat(opts.tooltipAppearance)) {
        painter->setPen(QPen(palette.toolTipText(), 0));
        drawRect(painter, r);
    }
    return true;
}

bool
Style::drawPrimitiveQtcBackground(PrimitiveElement,
                                  const QStyleOption *option,
                                  QPainter *painter,
                                  const QWidget*) const
{
    const QRect &r = option->rect;
    const QPalette &palette(option->palette);
    State state = option->state;
    if (auto bgnd = styleOptCast<BgndOption>(option)) {
        if (state & QtC_StateKWin) {
            QColor col(palette.brush(QPalette::Window).color());
            int opacity(col.alphaF() * 100);
            col.setAlphaF(1.0);
            drawBackground(painter, col, r, opacity, BGND_WINDOW,
                           bgnd->app, bgnd->path);
            // APPEARANCE_RAISED is used to signal flat background,
            // but have background image!
            if (bgnd->app != APPEARANCE_FLAT) {
                painter->setClipRect(bgnd->rect, Qt::IntersectClip);
                drawBackgroundImage(painter, true,
                                    (opts.bgndImage.type == IMG_FILE &&
                                     opts.bgndImage.onBorder) ? bgnd->rect :
                                    bgnd->widgetRect);
            }
        }
    }
    return true;
}

bool
Style::drawPrimitivePanelItemViewItem(PrimitiveElement,
                                      const QStyleOption *option,
                                      QPainter *painter,
                                      const QWidget *widget) const
{
    auto v4Opt = styleOptCast<QStyleOptionViewItemV4>(option);
    auto view = qobject_cast<const QAbstractItemView*>(widget);
    QRect r = option->rect;
    const QPalette &palette(option->palette);
    State state = option->state;
    bool reverse = option->direction == Qt::RightToLeft;
    bool hover = (state & State_MouseOver && state & State_Enabled &&
                  (!view ||
                   view->selectionMode() != QAbstractItemView::NoSelection));
    bool hasCustomBackground = (v4Opt->backgroundBrush.style() != Qt::NoBrush &&
                                !(option->state & State_Selected));
    bool hasSolidBackground =
        (!hasCustomBackground ||
         v4Opt->backgroundBrush.style() == Qt::SolidPattern);
    if (!hover && !(state & State_Selected) && !hasCustomBackground &&
        !(v4Opt->features & QStyleOptionViewItem::Alternate)) {
        return true;
    }
    QPalette::ColorGroup cg(state & State_Enabled ? state & State_Active ?
                            QPalette::Normal : QPalette::Inactive :
                            QPalette::Disabled);
    if (v4Opt && (v4Opt->features & QStyleOptionViewItem::Alternate)) {
        painter->fillRect(r, option->palette.brush(cg,
                                                   QPalette::AlternateBase));
    }
    if (!hover && !(state & State_Selected) && !hasCustomBackground) {
        return true;
    }
    if (hasCustomBackground) {
        painter->save();
        painter->setBrushOrigin(r.topLeft());
        painter->fillRect(r, v4Opt->backgroundBrush);
        painter->restore();
    }
    if (state & State_Selected || hover) {
        if (!widget) {
            widget = getWidget(painter);
            if (widget) {
                widget = widget->parentWidget();
            }
        }
        QColor color = (hasCustomBackground && hasSolidBackground ?
                        v4Opt->backgroundBrush.color() :
                        palette.color(cg, QPalette::Highlight));
        bool square = ((opts.square & SQUARE_LISTVIEW_SELECTION) &&
                       ((widget && !widget->inherits("KFilePlacesView") &&
                         (qobject_cast<const QTreeView*>(widget) ||
                          (qobject_cast<const QListView*>(widget) &&
                           ((const QListView*)widget)->viewMode() !=
                           QListView::IconMode)))));
        bool modAlpha = (!(state & State_Active) &&
                         m_inactiveChangeSelectionColor);
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
        if (square) {
            drawBevelGradient(color, painter, r, true, false,
                              opts.selectionAppearance, WIDGET_SELECTION);
        } else {
            QPixmap pix;
            QString key;
            key.sprintf("qtc-sel-%x-%x", r.height(), color.rgba());
            if (!m_usePixmapCache || !QPixmapCache::find(key, pix)) {
                pix = QPixmap(QSize(24, r.height()));
                pix.fill(Qt::transparent);
                QPainter pixPainter(&pix);
                QRect border(0, 0, pix.width(), pix.height());
                double radius(qtcGetRadius(&opts, r.width(), r.height(),
                                           WIDGET_OTHER, RADIUS_SELECTION));
                pixPainter.setRenderHint(QPainter::Antialiasing, true);
                drawBevelGradient(color, &pixPainter, border,
                                  buildPath(QRectF(border), WIDGET_OTHER,
                                            ROUNDED_ALL, radius), true,
                                  false, opts.selectionAppearance,
                                  WIDGET_SELECTION, false);
                if (opts.borderSelection) {
                    pixPainter.setBrush(Qt::NoBrush);
                    pixPainter.setPen(color);
                    pixPainter.drawPath(buildPath(border, WIDGET_SELECTION,
                                                  ROUNDED_ALL, radius));
                }
                pixPainter.end();
                if (m_usePixmapCache) {
                    QPixmapCache::insert(key, pix);
                }
            }
            bool roundedLeft = false;
            bool roundedRight = false;
            if (v4Opt) {
                roundedLeft = (v4Opt->viewItemPosition ==
                               QStyleOptionViewItemV4::Beginning);
                roundedRight = (v4Opt->viewItemPosition ==
                                QStyleOptionViewItemV4::End);
                if (oneOf(v4Opt->viewItemPosition,
                          QStyleOptionViewItemV4::OnlyOne,
                          QStyleOptionViewItemV4::Invalid) ||
                    (view && (view->selectionBehavior() !=
                              QAbstractItemView::SelectRows))) {
                    roundedLeft = roundedRight = true;
                }
            }
            int size = (roundedLeft && roundedRight ?
                        qMin(8, r.width() / 2) : 8);
            if (!reverse ? roundedLeft : roundedRight) {
                painter->drawPixmap(r.topLeft(), pix.copy(0, 0, size,
                                                          r.height()));
                r.adjust(size, 0, 0, 0);
            }
            if (!reverse ? roundedRight : roundedLeft) {
                painter->drawPixmap(r.right() - size + 1, r.top(),
                                    pix.copy(24 - size, 0, size, r.height()));
                r.adjust(0, 0, -size, 0);
            }
            if (r.isValid()) {
                painter->drawTiledPixmap(r, pix.copy(7, 0, 8, r.height()));
            }
        }
    }
    return true;
}

bool
Style::drawPrimitiveFrameTabWidget(PrimitiveElement,
                                   const QStyleOption *option,
                                   QPainter *painter,
                                   const QWidget *widget) const
{
    const QRect &r = option->rect;
    bool reverse = option->direction == Qt::RightToLeft;
    int round = opts.square & SQUARE_TAB_FRAME ? ROUNDED_NONE : ROUNDED_ALL;

    if (auto twf = styleOptCast<QStyleOptionTabWidgetFrame>(option)) {
        if ((opts.round || opts.tabBgnd == 0) &&
            widget && qobject_cast<const QTabWidget*>(widget)) {
            struct QtcTabWidget: public QTabWidget {
                bool
                tabsVisible() const
                {
                    return tabBar() && tabBar()->isVisible();
                }
                QRect
                currentTabRect() const
                {
                    return tabBar()->tabRect(tabBar()->currentIndex());
                }
            };
            const QTabWidget *tw = (const QTabWidget*)widget;
            if (tw->count() > 0 &&
                ((const QtcTabWidget*)widget)->tabsVisible()) {
                if (!reverse && opts.tabBgnd == 0) {
                    // Does not work for reverse :-(
                    QRect tabRect =
                        ((const QtcTabWidget*)widget)->currentTabRect();
                    int adjust = (opts.tabMouseOver == TAB_MO_GLOW &&
                                  !(opts.thin & THIN_FRAMES) ? 2 : 1);
                    switch (tw->tabPosition()) {
                    case QTabWidget::South:
                        tabRect = QRect(tabRect.x() + adjust,
                                        r.y() + r.height() - 2,
                                        tabRect.width() - 2 * adjust, 4);
                        break;
                    case QTabWidget::North: {
                        int leftAdjust =
                            qtcMax(twf->leftCornerWidgetSize.width(), 0);
                        tabRect.adjust(leftAdjust + adjust, 0,
                                       leftAdjust - adjust, 2);
                        break;
                    }
                    case QTabWidget::West:
                        tabRect.adjust(0, adjust, 2, -adjust);
                        break;
                    case QTabWidget::East:
                        tabRect = QRect(r.x() + r.width() - 2,
                                        tabRect.y() + adjust, 4,
                                        tabRect.height() - 2 * adjust);
                        break;
                    }
                    painter->setClipRegion(QRegion(r).subtracted(tabRect),
                                           Qt::IntersectClip);
                }
                if (!(opts.square & SQUARE_TAB_FRAME) &&
                    tw->currentIndex() == 0) {
                    bool reverse = twf->direction == Qt::RightToLeft;
                    switch (tw->tabPosition()) {
                    case QTabWidget::North:
                        if (reverse && twf->rightCornerWidgetSize.isEmpty()) {
                            round -= CORNER_TR;
                        } else if (!reverse &&
                                   twf->leftCornerWidgetSize.isEmpty()) {
                            round -= CORNER_TL;
                        }
                        break;
                    case QTabWidget::South:
                        if (reverse && twf->rightCornerWidgetSize.isEmpty()) {
                            round -= CORNER_BR;
                        } else if (!reverse &&
                                   twf->leftCornerWidgetSize.isEmpty()) {
                            round -= CORNER_BL;
                        }
                        break;
                    case QTabWidget::West:
                        round -= CORNER_TL;
                        break;
                    case QTabWidget::East:
                        round -= CORNER_TR;
                        break;
                    }
                }
            }
        }
    }

    QStyleOption opt(*option);
    const QColor *use = backgroundColors(option);

    opt.state |= State_Enabled;
    if (opts.tabBgnd != 0) {
        QColor bgnd(shade(use[ORIGINAL_SHADE], TO_FACTOR(opts.tabBgnd)));
        painter->fillRect(r.adjusted(0, 1, 0, -1), bgnd);
        painter->fillRect(r.adjusted(1, 0, -1, 0), bgnd);
    }
    drawBorder(painter, r, &opt, round, use, WIDGET_TAB_FRAME,
               opts.borderTab ? BORDER_LIGHT : BORDER_RAISED, false);
    return true;
}

bool
Style::drawPrimitiveFrameWindow(PrimitiveElement,
                                const QStyleOption *option,
                                QPainter *painter,
                                const QWidget*) const
{
    const QRect &r = option->rect;
    State state = option->state;
    const QPalette &palette(option->palette);
    bool colTbarOnly = opts.windowBorder & WINDOW_BORDER_COLOR_TITLEBAR_ONLY;
    bool fillBgnd = (!(state & QtC_StateKWin) && !m_isPreview &&
                     !qtcIsFlatBgnd(opts.bgndAppearance));
    const QColor *bgndCols =
        (colTbarOnly || fillBgnd ?
         backgroundColors(palette.color(QPalette::Active,
                                        QPalette::Window)) : 0L);
    const QColor *borderCols = (colTbarOnly ? bgndCols :
                                theThemedApp == APP_KWIN ?
                                buttonColors(option) :
                                getMdiColors(option, state & State_Active));
    QColor light = borderCols[0];
    QColor dark = (option->version == TBAR_BORDER_VERSION_HACK + 2 ?
                   palette.color(QPalette::Active, QPalette::Shadow) :
                   borderCols[option &&
                              option->version == TBAR_BORDER_VERSION_HACK ?
                              0 : QTC_STD_BORDER]);
    bool isKWin = state & QtC_StateKWin;
    bool addLight = (opts.windowBorder & WINDOW_BORDER_ADD_LIGHT_BORDER &&
                     (!isKWin || qtcGetWindowBorderSize(false).sides > 1));
    light.setAlphaF(1.0);
    dark.setAlphaF(1.0);
    if (fillBgnd) {
        painter->fillRect(r, bgndCols[ORIGINAL_SHADE]);
    }
    if (opts.round < ROUND_SLIGHT || !isKWin ||
        (state & QtC_StateKWinNotFull && state & QtC_StateKWin)) {
        QPAINTER_RENDERHIT_AA_MAYBE_OFF(painter);
        if (addLight) {
            painter->setPen(light);
            painter->drawLine(r.x() + 1, r.y(), r.x() + 1,
                              r.y() + r.height() - 1);
        }
        painter->setPen(dark);
        drawRect(painter, r);
    } else {
        if (addLight) {
            QPAINTER_RENDERHIT_AA_MAYBE_OFF(painter);
            painter->setPen(light);
            painter->drawLine(r.x() + 1, r.y(), r.x() + 1,
                              r.y() + r.height() -
                              (1 + (opts.round > ROUND_SLIGHT &&
                                    state & QtC_StateKWin ? 3 : 1)));
        }
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(dark);
        painter->drawPath(buildPath(r, WIDGET_OTHER, ROUNDED_ALL,
                                    opts.round > ROUND_SLIGHT &&
                                    state & QtC_StateKWin ? 6.0 : 2.0));
        if (opts.round >= ROUND_FULL && !(state & QtC_StateKWinCompositing)) {
            QColor col(opts.windowBorder & WINDOW_BORDER_COLOR_TITLEBAR_ONLY ?
                       backgroundColors(option)[QTC_STD_BORDER] :
                       buttonColors(option)[QTC_STD_BORDER]);
            QPAINTER_RENDERHIT_AA_MAYBE_OFF(painter);
            painter->setPen(col);
            painter->drawPoint(r.x() + 2, r.y() + r.height() - 3);
            painter->drawPoint(r.x() + r.width() - 3, r.y() + r.height() - 3);
            painter->drawLine(r.x() + 1, r.y() + r.height() - 5,
                              r.x() + 1, r.y() + r.height() - 4);
            painter->drawLine(r.x() + 3, r.y() + r.height() - 2,
                              r.x() + 4, r.y() + r.height() - 2);
            painter->drawLine(r.x() + r.width() - 2, r.y() + r.height() - 5,
                              r.x() + r.width() - 2, r.y() + r.height() - 4);
            painter->drawLine(r.x() + r.width() - 4, r.y() + r.height() - 2,
                              r.x() + r.width() - 5, r.y() + r.height() - 2);
        }
    }
    return true;
}

bool
Style::drawPrimitiveButton(PrimitiveElement element, const QStyleOption *option,
                           QPainter *painter, const QWidget *widget) const
{
    const QRect &r = option->rect;
    State state = option->state;
    if (state & STATE_DWT_BUTTON &&
        (opts.dwtSettings & DWT_BUTTONS_AS_PER_TITLEBAR)) {
        return true;
    }
    bool doEtch = opts.buttonEffect != EFFECT_NONE;

    // This fixes the "Sign in" button at mail.lycos.co.uk
    // ...basically if KHTML gices us a fully transparent background colour,
    // then dont paint the button.
    if (option->palette.button().color().alpha() == 0) {
        if (state & State_MouseOver && state & State_Enabled &&
            opts.coloredMouseOver == MO_GLOW && doEtch &&
            !(opts.thin & THIN_FRAMES)) {
            drawGlow(painter, r, WIDGET_STD_BUTTON);
        }
        return true;
    }
    if (!widget) {
        widget = getWidget(painter);
    }
    const QColor *use = buttonColors(option);
    bool isDefault = false;
    bool isFlat = false;
    bool isKWin = state & QtC_StateKWin;
    bool isDown = state & State_Sunken || state & State_On;
    bool isOnListView = (!isKWin && widget &&
                         qobject_cast<const QAbstractItemView*>(widget));
    QStyleOption opt(*option);
    if (element == PE_PanelButtonBevel) {
        opt.state |= State_Enabled;
    }
    if (auto button = styleOptCast<QStyleOptionButton>(option)) {
        isDefault = ((button->features & QStyleOptionButton::DefaultButton) &&
                     (button->state & State_Enabled));
        isFlat = button->features & QStyleOptionButton::Flat;
    }
    if (!(opt.state & State_Enabled)) {
        opt.state &= ~State_MouseOver;
    }
    // For some reason with OO.o not all buttons are set as raised!
    if (!(opt.state & State_AutoRaise)) {
        opt.state |= State_Raised;
    }

    isDefault = (isDefault ||
                 (doEtch && oneOf(opts.focus, FOCUS_FULL, FOCUS_FILLED) &&
                  opts.coloredMouseOver == MO_GLOW &&
                  opt.state & State_HasFocus && opt.state & State_Enabled));
    if (isFlat && !isDown && !(opt.state & State_MouseOver)) {
        return true;
    }

    if (isOnListView) {
        opt.state |= State_Horizontal | State_Raised;
    }
    if (isDefault && state & State_Enabled &&
        oneOf(opts.defBtnIndicator, IND_TINT, IND_SELECTED)) {
        use = m_defBtnCols;
    } else if (state & STATE_DWT_BUTTON && widget &&
               opts.titlebarButtons & TITLEBAR_BUTTON_COLOR &&
               coloredMdiButtons(state & State_Active,
                                 state & State_MouseOver) &&
               !(opts.titlebarButtons & TITLEBAR_BUTTON_COLOR_SYMBOL)) {
        if (constDwtClose == widget->objectName()) {
            use = m_titleBarButtonsCols[TITLEBAR_CLOSE];
        } else if (constDwtFloat == widget->objectName()) {
            use = m_titleBarButtonsCols[TITLEBAR_MAX];
        } else if (qtcCheckType<QDockWidget>(getParent<2>(widget)) &&
                   widget->parentWidget()->inherits("KoDockWidgetTitleBar")) {
            QDockWidget *dw = (QDockWidget*)getParent<2>(widget);
            QWidget *koDw = widget->parentWidget();
            int fw = (dw->isFloating() ?
                      pixelMetric(QStyle::PM_DockWidgetFrameWidth, 0, dw) : 0);
            QRect geom = widget->geometry();
            QStyleOptionDockWidget dwOpt;
            dwOpt.initFrom(dw);
            dwOpt.rect = QRect(QPoint(fw, fw),
                               QSize(koDw->geometry().width() - (fw * 2),
                                     koDw->geometry().height() - (fw * 2)));
            dwOpt.title = dw->windowTitle();
            dwOpt.closable = ((dw->features() &
                               QDockWidget::DockWidgetClosable) ==
                              QDockWidget::DockWidgetClosable);
            dwOpt.floatable = ((dw->features() &
                                QDockWidget::DockWidgetFloatable) ==
                               QDockWidget::DockWidgetFloatable);
            if (dwOpt.closable &&
                subElementRect(QStyle::SE_DockWidgetCloseButton, &dwOpt,
                               getParent<2>(widget)) == geom) {
                use = m_titleBarButtonsCols[TITLEBAR_CLOSE];
            } else if (dwOpt.floatable &&
                       subElementRect(QStyle::SE_DockWidgetFloatButton, &dwOpt,
                                      getParent<2>(widget)) == geom) {
                use = m_titleBarButtonsCols[TITLEBAR_MAX];
            } else {
                use = m_titleBarButtonsCols[TITLEBAR_SHADE];
            }
        }
    }
    if (isKWin) {
        opt.state |= STATE_KWIN_BUTTON;
    }
    bool coloredDef = (isDefault && state & State_Enabled &&
                       opts.defBtnIndicator == IND_COLORED);
    if (widget && qobject_cast<const QAbstractButton*>(widget) &&
        static_cast<const QAbstractButton*>(widget)->isCheckable()) {
        opt.state |= STATE_TOGGLE_BUTTON;
    }

    drawLightBevel(painter, r, &opt, widget, ROUNDED_ALL,
                   coloredDef ? m_defBtnCols[MO_DEF_BTN] :
                   getFill(&opt, use, false,
                           isDefault && state & State_Enabled &&
                           opts.defBtnIndicator == IND_DARKEN),
                   coloredDef ? m_defBtnCols : use, true,
                   isKWin || state & STATE_DWT_BUTTON ?
                   WIDGET_MDI_WINDOW_BUTTON :
                   isOnListView ? WIDGET_NO_ETCH_BTN :
                   isDefault && state & State_Enabled ?
                   WIDGET_DEF_BUTTON :
                   state & STATE_TBAR_BUTTON ? WIDGET_TOOLBAR_BUTTON :
                   WIDGET_STD_BUTTON);

    if (isDefault && state & State_Enabled) {
        switch (opts.defBtnIndicator) {
        case IND_CORNER: {
            QPainterPath path;
            int offset = isDown ? 5 : 4;
            int etchOffset = doEtch ? 1 : 0;
            double xd = r.x() + 0.5;
            double yd = r.y() + 0.5;
            const QColor *cols = m_focusCols;

            path.moveTo(xd + offset + etchOffset, yd + offset + etchOffset);
            path.lineTo(xd + offset + 6 + etchOffset, yd + offset + etchOffset);
            path.lineTo(xd + offset + etchOffset, yd + offset + 6 + etchOffset);
            path.lineTo(xd + offset + etchOffset, yd + offset + etchOffset);
            painter->setBrush(cols[isDown ? 0 : 4]);
            setPainterPen(painter, cols[isDown ? 0 : 4], QPENWIDTH1);
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->drawPath(path);
            QPAINTER_RENDERHIT_AA_MAYBE_OFF(painter);
            break;
        }
        case IND_COLORED: {
            int offset = COLORED_BORDER_SIZE + (doEtch ? 1 : 0);
            drawBevelGradient(getFill(&opt, use), painter,
                              r.adjusted(offset, offset, -offset, -offset),
                              true, state & (State_On | State_Sunken),
                              opts.appearance, WIDGET_STD_BUTTON);
        }
        default:
            break;
        }
    }
    return true;
}

bool
Style::drawPrimitivePanelMenu(PrimitiveElement, const QStyleOption *option,
                              QPainter *painter, const QWidget*) const
{
    const QRect &r = option->rect;
    double radius = opts.round >= ROUND_FULL ? 5.0 : 2.5;
    const QColor *use = popupMenuCols(option);
    painter->setClipRegion(r);
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    if (!opts.popupBorder) {
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(use[ORIGINAL_SHADE]);
        painter->drawPath(buildPath(r, WIDGET_OTHER, ROUNDED_ALL, radius));
        QPAINTER_RENDERHIT_AA_MAYBE_OFF(painter);
    }
    if (!(opts.square & SQUARE_POPUP_MENUS)) {
        painter->setClipRegion(windowMask(r, opts.round > ROUND_SLIGHT),
                               Qt::IntersectClip);
    }

    // In case the gradient uses alpha, we need to fill with the background
    // colour - this makes it consistent with Gtk.
    if (opts.menuBgndOpacity == 100) {
        painter->fillRect(r, option->palette.brush(QPalette::Background));
    }
    drawBackground(painter, popupMenuCols()[ORIGINAL_SHADE], r,
                   opts.menuBgndOpacity, BGND_MENU, opts.menuBgndAppearance);
    // FIXME, workaround only, the non transparent part of the image will have
    // a different overall opacity.
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    drawBackgroundImage(painter, false, r);
    // TODO: draw border in other functions.
    if (opts.popupBorder) {
        EGradientBorder border =
            qtcGetGradient(opts.menuBgndAppearance, &opts)->border;
        painter->setClipping(false);
        painter->setPen(use[QTC_STD_BORDER]);
        if (opts.square & SQUARE_POPUP_MENUS) {
            drawRect(painter, r);
        } else {
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->drawPath(buildPath(r, WIDGET_OTHER, ROUNDED_ALL, radius));
        }
        if (qtcUseBorder(border) &&
            opts.menuBgndAppearance != APPEARANCE_FLAT) {
            QRect ri(r.adjusted(1, 1, -1, -1));
            painter->setPen(use[0]);
            if (border == GB_LIGHT) {
                if (opts.square & SQUARE_POPUP_MENUS) {
                    drawRect(painter, ri);
                } else {
                    painter->drawPath(buildPath(ri, WIDGET_OTHER, ROUNDED_ALL,
                                                radius - 1.0));
                }
            } else if (opts.square & SQUARE_POPUP_MENUS) {
                if (border != GB_3D) {
                    painter->drawLine(ri.x(), ri.y(), ri.x() + ri.width() - 1,
                                      ri.y());
                    painter->drawLine(ri.x(), ri.y(), ri.x(),
                                      ri.y() + ri.height() - 1);
                }
                painter->setPen(use[FRAME_DARK_SHADOW]);
                painter->drawLine(ri.x(), ri.y() + ri.height() - 1,
                                  ri.x() + ri.width() - 1,
                                  ri.y() + ri.height() - 1);
                painter->drawLine(ri.x() + ri.width() - 1, ri.y(),
                                  ri.x() + ri.width() - 1,
                                  ri.y() + ri.height() - 1);
            } else {
                QPainterPath tl;
                QPainterPath br;
                buildSplitPath(ri, ROUNDED_ALL, radius - 1.0, tl, br);
                if (border != GB_3D) {
                    painter->drawPath(tl);
                }
                painter->setPen(use[FRAME_DARK_SHADOW]);
                painter->drawPath(br);
            }
        }
    }
    return true;
}

bool
Style::drawPrimitiveFrameFocusRect(PrimitiveElement, const QStyleOption *option,
                                   QPainter *painter,
                                   const QWidget *widget) const
{
    const QRect &r = option->rect;
    State state = option->state;
    const QPalette &palette(option->palette);

    if (FOCUS_NONE==opts.focus)
        return true;

    if (auto focusFrame = styleOptCast<QStyleOptionFocusRect>(option)) {
        if (!(focusFrame->state & State_KeyboardFocusChange) ||
            (widget && widget->inherits("QComboBoxListView"))) {
            return true;
        }
        if (widget && opts.focus == FOCUS_GLOW) {
            if (qobject_cast<const QAbstractButton*>(widget)) {
                if (!qobject_cast<const QToolButton*>(widget) ||
                    !static_cast<const QToolButton*>(widget)->autoRaise()) {
                    return true;
                }
            } else if (qobject_cast<const QComboBox*>(widget) ||
                       qobject_cast<const QGroupBox*>(widget) ||
                       qobject_cast<const QDial*>(widget)) {
                return true;
            }
        }
        QRect r2(r);
        if (widget && (qobject_cast<const QCheckBox*>(widget) ||
                       qobject_cast<const QRadioButton*>(widget)) &&
            ((QAbstractButton*)widget)->text().isEmpty() &&
            r.height() <= widget->rect().height() - 2 &&
            r.width() <= widget->rect().width() - 2 &&
            r.x() >= 1 && r.y() >= 1) {
            int adjust = qMin(qMin(abs(widget->rect().x() - r.x()), 2),
                              abs(widget->rect().y() - r.y()));
            r2.adjust(-adjust, -adjust, adjust, adjust);
        }

        if (widget && qobject_cast<const QGroupBox*>(widget)) {
            r2.adjust(0, 2, 0, 0);
        }
        if (opts.focus == FOCUS_STANDARD) {
            // Taken from QWindowsStyle...
            painter->setBackgroundMode(Qt::TransparentMode);
            QColor bgCol(focusFrame->backgroundColor);
            if (!bgCol.isValid()) {
                bgCol = painter->background().color();
            }
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
        } else {
            // Figuring out in what beast we are painting...
            bool view(state & State_Item ||
                      ((widget &&
                        qobject_cast<const QAbstractScrollArea*>(widget)) ||
                       (widget && widget->parent() &&
                        qobject_cast<const QAbstractScrollArea*>(
                            widget->parent()))));
            if (!view && !widget) {
                // Try to determine if we are in a KPageView...
                const QWidget *wid = getWidget(painter);
                if (wid && wid->parentWidget()) {
                    if (wid->parentWidget()->inherits(
                            "KDEPrivate::KPageListView")) {
                        r2.adjust(2, 2, -2, -2);
                        view = true;
                    } else if (theThemedApp == APP_KONTACT &&
                               (wid->parentWidget()->inherits(
                                    "KMail::MainFolderView") ||
                                wid->parentWidget()->inherits(
                                    "MessageList::Core::View"))) {
                        view = true;
                    }
                }
            }
            QColor c(view && state & State_Selected ?
                     palette.highlightedText().color() :
                     m_focusCols[FOCUS_SHADE(state & State_Selected)]);

            if (oneOf(opts.focus, FOCUS_LINE, FOCUS_GLOW)) {
                if (!(state & State_Horizontal) && widget &&
                    qobject_cast<const QTabBar*>(widget)) {
                    drawFadedLine(painter, QRect(r2.x() + r2.width() - 1,
                                                 r2.y(), 1, r2.height()),
                                  c, true, true, false);
                } else {
                    drawFadedLine(painter,
                                  QRect(r2.x(), r2.y() + r2.height() -
                                        (view ? 3 : 1), r2.width(), 1),
                                  c, true, true, true);
                }
            } else {
                painter->setPen(c);
                if (opts.focus == FOCUS_FILLED) {
                    c.setAlphaF(FOCUS_ALPHA);
                    painter->setBrush(c);
                }
                if (opts.round != ROUND_NONE) {
                    bool square((opts.square & SQUARE_LISTVIEW_SELECTION) &&
                                (((widget &&
                                   !widget->inherits("KFilePlacesView") &&
                                   (qobject_cast<const QTreeView*>(widget) ||
                                    (qobject_cast<const QListView*>(widget) &&
                                     ((const QListView*)widget)->viewMode() !=
                                     QListView::IconMode)))) ||
                                 (!widget && view)));
                    painter->setRenderHint(QPainter::Antialiasing, true);
                    painter->drawPath(
                        buildPath(r2, WIDGET_SELECTION, ROUNDED_ALL,
                                  square ? SLIGHT_INNER_RADIUS :
                                  qtcGetRadius(&opts, r2.width(),
                                               r2.height(), WIDGET_OTHER,
                                               oneOf(opts.focus, FOCUS_FULL,
                                                     FOCUS_FILLED) ?
                                               RADIUS_EXTERNAL :
                                               RADIUS_SELECTION)));
                } else {
                    drawRect(painter, r2);
                }
            }
        }
    }
    return true;
}

bool
Style::drawPrimitiveIndicatorToolBarHandle(PrimitiveElement,
                                           const QStyleOption *option,
                                           QPainter *painter,
                                           const QWidget*) const
{
    drawHandleMarkers(painter, option->rect, option, true, opts.handles);
    return true;
}

bool
Style::drawPrimitiveIndicatorRadioButton(PrimitiveElement,
                                         const QStyleOption *option,
                                         QPainter *painter,
                                         const QWidget *widget) const
{
    const QRect &r = option->rect;
    State state = option->state;
    const QPalette &palette(option->palette);
    bool isOO = isOOWidget(widget);
    // TODO: WTF???
    bool selectedOOMenu = (isOO && oneOf(r, QRect(0, 0, 15, 15),
                                         QRect(0, 0, 14, 15)) &&
                           // OO.o 3.2 =14x15?
                           oneOf(state, State_Sunken | State_Enabled,
                                 State_Sunken | State_Enabled |
                                 State_Selected));

    if (isOO) {
        painter->fillRect(r, palette.brush(QPalette::Background));
    }
    if (selectedOOMenu) {
        drawPrimitive(PE_IndicatorCheckBox, option, painter, widget);
    } else {
        bool menu = state & STATE_MENU;
        int x = r.x();
        int y = r.y();
        if (opts.crButton) {
            const QColor *use = checkRadioColors(option);
            QStyleOption opt(*option);
            bool doEtch = opts.buttonEffect != EFFECT_NONE;
            QRect rect(r.x(), r.y(), opts.crSize + (doEtch ? 2 : 0),
                       opts.crSize + (doEtch ? 2 : 0));

            if (opts.crSize != CR_SMALL_SIZE && menu) {
                rect.adjust(0, -1, 0, -1);
                y++;
            }
            if (isOO && r == QRect(0, 0, opts.crSize, opts.crSize)) {
                rect.adjust(-1, -1, -1, -1);
                --x;
                --y;
            }
            if (menu || selectedOOMenu) {
                opt.state &= ~(State_MouseOver | State_Sunken);
            }
            opt.state &= ~State_On;
            opt.state |= State_Raised;
            opt.rect = rect;

            if (doEtch) {
                x++;
                y++;
            }
            if (opts.crSize != CR_SMALL_SIZE && menu) {
                y -= 2;
            }
            painter->setRenderHint(QPainter::Antialiasing, true);
            drawLightBevel(painter, rect, &opt, widget, ROUNDED_ALL,
                           getFill(&opt, use, true, false), use, true,
                           WIDGET_RADIO_BUTTON);
        } else {
            bool sunken = !menu && !selectedOOMenu && (state & State_Sunken);
            bool doEtch = (!menu && r.width() >= opts.crSize + 2 &&
                           r.height() >= opts.crSize + 2 &&
                           opts.buttonEffect != EFFECT_NONE);
            bool mo = (!sunken && state & State_MouseOver &&
                       state & State_Enabled);
            bool glow = doEtch && opts.coloredMouseOver == MO_GLOW && mo;
            bool coloredMo = (opts.coloredMouseOver != MO_NONE && !glow &&
                              mo && !sunken);
            bool lightBorder = DRAW_LIGHT_BORDER(false, WIDGET_TROUGH,
                                                 APPEARANCE_INVERTED);
            bool doneShadow = false;
            QRect rect = doEtch ? r.adjusted(1, 1, -1, -1) : r;
            const QColor *bc = sunken ? 0L : borderColors(option, 0L);
            const QColor *btn = checkRadioColors(option);
            const QColor *use = bc ? bc : btn;
            if (doEtch) {
                x++;
                y++;
            }
            const QColor &bgnd(state & State_Enabled && !sunken ?
                               opts.coloredMouseOver == MO_NONE &&
                               !opts.crHighlight && mo ?
                               use[CR_MO_FILL] :
                               palette.base().color() :
                               palette.background().color());
            QPainterPath path;

            path.addEllipse(QRectF(rect).adjusted(0.5, 0.5, -1.0, -1.0));
            drawBevelGradient(bgnd, painter, rect.adjusted(1, 1, -1, -1),
                              path, true, false, APPEARANCE_INVERTED,
                              WIDGET_TROUGH);
            painter->setRenderHint(QPainter::Antialiasing, true);
            if (coloredMo) {
                painter->setBrush(Qt::NoBrush);
                setPainterPen(painter, use[CR_MO_FILL], QPENWIDTH1);
                painter->drawArc(QRectF(x + 1, y + 1, opts.crSize - 2,
                                        opts.crSize - 2), 0, 360 * 16);
                painter->drawArc(QRectF(x + 2, y + 2, opts.crSize - 4,
                                        opts.crSize - 4), 0, 360 * 16);
            }
            painter->setBrush(Qt::NoBrush);
            if (!doneShadow && doEtch &&
                (glow || opts.buttonEffect != EFFECT_NONE || sunken)) {
                QColor topCol = glow ? m_mouseOverCols[GLOW_MO] : Qt::black;
                if (!glow) {
                    topCol.setAlphaF(ETCH_RADIO_TOP_ALPHA);
                }
                setPainterPen(painter, topCol, QPENWIDTH1);
                painter->drawArc(QRectF(x - 0.5, y - 0.5, opts.crSize + 1,
                                        opts.crSize + 1), 45 * 16, 180 * 16);
                if (!glow) {
                    painter->setPen(getLowerEtchCol(widget));
                }
                painter->drawArc(QRectF(x - 0.5, y - 0.5, opts.crSize + 1,
                                        opts.crSize + 1), 225 * 16, 180 * 16);
            }
            setPainterPen(painter, use[BORDER_VAL(state & State_Enabled)], QPENWIDTH1);
            painter->drawArc(QRectF(x + 0.25, y + 0.25, opts.crSize - 0.5,
                                    opts.crSize - 0.5), 0, 360 * 16);
            if (!coloredMo) {
                setPainterPen(painter, btn[state & State_MouseOver ? 3 : 4], QPENWIDTH1);
                painter->drawArc(QRectF(x + 0.75, y + 0.75, opts.crSize - 1.5,
                                        opts.crSize - 1.5),
                                 lightBorder ? 0 : 45 * 16,
                                 lightBorder ? 360 * 16 : 180 * 16);
            }
        }
        if (state & State_On || selectedOOMenu) {
            QPainterPath path;
            double radius = opts.smallRadio ? 2.75 : 3.75;
            double offset = opts.crSize / 2.0 - radius;
            path.addEllipse(QRectF(x + offset, y + offset,
                                   radius * 2.0, radius * 2.0));
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->fillPath(path, checkRadioCol(option));
        }
    }
    return true;
}

bool
Style::drawPrimitiveIndicatorCheckBox(PrimitiveElement element,
                                      const QStyleOption *option,
                                      QPainter *painter,
                                      const QWidget *widget) const
{
    const QRect &r = option->rect;
    State state = option->state;
    const QPalette &palette(option->palette);
    bool menu = state & STATE_MENU;
    bool view = state & STATE_VIEW;
    bool doEtch = (opts.buttonEffect != EFFECT_NONE &&
                   (opts.crButton ||
                    (PE_IndicatorMenuCheckMark != element && !menu &&
                     r.width() >= opts.crSize + 2 &&
                     r.height() >= opts.crSize + 2)));
    bool isOO = isOOWidget(widget);
    bool selectedOOMenu = (isOO && oneOf(r, QRect(0, 0, 15, 15),
                                         QRect(0, 0, 14, 15)) &&
                           // OO.o 3.2 =14x15?
                           oneOf(state, State_Sunken | State_Enabled,
                                 State_Sunken | State_Enabled |
                                 State_Selected));
    int crSize = opts.crSize + (doEtch ? 2 : 0);
    QRect rect(r.x(), r.y() + (view ? -1 : 0), crSize, crSize);

    // For OO.o 3.2 need to fill widget background!
    if (isOO) {
        painter->fillRect(r, palette.brush(QPalette::Window));
    }

    if (selectedOOMenu) {
        if (r == QRect(0, 0, 14, 15)) { // OO.o 3.2 =14x15?
            rect.adjust(-1, -1, -1, -1);
        }
        setPainterPen(painter, option ? option->palette.text().color() :
                        QApplication::palette().text().color(), QPENWIDTH1);
        drawRect(painter, r);
        // LibreOffice its 15x15 - and arrow is not centred, so adjust this...
        if (r == QRect(0, 0, 15, 15)) {
            rect.adjust(-1, -1, -1, -1);
        }
    } else {
        if (isOO && r == QRect(0, 0, opts.crSize, opts.crSize)) {
            rect.adjust(0, -1, 0, -1);
        }

        if (opts.crSize != CR_SMALL_SIZE) {
            if (menu) {
                rect.adjust(0, -1, 0, -1);
            } else if (r.height() > crSize) {
                // Can only adjust position if there is space!
                // ...when used in a listview, usually there is no space.
                rect.adjust(0, 1, 0, 1);
            }
        }

        if (opts.crButton) {
            const QColor *use(checkRadioColors(option));
            QStyleOption opt(*option);
            if (menu || selectedOOMenu) {
                opt.state &= ~(State_MouseOver | State_Sunken);
            }
            opt.state &= ~State_On;
            opt.state |= State_Raised;
            opt.rect = rect;
            painter->setRenderHint(QPainter::Antialiasing, true);
            drawLightBevel(painter, rect, &opt, widget, ROUNDED_ALL,
                           getFill(&opt, use, true, false),
                           use, true, WIDGET_CHECKBOX);
        } else {
            bool sunken = !menu && !selectedOOMenu && (state & State_Sunken);
            bool mo = (!sunken && state & State_MouseOver &&
                       state & State_Enabled);
            bool glow = doEtch && opts.coloredMouseOver == MO_GLOW && mo;
            const QColor *bc = sunken ? 0L : borderColors(option, 0L);
            const QColor *btn = checkRadioColors(option);
            const QColor *use = bc ? bc : btn;
            const QColor &bgnd(state & State_Enabled && !sunken ?
                               opts.coloredMouseOver == MO_NONE &&
                               !opts.crHighlight && mo ? use[CR_MO_FILL] :
                               palette.base().color() :
                               palette.background().color());
            bool lightBorder = DRAW_LIGHT_BORDER(false, WIDGET_TROUGH,
                                                 APPEARANCE_INVERTED);
            rect = QRect(doEtch ? rect.adjusted(1, 1, -1, -1) : rect);
            if (qtcIsFlat(opts.appearance)) {
                painter->fillRect(rect.adjusted(1, 1, -1, -1), bgnd);
            } else {
                drawBevelGradient(bgnd, painter, rect.adjusted(1, 1, -1, -1),
                                  true, false, APPEARANCE_INVERTED,
                                  WIDGET_TROUGH);
            }

            if (opts.coloredMouseOver != MO_NONE && !glow && mo) {
                painter->setRenderHint(QPainter::Antialiasing, true);
                setPainterPen(painter, use[CR_MO_FILL], QPENWIDTH1);
                drawAaRect(painter, rect.adjusted(1, 1, -1, -1));
                QPAINTER_RENDERHIT_AA_MAYBE_OFF(painter);
            } else {
                setPainterPen(painter, midColor(state & State_Enabled ?
                                         palette.base().color() :
                                         palette.background().color(), use[3]), QPENWIDTH1);
                if (lightBorder) {
                    drawRect(painter, rect.adjusted(1, 1, -1, -1));
                } else {
                    painter->drawLine(rect.x() + 1, rect.y() + 1,
                                      rect.x() + 1,
                                      rect.y() + rect.height() - 2);
                    painter->drawLine(rect.x() + 1, rect.y() + 1,
                                      rect.x() + rect.width() - 2,
                                      rect.y() + 1);
                }
            }

            painter->setRenderHint(QPainter::Antialiasing, true);
            if (doEtch && !view) {
                if (glow && !(opts.thin & THIN_FRAMES)) {
                    drawGlow(painter, r, WIDGET_CHECKBOX);
                } else {
                    drawEtch(painter, r, widget, WIDGET_CHECKBOX,
                             opts.buttonEffect == EFFECT_SHADOW &&
                             opts.crButton ? !sunken : false);
                }
            }
            drawBorder(painter, rect, option, ROUNDED_ALL, use,
                       WIDGET_CHECKBOX);
            painter->setRenderHint(QPainter::Antialiasing, QPAINTER_ANTIALIAS_MAYBE_ON);
        }
    }
    if (state & State_On || selectedOOMenu) {
        QPixmap *pix = getPixmap(checkRadioCol(option), PIX_CHECK, 1.0);

        painter->drawPixmap(rect.center().x() - pix->width() / 2,
                            rect.center().y() - pix->height() / 2, *pix);
    } else if (state & State_NoChange) {
        // tri-state
        int x(rect.center().x()), y(rect.center().y());

        setPainterPen(painter, checkRadioCol(option), QPENWIDTH1);
        painter->drawLine(x - 3, y, x + 3, y);
        painter->drawLine(x - 3, y + 1, x + 3, y + 1);
    }
    return true;
}

bool
Style::drawPrimitiveFrameLineEdit(PrimitiveElement, const QStyleOption *option,
                                  QPainter *painter,
                                  const QWidget *widget) const
{
    const QRect &r = option->rect;
    State state = option->state;
    const QPalette &palette(option->palette);
    if (auto lineEdit = styleOptCast<QStyleOptionFrame>(option)) {
        if ((lineEdit->lineWidth > 0 || isOOWidget(widget)) &&
            !(widget &&
              (qobject_cast<const QComboBox*>(widget->parentWidget()) ||
               qobject_cast<const QAbstractSpinBox*>(
                   widget->parentWidget())))) {
            QStyleOptionFrame opt(*lineEdit);
            if (opt.state & State_Enabled && state & State_ReadOnly) {
                opt.state ^= State_Enabled;
            }
            if (opts.buttonEffect != EFFECT_NONE && opts.etchEntry &&
                theThemedApp == APP_ARORA && widget && widget->parentWidget() &&
                strcmp(widget->metaObject()->className(), "LocationBar") == 0) {
                const QToolBar *tb = getToolBar(widget->parentWidget());
                if (tb) {
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
                        r2.setY(-widget->mapTo((QWidget*)tb,
                                               QPoint(r.x(), r.y())).y());
                        r2.setHeight(tb->rect().height());
                    }
                    painter->setClipRegion(
                        QRegion(r2).subtracted(QRegion(r2.adjusted(2, 2,
                                                                   -2, -2))));
                    drawMenuOrToolBarBackground(widget, painter, r2, &opt,
                                                false, true);
                    painter->restore();
                }
            }
            bool isOO = isOOWidget(widget);
            QRect rect(r);
            int round = ROUNDED_ALL;
            if (isOO) {
                // This (hopefull) checks is we're OO.o 3.2 - in which case
                // no adjustment is required...
                const QImage *img = getImage(painter);
                if (!img || img->rect() != r) {
                    // OO.o 3.1?
                    rect.adjust(1, 2, -1, -2);
                } else {
                    round = ROUNDED_NONE;
                    painter->fillRect(r, palette.brush(QPalette::Window));
                    rect.adjust(1, 1, -1, -1);
                }
            }
            drawEntryField(painter, rect, widget, &opt, round, isOO,
                           !isOO && opts.buttonEffect != EFFECT_NONE);
        }
    }
    return true;
}

bool
Style::drawPrimitivePanelLineEdit(PrimitiveElement,
                                  const QStyleOption *option, QPainter *painter,
                                  const QWidget *widget) const
{
    const QRect &r = option->rect;
    const QPalette &palette(option->palette);
    if (auto panel = styleOptCast<QStyleOptionFrame>(option)) {
        if (panel->lineWidth > 0) {
            QRect r2 = r.adjusted(1, 1, -1,
                                  opts.buttonEffect != EFFECT_NONE ? -2 : -1);
            painter->fillPath(buildPath(r2, WIDGET_ENTRY, ROUNDED_ALL,
                                        qtcGetRadius(&opts, r2.width(),
                                                     r2.height(), WIDGET_ENTRY,
                                                     RADIUS_INTERNAL)),
                              palette.brush(QPalette::Base));
            drawPrimitive(PE_FrameLineEdit, option, painter, widget);
        } else {
            painter->fillRect(r.adjusted(2, 2, -2, -2),
                              palette.brush(QPalette::Base));
        }
    }
    return true;
}

bool
Style::drawPrimitiveIndicatorDockWidgetResizeHandle(
    PrimitiveElement, const QStyleOption *option, QPainter *painter,
    const QWidget *widget) const
{
    State state = option->state;
    QStyleOption dockWidgetHandle = *option;
    bool horizontal = state & State_Horizontal;
    if (horizontal) {
        dockWidgetHandle.state &= ~State_Horizontal;
    } else {
        dockWidgetHandle.state |= State_Horizontal;
    }
    drawControl(CE_Splitter, &dockWidgetHandle, painter, widget);
    return true;
}

bool
Style::drawPrimitiveButtonTool(PrimitiveElement element,
                               const QStyleOption *option, QPainter *painter,
                               const QWidget *widget) const
{
    State state = option->state;
    const QRect &r = option->rect;
    if (oneOf(element, PE_FrameButtonTool, PE_PanelButtonTool)) {
        if (isMultiTabBarTab(getButton(widget, painter))) {
            if (!opts.stdSidebarButtons) {
                drawSideBarButton(painter, r, option, widget);
            } else if ((state & State_Enabled) ||
                       !(state & State_AutoRaise)) {
                QStyleOption opt(*option);
                opt.state |= STATE_TBAR_BUTTON;
                drawPrimitive(PE_PanelButtonCommand, &opt, painter, widget);
            }
            return true;
        }
    }
    bool dwt = widget && widget->inherits("QDockWidgetTitleButton");
    bool koDwt = (!dwt && widget && widget->parentWidget() &&
                  widget->parentWidget()->inherits("KoDockWidgetTitleBar"));
    if (((state & State_Enabled) || !(state & State_AutoRaise)) &&
        (!widget || !(dwt || koDwt) || (state & State_MouseOver))) {
        QStyleOption opt(*option);
        if (dwt || koDwt) {
            opt.state |= STATE_DWT_BUTTON;
        }
        drawPrimitive(PE_PanelButtonCommand, &opt, painter, widget);
    }
    return true;
}

bool
Style::drawPrimitiveFrameDockWidget(PrimitiveElement,
                                    const QStyleOption *option,
                                    QPainter *painter,
                                    const QWidget*) const
{
    const QRect &r = option->rect;
    const QColor *use = backgroundColors(option);
    painter->setPen(use[0]);
    painter->drawLine(r.x(), r.y(), r.x() + r.width() - 1, r.y());
    painter->drawLine(r.x(), r.y(), r.x(), r.y() + r.height() - 1);
    painter->setPen(use[opts.appearance == APPEARANCE_FLAT ? ORIGINAL_SHADE :
                        QTC_STD_BORDER]);
    painter->drawLine(r.x(), r.y() + r.height() - 1, r.x() + r.width() - 1,
                      r.y() + r.height() - 1);
    painter->drawLine(r.x() + r.width() - 1, r.y(), r.x() + r.width() - 1,
                      r.y() + r.height() - 1);
    return true;
}

bool
Style::drawPrimitiveFrameStatusBarOrMenu(PrimitiveElement element,
                                         const QStyleOption *option,
                                         QPainter *painter,
                                         const QWidget *widget) const
{
    if (element == PE_FrameStatusBar && !opts.drawStatusBarFrames) {
        return true;
    }
    const QRect &r = option->rect;
    if ((opts.square & SQUARE_POPUP_MENUS) &&
        (qtcIsFlatBgnd(opts.menuBgndAppearance) ||
         (opts.gtkComboMenus && widget && widget->parent() &&
          qobject_cast<const QComboBox*>(widget->parent())))) {
        const QColor *use = popupMenuCols(option);
        EGradientBorder border =
            qtcGetGradient(opts.menuBgndAppearance, &opts)->border;
        painter->setPen(use[QTC_STD_BORDER]);
        drawRect(painter, r);

        if (qtcUseBorder(border) &&
            opts.menuBgndAppearance != APPEARANCE_FLAT) {
            painter->setPen(use[0]);
            if (border == GB_LIGHT) {
                drawRect(painter, r.adjusted(1, 1, -1, -1));
            } else {
                if (border != GB_3D) {
                    painter->drawLine(r.x() + 1, r.y() + 1,
                                      r.x() + r.width() - 2, r.y() + 1);
                    painter->drawLine(r.x() + 1, r.y() + 1, r.x() + 1,
                                      r.y() + r.height() - 2);
                }
                painter->setPen(use[FRAME_DARK_SHADOW]);
                painter->drawLine(r.x() + 1, r.y() + r.height() - 2,
                                  r.x() + r.width() - 2,
                                  r.y() + r.height() - 2);
                painter->drawLine(r.x() + r.width() - 2, r.y() + 1,
                                  r.x() + r.width() - 2,
                                  r.y() + r.height() - 2);
            }
        }
    }
    return true;
}

bool
Style::drawPrimitiveFrameTabBarBase(PrimitiveElement,
                                    const QStyleOption *option,
                                    QPainter *painter,
                                    const QWidget *widget) const
{
    bool reverse = option->direction == Qt::RightToLeft;
    if (auto tbb = styleOptCast<QStyleOptionTabBarBase>(option)) {
        if (noneOf(tbb->shape, QTabBar::RoundedNorth, QTabBar::RoundedWest,
                   QTabBar::RoundedSouth, QTabBar::RoundedEast)) {
            return false;
        } else {
            static const int constSidePad = 16 * 2;
            const QColor *use(backgroundColors(option));
            QRegion region(tbb->rect);
            QLine topLine(tbb->rect.bottomLeft() - QPoint(0, 1),
                          tbb->rect.bottomRight() - QPoint(0, 1));
            QLine bottomLine(tbb->rect.bottomLeft(), tbb->rect.bottomRight());
            bool horiz = oneOf(tbb->shape, QTabBar::RoundedNorth,
                               QTabBar::RoundedSouth);
            double size = horiz ? tbb->rect.width() : tbb->rect.height();
            double tabRectSize = (horiz ? tbb->tabBarRect.width() :
                                  tbb->tabBarRect.height());
            double tabFadeSize = (tabRectSize + constSidePad > size ? 0.0 :
                                  1.0 - (tabRectSize + constSidePad) / size);
            double minFadeSize = 1.0 - (size - constSidePad) / size;
            double fadeSizeStart = minFadeSize;
            double fadeSizeEnd = (tabFadeSize < minFadeSize ? minFadeSize :
                                  (tabFadeSize > FADE_SIZE ? FADE_SIZE :
                                   tabFadeSize));
            if (reverse && horiz) {
                fadeSizeStart = fadeSizeEnd;
                fadeSizeEnd = minFadeSize;
            }
            region -= tbb->tabBarRect;
            painter->setClipRegion(region);
            bool fadeState = true;
            bool fadeEnd = true;
            // Dont fade start/end of tabbar in KDevelop's menubar
            if (theThemedApp == APP_KDEVELOP &&
                qtcCheckType<QMenuBar>(getParent<2>(widget)) &&
                qobject_cast<const QTabBar*>(widget)) {
                fadeState = fadeEnd = false;
            }
            drawFadedLine(painter, QRect(topLine.p1(), topLine.p2()),
                          tbb->shape == QTabBar::RoundedSouth &&
                          opts.appearance == APPEARANCE_FLAT ?
                          option->palette.background().color() :
                          use[tbb->shape == QTabBar::RoundedNorth ?
                              QTC_STD_BORDER :
                              (opts.borderTab ? 0 : FRAME_DARK_SHADOW)],
                          fadeState, fadeEnd, horiz,
                          fadeSizeStart, fadeSizeEnd);
            if (!(opts.thin & THIN_FRAMES)) {
                drawFadedLine(painter, QRect(bottomLine.p1(), bottomLine.p2()),
                              use[tbb->shape == QTabBar::RoundedNorth ?
                                  0 : QTC_STD_BORDER],
                              fadeState, fadeEnd, horiz, fadeSizeStart,
                              fadeSizeEnd);
            }
        }
    }
    return true;
}
}
