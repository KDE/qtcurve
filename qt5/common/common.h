/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
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

#ifndef __COMMON_H__
#define __COMMON_H__

#include "config.h"
#include <qtcurve-utils/shade.h>
#include <qtcurve-utils/utils.h>

/*
    The following #define disables the rounding when scrollbar type==none.
#define SIMPLE_SCROLLBARS
*/

/*
    The following #define controls whether a scrollbar's slider should overlap
    the scrollbar buttons when at min/max. This removes the thick looking line
    between the slider and the buttons.
*/
#define INCREASE_SB_SLIDER

#include <QtConfig>
#include <QApplication>
#include <map>
#include <set>
#include <QString>

#define IS_BLACK(A) (0==(A).red() && 0==(A).green() && 0==(A).blue())

#include <QPixmap>
class QColor;

#include <QSet>
typedef QSet<QString> Strings;

#define SETTINGS_GROUP        "Settings"
#define KWIN_GROUP            "KWin"

/* qtc_<theme name>.themerc support */
#define KDE_PREFIX(V) (QTC_KDE4_PREFIX)
#define THEME_DIR    "/share/apps/kstyle/themes/"
#define THEME_DIR4   "/share/kde4/apps/kstyle/themes/"
#define THEME_PREFIX "qtc_"
#define THEME_SUFFIX ".themerc"
#define BORDER_SIZE_FILE "windowBorderSizes"

#define LV_SIZE      7

#define LARGE_ARR_WIDTH  7
#define LARGE_ARR_HEIGHT 4

#define NUM_EXTRA_SHADES 3

enum {
    ALPHA_ETCH_LIGHT = 0,
    ALPHA_ETCH_DARK,
    NUM_STD_ALPHAS
};

#define TOTAL_SHADES (QTC_NUM_STD_SHADES + NUM_EXTRA_SHADES)
#define ORIGINAL_SHADE   TOTAL_SHADES

#define SHADE_ORIG_HIGHLIGHT (QTC_NUM_STD_SHADES)
#define SHADE_4_HIGHLIGHT (QTC_NUM_STD_SHADES + 1)
#define SHADE_2_HIGHLIGHT (QTC_NUM_STD_SHADES + 2)

#define DEFAULT_CONTRAST 7

#define THIN_SBAR_MOD  ((opts.sliderWidth<DEFAULT_SLIDER_WIDTH ? 3 : opts.sliderWidth>DEFAULT_SLIDER_WIDTH ? (opts.sliderWidth-9)/2 : 4)+(EFFECT_NONE==opts.buttonEffect ? 1 : 0))
#define SLIDER_SIZE (opts.sliderWidth<DEFAULT_SLIDER_WIDTH ? DEFAULT_SLIDER_WIDTH-2 : opts.sliderWidth)
#define CIRCULAR_SLIDER_SIZE 15
#define GLOW_MO           1 /*ORIGINAL_SHADE*/
#define GLOW_DEFBTN       1
#define GLOW_ALPHA(DEF)   ((DEF) ? 0.5 : 0.65)
#define DEF_BNT_TINT      0.4
#define ENTRY_INNER_ALPHA 0.4
#define INACTIVE_SEL_ALPHA 0.5

#define SUNKEN_BEVEL_DARK_ALPHA(X)  (X.value()/800.0) // 0.25
#define SUNKEN_BEVEL_LIGHT_ALPHA(X) (X.value()/500.0) // 0.40

#define BLEND_TITLEBAR     (opts.menubarAppearance==opts.titlebarAppearance && opts.menubarAppearance==opts.inactiveTitlebarAppearance && \
                           !(opts.windowBorder&WINDOW_BORDER_BLEND_TITLEBAR) && SHADE_WINDOW_BORDER==opts.shadeMenubars && opts.windowDrag)

#define PBAR_BORDER        4
#define ARROW_MO_SHADE     4
#define LOWER_BORDER_ALPHA 0.35
#define BORDER_VAL(E) (/*(E) ?*/ QTC_STD_BORDER/* : DISABLED_BORDER*/)
#define SLIDER_MO_BORDER_VAL 3

#define FRAME_DARK_SHADOW 2
#define FOCUS_SHADE(SEL)         (FOCUS_GLOW==opts.focus ? GLOW_MO : ((SEL) ? 3 : ORIGINAL_SHADE))
#define MENU_STRIPE_SHADE (opts.lighterPopupMenuBgnd ? ORIGINAL_SHADE : 2)
#define MENU_SEP_SHADE    (opts.lighterPopupMenuBgnd ? 4 : 3)

#define BGND_STRIPE_SHADE 0.95

#define INVERT_SHADE(A) (1.0+(1.0-(A)))

#define TOOLBAR_SEP_GAP        (opts.fadeLines ? 5 : 6)
#define FADE_SIZE              0.4

#define IS_GLASS(A) qtcOneOf(A, APPEARANCE_DULL_GLASS, APPEARANCE_SHINY_GLASS)
#define IS_CUSTOM(A) ((A)>=APPEARANCE_CUSTOM1 && (A)<(APPEARANCE_CUSTOM1+NUM_CUSTOM_GRAD))

#define MENUBAR_DARK_LIMIT 160
#define TOO_DARK(A) ((A).red()<MENUBAR_DARK_LIMIT || (A).green()<MENUBAR_DARK_LIMIT || (A).blue()<MENUBAR_DARK_LIMIT)

#define TO_FACTOR(A) ((100.0+((double)(A)))/100.0)
#define DEFAULT_HIGHLIGHT_FACTOR                   3
#define DEFAULT_SPLITTER_HIGHLIGHT_FACTOR          3
#define DEFAULT_CR_HIGHLIGHT_FACTOR                0
#define DEFAULT_EXPANDER_HIGHLIGHT_FACTOR          3
#define MAX_HIGHLIGHT_FACTOR                      50
#define MIN_HIGHLIGHT_FACTOR                     -50
#define MENUBAR_DARK_FACTOR        TO_FACTOR(-3)
#define LV_HEADER_DARK_FACTOR      TO_FACTOR(-10)
#define DEF_POPUPMENU_LIGHT_FACTOR                 2
#define MIN_LIGHTER_POPUP_MENU                  -100
#define MAX_LIGHTER_POPUP_MENU                   100

#define MIN_GB_FACTOR -50
#define MAX_GB_FACTOR  50
#define DEF_GB_FACTOR  -3

#define TO_ALPHA(A) (qtcAbs(A) / 100.0)
#define DEF_COLOR_SEL_TAB_FACTOR  25
#define MIN_COLOR_SEL_TAB_FACTOR   0
#define MAX_COLOR_SEL_TAB_FACTOR 100

#define DEF_TAB_BGND         0
#define MIN_TAB_BGND        -5
#define MAX_TAB_BGND         5

#define DEFAULT_MENU_DELAY 225
#define MIN_MENU_DELAY       1
#define MAX_MENU_DELAY     500

#define DEFAULT_SLIDER_WIDTH  15
#define MIN_SLIDER_WIDTH_ROUND 7
#define MIN_SLIDER_WIDTH_THIN_GROOVE 9
#define MIN_SLIDER_WIDTH       5
#define MAX_SLIDER_WIDTH      31

#define SIZE_GRIP_SIZE 12

#define USE_GLOW_FOCUS(mouseOver) (FOCUS_GLOW==opts.focus && (MO_GLOW!=opts.coloredMouseOver || !(mouseOver)))

#define MENUBAR_GLASS_SELECTED_DARK_FACTOR 0.9
#define MENUITEM_FADE_SIZE 48

#define NUM_SPLITTER_DASHES 21

#define WIDGET_BUTTON(w)                                                \
    qtcOneOf(w, WIDGET_STD_BUTTON, WIDGET_DEF_BUTTON, WIDGET_CHECKBOX,  \
             WIDGET_RADIO_BUTTON, WIDGET_DIAL, WIDGET_COMBO,            \
             WIDGET_COMBO_BUTTON, WIDGET_MDI_WINDOW_BUTTON,             \
             WIDGET_TOOLBAR_BUTTON)
#define ETCH_WIDGET(w)                                                  \
    (qtcOneOf(w, WIDGET_STD_BUTTON, WIDGET_DEF_BUTTON, WIDGET_SLIDER_TROUGH, \
              WIDGET_CHECKBOX, WIDGET_RADIO_BUTTON, WIDGET_DIAL,        \
              WIDGET_FILLED_SLIDER_TROUGH, WIDGET_MDI_WINDOW_BUTTON,    \
              WIDGET_TOOLBAR_BUTTON) ||                                 \
     ((w) == WIDGET_SLIDER && opts.coloredMouseOver == MO_GLOW))

#define SLIDER(w) qtcOneOf(w, WIDGET_SB_SLIDER, WIDGET_SLIDER)
#define CIRCULAR_SLIDER(w) (WIDGET_SLIDER==(w) && SLIDER_CIRCULAR==opts.sliderStyle)

#define MODIFY_AGUA_X(A, X) (APPEARANCE_AGUA==(A) ?  (X) : (A))
#define MODIFY_AGUA(A)      MODIFY_AGUA_X((A), APPEARANCE_AGUA_MOD)
#define AGUA_MAX 32.0
#define AGUA_MID_SHADE 0.85

#define COLORED_BORDER_SIZE 3
#define PROGRESS_CHUNK_WIDTH 10
#define STRIPE_WIDTH 10
#define DRAW_LIGHT_BORDER(SUKEN, WIDGET, APP)                           \
    (!(SUKEN) && (qtcGetGradient(APP, &opts)->border == GB_LIGHT) &&    \
     (WIDGET) != WIDGET_MENU_ITEM && !IS_TROUGH(WIDGET) &&              \
     ((WIDGET) != WIDGET_DEF_BUTTON || opts.defBtnIndicator != IND_COLORED))

#define DRAW_3D_FULL_BORDER(SUNKEN, APP) \
    (!(SUNKEN) && GB_3D_FULL==qtcGetGradient((APP), &opts)->border)

#define DRAW_3D_BORDER(SUNKEN, APP) \
    (!(SUNKEN) && GB_3D==qtcGetGradient((APP), &opts)->border)

#define DRAW_SHINE(SUNKEN, APP) \
    (!(SUNKEN) && GB_SHINE==qtcGetGradient((APP), &opts)->border)

#define LIGHT_BORDER(APP) (APPEARANCE_DULL_GLASS==(APP) ? 1 : 0)

#define PROGRESS_ANIMATION 100
#define MIN_SLIDER_SIZE(A) (LINE_DOTS==(A) ? 24 : 20)

#define CR_SMALL_SIZE 13
#define CR_LARGE_SIZE 15

#define TAB_APP(A)                                                      \
    (qtcOneOf(A, APPEARANCE_BEVELLED, APPEARANCE_SPLIT_GRADIENT) ?      \
     APPEARANCE_GRADIENT : (A))
#define NORM_TAB_APP TAB_APP(opts.tabAppearance)
#define SEL_TAB_APP  TAB_APP(opts.activeTabAppearance)

#define SLIDER_MO_SHADE  (SHADE_SELECTED==opts.shadeSliders ? 1 : (SHADE_BLEND_SELECTED==opts.shadeSliders ? 0 : ORIGINAL_SHADE))
#define SLIDER_MO_PLASTIK_BORDER                                        \
    (qtcOneOf(opts.shadeSliders, SHADE_SELECTED, SHADE_BLEND_SELECTED) ? 2 : 1)
#define SLIDER_MO_LEN                                   \
    (opts.sliderStyle == SLIDER_TRIANGULAR ? 2 :        \
     (qtcOneOf(opts.shadeSliders, SHADE_SELECTED,       \
               SHADE_BLEND_SELECTED) ? 4 : 3))
#define SB_SLIDER_MO_LEN(A)                                             \
    ((A) < 22 && !(opts.round >= ROUND_FULL) ? 2 :                      \
     ((A) < 32 || qtcNoneOf(opts.shadeSliders, SHADE_SELECTED,          \
                            SHADE_BLEND_SELECTED) ? 4 : 6))

#define CR_MO_FILL          1
#define MO_DEF_BTN          2
#define MO_PLASTIK_DARK(W)  (WIDGET_DEF_BUTTON==(W) && IND_COLORED==opts.defBtnIndicator ? 3 : 2) /*? 2 : 1) */
#define MO_PLASTIK_LIGHT(W) (WIDGET_DEF_BUTTON==(W) && IND_COLORED==opts.defBtnIndicator ? 4 : 1) /*? 2 : 0) */

#define MO_STD_DARK(W)                                          \
    (opts.coloredMouseOver == MO_GLOW ? 1 : MO_PLASTIK_DARK(W))
#define MO_STD_LIGHT(W, S)                                              \
    (opts.coloredMouseOver == MO_GLOW ? 1 : MO_PLASTIK_LIGHT(W))

#define FOCUS_ALPHA              0.08
#define BORDER_BLEND_ALPHA(W)                                   \
    (qtcOneOf(W, WIDGET_ENTRY, WIDGET_SCROLLVIEW) ? 0.45 : 0.7)

#define ETCH_TOP_ALPHA           0.055
#define ETCH_BOTTOM_ALPHA        0.1
// #define ETCH_RADIO_TOP_ALPHA     0.055
#define ETCH_RADIO_TOP_ALPHA     0.09

#define RINGS_INNER_ALPHA(T) qtc_ring_alpha[IMG_PLAIN_RINGS==(T) ? 1 : 0] //(IMG_PLAIN_RINGS==opts.bgndImage.type ? 0.25 :  0.125)
#define RINGS_OUTER_ALPHA    qtc_ring_alpha[2] //0.5
#define RINGS_WIDTH(T)       (IMG_SQUARE_RINGS==T ? 260 : 450)
#define RINGS_HEIGHT(T)      (IMG_SQUARE_RINGS==T ? 220 : 360)

#define RINGS_SQUARE_LARGE_ALPHA (RINGS_OUTER_ALPHA*0.675)
#define RINGS_SQUARE_SMALL_ALPHA (RINGS_OUTER_ALPHA*0.50)
#define RINGS_SQUARE_LINE_WIDTH  20.0
#define RINGS_SQUARE_RADIUS      18.0
#define RINGS_SQUARE_LARGE_SIZE  120.0
#define RINGS_SQUARE_SMALL_SIZE  100.0
#define GLOW_PROG_ALPHA 0.55

#include <QStyle>
typedef enum {
    QtC_Round = QStyle::PM_CustomBase,
    QtC_TitleAlignment,
    QtC_TitleBarButtons,
    QtC_TitleBarIcon,
    QtC_TitleBarIconColor,
    QtC_TitleBarEffect,
    QtC_BlendMenuAndTitleBar,
    QtC_ShadeMenubarOnlyWhenActive,
    QtC_ToggleButtons,
    QtC_MenubarColor,
    QtC_WindowBorder,
    QtC_CustomBgnd,
    QtC_TitleBarApp
} QtCMetrics;

#define QtC_StateKWin            ((QStyle::StateFlag)0x10000000)
// PE_FrameWindow
#define QtC_StateKWinNotFull     ((QStyle::StateFlag)0x20000000)
// CC_TitleBar
#define QtC_StateKWinFillBgnd    ((QStyle::StateFlag)0x20000000)
#define QtC_StateKWinNoBorder    ((QStyle::StateFlag)0x40000000)
#define QtC_StateKWinCompositing ((QStyle::StateFlag)0x80000000)
#define QtC_StateKWinTabDrag     ((QStyle::StateFlag)0x00000001)

#define QtC_PE_DrawBackground    ((QStyle::PrimitiveElement)(QStyle::PE_CustomBase+10000))

#define CLOSE_COLOR              QColor(191, 82, 82)
#define DARK_WINDOW_TEXT(A)  ((A).red()<230 || (A).green()<230 || (A).blue()<230)
#define HOVER_BUTTON_ALPHA(A)    (DARK_WINDOW_TEXT(A) ? 0.25 : 0.65)
#define WINDOW_TEXT_SHADOW_ALPHA(A) (EFFECT_SHADOW==(A) ? 0.10 : 0.60)
#define WINDOW_SHADOW_COLOR(A)      (EFFECT_SHADOW==(A) ? Qt::black : Qt::white)

#define QTCURVE_PREVIEW_CONFIG      "QTCURVE_PREVIEW_CONFIG"
#define QTCURVE_PREVIEW_CONFIG_FULL "QTCURVE_PREVIEW_CONFIG_FULL"

typedef enum {
    DWT_BUTTONS_AS_PER_TITLEBAR    = 0x0001,
    DWT_COLOR_AS_PER_TITLEBAR      = 0x0002,
    DWT_FONT_AS_PER_TITLEBAR       = 0x0004,
    DWT_TEXT_ALIGN_AS_PER_TITLEBAR = 0x0008,
    DWT_EFFECT_AS_PER_TITLEBAR     = 0x0010,
    DWT_ROUND_TOP_ONLY             = 0x0020,
    DWT_ICON_COLOR_AS_PER_TITLEBAR = 0x0040
} EDwtSettingsFlags;

typedef enum {
    TITLEBAR_BUTTON_ROUND                   = 0x0001,
    TITLEBAR_BUTTON_HOVER_FRAME             = 0x0002,
    TITLEBAR_BUTTON_HOVER_SYMBOL            = 0x0004,
    TITLEBAR_BUTTON_NO_FRAME                = 0x0008,
    TITLEBAR_BUTTON_COLOR                   = 0x0010,
    TITLEBAR_BUTTON_COLOR_INACTIVE          = 0x0020,
    TITLEBAR_BUTTON_COLOR_MOUSE_OVER        = 0x0040,
    TITLEBAR_BUTTON_STD_COLOR               = 0x0080,
    TITLEBAR_BUTTON_COLOR_SYMBOL            = 0x0100,
    TITLEBAR_BUTTON_HOVER_SYMBOL_FULL       = 0x0200,
    TITLEBAR_BUTTON_SUNKEN_BACKGROUND       = 0x0400,
    TITLEBAR_BUTTOM_ARROW_MIN_MAX           = 0x0800,
    TITLEBAR_BUTTOM_HIDE_ON_INACTIVE_WINDOW = 0x1000,
    TITLEBAR_BUTTON_ICON_COLOR              = 0x2000,
    TITLEBAR_BUTTON_USE_HOVER_COLOR         = 0x4000
} ETitleBarButtonFlags;

typedef enum {
    TITLEBAR_ICON_NONE,
    TITLEBAR_ICON_MENU_BUTTON,
    TITLEBAR_ICON_NEXT_TO_TITLE
} ETitleBarIcon;

typedef enum {
    TITLEBAR_CLOSE,
    TITLEBAR_MIN,
    TITLEBAR_MAX,
    TITLEBAR_HELP,
    TITLEBAR_MENU,
    TITLEBAR_SHADE,
    TITLEBAR_ALL_DESKTOPS,
    TITLEBAR_KEEP_ABOVE,
    TITLEBAR_KEEP_BELOW,
    NUM_TITLEBAR_BUTTONS
} ETitleBarButtons;

#define TBAR_VERSION_HACK        65535
#define TBAR_BORDER_VERSION_HACK (TBAR_VERSION_HACK+1000)

typedef std::map<int, QColor> TBCols;

typedef enum
{
    WINDOW_BORDER_COLOR_TITLEBAR_ONLY            = 0x01, // colorTitlebarOnly
    WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR = 0x02, // titlebarMenuColor
    WINDOW_BORDER_ADD_LIGHT_BORDER               = 0x04, // titlebarBorder
    WINDOW_BORDER_BLEND_TITLEBAR                 = 0x08, // titlebarBlend
    WINDOW_BORDER_SEPARATOR                      = 0x10,
    WINDOW_BORDER_FILL_TITLEBAR                  = 0x20
} EWindowBorder;

typedef enum
{
    IMG_NONE,
    IMG_BORDERED_RINGS,
    IMG_PLAIN_RINGS,
    IMG_SQUARE_RINGS,
    IMG_FILE
} EImageType;

typedef struct {
    QString   file;
    QPixmap   img;
} QtCPixmap;

typedef enum {
    PP_TL,
    PP_TM,
    PP_TR,
    PP_BL,
    PP_BM,
    PP_BR,
    PP_LM,
    PP_RM,
    PP_CENTRED,
} EPixPos;

typedef struct
{
    EImageType type;
    bool       loaded,
               onBorder;
    QtCPixmap  pixmap;
    int        width,
               height;
    EPixPos    pos;
} QtCImage;

typedef enum
{
    THIN_BUTTONS    = 0x0001,
    THIN_MENU_ITEMS = 0x0002,
    THIN_FRAMES     = 0x0004
} EThinFlags;

typedef enum
{
    SQUARE_NONE               = 0x0000,
    SQUARE_ENTRY              = 0x0001,
    SQUARE_PROGRESS           = 0x0002,
    SQUARE_SCROLLVIEW         = 0x0004,
    SQUARE_LISTVIEW_SELECTION = 0x0008,
    SQUARE_FRAME              = 0x0010,
    SQUARE_TAB_FRAME          = 0x0020,
    SQUARE_SLIDER             = 0x0040,
    SQUARE_SB_SLIDER          = 0x0080,
    SQUARE_WINDOWS            = 0x0100,
    SQUARE_TOOLTIPS           = 0x0200,
    SQUARE_POPUP_MENUS        = 0x0400,

    SQUARE_ALL                = 0xFFFF
} ESquare;

typedef enum
{
    WM_DRAG_NONE             = 0,
    WM_DRAG_MENUBAR          = 1,
    WM_DRAG_MENU_AND_TOOLBAR = 2,
    WM_DRAG_ALL              = 3
} EWmDrag;

typedef enum
{
    EFFECT_NONE,
    EFFECT_ETCH,
    EFFECT_SHADOW
} EEffect;

typedef enum {
    PIX_CHECK,
    PIX_DOT
} EPixmap;

typedef enum
{
    WIDGET_TAB_TOP,
    WIDGET_TAB_BOT,
    WIDGET_STD_BUTTON,
    WIDGET_DEF_BUTTON,
    WIDGET_TOOLBAR_BUTTON,
    WIDGET_LISTVIEW_HEADER,
    WIDGET_SLIDER,
    WIDGET_SLIDER_TROUGH,
    WIDGET_FILLED_SLIDER_TROUGH,
    WIDGET_SB_SLIDER,
    WIDGET_SB_BUTTON,
    WIDGET_SB_BGND,
    WIDGET_TROUGH,
    WIDGET_CHECKBOX,
    WIDGET_RADIO_BUTTON,
    WIDGET_COMBO,
    WIDGET_COMBO_BUTTON,
    WIDGET_MENU_ITEM,
    WIDGET_PROGRESSBAR,
    WIDGET_PBAR_TROUGH,
#ifndef QTC_UTILS_QT
    WIDGET_ENTRY_PROGRESSBAR,
    WIDGET_TOGGLE_BUTTON,
    WIDGET_SPIN_UP,
    WIDGET_SPIN_DOWN,
    WIDGET_UNCOLOURED_MO_BUTTON,
#else // QTC_UTILS_QT
    WIDGET_CHECKBUTTON,        // Qt4 only
    WIDGET_MDI_WINDOW,         // Qt4 only
    WIDGET_MDI_WINDOW_TITLE,   // Qt4 only
    WIDGET_MDI_WINDOW_BUTTON,  // Qt4 only
    WIDGET_DOCK_WIDGET_TITLE,
    WIDGET_DIAL,
#endif // QTC_UTILS_QT
    WIDGET_SPIN,
    WIDGET_ENTRY,
    WIDGET_SCROLLVIEW,
    WIDGET_SELECTION,
    WIDGET_FRAME,
    WIDGET_NO_ETCH_BTN,
    WIDGET_MENU_BUTTON,        // Qt4 only
    WIDGET_FOCUS,
    WIDGET_TAB_FRAME,
    WIDGET_TOOLTIP,
    WIDGET_OTHER
} EWidget;

typedef enum
{
    APP_ALLOW_BASIC,
    APP_ALLOW_FADE,
    APP_ALLOW_STRIPED,
    APP_ALLOW_NONE
} EAppAllow;

#define IS_SLIDER(w) qtcOneOf(w, WIDGET_SLIDER, WIDGET_SB_SLIDER)
#define IS_TROUGH(w) qtcOneOf(w, WIDGET_SLIDER_TROUGH, WIDGET_PBAR_TROUGH, \
                              WIDGET_TROUGH, WIDGET_FILLED_SLIDER_TROUGH)

typedef enum {
    IND_CORNER,
    IND_FONT_COLOR,
    IND_COLORED,
    IND_TINT,
    IND_GLOW,
    IND_DARKEN,
    IND_SELECTED,
    IND_NONE
} EDefBtnIndicator;

typedef enum
{
    LINE_NONE,
    LINE_SUNKEN,
    LINE_FLAT,
    LINE_DOTS,
    LINE_1DOT,
    LINE_DASHES,
} ELine;

typedef enum
{
    TB_NONE,
    TB_LIGHT,
    TB_DARK,
    TB_LIGHT_ALL,
    TB_DARK_ALL
} ETBarBorder;

typedef enum
{
    TBTN_STANDARD,
    TBTN_RAISED,
    TBTN_JOINED
} ETBarBtn;

typedef enum
{
    BORDER_FLAT,
    BORDER_RAISED,
    BORDER_SUNKEN,
    BORDER_LIGHT
} EBorder;

/*
    This whole EShade enum is a complete mess!
    For menubars, we dont blend - so blend is selected, and selected is darken
    For check/radios - we dont blend, so blend is selected, and we dont allow darken
*/
typedef enum {
    SHADE_NONE,
    SHADE_CUSTOM,
    SHADE_SELECTED,
    SHADE_BLEND_SELECTED,
    SHADE_DARKEN,
    SHADE_WINDOW_BORDER
} EShade;

typedef enum {
    ECOLOR_BASE,
    ECOLOR_BACKGROUND,
    ECOLOR_DARK,
} EColor;

typedef enum {
    ROUND_NONE,
    ROUND_SLIGHT,
    ROUND_FULL,
    ROUND_EXTRA,
    ROUND_MAX
} ERound;

typedef enum {
    GB_LBL_BOLD = 0x01,
    GB_LBL_CENTRED = 0x02,
    GB_LBL_INSIDE = 0x04,
    GB_LBL_OUTSIDE = 0x08
} EGBLabel;

typedef enum {
    MO_NONE,
    MO_COLORED,
    MO_COLORED_THICK,
    MO_PLASTIK,
    MO_GLOW
} EMouseOver;

typedef enum {
    STRIPE_NONE,
    STRIPE_PLAIN,
    STRIPE_DIAGONAL,
    STRIPE_FADE
} EStripe;

typedef enum {
    SLIDER_PLAIN,
    SLIDER_ROUND,
    SLIDER_PLAIN_ROTATED,
    SLIDER_ROUND_ROTATED,
    SLIDER_TRIANGULAR,
    SLIDER_CIRCULAR
} ESliderStyle;

typedef enum {
    FOCUS_STANDARD,
    FOCUS_RECTANGLE,
    FOCUS_FULL,
    FOCUS_FILLED,
    FOCUS_LINE,
    FOCUS_GLOW,
    FOCUS_NONE
} EFocus;

typedef enum {
    TAB_MO_TOP,
    TAB_MO_BOTTOM,
    TAB_MO_GLOW
} ETabMo;

typedef enum {
    GT_HORIZ,
    GT_VERT
} EGradType;

typedef enum {
    GLOW_NONE,
    GLOW_START,
    GLOW_MIDDLE,
    GLOW_END
} EGlow;

enum
{
    HIDE_NONE     = 0x00,
    HIDE_KEYBOARD = 0x01,
    HIDE_KWIN     = 0x02
};

typedef enum {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_FULL_CENTER,
    ALIGN_RIGHT
} EAlign;

struct GradientStop {
    GradientStop(double p=0.0, double v=0.0, double a=1.0) : pos(p), val(v), alpha(a) { }

    bool operator==(const GradientStop &o) const
    {
        return (qtcEqual(pos, o.pos) && qtcEqual(val, o.val) &&
                qtcEqual(alpha, o.alpha));
    }

    bool operator<(const GradientStop &o) const
    {
        return pos<o.pos || (qtcEqual(pos, o.pos) &&
                             (val < o.val || (qtcEqual(val, o.val) &&
                                              alpha < o.alpha)));
    }

    double pos,
           val,
           alpha;
};

typedef struct
{
    int titleHeight,
        toolTitleHeight,
        bottom,
        sides;
} WindowBorders;

struct GradientStopCont : public std::set<GradientStop>
{
    GradientStopCont fix() const
    {
        GradientStopCont c(*this);
        if(size())
        {
            GradientStopCont::const_iterator   first(c.begin());
            GradientStopCont::reverse_iterator last(c.rbegin());

            if((*first).pos>0.001)
                c.insert(GradientStop(0.0, 1.0));
            if((*last).pos<0.999)
                c.insert(GradientStop(1.0, 1.0));
        }
        return c;
    }
};
struct Gradient {
    Gradient() : border(GB_3D) { }

    bool operator==(const Gradient &o) const
    {
        return border==o.border && stops==o.stops;
    }
    EGradientBorder  border;
    GradientStopCont stops;
};

#define USE_CUSTOM_SHADES(A) ((A).customShades[0]>0.00001)
#define USE_CUSTOM_ALPHAS(A) ((A).customAlphas[0]>0.00001)

typedef std::map<EAppearance, Gradient> GradientCont;
typedef struct {
    int              version,
                     contrast,
                     passwordChar,
                     highlightFactor,
                     lighterPopupMenuBgnd,
                     menuDelay,
                     sliderWidth,
                     tabBgnd,
                     colorSelTab,
                     expanderHighlight,
                     crHighlight,
                     splitterHighlight,
                     crSize,
                     gbFactor,
                     gbLabel,
                     thin;
    ERound           round;
    bool             embolden,
                     highlightTab,
                     roundAllTabs,
                     animatedProgress,
                     customMenuTextColor,
                     menubarMouseOver,
                     useHighlightForMenu,
                     shadeMenubarOnlyWhenActive,
                     lvButton,
                     drawStatusBarFrames,
                     fillSlider,
                     roundMbTopOnly,
                     gtkScrollViews,
                     stdSidebarButtons,
                     toolbarTabs,
                     gtkComboMenus,
                     mapKdeIcons,
                     gtkButtonOrder,
                     fadeLines,
                     reorderGtkButtons,
                     borderMenuitems,
                     colorMenubarMouseOver,
                     darkerBorders,
                     vArrows,
                     xCheck,
                     crButton,
                     smallRadio,
                     fillProgress,
                     comboSplitter,
                     highlightScrollViews,
                     etchEntry,
                     colorSliderMouseOver,
                     thinSbarGroove,
                     flatSbarButtons,
                     borderSbarGroove,
                     borderProgress,
                     popupBorder,
                     unifySpinBtns,
                     unifyCombo,
                     unifySpin,
                     borderTab,
                     borderInactiveTab,
                     doubleGtkComboArrow,
                     menuIcons,
                     stdBtnSizes,
                     forceAlternateLvCols,
                     invertBotTab,
                     boldProgress,
                     coloredTbarMo,
                     borderSelection,
                     stripedSbar,
                     shadePopupMenu,
                     hideShortcutUnderline;
    EFrame           groupBox;
    EGlow            glowProgress;
    bool             lvLines;
    EGradType        bgndGrad,
                     menuBgndGrad;
    int              menubarHiding,
                     statusbarHiding,
                     square,
                     windowDrag,
                     windowBorder,
                     bgndOpacity,
                     menuBgndOpacity,
                     dlgOpacity;
    int              dwtSettings;
    int              titlebarButtons;
    TBCols           titlebarButtonColors;
    ETitleBarIcon    titlebarIcon;
    EStripe          stripedProgress;
    ESliderStyle     sliderStyle;
    EMouseOver       coloredMouseOver;
    ETBarBorder      toolbarBorders;
    ETBarBtn         tbarBtns;
    EDefBtnIndicator defBtnIndicator;
    ELine            sliderThumbs,
                     handles,
                     toolbarSeparators,
                     splitters;
    ETabMo           tabMouseOver;
/* NOTE: If add an appearance setting, increase the number of custmo gradients to match! */
    EAppearance      appearance,
                     bgndAppearance,
                     menuBgndAppearance,
                     menubarAppearance,
                     menuitemAppearance,
                     toolbarAppearance,
                     lvAppearance,
                     tabAppearance,
                     activeTabAppearance,
                     sliderAppearance,
                     titlebarAppearance,
                     inactiveTitlebarAppearance,
                     titlebarButtonAppearance,
                     dwtAppearance,
                     selectionAppearance,
                     menuStripeAppearance,
                     progressAppearance,
                     progressGrooveAppearance,
                     grooveAppearance,
                     sunkenAppearance,
                     sbarBgndAppearance,
                     sliderFill,
                     tooltipAppearance,
                     tbarBtnAppearance;
    EShade           shadeSliders,
                     shadeMenubars,
                     menuStripe,
                     shadeCheckRadio,
                     comboBtn,
                     sortedLv,
                     crColor,
                     progressColor;
    EColor           progressGrooveColor;
    EEffect          buttonEffect,
                     tbarBtnEffect;
    EScrollbar       scrollbarType;
    EFocus           focus;
    QColor customMenubarsColor,
                     customSlidersColor,
                     customMenuNormTextColor,
                     customMenuSelTextColor,
                     customMenuStripeColor,
                     customCheckRadioColor,
                     customComboBtnColor,
                     customSortedLvColor,
                     customCrBgndColor,
                     customProgressColor;
    EShading         shading;
    EAlign           titlebarAlignment;
    EEffect          titlebarEffect;
    bool             centerTabText;
    double           customShades[QTC_NUM_STD_SHADES],
                     customAlphas[NUM_STD_ALPHAS];
    GradientCont     customGradient;
    QtCPixmap        bgndPixmap;
    QtCPixmap        menuBgndPixmap;
    QtCImage         bgndImage,
                     menuBgndImage;
    /* NOTE: If add any more settings here, need to alter copyOpts/freeOpts/defaultSettings in config_file.c */
    Strings          noBgndGradientApps,
                     noBgndOpacityApps,
                     noMenuBgndOpacityApps,
                     noBgndImageApps;
    Strings          noMenuStripeApps;
    Strings          menubarApps,
                     statusbarApps,
                     useQtFileDialogApps,
                     windowDragWhiteList,
                     windowDragBlackList;
} Options;

void qtcSetupGradient(Gradient *grad, EGradientBorder border, int numStops, ...);
const Gradient *qtcGetGradient(EAppearance app, const Options *opts);

QTC_ALWAYS_INLINE static inline bool
qtcDrawMenuBorder(const Options *opts)
{
    return (opts->menuBgndAppearance != APPEARANCE_FLAT &&
            opts->version >= qtcMakeVersion(1, 7) &&
            qtcUseBorder(qtcGetGradient(opts->menuBgndAppearance,
                                        opts)->border));
}

QTC_ALWAYS_INLINE static inline bool
qtcIsCustomBgnd(const Options *opts)
{
    return (!qtcIsFlatBgnd(opts->bgndAppearance) ||
            opts->bgndImage.type != IMG_NONE ||
            opts->bgndOpacity != 100 || opts->dlgOpacity != 100);
}

#ifdef QTC_QT5_ENABLE_KDE
#include <kcolorutils.h>
#define tint(COLA, COLB, FACTOR) KColorUtils::tint((COLA), (COLB), (FACTOR))
#define midColor(COLA, COLB) KColorUtils::mix((COLA), (COLB), 0.5)
#else
#include <qtcurve-utils/color.h>
#define tint(COLA, COLB, FACTOR) qtcColorTint(&(COLA), &(COLB), (FACTOR))
#define midColor(COLA, COLB) qtcColorMix(&(COLA), &(COLB), 0.5)
#endif

EAppearance qtcWidgetApp(EWidget w, const Options *opts, bool active=true);

typedef enum
{
    RADIUS_SELECTION,
    RADIUS_INTERNAL,
    RADIUS_EXTERNAL,
    RADIUS_ETCH
} ERadius;

#define MIN_ROUND_MAX_HEIGHT    12
#define MIN_ROUND_MAX_WIDTH     24
#define BGND_SHINE_SIZE 300
#define BGND_SHINE_STEPS  8

#define MIN_ROUND_FULL_SIZE     8
#define MIN_ROUND_EXTRA_SIZE(W) (WIDGET_SPIN==(W) ? 7 : 14)

#define IS_MAX_ROUND_WIDGET(A)                          \
    qtcOneOf(A, WIDGET_STD_BUTTON, WIDGET_DEF_BUTTON)
#define IS_EXTRA_ROUND_WIDGET(A)                                        \
    qtcNoneOf(A, WIDGET_MENU_ITEM, WIDGET_TAB_FRAME, WIDGET_PBAR_TROUGH, \
              WIDGET_PROGRESSBAR, WIDGET_MDI_WINDOW, WIDGET_MDI_WINDOW_TITLE)

#define EXTRA_INNER_RADIUS   3.5
#define EXTRA_OUTER_RADIUS   4.5
#define EXTRA_ETCH_RADIUS    5.5
#define FULL_INNER_RADIUS    1.5
#define FULL_OUTER_RADIUS    2.5
#define FULL_ETCH_RADIUS     3.5

#define SLIGHT_INNER_RADIUS  0.75
#define SLIGHT_OUTER_RADIUS  1.75
#define SLIGHT_ETCH_RADIUS   2.75

#define MAX_RADIUS_INTERNAL 9.0
#define MAX_RADIUS_EXTERNAL (MAX_RADIUS_INTERNAL+2.0)

ERound qtcGetWidgetRound(const Options *opts, int w, int h, EWidget widget);
double qtcGetRadius(const Options *opts, int w, int h, EWidget widget,
                    ERadius rad);

#endif // __COMMON_H__
