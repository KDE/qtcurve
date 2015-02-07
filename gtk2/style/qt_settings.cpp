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

#include "config.h"

#include "qt_settings.h"

#include <qtcurve-utils/color.h>
#include <qtcurve-utils/log.h>
#include <qtcurve-utils/dirs.h>
#include <qtcurve-utils/strs.h>
#include <qtcurve-utils/process.h>

#include <common/config_file.h>
#include "helpers.h"
#include <dirent.h>
#include <locale.h>
#include <gmodule.h>

QtCPalette qtcPalette;
Options opts;

#define MAX_APP_NAME_LEN 32

#define KDE_CFG_DIR "/share/config/"
#define KDE4_SYS_CFG_DIR "/share/kde4/config/"
#define KDEGLOBALS_FILE "kdeglobals"
#define KDEGLOBALS_SYS_FILE "system.kdeglobals"

#define qtc_gtkrc_printf(args...)                       \
    gtk_rc_parse_string(QTC_LOCAL_BUFF_PRINTF(args))

#if defined(__MACH__) || defined(__APPLE__)
/* This code is public domain -- Will Hartung 4/9/09 */
// http://stackoverflow.com/questions/735126/are-there-alternate-implementations-of-gnu-getline-interface
#include <stdio.h>
#include <stdlib.h>
#include <AvailabilityMacros.h>

#if __MAC_OS_X_VERSION_MAX_ALLOWED < 1070
static size_t getline(char **lineptr, size_t *n, FILE *stream)
{
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL) {
        return -1;
    }
    if (stream == NULL) {
        return -1;
    }
    if (n == NULL) {
        return -1;
    }
    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    if (bufptr == NULL) {
        bufptr = malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
    while (c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size = size + 128;
            bufptr = realloc(bufptr, size);
            if (bufptr == NULL) {
                return -1;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}
#endif //pre 10.7
#endif

static char*
getKdeHome()
{
    static char *kdeHome = NULL;
    if (kdeHome) {
        return kdeHome;
    }
    size_t len = 0;
    kdeHome = qtcPopenStdout(
        "kde4-config", (const char *const[]){"kde4-config", "--expandvars",
                "--localprefix", NULL}, 300, &len);
    if (kdeHome && kdeHome[strspn(kdeHome, " \t\b\n\f\v")]) {
        if (kdeHome[len - 1] == '\n') {
            kdeHome[len - 1] = '\0';
        }
        return kdeHome;
    }
    kdeHome = getenv(getuid() ? "KDEHOME" : "KDEROOTHOME");
    if (!kdeHome) {
        // FIXME
        static char kdeHomeStr[MAX_CONFIG_FILENAME_LEN + 1];
        const char *home = qtcGetHome();
        if (strlen(home) < (MAX_CONFIG_FILENAME_LEN - 5)) {
            sprintf(kdeHomeStr, "%s/.kde", home);
            kdeHome = kdeHomeStr;
        }
    }
    return kdeHome;
}

static const char*
kdeFile(const char *f)
{
    static char kg[MAX_CONFIG_FILENAME_LEN + 1] = {'\0'};

    char *kdehome = getKdeHome();

    if (kdehome && strlen(kdehome) <
        (MAX_CONFIG_FILENAME_LEN - (strlen(KDE_CFG_DIR) + strlen(f)))) {
        sprintf(kg, "%s" KDE_CFG_DIR "%s", kdehome, f);
    }

    return kg;
}

static const char*
kdeGlobals()
{
    return kdeFile(KDEGLOBALS_FILE);
}

static const char*
kwinrc()
{
    return kdeFile("kwinrc");
}

#define HICOLOR_ICONS "hicolor"
#define HICOLOR_LEN 7
#define ICON_FOLDER "/share/icons/"
#define ICON_FOLDER_SLEN 13
#define DEFAULT_ICON_PREFIX "/usr/share/icons"
#define DEFAULT_ICON_PREFIX_LEN 16

static GdkColor
setGdkColor(int r, int g, int b)
{
    GdkColor col;

    col.red = toGtkColor(r);
    col.green = toGtkColor(g);
    col.blue = toGtkColor(b);
    return col;
}

#define DEFAULT_KDE_ACT_PAL                                           \
    "active=#000000^e#dddfe4^e#ffffff^e#ffffff^e#555555^e#c7c7c7^e"   \
    "#000000^e#ffffff^e#000000^e#ffffff^e#efefef^e#000000^e#678db2^e" \
    "#ffffff^e#0000ee^e#52188b^e"

#define DEFAULT_KDE_DIS_PAL                                           \
    "disabled=#000000^e#dddfe4^e#ffffff^e#ffffff^e#555555^e#c7c7c7^e" \
    "#c7c7c7^e#ffffff^e#000000^e#ffffff^e#efefef^e#000000^e#678db2^e" \
    "#ffffff^e#0000ee^e#52188b^e"

#ifdef READ_INACTIVE_PAL
#define DEFAULT_KDE_INACT_PAL                                         \
    "inactive=#000000^e#dddfe4^e#ffffff^e#ffffff^e#555555^e#c7c7c7^e" \
    "#000000^e#ffffff^e#000000^e#ffffff^e#efefef^e#000000^e#678db2^e" \
    "#ffffff^e#0000ee^e#52188b^e"
#endif

#define DEFAULT_KDE_FONT "Sans Serif"
#define DEFAULT_KDE_FONT_SIZE 10.0
#define MAX_LINE_LEN 1024

QtData qtSettings;

#define defaultIcons() ("oxygen")

enum {
    SECT_NONE = 0x000001,
    SECT_PALETTE = 0x000002,
    SECT_GENERAL = 0x000004,
    SECT_KDE = 0x000008,
    SECT_ICONS = 0x000010,
    SECT_TOOLBAR_STYLE = 0x000020,
    SECT_MAIN_TOOLBAR_ICONS = 0x000040,
    SECT_SMALL_ICONS = 0x000080,

    SECT_KDE4_COL_BUTTON = 0x000100,
    SECT_KDE4_COL_SEL = 0x000200,
    SECT_KDE4_COL_TOOLTIP = 0x000400,
    SECT_KDE4_COL_VIEW = 0x000800,
    SECT_KDE4_COL_WINDOW = 0x001000,

    SECT_KDE4_EFFECT_DISABLED = 0x002000,
    SECT_KDE4_EFFECT_INACTIVE = 0x004000,

    SECT_KDE4_COL_WM = 0x008000,

    SECT_KWIN_COMPOS = 0x010000
};

#define ALL_KDE4_PAL_SETTINGS                                           \
    (SECT_KDE4_COL_BUTTON | SECT_KDE4_COL_SEL | SECT_KDE4_COL_TOOLTIP | \
     SECT_KDE4_COL_VIEW | SECT_KDE4_COL_WINDOW |                        \
     SECT_KDE4_EFFECT_DISABLED | SECT_KDE4_EFFECT_INACTIVE | SECT_KDE4_COL_WM)
/*
  Qt uses the following predefined weights:
    Light    = 25,
    Normal   = 50,
    DemiBold = 63,
    Bold     = 75,
    Black    = 87

  ...need to categorize mid way, i.e. 0->37 = light, 38->56 = normal, etc...
*/

enum {
    WEIGHT_NORMAL = 38, /* Less than this = light */
    WEIGHT_DEMIBOLD = 57,
    WEIGHT_BOLD = 69,
    WEIGHT_BLACK = 81
};

static const char*
weightStr(int w)
{
    if (w < WEIGHT_NORMAL) {
        return "light";
    } else if (w < WEIGHT_DEMIBOLD) {
        return "";
    } else if (w < WEIGHT_BOLD) {
        return "demibold";
    } else if (w < WEIGHT_BLACK) {
        return "bold";
    } else {
        return "black";
    }
}

static const char*
italicStr(int i)
{
    return i ? "Italic" : "";
}

enum {
    RD_ACT_PALETTE = 0x000001,
    RD_DIS_PALETTE = 0x000002,
    RD_INACT_PALETTE = 0x000004,
    RD_FONT = 0x000008,
    RD_CONTRAST = 0x000010,
    RD_ICONS = 0x000020,
    RD_TOOLBAR_STYLE = 0x000040,
    RD_TOOLBAR_ICON_SIZE = 0x000080,
    RD_BUTTON_ICONS = 0x000100,
    RD_SMALL_ICON_SIZE = 0x000200,
    RD_LIST_COLOR = 0x000400,
#ifdef QTC_GTK2_STYLE_SUPPORT
    RD_STYLE = 0x000800,
#else
    RD_STYLE = 0x000000,
#endif
    RD_LIST_SHADE = 0x001000,
    RD_KDE4_PAL = 0x002000,

    RD_MENU_FONT = 0x004000,
    RD_TB_FONT = 0x008000,

    RD_DRAG_DIST = 0x010000,
    RD_DRAG_TIME = 0x020000
};

#ifdef QTC_GTK2_STYLE_SUPPORT
static char*
themeFileSub(const char *prefix, const char *name,
             QtcStrBuff *str_buff, const char *sub)
{
    if (qtcIsRegFile(QTC_LOCAL_BUFF_CAT_STR(*str_buff, prefix, "/", sub, "/",
                                            name, THEME_SUFFIX))) {
        return str_buff->p;
    }
    return NULL;
}

static char*
themeFile(const char *prefix, const char *name, QtcStrBuff *str_buff)
{
    char *f = themeFileSub(prefix, name, str_buff, THEME_DIR);
    if (!f) {
        f = themeFileSub(prefix, name, str_buff, THEME_DIR4);
    }
    return f;
}
#endif

typedef enum {
    BackgroundAlternate,
    BackgroundNormal,
    DecorationFocus,
    DecorationHover,
    ForegroundNormal,

    UnknownColor
} ColorType;

typedef enum  // Taken from "kcolorscheme.cpp"
{
    // Effects
    Intensity = 0,
    Color = 1,
    Contrast = 2,
    // Intensity
    IntensityNoEffect = 0,
    IntensityShade = 1,
    IntensityDarken = 2,
    IntensityLighten = 3,
    // Color
    ColorNoEffect = 0,
    ColorDesaturate = 1,
    ColorFade = 2,
    ColorTint = 3,
    // Contrast
    ContrastNoEffect = 0,
    ContrastFade = 1,
    ContrastTint = 2
} ColAdjustEffects;

typedef struct
{
    double           amount;
    ColAdjustEffects effect;
} ColAdjust;

typedef struct
{
    GdkColor  col;
    ColAdjust intensity,
              color,
              contrast;
    gboolean  enabled;
} ColorEffect;

typedef enum
{
    EFF_DISABLED,
    EFF_INACTIVE
} Effect;

static ColorType getColorType(const char *line)
{
    if(0==strncasecmp(line, "BackgroundAlternate=", 20))
        return BackgroundAlternate;
    if(0==strncasecmp(line, "BackgroundNormal=", 17))
        return BackgroundNormal;
    if(0==strncasecmp(line, "ForegroundNormal=", 17))
        return ForegroundNormal;
    if(0==strncasecmp(line, "DecorationFocus=", 16))
        return DecorationFocus;
    if(0==strncasecmp(line, "DecorationHover=", 16))
        return DecorationHover;
    return UnknownColor;
}

static GdkColor readColor(const char *line)
{
    const char *eq = strchr(line, '=');
    GdkColor col;
    int red;
    int green;
    int blue;

    if (eq && ++eq && *eq != '\0' &&
        sscanf(eq, "%d,%d,%d", &red, &green, &blue) == 3) {
        col.red = toGtkColor(red);
        col.green = toGtkColor(green);
        col.blue = toGtkColor(blue);
    } else {
        col.red = col.blue = col.green = 0;
    }
    return col;
}

static int readInt(const char *line, int offset)
{
    return line[offset]!='\0' ? atoi(&line[offset]) : 0;
}

static double readDouble(const char *line, int offset)
{
    return line[offset]!='\0' ? g_ascii_strtod(&line[offset], NULL) : 0;
}

static gboolean readBool(const char *line, int offset)
{
    return line[offset]!='\0' ? 0==strncasecmp(&line[offset], "true", 4) : false;
}

typedef struct
{
    int   weight,
          italic,
          fixedW;
    float size;
    char  family[MAX_CONFIG_INPUT_LINE_LEN+1];
} QtFontDetails;

static void initFont(QtFontDetails *f, gboolean setFamily)
{
    f->weight=WEIGHT_NORMAL,
    f->italic=0;
    f->fixedW=0;
    f->size=DEFAULT_KDE_FONT_SIZE;
    if(setFamily)
        strcpy(f->family, DEFAULT_KDE_FONT);
    else
        f->family[0]='\0';
}

static void parseFontLine(const char *line, QtFontDetails *font)
{
    int           n=-1;
    char          *l,
                  fontLine[MAX_CONFIG_INPUT_LINE_LEN+1];
    QtFontDetails rc;

    initFont(&rc, false);
    memcpy(fontLine, line, MAX_CONFIG_INPUT_LINE_LEN+1);
    l=strtok(fontLine, "=");

    while(l)
    {
        switch(n)
        {
            case 0:  /* Family - and foundry(maybe!) (ignore X11 and XFT) */
            {
                char *dash=NULL;

                if(NULL!=(dash=strchr(l, '-')))
                {
                    *dash='\0';
                    l=++dash;
                }

                strcpy(rc.family, l);
                break;
            }
            case 1:  /* Point size */
                sscanf(l, "%f", &rc.size);
                break;
            case 4:  /* Weight */
                sscanf(l, "%d", &rc.weight);
                break;
            case 5:  /* Slant */
                sscanf(l, "%d", &rc.italic);
                break;
            case 8:  /* Spacing */
                sscanf(l, "%d", &rc.fixedW);
                break;
            default:
                break;
        }

        n++;
        if(n>8 && '\0'!=font->family[0])
        {
            font->weight=rc.weight;
            font->italic=rc.italic;
            font->fixedW=rc.fixedW;
            font->size=rc.size;
            strcpy(font->family, rc.family);
            break;
        }
        l=strtok(NULL, ",");
    }
}

static void setFont(QtFontDetails *font, int f)
{
    if(qtSettings.fonts[f])
    {
        free(qtSettings.fonts[f]);
        qtSettings.fonts[f]=NULL;
    }
    if(FONT_GENERAL==f && qtSettings.fonts[FONT_BOLD])
    {
        free(qtSettings.fonts[FONT_BOLD]);
        qtSettings.fonts[FONT_BOLD]=NULL;
    }

    qtSettings.fonts[f] = (char*)malloc(
        strlen(font->family) + 1 +
        strlen(weightStr(font->weight)) + 1 +
        strlen(italicStr(font->italic)) + 1 +
        20+  /* point size */ +1);

    sprintf(qtSettings.fonts[f], "%s %s %s %f",
            font->family,
            weightStr(font->weight),
            italicStr(font->italic),
            font->size);

    /* Qt uses a bold font for progressbars, try to mimic this... */
    if(FONT_GENERAL==f && font->weight>=WEIGHT_NORMAL && font->weight<WEIGHT_DEMIBOLD)
    {
        qtSettings.fonts[FONT_BOLD] = (char*)malloc(
            strlen(font->family) + 1 +
            strlen(weightStr(WEIGHT_BOLD)) + 1 +
            strlen(italicStr(font->italic)) + 1 +
            20+  /* point size */ +1);

        sprintf(qtSettings.fonts[FONT_BOLD], "%s %s %s %f",
                font->family,
                weightStr(WEIGHT_BOLD),
                italicStr(font->italic),
                font->size);
    }
    if(qtSettings.debug)
        printf(DEBUG_PREFIX"Font[%d] - %s\n", f, qtSettings.fonts[f]);
}

#define MIX(a, b, bias) (a + ((b - a) * bias))
GdkColor mixColors(const GdkColor *c1, const GdkColor *c2, double bias)
{
    if (bias <= 0.0 || isnan(bias)) return *c1;
    if (bias >= 1.0) return *c2;

    {
    double   r1=c1->red/65535.0,
             g1=c1->green/65535.0,
             b1=c1->blue/65535.0,
             r2=c2->red/65535.0,
             g2=c2->green/65535.0,
             b2=c2->blue/65535.0;
    GdkColor col;

    col.red=(int)(65535.0*MIX(r1, r2, bias));
    col.green=(int)(65535.0*MIX(g1, g2, bias));
    col.blue=(int)(65535.0*MIX(b1, b2, bias));

    return col;
    }
}

static void readKwinrc()
{
    FILE *f=fopen(kwinrc(), "r");

    if(f)
    {
        int  section=SECT_NONE;
        char line[MAX_CONFIG_INPUT_LINE_LEN+1];

        if(qtSettings.debug)
            printf(DEBUG_PREFIX"Reading kwinrc\n");

        while(NULL!=fgets(line, MAX_CONFIG_INPUT_LINE_LEN, f))
            if(line[0]=='[')
            {
                if(0==strncasecmp(line, "[Compositing]", 13))
                    section=SECT_KWIN_COMPOS;
                else
                    section=SECT_NONE;
            }
            else if (SECT_KWIN_COMPOS==section && 0==strncasecmp(line, "Backend=", 8))
            {
                if (strstr(line, "=XRender"))
                    opts.square|=SQUARE_POPUP_MENUS|SQUARE_TOOLTIPS;
                break;
            }
        fclose(f);
    }
}

static void readKdeGlobals(const char *rc, int rd, bool first)
{
    ColorEffect   effects[2];
    int found = 0;
    int colorsFound = 0;
    char          line[MAX_CONFIG_INPUT_LINE_LEN+1];
    FILE          *f=fopen(rc, "r");
    QtFontDetails fonts[FONT_NUM_STD];

    for (int i = 0;i < FONT_NUM_STD;++i) {
        initFont(&fonts[i], true);
    }

    if(first)
    {
        // Set defaults!
        effects[EFF_DISABLED].col.red=112;
        effects[EFF_DISABLED].col.green=111;
        effects[EFF_DISABLED].col.blue=110;
        effects[EFF_DISABLED].color.amount=0.0;
        effects[EFF_DISABLED].color.effect=ColorNoEffect;
        effects[EFF_DISABLED].contrast.amount=0.65;
        effects[EFF_DISABLED].contrast.effect=ContrastFade;
        effects[EFF_DISABLED].intensity.amount=0.1;
        effects[EFF_DISABLED].intensity.effect=IntensityDarken;
        effects[EFF_DISABLED].enabled=true;
        effects[EFF_INACTIVE].col.red=112;
        effects[EFF_INACTIVE].col.green=111;
        effects[EFF_INACTIVE].col.blue=110;
        effects[EFF_INACTIVE].color.amount=0.0;
        effects[EFF_INACTIVE].color.effect=ColorNoEffect;
        effects[EFF_INACTIVE].contrast.amount=0.0;
        effects[EFF_INACTIVE].contrast.effect=ContrastNoEffect;
        effects[EFF_INACTIVE].intensity.amount=0.0;
        effects[EFF_INACTIVE].intensity.effect=IntensityNoEffect;
        effects[EFF_INACTIVE].enabled=false;

        qtSettings.colors[PAL_ACTIVE][COLOR_BUTTON]=setGdkColor(232, 231, 230);
        qtSettings.colors[PAL_ACTIVE][COLOR_BUTTON_TEXT]=setGdkColor(20, 19, 18);
        qtSettings.colors[PAL_ACTIVE][COLOR_SELECTED]=setGdkColor(65, 139, 212);
        qtSettings.colors[PAL_ACTIVE][COLOR_TEXT_SELECTED]=setGdkColor(255, 255, 255);
        qtSettings.colors[PAL_ACTIVE][COLOR_TOOLTIP]=setGdkColor(192, 218, 255);
        qtSettings.colors[PAL_ACTIVE][COLOR_TOOLTIP_TEXT]=setGdkColor(20, 19, 18);
        qtSettings.colors[PAL_ACTIVE][COLOR_BACKGROUND]=setGdkColor(255, 255, 255);
        qtSettings.colors[PAL_ACTIVE][COLOR_TEXT]=setGdkColor(20, 19, 18);
        qtSettings.colors[PAL_ACTIVE][COLOR_LV]=setGdkColor(248, 247, 246);
        qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW]=setGdkColor(233, 232, 232);
        qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_TEXT]=setGdkColor(20, 19, 18);
        qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_BORDER]=qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW];
        qtSettings.colors[PAL_INACTIVE][COLOR_WINDOW_BORDER]=qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW];
        qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_BORDER_TEXT]=qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_TEXT];
        qtSettings.colors[PAL_INACTIVE][COLOR_WINDOW_BORDER_TEXT]=qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_TEXT];

        qtSettings.colors[PAL_ACTIVE][COLOR_FOCUS]=setGdkColor( 43, 116, 199);
        qtSettings.colors[PAL_ACTIVE][COLOR_HOVER]=setGdkColor(119, 183, 255);
    }

    if(f)
    {
        int section=SECT_NONE;

        if(qtSettings.debug)
            printf(DEBUG_PREFIX"Reading kdeglobals - %s\n", rc);

        while(found!=rd && NULL!=fgets(line, MAX_CONFIG_INPUT_LINE_LEN, f))
            if(line[0]=='[')
            {
                if(0==strncasecmp(line, "[Icons]", 7))
                    section=SECT_ICONS;
                else if(0==strncasecmp(line, "[Toolbar style]", 15))
                    section=SECT_TOOLBAR_STYLE;
                else if(0==strncasecmp(line, "[MainToolbarIcons]", 18))
                    section=SECT_MAIN_TOOLBAR_ICONS;
                else if(0==strncasecmp(line, "[SmallIcons]", 12))
                    section=SECT_SMALL_ICONS;
                else if(0==strncasecmp(line, "[Colors:View]", 13))
                    section=SECT_KDE4_COL_VIEW;
                else if(0==strncasecmp(line, "[Colors:Button]", 15))
                    section=SECT_KDE4_COL_BUTTON;
                else if(0==strncasecmp(line, "[Colors:Selection]", 18))
                    section=SECT_KDE4_COL_SEL;
                else if(0==strncasecmp(line, "[Colors:Tooltip]", 16))
                    section=SECT_KDE4_COL_TOOLTIP;
                else if(0==strncasecmp(line, "[Colors:Window]", 15))
                    section=SECT_KDE4_COL_WINDOW;
                else if(0==strncasecmp(line, "[ColorEffects:Disabled]", 23))
                    section=SECT_KDE4_EFFECT_DISABLED;
                else if(0==strncasecmp(line, "[ColorEffects:Inactive]", 23))
                    section=SECT_KDE4_EFFECT_INACTIVE;
                else if(0==strncasecmp(line, "[General]", 9))
                    section=SECT_GENERAL;
                else if(0==strncasecmp(line, "[KDE]", 5))
                    section=SECT_KDE;
                else if(0==strncasecmp(line, "[WM]", 4))
                    section=SECT_KDE4_COL_WM;
                else
                {
                    section=SECT_NONE;
                    if(colorsFound==ALL_KDE4_PAL_SETTINGS)
                        found|=RD_KDE4_PAL;
                    if(found==rd)
                        break;
                }
            }
            else if (SECT_ICONS==section && rd&RD_ICONS && !(found&RD_ICONS) &&
                     0==strncasecmp(line, "Theme=", 6)) {
                char *eq = strstr(line, "=");

                if(eq && ++eq) {
                    unsigned int len = strlen(eq);

                    qtSettings.icons = (char*)realloc(qtSettings.icons,
                                                      len + 1);
                    strcpy(qtSettings.icons, eq);
                    if('\n'==qtSettings.icons[len-1])
                        qtSettings.icons[len-1]='\0';
                }
                found|=RD_ICONS;
            }
            else if (SECT_SMALL_ICONS==section && rd&RD_SMALL_ICON_SIZE && !(found&RD_SMALL_ICON_SIZE) &&
                     0==strncasecmp(line, "Size=", 5))
            {
                int size=readInt(line, 5);

                if(0!=size)
                {
                    qtSettings.iconSizes.smlTbSize=size;
                    qtSettings.iconSizes.btnSize=size;
                    qtSettings.iconSizes.mnuSize=size;
                    found|=RD_SMALL_ICON_SIZE;
                }
            }
            else if (SECT_TOOLBAR_STYLE==section && rd&RD_TOOLBAR_STYLE &&
                     !(found&RD_TOOLBAR_STYLE) &&
                     0==strncasecmp(line, "ToolButtonStyle=", 16)) {
                char *eq = strstr(line, "=");

                if(eq && ++eq)
                {
                    if(0==strncasecmp(eq, "IconOnly", 8))
                        qtSettings.toolbarStyle=GTK_TOOLBAR_ICONS;
                    else if(0==strncasecmp(eq, "TextOnly", 8))
                        qtSettings.toolbarStyle=GTK_TOOLBAR_TEXT;
                    else if (0==strncasecmp(eq, "TextBesideIcon", 14))
                        qtSettings.toolbarStyle=GTK_TOOLBAR_BOTH_HORIZ;
                    else if (0==strncasecmp(eq, "TextUnderIcon", 13))
                        qtSettings.toolbarStyle=GTK_TOOLBAR_BOTH;
                    found|=RD_TOOLBAR_STYLE;
                }
            }
            else if (SECT_MAIN_TOOLBAR_ICONS==section && rd&RD_TOOLBAR_ICON_SIZE &&
                        !(found&RD_TOOLBAR_ICON_SIZE) && 0==strncasecmp(line, "Size=", 5))
            {
                qtSettings.iconSizes.tbSize = readInt(line, 5);
                found|=RD_TOOLBAR_ICON_SIZE;
            }
            else if (SECT_KDE==section && rd&RD_BUTTON_ICONS && !(found&RD_BUTTON_ICONS) &&
                        0==strncasecmp(line, "ShowIconsOnPushButtons=", 23))
            {
                qtSettings.buttonIcons=readBool(line, 23);
                found|=RD_BUTTON_ICONS;
            }
//             else if (SECT_KDE==section && rd&RD_DRAG_DIST && !(found&RD_DRAG_DIST) &&
//                         0==strncasecmp(line, "StartDragDist=", 14))
//             {
//                 qtSettings.startDragDist=readInt(line, 14);
//                 found|=RD_DRAG_DIST;
//             }
            else if (SECT_KDE==section && rd&RD_DRAG_TIME && !(found&RD_DRAG_TIME) &&
                        0==strncasecmp(line, "StartDragTime=", 14))
            {
                qtSettings.startDragTime=readInt(line, 14);
                found|=RD_DRAG_TIME;
            }
            else if (SECT_KDE4_COL_WM==section && rd&RD_KDE4_PAL &&
                     !(found&RD_KDE4_PAL)) {
                colorsFound|=section;
                if(0==strncasecmp(line, "activeBackground=", 17))
                    qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_BORDER]=readColor(line);
                else if(0==strncasecmp(line, "activeForeground=", 17))
                    qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_BORDER_TEXT]=readColor(line);
                else if(0==strncasecmp(line, "inactiveBackground=", 19))
                    qtSettings.colors[PAL_INACTIVE][COLOR_WINDOW_BORDER]=readColor(line);
                else if(0==strncasecmp(line, "inactiveForeground=", 19))
                    qtSettings.colors[PAL_INACTIVE][COLOR_WINDOW_BORDER_TEXT]=readColor(line);
            } else if (section>=SECT_KDE4_COL_BUTTON &&
                       section<=SECT_KDE4_COL_WINDOW &&
                       rd&RD_KDE4_PAL && !(found&RD_KDE4_PAL)) {
                ColorType colorType=getColorType(line);

                colorsFound|=section;
                if(UnknownColor!=colorType)
                {
                    GdkColor color=readColor(line);

                    switch(section)
                    {
                        case SECT_KDE4_COL_BUTTON:
                            switch(colorType) {
                            case BackgroundNormal:
                                qtSettings.colors[PAL_ACTIVE][COLOR_BUTTON]=color;
                                break;
                            case ForegroundNormal:
                                qtSettings.colors[PAL_ACTIVE][COLOR_BUTTON_TEXT]=color;
                                break;
                            case DecorationFocus:
                                qtSettings.colors[PAL_ACTIVE][COLOR_FOCUS]=color;
                                break;
                            case DecorationHover:
                                qtSettings.colors[PAL_ACTIVE][COLOR_HOVER]=color;
                                break;
                            default:
                                break;
                            }
                            break;
                        case SECT_KDE4_COL_SEL:
                            if(BackgroundNormal==colorType)
                                qtSettings.colors[PAL_ACTIVE][COLOR_SELECTED]=color;
                            else if(ForegroundNormal==colorType)
                                qtSettings.colors[PAL_ACTIVE][COLOR_TEXT_SELECTED]=color;
                            break;
                        case SECT_KDE4_COL_TOOLTIP:
                            if(BackgroundNormal==colorType)
                                qtSettings.colors[PAL_ACTIVE][COLOR_TOOLTIP]=color;
                            else if(ForegroundNormal==colorType)
                                qtSettings.colors[PAL_ACTIVE][COLOR_TOOLTIP_TEXT]=color;
                            break;
                        case SECT_KDE4_COL_VIEW:
                            if(BackgroundNormal==colorType)
                                qtSettings.colors[PAL_ACTIVE][COLOR_BACKGROUND]=color;
                            else if(ForegroundNormal==colorType)
                                qtSettings.colors[PAL_ACTIVE][COLOR_TEXT]=color;
                            else if(BackgroundAlternate==colorType)
                                qtSettings.colors[PAL_ACTIVE][COLOR_LV]=color;
                            break;
                        case SECT_KDE4_COL_WINDOW:
                            if(BackgroundNormal==colorType)
                                qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW]=color;
                            else if(ForegroundNormal==colorType)
                                qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_TEXT]=color;
                            break;
                        default:
                            break;
                    }
                }
            } else if (SECT_GENERAL==section && rd&RD_FONT && !(found&RD_FONT) &&
                       0==strncasecmp(line, "font=", 5)) {
                parseFontLine(line, &fonts[FONT_GENERAL]);
                found|=RD_FONT;
            }
            else if (SECT_GENERAL==section && rd&RD_MENU_FONT && !(found&RD_MENU_FONT) &&
                     0==strncasecmp(line, "menuFont=", 9))
            {
                parseFontLine(line, &fonts[FONT_MENU]);
                found|=RD_MENU_FONT;
            }
            else if (SECT_GENERAL==section && rd&RD_TB_FONT && !(found&RD_TB_FONT) &&
                     0==strncasecmp(line, "toolBarFont=", 12))
            {
                parseFontLine(line, &fonts[FONT_TOOLBAR]);
                found|=RD_TB_FONT;
            } else if (rd&RD_CONTRAST && !(found&RD_CONTRAST) &&
                       SECT_KDE==section && 0==strncasecmp(line, "contrast=", 9)) {
                opts.contrast=readInt(line, 9);
                if(opts.contrast>10 || opts.contrast<0)
                    opts.contrast=DEFAULT_CONTRAST;
                found|=RD_CONTRAST;
            }
#ifdef QTC_GTK2_STYLE_SUPPORT
            else if(SECT_GENERAL==section && rd&RD_STYLE && !(found&RD_STYLE) &&
                    0==strncasecmp(line, "widgetStyle=", 12)) {
                int len=strlen(line);
                qtSettings.styleName=realloc(qtSettings.styleName, strlen(&line[12])+1);
                if('\n'==line[len-1])
                    line[len-1]='\0';
                strcpy(qtSettings.styleName, &line[12]);
                found|=RD_STYLE;
            }
#endif
            else if((SECT_KDE4_EFFECT_DISABLED==section ||
                     SECT_KDE4_EFFECT_INACTIVE==section) &&
                    rd&RD_KDE4_PAL && !(found&RD_KDE4_PAL)) {
                colorsFound|=section;
                Effect eff=SECT_KDE4_EFFECT_DISABLED==section ? EFF_DISABLED : EFF_INACTIVE;
                if(0==strncasecmp(line, "Color=", 6))
                    effects[eff].col=readColor(line);
                else if(0==strncasecmp(line, "ColorAmount=", 12))
                    effects[eff].color.amount=readDouble(line, 12);
                else if(0==strncasecmp(line, "ColorEffect=", 12))
                    effects[eff].color.effect =
                        (ColAdjustEffects)readInt(line, 12);
                else if(0==strncasecmp(line, "ContrastAmount=", 15))
                    effects[eff].contrast.amount=readDouble(line, 15);
                else if(0==strncasecmp(line, "ContrastEffect=", 15))
                    effects[eff].contrast.effect =
                        (ColAdjustEffects)readInt(line, 15);
                else if(0==strncasecmp(line, "IntensityAmount=", 16))
                    effects[eff].intensity.amount=readDouble(line, 16);
                else if(0==strncasecmp(line, "IntensityEffect=", 16))
                    effects[eff].intensity.effect =
                        (ColAdjustEffects)readInt(line, 16);
                else if(0==strncasecmp(line, "Enable=", 7))
                    effects[eff].enabled=readBool(line, 7);
                else if(0==strncasecmp(line, "ChangeSelectionColor=", 21))
                    qtSettings.inactiveChangeSelectionColor=readBool(line, 21);
            }
            else if(SECT_GENERAL==section && rd&RD_LIST_SHADE && !(found&RD_LIST_SHADE) &&
                    0==strncasecmp(line, "shadeSortColumn=", 16))
            {
                qtSettings.shadeSortedList=readBool(line, 16);
                found|=RD_LIST_SHADE;
            }
            else if(found==rd)
                break;

        fclose(f);
    }

    int eff = 0;
    double contrast = 0.1 * opts.contrast;
    double y;

    contrast = (1.0 > contrast ? (-1.0 < contrast ? contrast : -1.0) : 1.0);
    y = qtcColorLuma(&qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW]);

    if(y<0.006)
        qtSettings.colors[PAL_ACTIVE][COLOR_MID]=qtcColorShade(&qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW], 0.01 + 0.20 * contrast, 0.0);
    else if(y>0.93)
        qtSettings.colors[PAL_ACTIVE][COLOR_MID]=qtcColorShade(&qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW], -0.02 - 0.20 * contrast, 0.0);
    else {
        double darkAmount =  (-y) * (0.55 + contrast * 0.35);

        qtSettings.colors[PAL_ACTIVE][COLOR_MID]=qtcColorShade(&qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW], (0.35 + 0.15 * y) * darkAmount, 0.0);
    }

    for (eff=0; eff<2; ++eff) {
        int p=0==eff ? PAL_DISABLED : PAL_INACTIVE;
        memcpy(qtSettings.colors[p], qtSettings.colors[PAL_ACTIVE], sizeof(GdkColor) * COLOR_NUMCOLORS_STD);
        if(effects[eff].enabled) {
            int col;
            for(col=0; col<COLOR_NUMCOLORS_STD; ++col) {
                switch(effects[eff].intensity.effect) {
                case IntensityShade:
                    qtSettings.colors[p][col] = qtcColorShade(&qtSettings.colors[p][col], effects[eff].intensity.amount, 0.0);
                    break;
                case IntensityDarken:
                    qtSettings.colors[p][col] = qtcColorDarken(&qtSettings.colors[p][col], effects[eff].intensity.amount, 1.0);
                    break;
                case IntensityLighten:
                    qtSettings.colors[p][col] =
                        qtcColorLighten(
                            &qtSettings.colors[p][col],
                            effects[eff].intensity.amount, 1.0);
                default:
                    break;
                }
                switch (effects[eff].color.effect) {
                case ColorDesaturate:
                    qtSettings.colors[p][col] = qtcColorDarken(&qtSettings.colors[p][col], 0.0, 1.0 - effects[eff].color.amount);
                    break;
                case ColorFade:
                    qtSettings.colors[p][col] = qtcColorMix(&qtSettings.colors[p][col], &effects[eff].col, effects[eff].color.amount);
                    break;
                case ColorTint:
                    qtSettings.colors[p][col] = qtcColorTint(&qtSettings.colors[p][col], &effects[eff].col, effects[eff].color.amount);
                default:
                    break;
                }

                if (COLOR_BUTTON_TEXT==col || COLOR_TEXT==col ||
                    COLOR_WINDOW_TEXT==col || COLOR_TOOLTIP_TEXT==col) {
                    int other = (COLOR_BUTTON_TEXT==col ? COLOR_BUTTON :
                                 COLOR_WINDOW_TEXT==col ? COLOR_WINDOW :
                                 COLOR_TEXT==col ? COLOR_BACKGROUND :
                                 COLOR_TOOLTIP);

                    switch (effects[eff].contrast.effect) {
                    case ContrastFade:
                        qtSettings.colors[p][col]=qtcColorMix(&qtSettings.colors[p][col],
                                                              &qtSettings.colors[PAL_DISABLED][other],
                                                              effects[eff].contrast.amount);
                        break;
                    case ContrastTint:
                        qtSettings.colors[p][col]=qtcColorTint(&qtSettings.colors[p][col],
                                                               &qtSettings.colors[PAL_DISABLED][other],
                                                               effects[eff].contrast.amount);
                    default:
                        break;
                    }
                }
            }
        }
    }

    qtSettings.colors[PAL_INACTIVE][COLOR_SELECTED]=qtSettings.colors[PAL_ACTIVE][COLOR_SELECTED];
#if 0 // Use alpha values instead...
    if(qtSettings.inactiveChangeSelectionColor)
        if(effects[PAL_INACTIVE].enabled)
            qtSettings.colors[PAL_INACTIVE][COLOR_SELECTED]=qtcColorTint(&qtSettings.colors[PAL_INACTIVE][COLOR_WINDOW],
                                                                         &qtSettings.colors[PAL_ACTIVE][COLOR_SELECTED],
                                                                         0.4);
        else
            qtSettings.inactiveChangeSelectionColor=false;
#endif

    if(rd&RD_ICONS && !qtSettings.icons)
    {
        qtSettings.icons=(char *)realloc(qtSettings.icons, strlen(defaultIcons())+1);
        strcpy(qtSettings.icons, defaultIcons());
    }
    if(rd&RD_TOOLBAR_STYLE && !(found&RD_TOOLBAR_STYLE))
        qtSettings.toolbarStyle=GTK_TOOLBAR_ICONS;
    if(rd&RD_BUTTON_ICONS && !(found&RD_BUTTON_ICONS))
        qtSettings.buttonIcons=true;
    if(rd&RD_LIST_SHADE && !(found&RD_LIST_SHADE))
        qtSettings.shadeSortedList=true;
    //if(rd&RD_FONT && (found&RD_FONT || (!qtSettings.fonts[FONT_GENERAL] && kde4)))  /* No need to check if read in */
    if(rd&RD_FONT && found&RD_FONT)
        setFont(&fonts[FONT_GENERAL], FONT_GENERAL);
    if(rd&RD_MENU_FONT && found&RD_MENU_FONT)
        setFont(&fonts[FONT_MENU], FONT_MENU);
    if(rd&RD_TB_FONT && found&RD_TB_FONT)
        setFont(&fonts[FONT_TOOLBAR], FONT_TOOLBAR);
}

static int qt_refs = 0;

static const char*
kdeIconsPrefix()
{
    static const char *kdeIcons = NULL;
    if (kdeIcons) {
        return kdeIcons;
    }
    size_t len = 0;
    char *res = qtcPopenStdout("kde4-config", (const char * const[]){
            "kde4-config", "--expandvars", "--install", "icon", NULL},
        300, &len);
    if (res && res[strspn(res, " \t\b\n\f\v")]) {
        if (res[len - 1]=='\n') {
            res[len - 1]='\0';
        }
        kdeIcons = res;
        return kdeIcons;
    }
    kdeIcons = (QTC_KDE4_ICONS_PREFIX && strlen(QTC_KDE4_ICONS_PREFIX) > 2 ?
                QTC_KDE4_ICONS_PREFIX : DEFAULT_ICON_PREFIX);
    return kdeIcons;
}

static char *getIconPath()
{
    static char *path=NULL;
    char        *kdeHome=getKdeHome();
    const char  *kdePrefix=kdeIconsPrefix(),
                *defIcons=defaultIcons();
    gboolean    nonDefIcons=qtSettings.icons && strcmp(qtSettings.icons, defIcons);
    unsigned len = strlen("pixmap_path \"");
    unsigned kdeHomeLen = kdeHome ? strlen(kdeHome) : 0;
    unsigned kdeIconPrefixLen = strlen(kdePrefix);
    unsigned iconLen = qtSettings.icons ? strlen(qtSettings.icons) : 0;
    unsigned defIconsLen = strlen(defIcons);
    gboolean addDefaultPrefix = strcmp(kdePrefix, DEFAULT_ICON_PREFIX);

    if(nonDefIcons)
    {
        if(kdeHome)
        {
            len+=kdeHomeLen;
            len+=ICON_FOLDER_SLEN;
            len+=iconLen;
            len++;
        }
        if(kdeIconPrefixLen)
        {
            len+=kdeIconPrefixLen;
            len++;
            len+=iconLen;
            len++;
        }
        if(addDefaultPrefix)
        {
            len+=DEFAULT_ICON_PREFIX_LEN;
            len++;
            len+=iconLen;
            len++;
        }
    }

    if(kdeHome)
    {
        len+=kdeHomeLen;
        len+=ICON_FOLDER_SLEN;
        len+=defIconsLen;
        len++;
    }
    if(kdeIconPrefixLen)
    {
        len+=kdeIconPrefixLen;
        len++;
        len+=defIconsLen;
        len++;
    }
    if(addDefaultPrefix)
    {
        len+=DEFAULT_ICON_PREFIX_LEN;
        len++;
        len+=defIconsLen;
        len++;
    }
    if(kdeHome)
    {
        len+=kdeHomeLen;
        len+=ICON_FOLDER_SLEN;
        len+=HICOLOR_LEN;
        len++;
    }
    if(kdeIconPrefixLen)
    {
        len+=kdeIconPrefixLen;
        len++;
        len+=HICOLOR_LEN;
        len++;
    }
    if(addDefaultPrefix)
    {
        len+=DEFAULT_ICON_PREFIX_LEN;
        len++;
        len+=HICOLOR_LEN;
        len++;
    }
    len++;

    if(path && len!=(strlen(path)+1))
        free(path);

    if(!path)
        path=(char *)malloc(len+1);

    strcpy(path, "pixmap_path \"");

    if(nonDefIcons)
    {
        if(kdeHome)
        {
            strcat(path, kdeHome);
            strcat(path, ICON_FOLDER);
            strcat(path, qtSettings.icons);
            strcat(path, ":");
        }
        if(kdeIconPrefixLen)
        {
            strcat(path, kdePrefix);
            strcat(path, "/");
            strcat(path, qtSettings.icons);
            strcat(path, ":");
        }
        if(addDefaultPrefix)
        {
            strcat(path, DEFAULT_ICON_PREFIX);
            strcat(path, "/");
            strcat(path, qtSettings.icons);
            strcat(path, ":");
        }
    }

    if(kdeHome)
    {
        strcat(path, kdeHome);
        strcat(path, ICON_FOLDER);
        strcat(path, defIcons);
        strcat(path, ":");
    }

    if(kdeIconPrefixLen)
    {
        strcat(path, kdePrefix);
        strcat(path, "/");
        strcat(path, defIcons);
        strcat(path, ":");
    }

    if(addDefaultPrefix)
    {
        strcat(path, DEFAULT_ICON_PREFIX);
        strcat(path, "/");
        strcat(path, defIcons);
        strcat(path, ":");
    }

    if(kdeHome)
    {
        strcat(path, kdeHome);
        strcat(path, ICON_FOLDER);
        strcat(path, HICOLOR_ICONS);
        strcat(path, ":");
    }

    if(kdeIconPrefixLen)
    {
        strcat(path, kdePrefix);
        strcat(path, "/");
        strcat(path, HICOLOR_ICONS);
        strcat(path, ":");
    }

    if(addDefaultPrefix)
    {
        strcat(path, DEFAULT_ICON_PREFIX);
        strcat(path, "/");
        strcat(path, HICOLOR_ICONS);
    }

    strcat(path, "\"");

    int plen = strlen(path);

    if (path[plen - 1] == ':') {
        path[plen - 1] = '\0';
    }

    if (qtSettings.debug && path) {
        printf(DEBUG_PREFIX "%s\n", path);
    }
    return path;
}

#define MAX_CSS_HOME     256
#define CSS_DEFAULT      ".default"
#define CSS_DEFAULT_ALT  "default."
#define USER_CHROME_DIR  "/chrome"
#define USER_CHROME_FILE "userChrome.css"
#define USER_CHROME_CSS  USER_CHROME_DIR "/" USER_CHROME_FILE
#define MAX_DEFAULT_NAME 16+strlen(CSS_DEFAULT)+strlen(USER_CHROME_CSS)

#define GUARD_STR      "Added by QtCurve -- do not remove"
#define MENU_GUARD_STR "MenuColors, " GUARD_STR

#define OLD_MENU_TEXT_STR "menubar > menu { color: HighlightText !important; } menubar > menu[_moz-menuactive=\"true\"] "\
                      "{ background-color : HighlightText !important; color: HighlightText !important; } "\
                      "/* " GUARD_STR " */\n"
#define MENU_TEXT_STR_FORMAT "menubar > menu { color: #%02x%02x%02x !important; } " \
                             "menubar > menu[_moz-menuactive=\"true\"][open=\"false\"] { color: #%02x%02x%02x !important; } "\
                             "menubar > menu[_moz-menuactive=\"true\"][open=\"true\"] { color: #%02x%02x%02x !important; } "\
                             "/* " MENU_GUARD_STR " */\n"
#define CSS_FILE_STR     "@import url(\"file://" QTC_GTK2_MOZILLA_DIR "/QtCurve.css\"); /* " GUARD_STR " */\n"
#define BTN_CSS_FILE_STR "@import url(\"file://" QTC_GTK2_MOZILLA_DIR "/QtCurve-KDEButtonOrder.css\"); /* " GUARD_STR " */\n"

static void processUserChromeCss(char *file, gboolean add_btn_css, gboolean add_menu_colors)
{
    FILE        *f=fopen(file, "r");
    char        *contents=NULL,
                *menu_text_str=NULL;
    gboolean    remove_menu_colors=false,
                remove_old_menu_colors=false;
#ifdef QTC_GTK2_MODIFY_MOZILLA
    gboolean    remove_btn_css=false,
                add_css=true;
#else
    QTC_UNUSED(add_btn_css);
#endif
    struct stat st;
    size_t      file_size=0,
                new_size=0;

    if(add_menu_colors)
    {
        GdkColor *std, *active;
        if(SHADE_WINDOW_BORDER==opts.shadeMenubars)
            std=&qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_BORDER_TEXT];
        else if(opts.customMenuTextColor)
            std=&opts.customMenuNormTextColor;
        else if(SHADE_BLEND_SELECTED==opts.shadeMenubars || SHADE_SELECTED==opts.shadeMenubars ||
                (SHADE_CUSTOM==opts.shadeMenubars && TOO_DARK(qtcPalette.menubar[ORIGINAL_SHADE])))
            std=&qtSettings.colors[PAL_ACTIVE][COLOR_TEXT_SELECTED];
        else
            std=&qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_TEXT];

        if(opts.customMenuTextColor)
            active=&opts.customMenuSelTextColor;
        else if(opts.useHighlightForMenu)
            active=&qtSettings.colors[PAL_ACTIVE][COLOR_TEXT_SELECTED];
        else
            active=&qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_TEXT];

        menu_text_str=(char *)malloc(strlen(MENU_TEXT_STR_FORMAT)+1);
        sprintf(menu_text_str, MENU_TEXT_STR_FORMAT,
                toQtColor(std->red), toQtColor(std->green), toQtColor(std->blue),
                toQtColor(std->red), toQtColor(std->green), toQtColor(std->blue),
                toQtColor(active->red), toQtColor(active->green), toQtColor(active->blue));
    }

    if(f)
    {
        if(0==fstat(fileno(f), &st))
        {
            file_size = st.st_size;
            new_size=file_size+strlen(MENU_TEXT_STR_FORMAT)+strlen(CSS_FILE_STR)+3;
            contents=(char *)malloc(new_size);

            if(contents)
            {
                char  *line=NULL;
                size_t len=0;

                contents[0]='\0';
                while(-1!=getline(&line, &len, f))
                {
                    gboolean write_line=true;

#ifdef QTC_GTK2_MODIFY_MOZILLA
                    if(0==strcmp(line, BTN_CSS_FILE_STR))
                    {
                        if (add_btn_css)
                            add_btn_css=false;
                        else
                            remove_btn_css=true, write_line=false;
                    }
                    else if(0==strcmp(line, CSS_FILE_STR))
                        add_css=false;
                    else
#endif
                    if(0==strcmp(line, OLD_MENU_TEXT_STR))
                        write_line=false, remove_old_menu_colors=true;
                    else if(NULL!=strstr(line, MENU_GUARD_STR))
                    {
                        if (add_menu_colors)
                        {
                            if(0==strcmp(menu_text_str, line))
                                add_menu_colors=false;
                            else
                                write_line=false;
                        }
                        else
                            remove_menu_colors=true, write_line=false;
                    }
                    if(write_line)
                        strcat(contents, line);
                }
                qtcFree(line);
            }
        }

        fclose(f);
    }

#ifdef QTC_GTK2_MODIFY_MOZILLA
    if(!contents || add_btn_css || add_menu_colors || add_css)
#else
    if(!contents || add_menu_colors)
#endif
    {
        if(!contents)
        {
            new_size=strlen(MENU_TEXT_STR_FORMAT)+strlen(BTN_CSS_FILE_STR)+strlen(CSS_FILE_STR)+4;

            contents=(char *)malloc(new_size);
            if(contents)
                contents[0]='\0';
        }

        if(contents)
        {
#ifdef QTC_GTK2_MODIFY_MOZILLA
            if(add_css)
            {
                char *css_contents=(char *)malloc(new_size);

                if(css_contents)
                {
                    css_contents[0]='\0';
                    strcat(css_contents, CSS_FILE_STR);
                    strcat(css_contents, contents);
                    free(contents);
                    contents=css_contents;
                }
            }
            if(add_btn_css)
            {
                char *css_contents=(char *)malloc(new_size);

                if(css_contents)
                {
                    css_contents[0]='\0';
                    strcat(css_contents, BTN_CSS_FILE_STR);
                    strcat(css_contents, contents);
                    free(contents);
                    contents=css_contents;
                }
            }
#endif
            if(add_menu_colors)  /* This can be on last line */
            {
                int len=strlen(contents);

                if(len && contents[len-1]!='\n')
                    strcat(contents, "\n");
                strcat(contents, menu_text_str);
            }
        }
    }

#ifdef QTC_GTK2_MODIFY_MOZILLA
    if(contents && (add_btn_css || remove_btn_css || add_menu_colors || remove_menu_colors || remove_old_menu_colors))
#else
    if(contents && (add_menu_colors || remove_menu_colors || remove_old_menu_colors))
#endif
    {
        f=fopen(file, "w");

        if(f)
        {
            fputs(contents, f);
            fclose(f);
        }
        free(contents);
    }
    qtcFree(menu_text_str);
}

static void processMozillaApp(gboolean add_btn_css, gboolean add_menu_colors, const char *app, gboolean under_moz)
{
    const char *home=qtcGetHome();

    if (home && (strlen(home)+strlen(app)+10+MAX_DEFAULT_NAME)<MAX_CSS_HOME) {
        char cssHome[MAX_CSS_HOME+1];
        DIR  *dir=NULL;

        sprintf(cssHome, under_moz ? "%s/.mozilla/%s/" : "%s/.%s/", home, app);

        if((dir=opendir(cssHome)))
        {
             struct dirent *dir_ent=NULL;

             for (dir_ent=readdir(dir); dir_ent; dir_ent=readdir(dir))
             {
                 char *str=NULL;

                 if(((str=strstr(dir_ent->d_name, CSS_DEFAULT)) && str>=dir_ent->d_name &&
                    '\0'==str[strlen(CSS_DEFAULT)]) ||
                    ((str=strstr(dir_ent->d_name, CSS_DEFAULT_ALT)) && str==dir_ent->d_name &&
                    '\0'!=str[strlen(CSS_DEFAULT_ALT)]))
                 {
                    char        sub[MAX_CSS_HOME];
#ifdef QTC_GTK2_MODIFY_MOZILLA
                    FILE        *userJs=NULL;
                    gboolean    alterUserJs=true;

                    /* Add custom user.js file */
                    sprintf(sub, "%s%s/user.js", cssHome, dir_ent->d_name);

                    if((userJs=fopen(sub, "r")))
                    {
                        char  *line=NULL;
                        size_t len=0;

                        while(-1!=getline(&line, &len, userJs))
                            if(strstr(line, "browser.preferences.instantApply"))
                            {
                                /* If instant-apply is set to true, then we cannot alter button order,
                                   as this produces sde effects (such as the preferences dialog's close
                                   button having no text! */
                                if(strstr(line, "true"))
                                    add_btn_css=false;
                                alterUserJs=false;
                                break;
                            }
                        fclose(userJs);
                        qtcFree(line);
                    }

                    if(alterUserJs && ((userJs=fopen(sub, "a"))))
                    {
                        fputs("\nuser_pref(\"browser.preferences.instantApply\", false);\n", userJs);
                        fclose(userJs);
                    }
#endif

                    /* Now do userChrome.css */
                    sprintf(sub, "%s%s%s/", cssHome, dir_ent->d_name,
                            USER_CHROME_DIR);

                    qtcMakePath(sub, 0755);
                    if (qtcIsDir(sub)) {
                        strcat(sub, USER_CHROME_FILE);
                        processUserChromeCss(sub, add_btn_css, add_menu_colors);
                    }
                }
            }

            closedir(dir);
        }
    }
}

static void getGtk2CfgFile(char **tmpStr, const char *f)
{
    *tmpStr=(char *)realloc(*tmpStr, strlen(qtcConfDir())+strlen(f)+1);
    sprintf(*tmpStr, "%s%s", qtcConfDir(), f);
}

static gboolean checkFileVersion(const char *fname, const char *versionStr, int versionStrLen)
{
    FILE     *f=fopen(fname, "r");
    gboolean diff=true;

    if(f)
    {
        if(0!=access(fname, W_OK)) /* If file is not writeable, then just pretend no difference */
            diff=false;
        else
        {
            static const int constVLen=32;

            char line[constVLen+1];
            int  numChars=qtcMin(constVLen, versionStrLen-1);

            diff=NULL==fgets(line, numChars+1, f) || memcmp(versionStr, line, numChars);
        }
        fclose(f);
    }

    return !diff;
}

static gboolean isMozApp(const char *app, const char *check)
{
    if (strcmp(app, check) == 0) {
        return true;
    } else if (qtcStrStartsWith(app, check)) {
        int app_len=strlen(app),
            check_len=strlen(check);

        if(check_len+4 == app_len && 0==strcmp(&app[check_len], "-bin"))
            return true;

        /* OK check for xulrunner-1.9 */
        {
        float dummy;
        if(app_len>(check_len+1) && 1==sscanf(&app[check_len+1], "%f", &dummy))
            return true;
        }
    }

    return false;
}

static gboolean excludedApp(Strings config)
{
    if (qtSettings.appName && config) {
        for (int i = 0;config[i];++i) {
            if (strcmp("gtk", config[i]) == 0 ||
                strcmp(qtSettings.appName, config[i]) == 0) {
                return true;
            }
        }
    }
    return false;
}

static QtcDebug
debugLevel()
{
    const char *dbg = getenv("QTCURVE_DEBUG");

    if (dbg) {
        switch (atoi(dbg)) {
        case 1:
            return DEBUG_SETTINGS;
        case 2:
            return DEBUG_ALL;
        default:
            return DEBUG_NONE;
        }
    }
    return DEBUG_NONE;
}

gboolean qtSettingsInit()
{
    if (0 == qt_refs++) {
        static int lastRead = 0;
        int now = time(NULL);
        qtSettings.app = GTK_APP_UNKNOWN;
        if (abs(now - lastRead) > 1) {
            char *locale = setlocale(LC_NUMERIC, NULL);
            char *path = NULL;
            QTC_DEF_STR_BUFF(str_buff, 4096, 1);
            char *tmpStr = NULL;
            GtkSettings *settings=NULL;

            setlocale(LC_NUMERIC, "C");
            qtSettings.icons=NULL;
            memset(qtSettings.fonts, 0, sizeof(char *)*FONT_NUM_TOTAL);
            qtSettings.iconSizes.smlTbSize=16;
            qtSettings.iconSizes.tbSize=22;
            qtSettings.iconSizes.dndSize=32;
            qtSettings.iconSizes.btnSize=16;
            qtSettings.iconSizes.mnuSize=16;
            qtSettings.iconSizes.dlgSize=32;
            qtSettings.colors[PAL_ACTIVE][COLOR_LV]=setGdkColor(0, 0, 0);
            qtSettings.colors[PAL_ACTIVE][COLOR_TOOLTIP_TEXT]=setGdkColor(0, 0, 0);
            qtSettings.colors[PAL_ACTIVE][COLOR_TOOLTIP]=setGdkColor(0xFF, 0xFF, 192);
#ifdef QTC_GTK2_STYLE_SUPPORT
            qtSettings.styleName=NULL;
#endif
            qtSettings.inactiveChangeSelectionColor=false;
            qtSettings.appName=NULL;
            /* qtSettings.startDragDist=4; */
            qtSettings.startDragTime=500;
            qtSettings.debug=debugLevel();
            opts.contrast=DEFAULT_CONTRAST;
            for (int i = 0;i < FONT_NUM_TOTAL;++i) {
                qtSettings.fonts[i] = NULL;
            }

            qtSettings.qt4 = false;
            // TODO: blacklist logic
            qtSettings.useAlpha =
                (opts.bgndOpacity < 100 || opts.dlgOpacity < 100 ||
                 opts.menuBgndOpacity < 100 ||
                 !(opts.square & SQUARE_POPUP_MENUS) ||
                 !(opts.square & SQUARE_TOOLTIPS));

            lastRead=now;

            {
            int        f=0;
            const char *files[]={QTC_GTK2_THEME_DIR"/kdeglobals", /* QtCurve supplied kdeglobals file */
                                 "/etc/kderc",
                                 "/etc/kde4/kdeglobals",
                                 "/etc/kde4rc",
                                 QTC_KDE4_PREFIX KDE4_SYS_CFG_DIR KDEGLOBALS_FILE,
                                 QTC_KDE4_PREFIX KDE4_SYS_CFG_DIR
                                 KDEGLOBALS_SYS_FILE,
                                 kdeGlobals(),
                                 0L};

            for(f=0; 0!=files[f]; ++f)
                readKdeGlobals(files[f], RD_ICONS|RD_SMALL_ICON_SIZE|RD_TOOLBAR_STYLE|RD_MENU_FONT|RD_TB_FONT|
                               RD_TOOLBAR_ICON_SIZE|RD_BUTTON_ICONS|RD_LIST_SHADE|
                               (RD_KDE4_PAL|RD_FONT|RD_CONTRAST|RD_STYLE|RD_DRAG_DIST|RD_DRAG_TIME),
                               0==f);
            }

#ifdef QTC_GTK2_STYLE_SUPPORT
            /* Only for testing - allows me to simulate Qt's -style parameter.
               e.g start Gtk2 app as follows:

               QTC_STYLE=qtc_klearlooks gtk-demo
            */
            {
                const char *env=getenv("QTC_STYLE");

                if(env && strlen(env))
                {
                    qtSettings.styleName=realloc(qtSettings.styleName, strlen(env));
                    strcpy(qtSettings.styleName, env);
                }
            }

            char *rcFile = NULL;
            /* Is the user using a non-default QtCurve style? */
            if (qtSettings.styleName &&
                qtcStrStartsWith(qtSettings.styleName, THEME_PREFIX)) {
                rcFile = themeFile(getKdeHome(),
                                   qtSettings.styleName, &str_buff);

                if (!rcFile) {
                    rcFile = themeFile(QTC_KDE4_PREFIX, qtSettings.styleName,
                                       &str_buff);
                }
            }

            qtcReadConfig(rcFile, &opts, 0L);
#else
            qtcReadConfig(0L, &opts, 0L);
#endif

#if GTK_CHECK_VERSION(2, 90, 0) /* Gtk3:TODO !!! */
            opts.square|=SQUARE_POPUP_MENUS;
            opts.bgndOpacity=opts.dlgOpacity=opts.menuBgndOpacity=100;
            opts.gtkComboMenus=true;
            qtSettings.useAlpha=false;
#endif

            /* Focus is messed up if not using glow focus*/
            if(!opts.gtkComboMenus && FOCUS_GLOW!=opts.focus)
                opts.gtkComboMenus=true;

#if 0
            if (opts.inactiveHighlight) {
                generateMidColor(&qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW],
                                 &qtSettings.colors[PAL_ACTIVE][COLOR_SELECTED],
                                 &qtSettings.inactiveSelectCol,
                                 INACTIVE_HIGHLIGHT_FACTOR);
            } else {
                qtSettings.inactiveSelectCol =
                    qtSettings.colors[PAL_INACTIVE][COLOR_SELECTED];
            }
#endif

            /* Check if we're firefox... */
            if ((qtSettings.appName = qtcGetProgName())) {
                bool firefox = (isMozApp(qtSettings.appName, "firefox") ||
                                isMozApp(qtSettings.appName, "iceweasel") ||
                                isMozApp(qtSettings.appName, "swiftfox") ||
                                isMozApp(qtSettings.appName, "xulrunner") ||
                                isMozApp(qtSettings.appName, "abrowser"));
                bool thunderbird =
                    (!firefox && (isMozApp(qtSettings.appName, "thunderbird") ||
                                  isMozApp(qtSettings.appName, "icedove")));
                bool mozThunderbird =
                    (!thunderbird && !firefox &&
                     isMozApp(qtSettings.appName, "mozilla-thunderbird"));
                bool seamonkey = (!thunderbird && !firefox && !mozThunderbird &&
                                  isMozApp(qtSettings.appName, "seamonkey"));

                if (firefox || thunderbird || mozThunderbird || seamonkey) {
                    GdkColor *menu_col=SHADE_CUSTOM==opts.shadeMenubars
                                        ? &opts.customMenubarsColor
                                        : &qtSettings.colors[PAL_ACTIVE][COLOR_SELECTED];
                    gboolean add_menu_colors=SHADE_BLEND_SELECTED==opts.shadeMenubars || SHADE_SELECTED==opts.shadeMenubars ||
                                             SHADE_WINDOW_BORDER==opts.shadeMenubars ||
                                             opts.customMenuTextColor || !opts.useHighlightForMenu ||
                                             (SHADE_CUSTOM==opts.shadeMenubars && TOO_DARK(*menu_col) ),
                             add_btn_css=false;

                    if (firefox) {
                        processMozillaApp(0, add_menu_colors, "firefox", true);
                    } else if (thunderbird) {
                        processMozillaApp(add_btn_css, add_menu_colors,
                                          "thunderbird", false);
                    } else if (mozThunderbird) {
                        processMozillaApp(add_btn_css, add_menu_colors,
                                          "mozilla-thunderbird", false);
                    }

                    qtSettings.app = (firefox ?
                                      GTK_APP_NEW_MOZILLA : GTK_APP_MOZILLA);
                    if (GTK_APP_MOZILLA == qtSettings.app)
                        qtSettings.app=GTK_APP_NEW_MOZILLA;
                    if(GTK_APP_NEW_MOZILLA!=qtSettings.app && APPEARANCE_FADE==opts.menuitemAppearance &&
                       (thunderbird || mozThunderbird))
                        opts.menuitemAppearance=APPEARANCE_FLAT;
                }
                else if(0==strcmp(qtSettings.appName, "soffice.bin"))
                    qtSettings.app=GTK_APP_OPEN_OFFICE;
                else if(0==strcmp(qtSettings.appName, "vmplayer"))
                    qtSettings.app=GTK_APP_VMPLAYER;
                else if (qtcStrStartsWith(qtSettings.appName, "gimp"))
                    qtSettings.app=GTK_APP_GIMP;
                else if(0==strcmp(qtSettings.appName, "java"))
                    qtSettings.app=GTK_APP_JAVA;
                else if(0==strcmp(qtSettings.appName, "evolution"))
                    qtSettings.app=GTK_APP_EVOLUTION;
                else if(0==strcmp(qtSettings.appName, "eclipse"))
                    qtSettings.app=GTK_APP_JAVA_SWT;
                else if (strcmp(qtSettings.appName, "ghb") == 0) {
                    qtSettings.app=GTK_APP_GHB;
                }
            }

            if(qtSettings.debug)
                printf(DEBUG_PREFIX"Application name: \"%s\"\n", qtSettings.appName ? qtSettings.appName : "<unknown>");

            /* Eclipse sets a application name, so if this is set then we're not a Swing java app */
            if(GTK_APP_JAVA==qtSettings.app && g_get_application_name() && 0!=strcmp(g_get_application_name(), "<unknown>"))
                qtSettings.app=GTK_APP_JAVA_SWT;

            if(GTK_APP_JAVA==qtSettings.app)
                opts.sliderStyle=SLIDER_PLAIN;

            if (qtcOneOf(qtSettings.app, GTK_APP_JAVA, GTK_APP_JAVA_SWT,
                         GTK_APP_OPEN_OFFICE) || QtCurve::isMozilla()) {
                opts.square |= SQUARE_POPUP_MENUS;
                opts.bgndAppearance = APPEARANCE_FLAT;
                opts.bgndImage.type = IMG_NONE;
                if (FRAME_SHADED == opts.groupBox)
                    opts.groupBox = FRAME_PLAIN;
                opts.gbFactor = 0;
                opts.highlightScrollViews = false;
            }

            if(!qtcIsFlat(opts.bgndAppearance) && excludedApp(opts.noBgndGradientApps))
                opts.bgndAppearance=APPEARANCE_FLAT;

            if(IMG_NONE!=opts.bgndImage.type && excludedApp(opts.noBgndImageApps))
                opts.bgndImage.type=IMG_NONE;

            if (qtcOneOf(qtSettings.app, GTK_APP_OPEN_OFFICE, GTK_APP_JAVA,
                         GTK_APP_JAVA_SWT) || QtCurve::isMozilla()) {
                opts.bgndOpacity = opts.dlgOpacity = opts.menuBgndOpacity = 100;
                qtSettings.useAlpha = false;
            }

            if(excludedApp(opts.noBgndOpacityApps))
                opts.bgndOpacity=opts.dlgOpacity=100;

            if(excludedApp(opts.noMenuBgndOpacityApps))
                opts.menuBgndOpacity=100, qtSettings.useAlpha=false;

            /* Disable usage of alpha channel for older configs, unless app is uing opacity... */
            if(qtSettings.useAlpha && opts.version<qtcMakeVersion(1, 7, 2) && 100==opts.menuBgndOpacity && 100==opts.dlgOpacity && 100==opts.bgndOpacity)
                qtSettings.useAlpha=false;

            if(opts.menuStripe && excludedApp(opts.noMenuStripeApps))
                opts.menuStripe=SHADE_NONE;

            if (GTK_APP_JAVA != qtSettings.app) {
                /* KDE's "apply colors to non-KDE apps" messes up firefox,
                   (and progress bar text) so need to fix this! */
                /* ...and inactive highlight!!! */
                GdkColor *highlightedMenuCol =
                    (opts.useHighlightForMenu ?
                     &qtSettings.colors[PAL_ACTIVE][COLOR_TEXT_SELECTED] :
                     &qtSettings.colors[PAL_ACTIVE][COLOR_TEXT]);
                qtc_gtkrc_printf(str_buff, "style \"" RC_SETTING "MTxt\""
                                 " {fg[ACTIVE]=\"#%02X%02X%02X\""
                                 " fg[PRELIGHT]=\"#%02X%02X%02X\"}"
                                 " style \"" RC_SETTING "PTxt\""
                                 " {fg[ACTIVE]=\"#%02X%02X%02X\""
                                 " fg[PRELIGHT]=\"#%02X%02X%02X\"}"
                                 " class \"*MenuItem\" style \"" RC_SETTING
                                 "MTxt\" widget_class \"*.*MenuItem*\" "
                                 "style \"" RC_SETTING "MTxt\" "
                                 " widget_class \"*.*ProgressBar\" "
                                 "style \"" RC_SETTING "PTxt\"",
                                 toQtColor(highlightedMenuCol->red),
                                 toQtColor(highlightedMenuCol->green),
                                 toQtColor(highlightedMenuCol->blue),
                                 toQtColor(highlightedMenuCol->red),
                                 toQtColor(highlightedMenuCol->green),
                                 toQtColor(highlightedMenuCol->blue),
                                 toQtColor(qtSettings.colors[PAL_ACTIVE]
                                           [COLOR_TEXT_SELECTED].red),
                                 toQtColor(qtSettings.colors[PAL_ACTIVE]
                                           [COLOR_TEXT_SELECTED].green),
                                 toQtColor(qtSettings.colors[PAL_ACTIVE]
                                           [COLOR_TEXT_SELECTED].blue),
                                 toQtColor(qtSettings.colors[PAL_ACTIVE]
                                           [COLOR_TEXT_SELECTED].red),
                                 toQtColor(qtSettings.colors[PAL_ACTIVE]
                                           [COLOR_TEXT_SELECTED].green),
                                 toQtColor(qtSettings.colors[PAL_ACTIVE]
                                           [COLOR_TEXT_SELECTED].blue));
                if (GTK_APP_OPEN_OFFICE == qtSettings.app) {
                    GdkColor *active = NULL;
                    GdkColor *inactive = NULL;

                    if (SHADE_WINDOW_BORDER == opts.shadeMenubars) {
                        active = (&qtSettings.colors[PAL_ACTIVE][
                                      opts.useHighlightForMenu ?
                                      COLOR_TEXT_SELECTED :
                                      COLOR_WINDOW_BORDER_TEXT]);
                        inactive = &qtSettings.colors[PAL_ACTIVE][
                            COLOR_WINDOW_BORDER_TEXT];
                    } else if (opts.customMenuTextColor) {
                        active = &opts.customMenuSelTextColor;
                        inactive = &opts.customMenuNormTextColor;
                    } else if (SHADE_BLEND_SELECTED == opts.shadeMenubars ||
                               SHADE_SELECTED == opts.shadeMenubars ||
                               (SHADE_CUSTOM == opts.shadeMenubars &&
                                TOO_DARK(qtcPalette.menubar[ORIGINAL_SHADE]))) {
                        active = &qtSettings.colors[PAL_ACTIVE][
                            COLOR_TEXT_SELECTED];
                        inactive = &qtSettings.colors[PAL_ACTIVE][
                            COLOR_TEXT_SELECTED];
                    }

                    if (active && inactive) {
                        qtc_gtkrc_printf(
                            str_buff, "style \"" RC_SETTING "MnuTxt\""
                            " {fg[NORMAL]=\"#%02X%02X%02X\" "
                            "fg[PRELIGHT]=\"#%02X%02X%02X\" "
                            "fg[ACTIVE]=\"#%02X%02X%02X\" "
                            "fg[SELECTED]=\"#%02X%02X%02X\" "
                            "text[NORMAL]=\"#%02X%02X%02X\"} "
                            "widget_class \"*<GtkMenuBar>*\" style \""
                            RC_SETTING "MnuTxt\" %s", toQtColor(inactive->red),
                            toQtColor(inactive->green),
                            toQtColor(inactive->blue), toQtColor(active->red),
                            toQtColor(active->green), toQtColor(active->blue),
                            toQtColor(active->red), toQtColor(active->green),
                            toQtColor(active->blue), toQtColor(active->red),
                            toQtColor(active->green), toQtColor(active->blue),
                            toQtColor(inactive->red),
                            toQtColor(inactive->green),
                            toQtColor(inactive->blue),
                            opts.shadePopupMenu ?
                            " widget_class \"*<GtkMenuItem>*\" style \""
                            RC_SETTING "MnuTxt\" " : " ");
                    }
                }
            }

            if (GTK_APP_VMPLAYER == qtSettings.app) {
                opts.shadeMenubars = SHADE_NONE;
                opts.menubarHiding = HIDE_NONE;
                opts.statusbarHiding = HIDE_NONE;
            }

            if (opts.mapKdeIcons && qtSettings.icons) {
                qtc_gtkrc_printf(str_buff, "gtk-icon-theme-name=\"%s\"",
                                 qtSettings.icons);
            }

            if(opts.mapKdeIcons && (path = getIconPath()))
            {
                const char *iconTheme=qtSettings.icons ? qtSettings.icons : "XX";
                /* '#' VERSION ' '<kde version> <..nums above..>\0 */
                int versionLen = (1 + strlen(qtcVersion()) + 1 +
                                  strlen(iconTheme) + 1 + 2 + 6 * 2 + 1);
                char *version = (char*)malloc(versionLen);

                getGtk2CfgFile(&tmpStr, "gtk-icons");
                sprintf(version, "#%s %s %02X%02X%02X%02X%02X%02X%02X",
                        qtcVersion(), iconTheme, 4,
                        qtSettings.iconSizes.smlTbSize,
                        qtSettings.iconSizes.tbSize,
                        qtSettings.iconSizes.dndSize,
                        qtSettings.iconSizes.btnSize,
                        qtSettings.iconSizes.mnuSize,
                        qtSettings.iconSizes.dlgSize);

                if (!checkFileVersion(tmpStr, version, versionLen)) {
                    static const char *constCmdStrFmt = "perl " QTC_GTK2_THEME_DIR "/map_kde_icons.pl " QTC_GTK2_THEME_DIR "/icons%d %s %d %d %d %d %d %d %d %s %s > %s.%d && mv %s.%d %s";

                    const char *kdeprefix = kdeIconsPrefix();
                    int fileNameLen = strlen(tmpStr);
                    char *cmdStr = (char*)malloc(strlen(constCmdStrFmt) + 2 +
                                                 4 * 6 + 2 + strlen(iconTheme) +
                                                 (kdeprefix ?
                                                  strlen(kdeprefix) :
                                                  DEFAULT_ICON_PREFIX_LEN) +
                                                 fileNameLen * 3 + 64 +
                                                 strlen(qtcVersion()) + 1);

                    sprintf(cmdStr, constCmdStrFmt, 4,
                            kdeprefix ? kdeprefix : DEFAULT_ICON_PREFIX, 4,
                            qtSettings.iconSizes.smlTbSize,
                            qtSettings.iconSizes.tbSize,
                            qtSettings.iconSizes.dndSize,
                            qtSettings.iconSizes.btnSize,
                            qtSettings.iconSizes.mnuSize,
                            qtSettings.iconSizes.dlgSize,
                            iconTheme, qtcVersion(), tmpStr, getpid(),
                            tmpStr, getpid(), tmpStr);
                    (void)system(cmdStr);
                    free(cmdStr);
                }
                free(version);
                gtk_rc_parse_string(path);
                gtk_rc_parse(tmpStr);
            }

            if((settings=gtk_settings_get_default()))
            {
                if(qtSettings.fonts[FONT_GENERAL])
                    g_object_set(settings, "gtk-font-name", qtSettings.fonts[FONT_GENERAL], NULL);

                gtk_settings_set_long_property(settings, "gtk-toolbar-style", qtSettings.toolbarStyle, "KDE-Settings");
                if(qtSettings.debug) printf(DEBUG_PREFIX "gtk-toolbar-style %d\n", qtSettings.toolbarStyle);
                if (gtk_check_version(2, 4, 0) == NULL) {
                    /* The following settings only apply for GTK>=2.4.0 */
                    if(qtSettings.debug) printf(DEBUG_PREFIX "gtk-button-images %d\n", qtSettings.buttonIcons);
                    gtk_settings_set_long_property(settings, "gtk-button-images", qtSettings.buttonIcons, "KDE-Settings");
#if 0
                    if(opts.drawStatusBarFrames)
                        gtk_rc_parse_string("style \"" RC_SETTING "StBar\""
                                            "{ GtkStatusbar::shadow-type = 1 }" /*GtkStatusbar::has-resize-grip = false }" */
                                            "class \"GtkStatusbar\" style"
                                            " \"" RC_SETTING "StBar\"");
                    else
                        gtk_rc_parse_string("style \"" RC_SETTING "SBar\""
                                            "{ GtkStatusbar::shadow-type = 0 }" /*GtkStatusbar::has-resize-grip = false }" */
                                            "class \"GtkStatusbar\" style"
                                            " \"" RC_SETTING "SBar\"");
#endif
                }

                /* The following settings only apply for GTK>=2.6.0 */
                if(!opts.gtkButtonOrder && NULL==gtk_check_version(2, 6, 0))
                    g_object_set(settings, "gtk-alternative-button-order", true, NULL);

                gtk_settings_set_long_property(settings, "gtk-menu-popup-delay", opts.menuDelay, "KDE-Settings");
            }

            if(qtSettings.fonts[FONT_GENERAL])
            {
                static const char *constFormat="style \"" RC_SETTING "Fnt\" {font_name=\"%s\"} "
                                               "widget_class \"*\" style \"" RC_SETTING "Fnt\" ";
                tmpStr=(char *)realloc(tmpStr, strlen(constFormat)+strlen(qtSettings.fonts[FONT_GENERAL])+1);

                sprintf(tmpStr, constFormat, qtSettings.fonts[FONT_GENERAL]);
                gtk_rc_parse_string(tmpStr);
            }

            if(qtSettings.fonts[FONT_BOLD] && qtSettings.fonts[FONT_GENERAL] && strcmp(qtSettings.fonts[FONT_BOLD], qtSettings.fonts[FONT_GENERAL]))
            {
                static const char *constBoldPrefix="style \"" RC_SETTING "BFnt\"{font_name=\"";
                static const char *constBoldSuffix="\"} class \"GtkProgress\" style \"" RC_SETTING "BFnt\" "
                                                   "widget_class \"*GtkProgress*\" style \"" RC_SETTING "BFnt\" ";

                if(opts.gbLabel&GB_LBL_BOLD)
                {
                    static const char *constStdPrefix="style \"" RC_SETTING "Fnt\"{font_name=\"";
                    static const char *constStdSuffix="\"} ";
                    static const char *constGrpBoxBoldSuffix="widget_class \"*Frame.GtkLabel\" style \"" RC_SETTING "BFnt\" "
                                                             "widget_class \"*Statusbar.*Frame.GtkLabel\" style \"" RC_SETTING "Fnt\"";
                    tmpStr=(char *)realloc(tmpStr, strlen(constStdPrefix)+strlen(qtSettings.fonts[FONT_GENERAL])+strlen(constStdSuffix)+
                                                   strlen(constBoldPrefix)+strlen(qtSettings.fonts[FONT_BOLD])+
                                                   (opts.boldProgress ? strlen(constBoldSuffix) : strlen(constStdSuffix))+
                                                   strlen(constGrpBoxBoldSuffix)+1);

                    sprintf(tmpStr, "%s%s%s%s%s%s%s",
                                    constStdPrefix, qtSettings.fonts[FONT_GENERAL], constStdSuffix,
                                    constBoldPrefix, qtSettings.fonts[FONT_BOLD], opts.boldProgress ? constBoldSuffix : constStdSuffix,
                                    constGrpBoxBoldSuffix);
                }
                else if(opts.boldProgress)
                {
                    tmpStr=(char *)realloc(tmpStr, strlen(constBoldPrefix)+strlen(qtSettings.fonts[FONT_BOLD])+strlen(constBoldSuffix)+1);
                    sprintf(tmpStr, "%s%s%s", constBoldPrefix, qtSettings.fonts[FONT_BOLD], constBoldSuffix);
                }

                gtk_rc_parse_string(tmpStr);
            }

            if(qtSettings.fonts[FONT_MENU] && qtSettings.fonts[FONT_GENERAL] && strcmp(qtSettings.fonts[FONT_MENU], qtSettings.fonts[FONT_GENERAL]))
            {
                static const char *constFormat="style \"" RC_SETTING "MFnt\" {font_name=\"%s\"} "
                                               "widget_class \"*.*MenuItem.*\" style \"" RC_SETTING "MFnt\" ";
                tmpStr=(char *)realloc(tmpStr, strlen(constFormat)+strlen(qtSettings.fonts[FONT_MENU])+1);

                sprintf(tmpStr, constFormat, qtSettings.fonts[FONT_MENU]);
                gtk_rc_parse_string(tmpStr);
            }

            if(qtSettings.fonts[FONT_TOOLBAR] && qtSettings.fonts[FONT_GENERAL] && strcmp(qtSettings.fonts[FONT_TOOLBAR], qtSettings.fonts[FONT_GENERAL]))
            {
                static const char *constFormat="style \"" RC_SETTING "TbFnt\" {font_name=\"%s\"} "
                                               "widget_class \"*.*Toolbar.*\" style \"" RC_SETTING "TbFnt\" ";
                tmpStr=(char *)realloc(tmpStr, strlen(constFormat)+strlen(qtSettings.fonts[FONT_TOOLBAR])+1);

                sprintf(tmpStr, constFormat, qtSettings.fonts[FONT_TOOLBAR]);
                gtk_rc_parse_string(tmpStr);
            }

            if((opts.thin&THIN_MENU_ITEMS))
                gtk_rc_parse_string("style \"" RC_SETTING "Mi\" {xthickness = 1 ythickness = 2 } "
                                    "class \"*MenuItem\" style \"" RC_SETTING "Mi\"");

            /* Set password character... */
/*
            {
                static const char *constPasswdStrFormat="style \"QtCPasswd\" { GtkEntry::invisible-char='$' } class \"*\" style \"QtCPasswd\"";

                tmpStr=(char *)realloc(tmpStr, strlen(constPasswdStrFormat)+16);
                sprintf(tmpStr, constPasswdStrFormat, opts.passwordChar);
                gtk_rc_parse_string(tmpStr);
            }
*/
            /* For some reason Firefox 3beta4 goes mad if GtkComboBoxEntry::appears-as-list = 1 !!!! */
            if(QtCurve::isMozilla())
                gtk_rc_parse_string("style \"" RC_SETTING "Mz\" { GtkComboBoxEntry::appears-as-list = 0 } class \"*\" style \"" RC_SETTING "Mz\"");
            else if(!opts.gtkComboMenus)
            {
                gtk_rc_parse_string("style \"" RC_SETTING "Cmb\" { GtkComboBox::appears-as-list = 1 } class \"*\" style \"" RC_SETTING "Cmb\"");
                gtk_rc_parse_string("style \"" RC_SETTING "Cmbf\" { xthickness=5 } widget_class \"*.GtkComboBox.GtkFrame\" style \"" RC_SETTING "Cmbf\"");
            }

            if (qtcOneOf(qtSettings.app, GTK_APP_MOZILLA, GTK_APP_JAVA) ||
                (opts.scrollbarType == SCROLLBAR_NONE && QtCurve::isMozilla())) {
                opts.scrollbarType = SCROLLBAR_WINDOWS;
            } else {
                static const char *constSbStrFormat="style \"" RC_SETTING "SBt\" "
                                                    "{ GtkScrollbar::has-backward-stepper=%d "
                                                      "GtkScrollbar::has-forward-stepper=%d "
                                                      "GtkScrollbar::has-secondary-backward-stepper=%d "
                                                      "GtkScrollbar::has-secondary-forward-stepper=%d } "
                                                    "class \"*\" style \"" RC_SETTING "SBt\"";
                tmpStr=(char *)realloc(tmpStr, strlen(constSbStrFormat)+1);

                if(GTK_APP_OPEN_OFFICE==qtSettings.app)
                {
                    if(SCROLLBAR_NEXT==opts.scrollbarType)
                        opts.scrollbarType=SCROLLBAR_KDE;
                    else if(SCROLLBAR_NONE==opts.scrollbarType)
                        opts.scrollbarType=SCROLLBAR_WINDOWS;
                }

                switch(opts.scrollbarType)
                {
                    default:
                    case SCROLLBAR_KDE:
                        sprintf(tmpStr, constSbStrFormat, 1, 1, 1, 0);
                        break;
                    case SCROLLBAR_WINDOWS:
                        sprintf(tmpStr, constSbStrFormat, 1, 1, 0, 0);
                        break;
                    case SCROLLBAR_PLATINUM:
                        sprintf(tmpStr, constSbStrFormat, 0, 1, 1, 0);
                        break;
                    case SCROLLBAR_NEXT:
                        sprintf(tmpStr, constSbStrFormat, 1, 0, 0, 1);
                        break;
                    case SCROLLBAR_NONE:
                        sprintf(tmpStr, constSbStrFormat, 0, 0, 0, 0);
                        break;
                }

                gtk_rc_parse_string(tmpStr);
            }

            /* Set cursor colours... */
            const char *constStrFormat="style \"" RC_SETTING "Crsr\" "
                "{ GtkWidget::cursor-color=\"#%02X%02X%02X\" "
                "GtkWidget::secondary-cursor-color=\"#%02X%02X%02X\" } "
                "class \"*\" style \"" RC_SETTING "Crsr\"";
            tmpStr=(char *)realloc(tmpStr, strlen(constStrFormat)+1);

            sprintf(tmpStr, constStrFormat,
                    qtSettings.colors[PAL_ACTIVE][COLOR_TEXT].red>>8,
                    qtSettings.colors[PAL_ACTIVE][COLOR_TEXT].green>>8,
                    qtSettings.colors[PAL_ACTIVE][COLOR_TEXT].blue>>8,
                    qtSettings.colors[PAL_ACTIVE][COLOR_TEXT].red>>8,
                    qtSettings.colors[PAL_ACTIVE][COLOR_TEXT].green>>8,
                    qtSettings.colors[PAL_ACTIVE][COLOR_TEXT].blue>>8);
            gtk_rc_parse_string(tmpStr);

            if(!opts.gtkScrollViews && NULL!=gtk_check_version(2, 12, 0))
                opts.gtkScrollViews=true;

            bool doEffect=EFFECT_NONE!=opts.buttonEffect;
            int  thickness=2;

            constStrFormat =
                "style \"" RC_SETTING "Etch2\" "
                "{ xthickness = 3 ythickness = %d} "
                "class \"*Button\" style \"" RC_SETTING "Etch2\""
                "class \"*GtkOptionMenu\" style \"" RC_SETTING "Etch2\"";

            tmpStr = (char*)realloc(tmpStr, strlen(constStrFormat) + 2);
            sprintf(tmpStr, constStrFormat, (opts.thin & THIN_BUTTONS) ||
                    !doEffect ? 1 : 2);
            gtk_rc_parse_string(tmpStr);

            constStrFormat =
                "style \"" RC_SETTING "EtchE\" { xthickness = %d "
                "ythickness = %d } style \"" RC_SETTING "EtchS\" "
                "{ xthickness = %d ythickness = %d } widget_class "
                "\"*Toolbar*GtkSpinButton\" style \"" RC_SETTING "EtchE\" "
                "class \"*GtkEntry\" style  \"" RC_SETTING "EtchE\" "
                "widget_class \"*Toolbar*Entry\" style \"" RC_SETTING "EtchE\" "
                "class \"*GtkSpinButton\" style \"" RC_SETTING "EtchS\" ";

            int thick = /*opts.etchEntry && doEffect ?*/ 4 /*: 3*/;
            tmpStr = (char*)realloc(tmpStr, strlen(constStrFormat) + 8);
            sprintf(tmpStr, constStrFormat, thick, thick, thick, thick);
            gtk_rc_parse_string(tmpStr);

            if (QtCurve::isMozilla()) {
                constStrFormat =
                    "style \"" RC_SETTING "EtchEM\" { xthickness = %d "
                    "ythickness = %d } widget_class "
                    "\"*GtkFixed*GtkSpinButton\" style \"" RC_SETTING
                    "EtchEM\" widget_class \"*GtkFixed*Entry\" style \""
                    RC_SETTING "EtchEM\" ";

                int thick = opts.etchEntry && doEffect ? 3 : 2;
                tmpStr = (char*)realloc(tmpStr, strlen(constStrFormat) + 4);
                sprintf(tmpStr, constStrFormat, thick, thick);
                gtk_rc_parse_string(tmpStr);
            }

            if(!opts.gtkScrollViews)
                gtk_rc_parse_string("style \"" RC_SETTING "SV\""
                                    " { GtkScrolledWindow::scrollbar-spacing = 0 "
                                      " GtkScrolledWindow::scrollbars-within-bevel = 1 } "
                                    "class \"GtkScrolledWindow\" style \"" RC_SETTING "SV\"");
            else if(opts.etchEntry)
                gtk_rc_parse_string("style \"" RC_SETTING "SV\""
                                    " { GtkScrolledWindow::scrollbar-spacing = 2 } "
                                    "class \"GtkScrolledWindow\" style \"" RC_SETTING "SV\"");

            /* Scrolled windows */
            if((opts.square&SQUARE_SCROLLVIEW))
                thickness=!opts.highlightScrollViews && (opts.gtkScrollViews || opts.thinSbarGroove || !opts.borderSbarGroove) ? 1 : 2;
            else if(opts.etchEntry)
                thickness=3;

            constStrFormat =
                "style \"" RC_SETTING "SVt\" { xthickness = %d "
                "ythickness = %d } class \"*GtkScrolledWindow\" style \""
                RC_SETTING "SVt\"";
            tmpStr = (char*)realloc(tmpStr, strlen(constStrFormat) + 1);
            sprintf(tmpStr, constStrFormat, thickness, thickness);
            gtk_rc_parse_string(tmpStr);

            constStrFormat =
                "style \"" RC_SETTING "Pbar\" { xthickness = %d "
                "ythickness = %d } widget_class \"*GtkProgressBar\" style \""
                RC_SETTING "Pbar\"";
            int pthickness = (opts.fillProgress ?
                              doEffect && opts.borderProgress ? 1 : 0 :
                              doEffect ? 2 : 1);
            tmpStr = (char*)realloc(tmpStr, strlen(constStrFormat) + 1);
            sprintf(tmpStr, constStrFormat, pthickness, pthickness);
            gtk_rc_parse_string(tmpStr);

            constStrFormat =
                "style \"" RC_SETTING "TT\" { xthickness = 4 "
                "ythickness = 4 bg[NORMAL] = \"#%02X%02X%02X\" "
                "fg[NORMAL] = \"#%02X%02X%02X\"} widget \"gtk-tooltips*\" "
                "style \"" RC_SETTING "TT\" widget \"gtk-tooltip*\" style \""
                RC_SETTING "TT\"";

            tmpStr = (char*)realloc(tmpStr, strlen(constStrFormat) + 1);
            sprintf(tmpStr, constStrFormat,
                    toQtColor(qtSettings.colors[PAL_ACTIVE][COLOR_TOOLTIP].red),
                    toQtColor(qtSettings.colors[PAL_ACTIVE][COLOR_TOOLTIP].green),
                    toQtColor(qtSettings.colors[PAL_ACTIVE][COLOR_TOOLTIP].blue),
                    toQtColor(
                        qtSettings.colors[PAL_ACTIVE][COLOR_TOOLTIP_TEXT].red),
                    toQtColor(
                        qtSettings.colors[PAL_ACTIVE][COLOR_TOOLTIP_TEXT].green),
                    toQtColor(
                        qtSettings.colors[PAL_ACTIVE][COLOR_TOOLTIP_TEXT].blue));
            gtk_rc_parse_string(tmpStr);

            if( EFFECT_NONE==opts.buttonEffect)
                gtk_rc_parse_string("style \"" RC_SETTING "Cmb\" { xthickness = 4 ythickness = 2 }"
                                    "widget_class \"*.GtkCombo.GtkEntry\" style \"" RC_SETTING "Cmb\"");

            if(opts.round>=ROUND_FULL && EFFECT_NONE!=opts.buttonEffect)
                gtk_rc_parse_string("style \"" RC_SETTING "Swt\" { xthickness = 3 ythickness = 2 }"
                                    "widget_class \"*.SwtFixed.GtkCombo.GtkButton\" style \"" RC_SETTING "Swt\""
                                    "widget_class \"*.SwtFixed.GtkCombo.GtkEntry\" style \"" RC_SETTING "Swt\"");


            gtk_rc_parse_string("style \"" RC_SETTING "MnuTb\" "
                                "{ xthickness=1 ythickness=1"
                                " GtkButton::focus-padding=0 GtkWidget::focus-line-width=0} "
                                "class \"*GtkMenuToolButton\" style \"" RC_SETTING "MnuTb\""
                                "widget_class \"*.GtkMenuToolButton.*Box.GtkToggleButton\" style \"" RC_SETTING "MnuTb\"");

            if(!opts.popupBorder)
                gtk_rc_parse_string("style \"" RC_SETTING "M\" { xthickness=0 ythickness=0 }\n"
                                    "class \"*GtkMenu\" style \"" RC_SETTING "M\"");
            else if(!qtcDrawMenuBorder(&opts) && !opts.borderMenuitems &&
                    opts.square & SQUARE_POPUP_MENUS)
                gtk_rc_parse_string("style \"" RC_SETTING "M\" { xthickness=1 ythickness=1 }\n"
                                    "class \"*GtkMenu\" style \"" RC_SETTING "M\"");

            constStrFormat =
                "style \""  RC_SETTING  "Tree\" { GtkTreeView::odd-row-color = "
                "\"#%02X%02X%02X\" GtkTreeView::even-row-color = "
                "\"#%02X%02X%02X\"} widget \"*GtkTreeView*\" style \""
                RC_SETTING "Tree\"";
            int alt = QtCurve::haveAlternateListViewCol() ? COLOR_LV : COLOR_BACKGROUND;

            tmpStr = (char*)realloc(tmpStr, strlen(constStrFormat) + 1);
            sprintf(tmpStr, constStrFormat,
                    toQtColor(qtSettings.colors[PAL_ACTIVE][alt].red),
                    toQtColor(qtSettings.colors[PAL_ACTIVE][alt].green),
                    toQtColor(qtSettings.colors[PAL_ACTIVE][alt].blue),
                    toQtColor(
                        qtSettings.colors[PAL_ACTIVE][COLOR_BACKGROUND].red),
                    toQtColor(
                        qtSettings.colors[PAL_ACTIVE][COLOR_BACKGROUND].green),
                    toQtColor(
                        qtSettings.colors[PAL_ACTIVE][COLOR_BACKGROUND].blue));
            gtk_rc_parse_string(tmpStr);

            if (!opts.useHighlightForMenu) {
                constStrFormat =
                    "style \"" RC_SETTING "Mnu\" { text[ACTIVE] = "
                    "\"#%02X%02X%02X\"  text[SELECTED] = \"#%02X%02X%02X\" } "
                    " class \"*MenuItem\" style \"" RC_SETTING "Mnu\""
                    " widget_class \"*MenuBar*MenuItem\" style \"" RC_SETTING
                    "Mnu\" widget_class \"*.GtkAccelMenuItem\" style \""
                    RC_SETTING "Mnu\" widget_class \"*.GtkRadioMenuItem\" "
                    "style \"" RC_SETTING "Mnu\" widget_class "
                    "\"*.GtkCheckMenuItem\" style \"" RC_SETTING "Mnu\""
                    " widget_class \"*.GtkImageMenuItem\" style \"" RC_SETTING
                    "Mnu\"";

                tmpStr = (char*)realloc(tmpStr, strlen(constStrFormat) + 1);
                sprintf(tmpStr, constStrFormat,
                        toQtColor(qtSettings.colors[PAL_ACTIVE][COLOR_TEXT].red),
                        toQtColor(qtSettings.colors[PAL_ACTIVE][COLOR_TEXT].green),
                        toQtColor(qtSettings.colors[PAL_ACTIVE][COLOR_TEXT].blue),
                        toQtColor(qtSettings.colors[PAL_ACTIVE][COLOR_TEXT].red),
                        toQtColor(qtSettings.colors[PAL_ACTIVE][COLOR_TEXT].green),
                        toQtColor(qtSettings.colors[PAL_ACTIVE][COLOR_TEXT].blue));
                gtk_rc_parse_string(tmpStr);
            }

            /* Mozilla seems to assume that all scrolledviews are square :-(
               So, set the xthickness and ythickness to 1, and in qtcurve.c draw these as square */
            if(QtCurve::isMozilla())
                gtk_rc_parse_string("style \"" RC_SETTING "SVm\""
                                    " { xthickness=1 ythickness=1 } "
                                    "widget_class \"GtkWindow.GtkFixed.GtkScrolledWindow\" style \"" RC_SETTING "SVm\"");

            if(TAB_MO_GLOW==opts.tabMouseOver)
                gtk_rc_parse_string("style \"" RC_SETTING "Tab\" { GtkNotebook::tab-overlap = 0 } class \"*GtkNotebook\" style \"" RC_SETTING "Tab\"");

            if (!opts.useHighlightForMenu &&
                GTK_APP_OPEN_OFFICE == qtSettings.app) {
                constStrFormat =
                    "style \"" RC_SETTING "OOMnu\" "
                    "{ bg[SELECTED] = \"#%02X%02X%02X\" } "
                    " class \"*Menu*\" style \"" RC_SETTING "OOMnu\" "
                    " widget_class \"*Menu*\" style \"" RC_SETTING "OOMnu\" ";


                QtCurve::shadeColors(&qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW], qtcPalette.background);
                tmpStr = (char*)realloc(tmpStr, strlen(constStrFormat)+1);
                sprintf(tmpStr, constStrFormat,
                        toQtColor(qtcPalette.background[4].red),
                        toQtColor(qtcPalette.background[4].green),
                        toQtColor(qtcPalette.background[4].blue));
                gtk_rc_parse_string(tmpStr);
            }

            if (DEFAULT_SLIDER_WIDTH != opts.sliderWidth) {
                constStrFormat =
                    "style \"" RC_SETTING "SbarW\" "
                    "{ GtkRange::slider_width = %d GtkRange::stepper_size = %d "
                    "  GtkScrollbar::min_slider_length = %d } "
                    " class \"*\" style \"" RC_SETTING "SbarW\" ";

                tmpStr = (char*)realloc(tmpStr, strlen(constStrFormat) + 16);
                sprintf(tmpStr, constStrFormat, opts.sliderWidth,
                        opts.sliderWidth, opts.sliderWidth+1);
                gtk_rc_parse_string(tmpStr);
            }

            gboolean customSliderW=DEFAULT_SLIDER_WIDTH!=opts.sliderWidth;
            int glowSize = (opts.buttonEffect != EFFECT_NONE &&
                            opts.coloredMouseOver == MO_GLOW ? 2 : 0);
            int length = (SLIDER_CIRCULAR==opts.sliderStyle ?
                          CIRCULAR_SLIDER_SIZE :
                          SLIDER_TRIANGULAR == opts.sliderStyle ? 11 :
                          SLIDER_PLAIN_ROTATED == opts.sliderStyle ||
                          SLIDER_ROUND_ROTATED == opts.sliderStyle ?
                          (customSliderW ? SLIDER_SIZE - 2 : 13) :
                          (customSliderW ? SLIDER_SIZE + 6 : 21)) + glowSize;
            int width = (SLIDER_CIRCULAR == opts.sliderStyle ?
                         CIRCULAR_SLIDER_SIZE :
                         SLIDER_TRIANGULAR == opts.sliderStyle ? 18 :
                         SLIDER_PLAIN_ROTATED == opts.sliderStyle ||
                         SLIDER_ROUND_ROTATED == opts.sliderStyle ?
                         (customSliderW ? SLIDER_SIZE + 6 : 21) :
                         (customSliderW ? SLIDER_SIZE - 2 : 13)) + glowSize;

            constStrFormat =
                "style \"" RC_SETTING "Sldr\" "
                "{GtkScale::slider_length = %d GtkScale::slider_width = %d} "
                "class \"*\" style \"" RC_SETTING "Sldr\"";

            tmpStr = (char*)realloc(tmpStr, strlen(constStrFormat) + 8);
            sprintf(tmpStr, constStrFormat, length, width);
            gtk_rc_parse_string(tmpStr);

            if(!opts.menuIcons)
                gtk_rc_parse_string("gtk-menu-images=0");

            if(opts.hideShortcutUnderline)
                gtk_rc_parse_string("gtk-auto-mnemonics=1");

            if(LINE_1DOT==opts.splitters)
                gtk_rc_parse_string("style \"" RC_SETTING "Spl\" { GtkPaned::handle_size=7 GtkPaned::handle_width = 7 } "
                                    "class \"*GtkWidget\" style \"" RC_SETTING "Spl\"");

            if (IMG_PLAIN_RINGS == opts.bgndImage.type ||
                IMG_BORDERED_RINGS == opts.bgndImage.type ||
                IMG_SQUARE_RINGS == opts.bgndImage.type ||
                IMG_PLAIN_RINGS == opts.menuBgndImage.type ||
                IMG_BORDERED_RINGS == opts.menuBgndImage.type ||
                IMG_SQUARE_RINGS == opts.menuBgndImage.type) {
                qtcCalcRingAlphas(
                    &qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW]);
            }

            if(QtCurve::isMozilla())
                opts.crSize=CR_SMALL_SIZE;

            if (opts.crSize != CR_LARGE_SIZE) {
                constStrFormat =
                    "style \"" RC_SETTING "CRSize\" "
                    "{ GtkCheckButton::indicator_size = %d }"
                    " class \"*\" style \"" RC_SETTING "CRSize\" ";
                tmpStr = (char*)realloc(tmpStr, strlen(constStrFormat) + 16);
                sprintf(tmpStr, constStrFormat, opts.crSize);
                gtk_rc_parse_string(tmpStr);
            }

#if 0
// Remove because, in KDE4 at least, if have two locked toolbars together then the last/first items are too close
            if(TB_NONE==opts.toolbarBorders)
                gtk_rc_parse_string("style \"" RC_SETTING "TbB\" { xthickness = 0 ythickness = 0 GtkToolbar::internal-padding = 0 }"
                                    " widget_class \"*<GtkToolbar>\" style  \"" RC_SETTING "TbB\"");
#endif

            if(TBTN_RAISED==opts.tbarBtns || TBTN_JOINED==opts.tbarBtns)
                gtk_rc_parse_string("style \"" RC_SETTING "TbJ\" { GtkToolbar::button-relief = 1 } "
                                    "widget_class \"*<GtkToolbar>\"  style \"" RC_SETTING "TbJ\"");

            QTC_FREE_LOCAL_BUFF(str_buff);
            qtcFree(tmpStr);

            if(opts.shadeMenubarOnlyWhenActive && SHADE_WINDOW_BORDER==opts.shadeMenubars &&
               EQUAL_COLOR(qtSettings.colors[PAL_ACTIVE][COLOR_WINDOW_BORDER], qtSettings.colors[PAL_INACTIVE][COLOR_WINDOW_BORDER]))
                opts.shadeMenubarOnlyWhenActive=false;

            if (!(opts.square&SQUARE_POPUP_MENUS) || !(opts.square&SQUARE_TOOLTIPS))
            {
                readKwinrc();
            }
            setlocale(LC_NUMERIC, locale);
        }
        return true;
    }
    return false;
}

#if 0
static void qtSettingsExit()
{
    qt_refs--;
    if (!qt_refs) {
        for(int i = 0;i < FONT_NUM_TOTAL;++i) {
            qtcFree(qtSettings.fonts[i]);
            qtSettings.fonts[i] = NULL;
        }
        qtcFree(qtSettings.icons);
        qtSettings.icons = NULL;
    }
}
#endif

#define SET_COLOR_PAL(st, rc, itm, ITEM, state, QTP_COL, PAL, USE_DIS) \
    st->itm[state]=rc->color_flags[state]&ITEM \
        ? rc->itm[state] \
        : qtSettings.colors[state==GTK_STATE_INSENSITIVE && USE_DIS\
                                ? PAL_DISABLED \
                                : PAL ][QTP_COL];

#define SET_COLOR_PAL_ACT(st, rc, itm, ITEM, state, QTP_COL) \
    st->itm[state]=rc->color_flags[state]&ITEM \
        ? rc->itm[state] \
        : qtSettings.colors[PAL_ACTIVE][QTP_COL];

#define SET_COLOR_X(st, rc, itm, ITEM, state, QTP_COL, USE_DIS) \
    SET_COLOR_PAL(st, rc, itm, ITEM, state, QTP_COL, PAL_ACTIVE, USE_DIS)

#define SET_COLOR(st, rc, itm, ITEM, state, QTP_COL) \
    SET_COLOR_PAL(st, rc, itm, ITEM, state, QTP_COL, PAL_ACTIVE, true)

void qtSettingsSetColors(GtkStyle *style, GtkRcStyle *rc_style)
{
    SET_COLOR(style, rc_style, bg, GTK_RC_BG, GTK_STATE_NORMAL, COLOR_WINDOW)
    SET_COLOR(style, rc_style, bg, GTK_RC_BG, GTK_STATE_SELECTED, COLOR_SELECTED)
    SET_COLOR_X(style, rc_style, bg, GTK_RC_BG, GTK_STATE_INSENSITIVE, COLOR_WINDOW, false)
    SET_COLOR(style, rc_style, bg, GTK_RC_BG, GTK_STATE_ACTIVE, COLOR_MID)
    SET_COLOR(style, rc_style, bg, GTK_RC_BG, GTK_STATE_PRELIGHT, COLOR_WINDOW)

    SET_COLOR(style, rc_style, base, GTK_RC_BASE, GTK_STATE_NORMAL, COLOR_BACKGROUND)
    SET_COLOR(style, rc_style, base, GTK_RC_BASE, GTK_STATE_SELECTED, COLOR_SELECTED)
    SET_COLOR_PAL_ACT(style, rc_style, base, GTK_RC_BASE, GTK_STATE_INSENSITIVE, COLOR_WINDOW)
    SET_COLOR(style, rc_style, base, GTK_RC_BASE, GTK_STATE_ACTIVE, COLOR_SELECTED)
    if(qtSettings.inactiveChangeSelectionColor)
        style->base[GTK_STATE_ACTIVE]=qtSettings.colors[PAL_INACTIVE][COLOR_SELECTED];
    SET_COLOR(style, rc_style, base, GTK_RC_BASE, GTK_STATE_PRELIGHT, COLOR_BACKGROUND)

    SET_COLOR(style, rc_style, text, GTK_RC_TEXT, GTK_STATE_NORMAL, COLOR_TEXT)
    SET_COLOR(style, rc_style, text, GTK_RC_TEXT, GTK_STATE_SELECTED, COLOR_TEXT_SELECTED)
    SET_COLOR(style, rc_style, text, GTK_RC_TEXT, GTK_STATE_INSENSITIVE, COLOR_TEXT)
    SET_COLOR(style, rc_style, text, GTK_RC_TEXT, GTK_STATE_ACTIVE, COLOR_TEXT_SELECTED)

//     if(opts.inactiveHighlight)
//         SET_COLOR(style, rc_style, text, GTK_RC_TEXT, GTK_STATE_ACTIVE, COLOR_TEXT)
//     else
//         SET_COLOR_PAL(style, rc_style, text, GTK_RC_TEXT, GTK_STATE_ACTIVE, COLOR_TEXT_SELECTED, PAL_INACTIVE)

    SET_COLOR(style, rc_style, text, GTK_RC_TEXT, GTK_STATE_PRELIGHT, COLOR_TEXT)

    SET_COLOR(style, rc_style, fg, GTK_RC_FG, GTK_STATE_NORMAL, COLOR_WINDOW_TEXT)
    SET_COLOR(style, rc_style, fg, GTK_RC_FG, GTK_STATE_SELECTED, COLOR_TEXT_SELECTED)
//     if (QtCurve::isMozilla())
        SET_COLOR(style, rc_style, fg, GTK_RC_FG, GTK_STATE_INSENSITIVE, COLOR_TEXT)
//     else
//         SET_COLOR(style, rc_style, fg, GTK_RC_FG, GTK_STATE_INSENSITIVE, COLOR_MID)
    SET_COLOR(style, rc_style, fg, GTK_RC_FG, GTK_STATE_ACTIVE, COLOR_WINDOW_TEXT)
    SET_COLOR(style, rc_style, fg, GTK_RC_FG, GTK_STATE_PRELIGHT, COLOR_WINDOW_TEXT)
}
