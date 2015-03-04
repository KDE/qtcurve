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

#include <qtcurve-utils/log.h>
#include <qtcurve-utils/map.h>
#include <qtcurve-utils/dirs.h>
#include <qtcurve-utils/strs.h>
#include <qtcurve-utils/color.h>

#include "common.h"
#include "config_file.h"

#include <fstream>

#define CONFIG_FILE               "stylerc"
#define OLD_CONFIG_FILE           "qtcurvestylerc"
#define VERSION_KEY               "version"

static int
c2h(char ch)
{
    return (ch>='0' && ch<='9') ? ch-'0' :
           (ch>='a' && ch<='f') ? 10+(ch-'a') :
           (ch>='A' && ch<='F') ? 10+(ch-'A') :
           0;
}

#define ATOH(str) ((c2h(*str)<<4)+c2h(*(str+1)))

void qtcSetRgb(GdkColor *col, const char *str)
{
    if (str && strlen(str)>6) {
        int offset = '#' == str[0] ? 1 : 0;
        col->red = ATOH(&str[offset]) << 8;
        col->green = ATOH(&str[offset + 2]) << 8;
        col->blue = ATOH(&str[offset + 4]) << 8;
        col->pixel = 0;
    } else {
        col->red = col->green = col->blue = col->pixel = 0;
    }
}

static bool
loadImage(const char *file, QtCPixmap *pixmap)
{
    pixmap->img = gdk_pixbuf_new_from_file(
        QtCurve::getConfFile(std::string(file)).c_str(), nullptr);
    return pixmap->img != nullptr;
}

static EDefBtnIndicator
toInd(const char *str, EDefBtnIndicator def)
{
    static const QtCurve::StrMap<EDefBtnIndicator> map{
        {"fontcolor", IND_FONT_COLOR},
        {"border", IND_FONT_COLOR},
        {"none", IND_NONE},
        {"corner", IND_CORNER},
        {"colored", IND_COLORED},
        {"tint", IND_TINT},
        {"glow", IND_GLOW},
        {"darken", IND_DARKEN},
        {"origselected", IND_SELECTED},
    };
    return map.search(str, def);
}

static ELine toLine(const char *str, ELine def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "dashes", 6))
            return LINE_DASHES;
        if(0==memcmp(str, "none", 4))
            return LINE_NONE;
        if(0==memcmp(str, "sunken", 6))
            return LINE_SUNKEN;
        if(0==memcmp(str, "dots", 4))
            return LINE_DOTS;
        if(0==memcmp(str, "flat", 4))
            return LINE_FLAT;
        if(0==memcmp(str, "1dot", 5))
            return LINE_1DOT;
    }
    return def;
}

static ETBarBorder toTBarBorder(const char *str, ETBarBorder def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "dark", 4))
            return 0==memcmp(&str[4], "-all", 4) ? TB_DARK_ALL : TB_DARK;
        if(0==memcmp(str, "none", 4))
            return TB_NONE;
        if(0==memcmp(str, "light", 5))
            return 0==memcmp(&str[5], "-all", 4) ? TB_LIGHT_ALL : TB_LIGHT;
    }
    return def;
}

static EMouseOver toMouseOver(const char *str, EMouseOver def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "true", 4) || 0==memcmp(str, "colored", 7))
            return MO_COLORED;
        if(0==memcmp(str, "thickcolored", 12))
            return MO_COLORED_THICK;
        if(0==memcmp(str, "plastik", 7))
            return MO_PLASTIK;
        if(0==memcmp(str, "glow", 4))
            return MO_GLOW;
        if(0==memcmp(str, "false", 4) || 0==memcmp(str, "none", 4))
            return MO_NONE;
    }
    return def;
}

static EAppearance toAppearance(const char *str, EAppearance def, EAppAllow allow, QtCPixmap *pix, bool checkImage)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "flat", 4))
            return APPEARANCE_FLAT;
        if(0==memcmp(str, "raised", 6))
            return APPEARANCE_RAISED;
        if(0==memcmp(str, "dullglass", 9))
            return APPEARANCE_DULL_GLASS;
        if(0==memcmp(str, "glass", 5) || 0==memcmp(str, "shinyglass", 10))
            return APPEARANCE_SHINY_GLASS;
        if(0==memcmp(str, "agua", 4))
            return APPEARANCE_AGUA;
        if(0==memcmp(str, "soft", 4))
            return APPEARANCE_SOFT_GRADIENT;
        if(0==memcmp(str, "gradient", 8) || 0==memcmp(str, "lightgradient", 13))
            return APPEARANCE_GRADIENT;
        if(0==memcmp(str, "harsh", 5))
            return APPEARANCE_HARSH_GRADIENT;
        if(0==memcmp(str, "inverted", 8))
            return APPEARANCE_INVERTED;
        if(0==memcmp(str, "darkinverted", 12))
            return APPEARANCE_DARK_INVERTED;
        if(0==memcmp(str, "splitgradient", 13))
            return APPEARANCE_SPLIT_GRADIENT;
        if(0==memcmp(str, "bevelled", 8))
            return APPEARANCE_BEVELLED;
        if(APP_ALLOW_FADE==allow && 0==memcmp(str, "fade", 4))
            return APPEARANCE_FADE;
        if(APP_ALLOW_STRIPED==allow && 0==memcmp(str, "striped", 7))
            return APPEARANCE_STRIPED;
        if(APP_ALLOW_NONE==allow && 0==memcmp(str, "none", 4))
            return APPEARANCE_NONE;
        if(nullptr!=pix && APP_ALLOW_STRIPED==allow && 0==memcmp(str, "file", 4) && strlen(str)>9)
            return loadImage(&str[5], pix) || !checkImage ? APPEARANCE_FILE : def;

        if(0==memcmp(str, "customgradient", 14) && strlen(str)>14)
        {
            int i=atoi(&str[14]);

            i--;
            if(i>=0 && i<NUM_CUSTOM_GRAD)
                return (EAppearance)(APPEARANCE_CUSTOM1+i);
        }
    }
    return def;
}

static EShade
toShade(const char *str, bool allowMenu, EShade def,
        bool menuShade, GdkColor *col)
{
    if(str && 0!=str[0])
    {
        /* true/false is from 0.25... */
        if((!menuShade && 0==memcmp(str, "true", 4)) || 0==memcmp(str, "selected", 8))
            return SHADE_BLEND_SELECTED;
        if(0==memcmp(str, "origselected", 12))
            return SHADE_SELECTED;
        if(allowMenu && (0==memcmp(str, "darken", 6) || (menuShade && 0==memcmp(str, "true", 4))))
            return SHADE_DARKEN;
        if(allowMenu && 0==memcmp(str, "wborder", 7))
            return SHADE_WINDOW_BORDER;
        if(0==memcmp(str, "custom", 6))
            return SHADE_CUSTOM;
        if('#'==str[0] && col)
        {
            qtcSetRgb(col, str);
            return SHADE_CUSTOM;
        }
        if(0==memcmp(str, "none", 4))
            return SHADE_NONE;
    }

    return def;
}

/* Prior to 0.42 round was a bool - so need to read 'false' as 'none' */
static ERound toRound(const char *str, ERound def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "none", 4) || 0==memcmp(str, "false", 5))
            return ROUND_NONE;
        if(0==memcmp(str, "slight", 6))
            return ROUND_SLIGHT;
        if(0==memcmp(str, "full", 4))
            return ROUND_FULL;
        if(0==memcmp(str, "extra", 5))
            return ROUND_EXTRA;
        if(0==memcmp(str, "max", 3))
            return ROUND_MAX;
    }

    return def;
}

static EScrollbar
toScrollbar(const char *str, EScrollbar def)
{
    return QtCurve::Config::loadValue<EScrollbar>(str, def);
}

static EFrame
toFrame(const char *str, EFrame def)
{
    return QtCurve::Config::loadValue<EFrame>(str, def);
}

static EEffect toEffect(const char *str, EEffect def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "none", 4))
            return EFFECT_NONE;
        if(0==memcmp(str, "shadow", 6))
            return EFFECT_SHADOW;
        if(0==memcmp(str, "etch", 4))
            return EFFECT_ETCH;
    }

    return def;
}

static Shading
toShading(const char *str, Shading def)
{
    return QtCurve::Config::loadValue<Shading>(str, def);
}

static EStripe toStripe(const char *str, EStripe def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "plain", 5) || 0==memcmp(str, "true", 4))
            return STRIPE_PLAIN;
        if(0==memcmp(str, "none", 4) || 0==memcmp(str, "false", 5))
            return STRIPE_NONE;
        if(0==memcmp(str, "diagonal", 8))
            return STRIPE_DIAGONAL;
        if(0==memcmp(str, "fade", 4))
            return STRIPE_FADE;
    }

    return def;
}

static ESliderStyle toSlider(const char *str, ESliderStyle def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "round", 5))
            return SLIDER_ROUND;
        if(0==memcmp(str, "plain", 5))
            return SLIDER_PLAIN;
        if(0==memcmp(str, "r-round", 7))
            return SLIDER_ROUND_ROTATED;
        if(0==memcmp(str, "r-plain", 7))
            return SLIDER_PLAIN_ROTATED;
        if(0==memcmp(str, "triangular", 10))
            return SLIDER_TRIANGULAR;
        if(0==memcmp(str, "circular", 8))
            return SLIDER_CIRCULAR;
    }

    return def;
}

static EColor toEColor(const char *str, EColor def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "base", 4))
            return ECOLOR_BASE;
        if(0==memcmp(str, "dark", 4))
            return ECOLOR_DARK;
        if(0==memcmp(str, "background", 10))
            return ECOLOR_BACKGROUND;
    }

    return def;
}

static EFocus toFocus(const char *str, EFocus def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "standard", 8))
            return FOCUS_STANDARD;
        if(0==memcmp(str, "rect", 4) || 0==memcmp(str, "highlight", 9))
            return FOCUS_RECTANGLE;
        if(0==memcmp(str, "filled", 6))
            return FOCUS_FILLED;
        if(0==memcmp(str, "full", 4))
            return FOCUS_FULL;
        if(0==memcmp(str, "line", 4))
            return FOCUS_LINE;
        if(0==memcmp(str, "glow", 4))
            return FOCUS_GLOW;
        if(0==memcmp(str, "none", 4))
            return FOCUS_NONE;
    }

    return def;
}

static ETabMo toTabMo(const char *str, ETabMo def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "top", 3))
            return TAB_MO_TOP;
        if(0==memcmp(str, "bot", 3))
            return TAB_MO_BOTTOM;
        if(0==memcmp(str, "glow", 4))
            return TAB_MO_GLOW;
    }

    return def;
}

static EGradType toGradType(const char *str, EGradType def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "horiz", 5))
            return GT_HORIZ;
        if(0==memcmp(str, "vert", 4))
            return GT_VERT;
    }
    return def;
}

static bool toLvLines(const char *str, bool def)
{
    if(str && 0!=str[0])
    {
#if 0
        if(0==memcmp(str, "true", 4) || 0==memcmp(str, "new", 3))
            return LV_NEW;
        if(0==memcmp(str, "old", 3))
            return LV_OLD;
        if(0==memcmp(str, "false", 5) || 0==memcmp(str, "none", 4))
            return LV_NONE;
#else
        return 0!=memcmp(str, "false", 5);
#endif
    }
    return def;
}

static EGradientBorder toGradientBorder(const char *str, bool *haveAlpha)
{
    if (str && str[0]) {
        *haveAlpha = strstr(str, "-alpha") ? true : false;
        if(0==memcmp(str, "light", 5) || 0==memcmp(str, "true", 4))
            return GB_LIGHT;
        if(0==memcmp(str, "none", 4))
            return GB_NONE;
        if(0==memcmp(str, "3dfull", 6))
            return GB_3D_FULL;
        if(0==memcmp(str, "3d", 2) || 0==memcmp(str, "false", 5))
            return GB_3D;
        if(0==memcmp(str, "shine", 5))
            return GB_SHINE;
    }
    return GB_3D;
}

#if defined CONFIG_DIALOG
static ETitleBarIcon toTitlebarIcon(const char *str, ETitleBarIcon def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "none", 4))
            return TITLEBAR_ICON_NONE;
        if(0==memcmp(str, "menu", 4))
            return TITLEBAR_ICON_MENU_BUTTON;
        if(0==memcmp(str, "title", 5))
            return TITLEBAR_ICON_NEXT_TO_TITLE;
    }
    return def;
}
#endif

static EImageType toImageType(const char *str, EImageType def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "none", 4))
            return IMG_NONE;
        if(0==memcmp(str, "plainrings", 10))
            return IMG_PLAIN_RINGS;
        if(0==memcmp(str, "rings", 5))
            return IMG_BORDERED_RINGS;
        if(0==memcmp(str, "squarerings", 11))
            return IMG_SQUARE_RINGS;
        if(0==memcmp(str, "file", 4))
            return IMG_FILE;
    }
    return def;
}

static EGlow toGlow(const char *str, EGlow def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "none", 4))
            return GLOW_NONE;
        if(0==memcmp(str, "start", 5))
            return GLOW_START;
        if(0==memcmp(str, "middle", 6))
            return GLOW_MIDDLE;
        if(0==memcmp(str, "end", 3))
            return GLOW_END;
    }
    return def;
}

static ETBarBtn toTBarBtn(const char *str, ETBarBtn def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "standard", 8))
            return TBTN_STANDARD;
        if(0==memcmp(str, "raised", 6))
            return TBTN_RAISED;
        if(0==memcmp(str, "joined", 6))
            return TBTN_JOINED;
    }
    return def;
}

WindowBorders
qtcGetWindowBorderSize(bool force)
{
    static WindowBorders def = {24, 18, 4, 4};
    static WindowBorders sizes = {-1, -1, -1, -1};

    if (sizes.titleHeight == -1 || force) {
        std::ifstream f(QtCurve::getConfFile(std::string(BORDER_SIZE_FILE)));
        if (f) {
            std::string line;
            std::getline(f, line);
            sizes.titleHeight = std::stoi(line);
            std::getline(f, line);
            sizes.toolTitleHeight = std::stoi(line);
            std::getline(f, line);
            sizes.bottom = std::stoi(line);
            std::getline(f, line);
            sizes.sides = std::stoi(line);
        }
    }

    return sizes.titleHeight<12 ? def : sizes;
}

static char*
qtcGetBarFileName(const char *app, const char *prefix)
{
    QtCurve::Str::Buff<1024> filename;
    return filename.cat(QtCurve::getConfDir(), prefix, app);
}

bool qtcBarHidden(const char *app, const char *prefix)
{
    return QtCurve::isRegFile(qtcGetBarFileName(app, prefix));
}

void qtcSetBarHidden(const char *app, bool hidden, const char *prefix)
{
    if(!hidden)
        unlink(qtcGetBarFileName(app, prefix));
    else
    {
        FILE *f=fopen(qtcGetBarFileName(app, prefix), "w");

        if(f)
            fclose(f);
    }
}

void qtcLoadBgndImage(QtCImage *img)
{
    if (!img->loaded &&
        ((img->width > 16 && img->width < 1024 && img->height > 16 &&
          img->height < 1024) || (img->width == 0 && img->height == 0))) {
        img->loaded = true;
        img->pixmap.img = nullptr;
        if (img->pixmap.file) {
            auto file = QtCurve::getConfFile(std::string(img->pixmap.file));
            img->pixmap.img = (img->width == 0) ?
                gdk_pixbuf_new_from_file(file.c_str(), nullptr) :
                gdk_pixbuf_new_from_file_at_scale(file.c_str(), img->width,
                                                  img->height, false, nullptr);
            if (img->pixmap.img && 0==img->width && img->pixmap.img) {
                img->width = gdk_pixbuf_get_width(img->pixmap.img);
                img->height = gdk_pixbuf_get_height(img->pixmap.img);
            }
        }
    }
}

static void
checkColor(EShade *s, GdkColor *c)
{
    if (*s == SHADE_CUSTOM && QtCurve::isBlack(*c)) {
        *s = SHADE_NONE;
    }
}

static char*
lookupCfgHash(GHashTable **cfg, const char *key, char *val)
{
    char *rv = nullptr;

    if (!*cfg) {
        *cfg = g_hash_table_new(g_str_hash, g_str_equal);
    } else {
        rv = (char*)g_hash_table_lookup(*cfg, key);
    }

    if (!rv && val) {
        g_hash_table_insert(*cfg, g_strdup(key), g_strdup(val));
        rv = (char *)g_hash_table_lookup(*cfg, key);
    }
    return rv;
}

static GHashTable * loadConfig(const char *filename)
{
    FILE       *f=fopen(filename, "r");
    GHashTable *cfg=nullptr;

    if(f)
    {
        char line[MAX_CONFIG_INPUT_LINE_LEN];

        while(nullptr!=fgets(line, MAX_CONFIG_INPUT_LINE_LEN-1, f))
        {
            char *eq=strchr(line, '=');
            int  pos=eq ? eq-line : -1;

            if(pos>0)
            {
                char *endl=strchr(line, '\n');

                if(endl)
                    *endl='\0';

                line[pos]='\0';

                lookupCfgHash(&cfg, line, &line[pos+1]);
            }
        }

        fclose(f);
    }

    return cfg;
}

static void
releaseConfig(GHashTable *cfg)
{
    g_hash_table_destroy(cfg);
}

static char*
readStringEntry(GHashTable *cfg, const char *key)
{
    return lookupCfgHash(&cfg, key, nullptr);
}

static int
readNumEntry(GHashTable *cfg, const char *key, int def)
{
    char *str = readStringEntry(cfg, key);

    return str ? atoi(str) : def;
}

static int
readVersionEntry(GHashTable *cfg, const char *key)
{
    char *str = readStringEntry(cfg, key);
    int major, minor, patch;

    return str && 3==sscanf(str, "%d.%d.%d", &major, &minor, &patch)
            ? qtcMakeVersion(major, minor, patch)
            : 0;
}

static bool
readBoolEntry(GHashTable *cfg, const char *key, bool def)
{
    char *str = readStringEntry(cfg, key);
    return str ? (memcmp(str, "true", 4) == 0 ? true : false) : def;
}

static void
readDoubleList(GHashTable *cfg, const char *key, double *list, int count)
{
    char *str=readStringEntry(cfg, key);

    if(str && 0!=str[0])
    {
        int  j,
             comma=0;
        bool ok=true;

        for(j=0; str[j]; ++j)
            if(','==str[j])
                comma++;

        ok=(count-1)==comma;
        if(ok)
        {
            for(j=0; j<comma+1 && str && ok; ++j)
            {
                char *c=strchr(str, ',');

                if(c || (str && count-1==comma))
                {
                    if(c)
                        *c='\0';
                    list[j]=g_ascii_strtod(str, nullptr);
                    str=c+1;
                }
                else
                    ok=false;
            }
        }

        if(!ok)
            list[0]=0;
    }
}

#define TO_LATIN1(A) (A)

#define CFG_READ_COLOR(ENTRY) do {                        \
        const char *str = readStringEntry(cfg, #ENTRY);   \
        if (str && 0 != str[0]) {                         \
            qtcSetRgb(&opts->ENTRY, str);                 \
        } else {                                          \
            opts->ENTRY = def->ENTRY;                     \
        }                                                 \
    } while (0)

#define CFG_READ_IMAGE(ENTRY) do {                                      \
        opts->ENTRY.type =                                              \
            toImageType(TO_LATIN1(readStringEntry(cfg, #ENTRY)),        \
                        def->ENTRY.type);                               \
        opts->ENTRY.loaded = false;                                     \
        if (IMG_FILE == opts->ENTRY.type) {                             \
            const char *file = readStringEntry(cfg, #ENTRY ".file");    \
            if (file) {                                                 \
                opts->ENTRY.pixmap.file = file;                         \
                opts->ENTRY.width = readNumEntry(cfg, #ENTRY ".width", 0); \
                opts->ENTRY.height = readNumEntry(cfg, #ENTRY ".height", 0); \
                opts->ENTRY.onBorder = readBoolEntry(cfg, #ENTRY ".onBorder", \
                                                     false);            \
                opts->ENTRY.pos = (EPixPos)readNumEntry(cfg, #ENTRY ".pos", \
                                                        (int)PP_TR);    \
            } else {                                                    \
                opts->ENTRY.type = IMG_NONE;                            \
            }                                                           \
        }                                                               \
    } while (0)

#define CFG_READ_STRING_LIST(ENTRY) do {                 \
        const char *str = readStringEntry(cfg, #ENTRY); \
        if (str && 0 != str[0]) {                        \
            opts->ENTRY = g_strsplit(str, ",", -1);      \
        } else if (def->ENTRY) {                         \
            opts->ENTRY = def->ENTRY;                    \
            def->ENTRY = nullptr;                           \
        }                                                \
    } while (0)

#define CFG_READ_BOOL(ENTRY) do {                               \
        opts->ENTRY = readBoolEntry(cfg, #ENTRY, def->ENTRY);   \
    } while (0)

#define CFG_READ_ROUND(ENTRY) do {                                      \
        opts->ENTRY = toRound(TO_LATIN1(readStringEntry(cfg, #ENTRY)),  \
                              def->ENTRY);                              \
    } while (0)

#define CFG_READ_INT(ENTRY) do {                                \
        opts->ENTRY = readNumEntry(cfg, #ENTRY, def->ENTRY);    \
    } while (0)

#define CFG_READ_INT_BOOL(ENTRY, DEF) do {                              \
        if (readBoolEntry(cfg, #ENTRY, false)) {                        \
            opts->ENTRY = DEF;                                          \
        } else {                                                        \
            opts->ENTRY = readNumEntry(cfg, #ENTRY, def->ENTRY);        \
        }                                                               \
    } while (0)

#define CFG_READ_TB_BORDER(ENTRY) do {                                  \
        opts->ENTRY = toTBarBorder(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                                   def->ENTRY);                         \
    } while (0)

#define CFG_READ_MOUSE_OVER(ENTRY) do {                                 \
        opts->ENTRY = toMouseOver(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                                  def->ENTRY);                          \
    } while (0)

#define CFG_READ_APPEARANCE(ENTRY, ALLOW) do {                          \
        opts->ENTRY = toAppearance(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                                   def->ENTRY, ALLOW, nullptr, false);     \
    } while (0)

#define CFG_READ_APPEARANCE_PIXMAP(ENTRY, ALLOW, PIXMAP, CHECK) do {    \
        opts->ENTRY = toAppearance(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                                   def->ENTRY, ALLOW, PIXMAP, CHECK);   \
    } while (0)

#define CFG_READ_STRIPE(ENTRY) do {                                     \
        opts->ENTRY = toStripe(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                               def->ENTRY);                             \
    } while (0)

#define CFG_READ_SLIDER(ENTRY) do {                                     \
        opts->ENTRY = toSlider(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                             def->ENTRY);                               \
    } while (0)

#define CFG_READ_DEF_BTN(ENTRY) do {                                    \
        opts->ENTRY = toInd(TO_LATIN1(readStringEntry(cfg, #ENTRY)),    \
                            def->ENTRY);                                \
    } while (0)

#define CFG_READ_LINE(ENTRY) do {                                       \
        opts->ENTRY = toLine(TO_LATIN1(readStringEntry(cfg, #ENTRY)),   \
                             def->ENTRY);                               \
    } while (0)

#define CFG_READ_SHADE(ENTRY, AD, MENU_STRIPE, COL) do {                \
        opts->ENTRY = toShade(TO_LATIN1(readStringEntry(cfg, #ENTRY)), AD, \
                              def->ENTRY, MENU_STRIPE, COL);            \
    } while (0)

#define CFG_READ_SCROLLBAR(ENTRY) do {                                  \
        opts->ENTRY = toScrollbar(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                                  def->ENTRY);                          \
    } while (0)

#define CFG_READ_FRAME(ENTRY) do {                                      \
        opts->ENTRY = toFrame(TO_LATIN1(readStringEntry(cfg, #ENTRY)),  \
                              def->ENTRY);                              \
    } while (0)

#define CFG_READ_EFFECT(ENTRY) do {                                     \
        opts->ENTRY = toEffect(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                               def->ENTRY);                             \
    } while (0)

#define CFG_READ_SHADING(ENTRY) do {                                    \
        opts->ENTRY = toShading(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                                def->ENTRY);                            \
    } while (0)

#define CFG_READ_ECOLOR(ENTRY) do {                                     \
        opts->ENTRY = toEColor(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                               def->ENTRY);                             \
    } while (0)

#define CFG_READ_FOCUS(ENTRY) do {                                      \
        opts->ENTRY = toFocus(TO_LATIN1(readStringEntry(cfg, #ENTRY)),  \
                              def->ENTRY);                              \
    } while (0)

#define CFG_READ_TAB_MO(ENTRY) do {                                     \
        opts->ENTRY = toTabMo(TO_LATIN1(readStringEntry(cfg, #ENTRY)),  \
                              def->ENTRY);                              \
    } while (0)

#define CFG_READ_GRAD_TYPE(ENTRY) do {                                  \
        opts->ENTRY = toGradType(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                                 def->ENTRY);                           \
    } while (0)

#define CFG_READ_LV_LINES(ENTRY) do {                                   \
        opts->ENTRY = toLvLines(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                                def->ENTRY);                            \
    } while (0)

#if defined CONFIG_DIALOG
#define CFG_READ_TB_ICON(ENTRY) do {                                    \
        opts->ENTRY = toTitlebarIcon(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                                     def->ENTRY);                       \
    } while (0)
#endif

#define CFG_READ_GLOW(ENTRY) do {                                       \
        opts->ENTRY = toGlow(TO_LATIN1(readStringEntry(cfg, #ENTRY)),   \
                             def->ENTRY);                               \
    } while (0)

#define CFG_READ_TBAR_BTN(ENTRY) do {                                   \
        opts->ENTRY = toTBarBtn(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                                def->ENTRY);                            \
    } while (0)

static void
checkAppearance(EAppearance *ap, Options *opts)
{
    if (*ap >= APPEARANCE_CUSTOM1 &&
        *ap < (APPEARANCE_CUSTOM1 + NUM_CUSTOM_GRAD)) {
        if (!opts->customGradient[*ap - APPEARANCE_CUSTOM1]) {
            if (ap == &opts->appearance) {
                *ap = APPEARANCE_FLAT;
            } else {
                *ap = opts->appearance;
            }
        }
    }
}

void qtcDefaultSettings(Options *opts);

static void
copyGradients(Options *src, Options *dest)
{
    if (src && dest && src != dest) {
        for (int i = 0;i < NUM_CUSTOM_GRAD;i++) {
            auto &srcGrad = src->customGradient[i];
            auto &destGrad = dest->customGradient[i];
            if (srcGrad && srcGrad->numStops > 0) {
                destGrad = qtcNew(Gradient);
                destGrad->numStops = srcGrad->numStops;
                destGrad->stops = qtcNew(GradientStop, destGrad->numStops);
                memcpy(destGrad->stops, srcGrad->stops,
                       sizeof(GradientStop) * destGrad->numStops);
                destGrad->border = srcGrad->border;
            } else {
                destGrad = nullptr;
            }
        }
    }
}

static void copyOpts(Options *src, Options *dest)
{
    if(src && dest && src!=dest)
    {
        memcpy(dest, src, sizeof(Options));
        dest->noBgndGradientApps=src->noBgndGradientApps;
        dest->noBgndOpacityApps=src->noBgndOpacityApps;
        dest->noMenuBgndOpacityApps=src->noMenuBgndOpacityApps;
        dest->noBgndImageApps=src->noBgndImageApps;
        dest->noMenuStripeApps=src->noMenuStripeApps;
        src->noBgndGradientApps=src->noBgndOpacityApps=src->noMenuBgndOpacityApps=src->noBgndImageApps=src->noMenuStripeApps=nullptr;
        memcpy(dest->customShades, src->customShades,
               sizeof(double) * QTC_NUM_STD_SHADES);
        memcpy(dest->customAlphas, src->customAlphas,
               sizeof(double) * NUM_STD_ALPHAS);
        copyGradients(src, dest);
    }
}

static void
freeOpts(Options *opts)
{
    if (opts) {
        if (opts->noBgndGradientApps)
            g_strfreev(opts->noBgndGradientApps);
        if (opts->noBgndOpacityApps)
            g_strfreev(opts->noBgndOpacityApps);
        if (opts->noMenuBgndOpacityApps)
            g_strfreev(opts->noMenuBgndOpacityApps);
        if (opts->noBgndImageApps)
            g_strfreev(opts->noBgndImageApps);
        if (opts->noMenuStripeApps)
            g_strfreev(opts->noMenuStripeApps);
        opts->noBgndGradientApps = nullptr;
        opts->noBgndOpacityApps = nullptr;
        opts->noMenuBgndOpacityApps = nullptr;
        opts->noBgndImageApps = nullptr;
        opts->noMenuStripeApps = nullptr;
        for (int i = 0;i < NUM_CUSTOM_GRAD;++i) {
            if (auto &grad = opts->customGradient[i]) {
                free(grad->stops);
                free(grad);
                grad = nullptr;
            }
        }
    }
}

void qtcCheckConfig(Options *opts)
{
    /* **Must** check appearance first, as the rest will default to this */
    checkAppearance(&opts->appearance, opts);
    checkAppearance(&opts->bgndAppearance, opts);
    checkAppearance(&opts->menuBgndAppearance, opts);
    checkAppearance(&opts->menubarAppearance, opts);
    checkAppearance(&opts->menuitemAppearance, opts);
    checkAppearance(&opts->toolbarAppearance, opts);
    checkAppearance(&opts->lvAppearance, opts);
    checkAppearance(&opts->tabAppearance, opts);
    checkAppearance(&opts->activeTabAppearance, opts);
    checkAppearance(&opts->sliderAppearance, opts);
    checkAppearance(&opts->selectionAppearance, opts);
    checkAppearance(&opts->titlebarAppearance, opts);
    checkAppearance(&opts->inactiveTitlebarAppearance, opts);
    checkAppearance(&opts->menuStripeAppearance, opts);
    checkAppearance(&opts->progressAppearance, opts);
    checkAppearance(&opts->progressGrooveAppearance, opts);
    checkAppearance(&opts->grooveAppearance, opts);
    checkAppearance(&opts->sunkenAppearance, opts);
    checkAppearance(&opts->sbarBgndAppearance, opts);
    checkAppearance(&opts->sliderFill, opts);
    checkAppearance(&opts->tooltipAppearance, opts);

    if(SHADE_BLEND_SELECTED==opts->shadeCheckRadio)
        opts->shadeCheckRadio=SHADE_SELECTED;

    checkColor(&opts->shadeMenubars, &opts->customMenubarsColor);
    checkColor(&opts->shadeSliders, &opts->customSlidersColor);
    checkColor(&opts->shadeCheckRadio, &opts->customCheckRadioColor);
    checkColor(&opts->menuStripe, &opts->customMenuStripeColor);
    checkColor(&opts->comboBtn, &opts->customComboBtnColor);
    checkColor(&opts->sortedLv, &opts->customSortedLvColor);
    if(APPEARANCE_BEVELLED==opts->toolbarAppearance)
        opts->toolbarAppearance=APPEARANCE_GRADIENT;
    else if(APPEARANCE_RAISED==opts->toolbarAppearance)
        opts->toolbarAppearance=APPEARANCE_FLAT;

    if(APPEARANCE_BEVELLED==opts->menubarAppearance)
        opts->menubarAppearance=APPEARANCE_GRADIENT;
    else if(APPEARANCE_RAISED==opts->menubarAppearance)
        opts->menubarAppearance=APPEARANCE_FLAT;

    if(APPEARANCE_BEVELLED==opts->sliderAppearance)
        opts->sliderAppearance=APPEARANCE_GRADIENT;

    if(APPEARANCE_BEVELLED==opts->tabAppearance)
        opts->tabAppearance=APPEARANCE_GRADIENT;

    if(APPEARANCE_BEVELLED==opts->activeTabAppearance)
        opts->activeTabAppearance=APPEARANCE_GRADIENT;

    if(APPEARANCE_RAISED==opts->selectionAppearance)
        opts->selectionAppearance=APPEARANCE_FLAT;
    else if(APPEARANCE_BEVELLED==opts->selectionAppearance)
        opts->selectionAppearance=APPEARANCE_GRADIENT;

    if(APPEARANCE_RAISED==opts->menuStripeAppearance)
        opts->menuStripeAppearance=APPEARANCE_FLAT;
    else if(APPEARANCE_BEVELLED==opts->menuStripeAppearance)
        opts->menuStripeAppearance=APPEARANCE_GRADIENT;

    if(opts->highlightFactor<MIN_HIGHLIGHT_FACTOR || opts->highlightFactor>MAX_HIGHLIGHT_FACTOR)
        opts->highlightFactor=DEFAULT_HIGHLIGHT_FACTOR;

    if(opts->crHighlight<MIN_HIGHLIGHT_FACTOR || opts->crHighlight>MAX_HIGHLIGHT_FACTOR)
        opts->crHighlight=DEFAULT_CR_HIGHLIGHT_FACTOR;

    if(opts->splitterHighlight<MIN_HIGHLIGHT_FACTOR || opts->splitterHighlight>MAX_HIGHLIGHT_FACTOR)
        opts->splitterHighlight=DEFAULT_SPLITTER_HIGHLIGHT_FACTOR;

    if(opts->expanderHighlight<MIN_HIGHLIGHT_FACTOR || opts->expanderHighlight>MAX_HIGHLIGHT_FACTOR)
        opts->expanderHighlight=DEFAULT_EXPANDER_HIGHLIGHT_FACTOR;

    if(0==opts->menuDelay) /* Qt seems to have issues if delay is 0 - so set this to 1 :-) */
        opts->menuDelay=MIN_MENU_DELAY;
    else if(opts->menuDelay<MIN_MENU_DELAY || opts->menuDelay>MAX_MENU_DELAY)
        opts->menuDelay=DEFAULT_MENU_DELAY;

    if(0==opts->sliderWidth%2)
        opts->sliderWidth++;

    if(opts->sliderWidth<MIN_SLIDER_WIDTH || opts->sliderWidth>MAX_SLIDER_WIDTH)
        opts->sliderWidth=DEFAULT_SLIDER_WIDTH;

    if(opts->sliderWidth<MIN_SLIDER_WIDTH_ROUND)
        opts->square|=SQUARE_SB_SLIDER;

    if(opts->sliderWidth<MIN_SLIDER_WIDTH_THIN_GROOVE)
        opts->thinSbarGroove=false;

    if(opts->sliderWidth<DEFAULT_SLIDER_WIDTH)
        opts->sliderThumbs=LINE_NONE;

    if(opts->lighterPopupMenuBgnd<MIN_LIGHTER_POPUP_MENU || opts->lighterPopupMenuBgnd>MAX_LIGHTER_POPUP_MENU)
        opts->lighterPopupMenuBgnd=DEF_POPUPMENU_LIGHT_FACTOR;

    if(opts->tabBgnd<MIN_TAB_BGND || opts->tabBgnd>MAX_TAB_BGND)
        opts->tabBgnd=DEF_TAB_BGND;

    if(opts->animatedProgress && !opts->stripedProgress)
        opts->animatedProgress=false;

    if(0==opts->gbFactor && FRAME_SHADED==opts->groupBox)
        opts->groupBox=FRAME_PLAIN;

    if(opts->gbFactor<MIN_GB_FACTOR || opts->gbFactor>MAX_GB_FACTOR)
        opts->gbFactor=DEF_GB_FACTOR;

    if(!opts->gtkComboMenus)
        opts->doubleGtkComboArrow=false;

    /* For now, only 2 sizes... */
    if(opts->crSize!=CR_SMALL_SIZE && opts->crSize!=CR_LARGE_SIZE)
        opts->crSize=CR_SMALL_SIZE;

/*
??
    if(SHADE_CUSTOM==opts->shadeMenubars || SHADE_BLEND_SELECTED==opts->shadeMenubars || !opts->borderMenuitems)
        opts->colorMenubarMouseOver=true;
*/

#ifndef CONFIG_DIALOG
    if(MO_GLOW==opts->coloredMouseOver && EFFECT_NONE==opts->buttonEffect)
        opts->coloredMouseOver=MO_COLORED_THICK;

    if(IND_GLOW==opts->defBtnIndicator && EFFECT_NONE==opts->buttonEffect)
        opts->defBtnIndicator=IND_TINT;

    if(opts->round>ROUND_EXTRA && FOCUS_GLOW!=opts->focus)
        opts->focus=FOCUS_LINE;

    if(EFFECT_NONE==opts->buttonEffect)
    {
        opts->etchEntry=false;
        if(FOCUS_GLOW==opts->focus)
            opts->focus=FOCUS_FULL;
    }

//     if(opts->squareScrollViews)
//         opts->highlightScrollViews=false;

    if(SHADE_WINDOW_BORDER==opts->shadeMenubars)
        opts->shadeMenubarOnlyWhenActive=true;

    if(MO_GLOW==opts->coloredMouseOver)
        opts->coloredTbarMo=true;

    if(ROUND_NONE==opts->round)
        opts->square=SQUARE_ALL;
#endif

    if(opts->bgndOpacity<0 || opts->bgndOpacity>100)
        opts->bgndOpacity=100;
    if(opts->dlgOpacity<0 || opts->dlgOpacity>100)
        opts->dlgOpacity=100;
    if(opts->menuBgndOpacity<0 || opts->menuBgndOpacity>100)
        opts->menuBgndOpacity = 100;

#ifndef CONFIG_DIALOG
    opts->bgndAppearance=MODIFY_AGUA(opts->bgndAppearance);
    opts->selectionAppearance=MODIFY_AGUA(opts->selectionAppearance);
    opts->lvAppearance=MODIFY_AGUA_X(opts->lvAppearance, APPEARANCE_LV_AGUA);
    opts->sbarBgndAppearance=MODIFY_AGUA(opts->sbarBgndAppearance);
    opts->tooltipAppearance=MODIFY_AGUA(opts->tooltipAppearance);
    opts->progressGrooveAppearance=MODIFY_AGUA(opts->progressGrooveAppearance);
    opts->menuBgndAppearance=MODIFY_AGUA(opts->menuBgndAppearance);
    opts->menuStripeAppearance=MODIFY_AGUA(opts->menuStripeAppearance);
    opts->grooveAppearance=MODIFY_AGUA(opts->grooveAppearance);
    opts->progressAppearance=MODIFY_AGUA(opts->progressAppearance);
    opts->sliderFill=MODIFY_AGUA(opts->sliderFill);
    opts->tabAppearance=MODIFY_AGUA(opts->tabAppearance);
    opts->activeTabAppearance=MODIFY_AGUA(opts->activeTabAppearance);
    opts->menuitemAppearance=MODIFY_AGUA(opts->menuitemAppearance);

    if(!opts->borderProgress && (!opts->fillProgress || !(opts->square&SQUARE_PROGRESS)))
        opts->borderProgress=true;

    opts->titlebarAppearance=MODIFY_AGUA(opts->titlebarAppearance);
    opts->inactiveTitlebarAppearance=MODIFY_AGUA(opts->inactiveTitlebarAppearance);

    if(opts->shadePopupMenu && SHADE_NONE==opts->shadeMenubars)
        opts->shadePopupMenu=false;

    if(opts->windowBorder&WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR &&
        (opts->windowBorder&WINDOW_BORDER_BLEND_TITLEBAR || SHADE_WINDOW_BORDER==opts->shadeMenubars))
        opts->windowBorder-=WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR;

    if(APPEARANCE_FLAT==opts->tabAppearance)
        opts->tabAppearance=APPEARANCE_RAISED;
    if(EFFECT_NONE==opts->buttonEffect)
        opts->etchEntry=false;
    if(opts->colorSliderMouseOver &&
        (SHADE_NONE==opts->shadeSliders || SHADE_DARKEN==opts->shadeSliders))
        opts->colorSliderMouseOver=false;
#endif /* ndef CONFIG_DIALOG */

    if(LINE_1DOT==opts->toolbarSeparators)
        opts->toolbarSeparators=LINE_DOTS;
}

bool qtcReadConfig(const char *file, Options *opts, Options *defOpts)
{
    bool checkImages=true;
    if (!file) {
        const char *env = getenv("QTCURVE_CONFIG_FILE");
        if (env && *env)
            return qtcReadConfig(env, opts, defOpts);
        auto filename = QtCurve::getConfFile(std::string(CONFIG_FILE));
        if (!QtCurve::isRegFile(filename.c_str())) {
            filename = QtCurve::getConfFile(std::string("/../"
                                                        OLD_CONFIG_FILE));
        }
        return qtcReadConfig(filename.c_str(), opts, defOpts);
    } else {
        GHashTable *cfg = loadConfig(file);

        if (cfg) {
            opts->version = readVersionEntry(cfg, VERSION_KEY);

            Options newOpts;
            Options *def=&newOpts;
            opts->noBgndGradientApps=opts->noBgndOpacityApps=opts->noMenuBgndOpacityApps=opts->noBgndImageApps=opts->noMenuStripeApps=nullptr;
            for (int i = 0;i < NUM_CUSTOM_GRAD;++i) {
                opts->customGradient[i] = nullptr;
            }

            if(defOpts)
                copyOpts(defOpts, &newOpts);
            else
                qtcDefaultSettings(&newOpts);
            if(opts!=def)
                copyGradients(def, opts);

            /* Check if the config file expects old default values... */
            if(opts->version<qtcMakeVersion(1, 6))
            {
                bool framelessGroupBoxes=readBoolEntry(cfg, "framelessGroupBoxes", true),
                     groupBoxLine=readBoolEntry(cfg, "groupBoxLine", true);
                opts->groupBox=framelessGroupBoxes ? (groupBoxLine ? FRAME_LINE : FRAME_NONE) : FRAME_PLAIN;
                opts->gbLabel=framelessGroupBoxes ? GB_LBL_BOLD : 0;
                opts->gbFactor=0;
                def->focus=FOCUS_LINE;
                def->crHighlight=3;
            } else {
                CFG_READ_FRAME(groupBox);
                CFG_READ_INT(gbLabel);
            }

            if(opts->version<qtcMakeVersion(1, 5))
            {
                opts->windowBorder=
                    (readBoolEntry(cfg, "colorTitlebarOnly", def->windowBorder&WINDOW_BORDER_COLOR_TITLEBAR_ONLY)
                                                                ? WINDOW_BORDER_COLOR_TITLEBAR_ONLY : 0)+
                    (readBoolEntry(cfg, "titlebarBorder", def->windowBorder&WINDOW_BORDER_ADD_LIGHT_BORDER)
                                                                ? WINDOW_BORDER_ADD_LIGHT_BORDER : 0)+
                    (readBoolEntry(cfg, "titlebarBlend", def->windowBorder&WINDOW_BORDER_BLEND_TITLEBAR)
                                                                ? WINDOW_BORDER_BLEND_TITLEBAR : 0);
            }
            else
                CFG_READ_INT(windowBorder);

            if(opts->version<qtcMakeVersion(1, 7))
            {
                opts->windowBorder|=WINDOW_BORDER_FILL_TITLEBAR;
                def->square=SQUARE_POPUP_MENUS;
            }

            if(opts->version<qtcMakeVersion(1, 4))
            {
                opts->square=
                    (readBoolEntry(cfg, "squareLvSelection", def->square&SQUARE_LISTVIEW_SELECTION) ? SQUARE_LISTVIEW_SELECTION : SQUARE_NONE)+
                    (readBoolEntry(cfg, "squareScrollViews", def->square&SQUARE_SCROLLVIEW) ? SQUARE_SCROLLVIEW : SQUARE_NONE)+
                    (readBoolEntry(cfg, "squareProgress", def->square&SQUARE_PROGRESS) ? SQUARE_PROGRESS : SQUARE_NONE)+
                    (readBoolEntry(cfg, "squareEntry", def->square&SQUARE_ENTRY)? SQUARE_ENTRY : SQUARE_NONE);
            }
            else
                CFG_READ_INT(square);
            if(opts->version<qtcMakeVersion(1, 7))
            {
                def->tbarBtns=TBTN_STANDARD;
                opts->thin=(readBoolEntry(cfg, "thinnerMenuItems", def->thin&THIN_MENU_ITEMS) ? THIN_MENU_ITEMS : 0)+
                           (readBoolEntry(cfg, "thinnerBtns", def->thin&THIN_BUTTONS) ? THIN_BUTTONS : 0);
            }
            else
            {
                CFG_READ_INT(thin);
            }
            if (opts->version < qtcMakeVersion(1, 6))
                opts->square |= SQUARE_TOOLTIPS;
            if (opts->version < qtcMakeVersion(1, 6, 1))
                opts->square |= SQUARE_POPUP_MENUS;
            if (opts->version < qtcMakeVersion(1, 2))
                def->crSize = CR_SMALL_SIZE;
            if (opts != def) {
                opts->customShades[0] = 0;
                opts->customAlphas[0] = 0;
                if (USE_CUSTOM_SHADES(*def)) {
                    memcpy(opts->customShades, def->customShades,
                           sizeof(double) * QTC_NUM_STD_SHADES);
                }
            }

            CFG_READ_INT(gbFactor);
            CFG_READ_INT(passwordChar);
            CFG_READ_ROUND(round);
            CFG_READ_INT(highlightFactor);
            CFG_READ_INT(menuDelay);
            CFG_READ_INT(sliderWidth);
            CFG_READ_INT(tabBgnd);
            CFG_READ_TB_BORDER(toolbarBorders);
            CFG_READ_APPEARANCE(appearance, APP_ALLOW_BASIC);
            if (opts->version<qtcMakeVersion(1, 8)) {
                opts->tbarBtnAppearance = APPEARANCE_NONE;
                opts->tbarBtnEffect = EFFECT_NONE;
            } else {
                CFG_READ_APPEARANCE(tbarBtnAppearance, APP_ALLOW_NONE);
                CFG_READ_EFFECT(tbarBtnEffect);
            }
            CFG_READ_APPEARANCE_PIXMAP(bgndAppearance, APP_ALLOW_STRIPED,
                                       &opts->bgndPixmap, checkImages);
            CFG_READ_GRAD_TYPE(bgndGrad);
            CFG_READ_GRAD_TYPE(menuBgndGrad);
            CFG_READ_INT_BOOL(lighterPopupMenuBgnd, def->lighterPopupMenuBgnd);
            CFG_READ_APPEARANCE_PIXMAP(menuBgndAppearance, APP_ALLOW_STRIPED,
                                       &opts->menuBgndPixmap, checkImages);

            if (opts->menuBgndAppearance == APPEARANCE_FLAT &&
                opts->lighterPopupMenuBgnd == 0 &&
                opts->version<qtcMakeVersion(1, 7)) {
                opts->menuBgndAppearance = APPEARANCE_RAISED;
            }

            CFG_READ_STRIPE(stripedProgress);
            CFG_READ_SLIDER(sliderStyle);
            CFG_READ_BOOL(animatedProgress);
            CFG_READ_BOOL(embolden);
            CFG_READ_DEF_BTN(defBtnIndicator);
            CFG_READ_LINE(sliderThumbs);
            CFG_READ_LINE(handles);
            CFG_READ_BOOL(highlightTab);
            CFG_READ_INT_BOOL(colorSelTab, DEF_COLOR_SEL_TAB_FACTOR);
            CFG_READ_BOOL(roundAllTabs);
            CFG_READ_TAB_MO(tabMouseOver);
            CFG_READ_SHADE(shadeSliders, true, false,
                           &opts->customSlidersColor);
            CFG_READ_SHADE(shadeMenubars, true, false,
                           &opts->customMenubarsColor);
            CFG_READ_SHADE(shadeCheckRadio, false, false,
                           &opts->customCheckRadioColor);
            CFG_READ_SHADE(sortedLv, true, false, &opts->customSortedLvColor);
            CFG_READ_SHADE(crColor,  true, false, &opts->customCrBgndColor);
            CFG_READ_SHADE(progressColor, false, false,
                           &opts->customProgressColor);
            CFG_READ_APPEARANCE(menubarAppearance, APP_ALLOW_BASIC);
            CFG_READ_APPEARANCE(menuitemAppearance, APP_ALLOW_FADE);
            CFG_READ_APPEARANCE(toolbarAppearance, APP_ALLOW_BASIC);
            CFG_READ_APPEARANCE(selectionAppearance, APP_ALLOW_BASIC);
            CFG_READ_LINE(toolbarSeparators);
            CFG_READ_LINE(splitters);
            CFG_READ_BOOL(customMenuTextColor);
            CFG_READ_MOUSE_OVER(coloredMouseOver);
            CFG_READ_BOOL(menubarMouseOver);
            CFG_READ_BOOL(useHighlightForMenu);
            CFG_READ_BOOL(shadeMenubarOnlyWhenActive);
            CFG_READ_TBAR_BTN(tbarBtns);
            CFG_READ_COLOR(customMenuSelTextColor);
            CFG_READ_COLOR(customMenuNormTextColor);
            CFG_READ_SCROLLBAR(scrollbarType);
            CFG_READ_EFFECT(buttonEffect);
            CFG_READ_APPEARANCE(lvAppearance, APP_ALLOW_BASIC);
            CFG_READ_APPEARANCE(tabAppearance, APP_ALLOW_BASIC);
            CFG_READ_APPEARANCE(activeTabAppearance, APP_ALLOW_BASIC);
            CFG_READ_APPEARANCE(sliderAppearance, APP_ALLOW_BASIC);
            CFG_READ_APPEARANCE(progressAppearance, APP_ALLOW_BASIC);
            CFG_READ_APPEARANCE(progressGrooveAppearance, APP_ALLOW_BASIC);
            CFG_READ_APPEARANCE(grooveAppearance, APP_ALLOW_BASIC);
            CFG_READ_APPEARANCE(sunkenAppearance, APP_ALLOW_BASIC);
            CFG_READ_APPEARANCE(sbarBgndAppearance, APP_ALLOW_BASIC);
            if (opts->version < qtcMakeVersion(1, 6)) {
                opts->tooltipAppearance = APPEARANCE_FLAT;
            } else {
                CFG_READ_APPEARANCE(tooltipAppearance, APP_ALLOW_BASIC);
            }

            CFG_READ_APPEARANCE(sliderFill, APP_ALLOW_BASIC);
            CFG_READ_ECOLOR(progressGrooveColor);
            CFG_READ_FOCUS(focus);
            CFG_READ_BOOL(lvButton);
            CFG_READ_LV_LINES(lvLines);
            CFG_READ_BOOL(drawStatusBarFrames);
            CFG_READ_BOOL(fillSlider);
            CFG_READ_BOOL(roundMbTopOnly);
            CFG_READ_BOOL(borderMenuitems);
            CFG_READ_BOOL(darkerBorders);
            CFG_READ_BOOL(vArrows);
            CFG_READ_BOOL(xCheck);
            CFG_READ_BOOL(fadeLines);
            CFG_READ_GLOW(glowProgress);
            CFG_READ_BOOL(colorMenubarMouseOver);
            CFG_READ_INT_BOOL(crHighlight, opts->highlightFactor);
            CFG_READ_BOOL(crButton);
            CFG_READ_BOOL(smallRadio);
            CFG_READ_BOOL(fillProgress);
            CFG_READ_BOOL(comboSplitter);
            CFG_READ_BOOL(highlightScrollViews);
            CFG_READ_BOOL(etchEntry);
            CFG_READ_INT_BOOL(splitterHighlight, opts->highlightFactor);
            CFG_READ_INT(crSize);
            CFG_READ_BOOL(flatSbarButtons);
            CFG_READ_BOOL(borderSbarGroove);
            CFG_READ_BOOL(borderProgress);
            CFG_READ_BOOL(popupBorder);
            CFG_READ_BOOL(unifySpinBtns);
            CFG_READ_BOOL(unifySpin);
            CFG_READ_BOOL(unifyCombo);
            CFG_READ_BOOL(borderTab);
            CFG_READ_BOOL(borderInactiveTab);
            CFG_READ_BOOL(thinSbarGroove);
            CFG_READ_BOOL(colorSliderMouseOver);
            CFG_READ_BOOL(menuIcons);
            CFG_READ_BOOL(forceAlternateLvCols);
            CFG_READ_BOOL(invertBotTab);
            CFG_READ_INT_BOOL(menubarHiding, HIDE_KEYBOARD);
            CFG_READ_INT_BOOL(statusbarHiding, HIDE_KEYBOARD);
            CFG_READ_BOOL(boldProgress);
            CFG_READ_BOOL(coloredTbarMo);
            CFG_READ_BOOL(borderSelection);
            CFG_READ_BOOL(stripedSbar);
            CFG_READ_INT_BOOL(windowDrag, WM_DRAG_MENUBAR);
            CFG_READ_BOOL(shadePopupMenu);
            CFG_READ_BOOL(hideShortcutUnderline);

#if defined CONFIG_DIALOG
            CFG_READ_BOOL(stdBtnSizes);
            CFG_READ_INT(titlebarButtons);
            CFG_READ_TB_ICON(titlebarIcon);
#endif
            CFG_READ_INT(bgndOpacity);
            CFG_READ_INT(menuBgndOpacity);
            CFG_READ_INT(dlgOpacity);
            CFG_READ_SHADE(menuStripe, true, true, &opts->customMenuStripeColor);
            CFG_READ_APPEARANCE(menuStripeAppearance, APP_ALLOW_BASIC);
            CFG_READ_SHADE(comboBtn, true, false, &opts->customComboBtnColor);
            CFG_READ_BOOL(gtkScrollViews);
            CFG_READ_BOOL(doubleGtkComboArrow);
            CFG_READ_BOOL(stdSidebarButtons);
            CFG_READ_BOOL(toolbarTabs);
            CFG_READ_BOOL(gtkComboMenus);
            CFG_READ_INT(expanderHighlight);
            CFG_READ_BOOL(mapKdeIcons);
            CFG_READ_BOOL(gtkButtonOrder);
            CFG_READ_BOOL(reorderGtkButtons);
            CFG_READ_APPEARANCE(titlebarAppearance, APP_ALLOW_NONE);
            CFG_READ_APPEARANCE(inactiveTitlebarAppearance, APP_ALLOW_NONE);

            if(APPEARANCE_BEVELLED==opts->titlebarAppearance)
                opts->titlebarAppearance=APPEARANCE_GRADIENT;
            else if(APPEARANCE_RAISED==opts->titlebarAppearance)
                opts->titlebarAppearance=APPEARANCE_FLAT;
            if((opts->windowBorder&WINDOW_BORDER_BLEND_TITLEBAR) && !(opts->windowBorder&WINDOW_BORDER_COLOR_TITLEBAR_ONLY))
                opts->windowBorder-=WINDOW_BORDER_BLEND_TITLEBAR;
            if(APPEARANCE_BEVELLED==opts->inactiveTitlebarAppearance)
                opts->inactiveTitlebarAppearance=APPEARANCE_GRADIENT;
            else if(APPEARANCE_RAISED==opts->inactiveTitlebarAppearance)
                opts->inactiveTitlebarAppearance=APPEARANCE_FLAT;
            CFG_READ_SHADING(shading);
            CFG_READ_IMAGE(bgndImage);
            CFG_READ_IMAGE(menuBgndImage);
            CFG_READ_STRING_LIST(noMenuStripeApps);
            CFG_READ_STRING_LIST(noBgndGradientApps);
            CFG_READ_STRING_LIST(noBgndOpacityApps);
            CFG_READ_STRING_LIST(noMenuBgndOpacityApps);
            CFG_READ_STRING_LIST(noBgndImageApps);
#ifdef CONFIG_DIALOG
            if (opts->version < qtcMakeVersion(1, 7, 2)) {
                opts->noMenuBgndOpacityApps << "gtk";
            }
#endif
            readDoubleList(cfg, "customShades", opts->customShades,
                           QTC_NUM_STD_SHADES);
            readDoubleList(cfg, "customAlphas", opts->customAlphas,
                           NUM_STD_ALPHAS);

            for (int i = 0;i < NUM_CUSTOM_GRAD;++i) {
                char gradKey[18];
                sprintf(gradKey, "customgradient%d", i + 1);
                if (char *str = readStringEntry(cfg, gradKey)) {
                    auto &grad = opts->customGradient[i];
                    int comma = 0;

                    for (int j = 0;str[j];j++) {
                        if (str[j] == ',') {
                            comma++;
                        }
                    }

                    if (comma && grad) {
                        free(grad->stops);
                        free(grad);
                        grad = nullptr;
                    }

                    if (comma>=4) {
                        char *c = strchr(str, ',');

                        if (c) {
                            bool haveAlpha = false;
                            EGradientBorder border =
                                toGradientBorder(str, &haveAlpha);
                            int parts = haveAlpha ? 3 : 2;
                            bool ok = 0 == comma % parts;

                            *c = '\0';

                            if (ok) {
                                grad = qtcNew(Gradient);
                                grad->numStops = comma / parts;
                                grad->stops = qtcNew(GradientStop,
                                                     grad->numStops);
                                grad->border = border;
                                str = c + 1;
                                for (int j = 0;j < comma && str && ok;
                                     j += parts) {
                                    int stop = j / parts;
                                    c = strchr(str, ',');

                                    if (c) {
                                        *c = '\0';
                                        grad->stops[stop].pos =
                                            g_ascii_strtod(str, nullptr);
                                        str = c + 1;
                                        c = str ? strchr(str, ',') : nullptr;

                                        if (c || str) {
                                            if (c) {
                                                *c = '\0';
                                            }
                                            grad->stops[stop].val =
                                                g_ascii_strtod(str, nullptr);
                                            str = c ? c + 1 : c;
                                            if (haveAlpha) {
                                                c = str ? strchr(str, ',') : nullptr;
                                                if (c || str) {
                                                    if (c) {
                                                        *c = '\0';
                                                    }
                                                    grad->stops[stop].alpha =
                                                        g_ascii_strtod(str, nullptr);
                                                    str = c ? c + 1 : c;
                                                } else {
                                                    ok = false;
                                                }
                                            } else {
                                                grad->stops[stop].alpha = 1.0;
                                            }
                                        } else {
                                            ok = false;
                                        }
                                    } else {
                                        ok = false;
                                    }
                                    ok = (ok && grad->stops[stop].pos >= 0 &&
                                          grad->stops[stop].pos <= 1.0 &&
                                          grad->stops[stop].val >= 0.0 &&
                                          grad->stops[stop].val <= 2.0 &&
                                          grad->stops[stop].alpha >= 0.0 &&
                                          grad->stops[stop].alpha <= 1.0);
                                }

                                if (ok) {
                                    int addStart = 0;
                                    int addEnd = 0;
                                    if (grad->stops[0].pos > 0.001)
                                        addStart = 1;
                                    if (grad->stops[grad->numStops - 1].pos
                                        < 0.999)
                                        addEnd = 1;

                                    if (addStart || addEnd) {
                                        int newSize = (grad->numStops +
                                                       addStart + addEnd);
                                        GradientStop *stops =
                                            qtcNew(GradientStop, newSize);
                                        if (addStart) {
                                            stops[0].pos = 0.0;
                                            stops[0].val = 1.0;
                                            stops[0].alpha = 1.0;
                                        }
                                        memcpy(&stops[addStart], grad->stops,
                                               sizeof(GradientStop) *
                                               grad->numStops);
                                        if (addEnd) {
                                            auto &stop =
                                                stops[grad->numStops + addStart];
                                            stop.pos = 1.0;
                                            stop.val = 1.0;
                                            stop.alpha = 1.0;
                                        }
                                        grad->numStops = newSize;
                                        free(grad->stops);
                                        grad->stops = stops;
                                    }
                                } else {
                                    free(grad->stops);
                                    free(grad);
                                    grad = nullptr;
                                }
                            }
                        }
                    }
                }
            }

            qtcCheckConfig(opts);

            if (!defOpts) {
                for (int i = 0;i < NUM_CUSTOM_GRAD;++i) {
                    free(def->customGradient[i]);
                }
            }
            releaseConfig(cfg);
            freeOpts(defOpts);
            return true;
        } else {
            if (defOpts) {
                copyOpts(defOpts, opts);
            } else {
                qtcDefaultSettings(opts);
            }
            return true;
        }
    }
    return false;
}

static const char * getSystemConfigFile()
{
    static const char *constFiles[] = {
        /* "/etc/qt4/"OLD_CONFIG_FILE,
           "/etc/qt3/"OLD_CONFIG_FILE,
           "/etc/qt/"OLD_CONFIG_FILE, */
        "/etc/" OLD_CONFIG_FILE,
        nullptr};

    for (int i = 0;constFiles[i];i++) {
        if (QtCurve::isRegFile(constFiles[i])) {
            return constFiles[i];
        }
    }
    return nullptr;
}

void qtcDefaultSettings(Options *opts)
{
    /* Set hard-coded defaults... */
    for (int i = 0;i < NUM_CUSTOM_GRAD;++i) {
        opts->customGradient[i] = nullptr;
    }
    opts->customGradient[APPEARANCE_CUSTOM1] = qtcNew(Gradient);
    opts->customGradient[APPEARANCE_CUSTOM2] = qtcNew(Gradient);
    qtcSetupGradient(opts->customGradient[APPEARANCE_CUSTOM1], GB_3D,3,0.0,1.2,0.5,1.0,1.0,1.0);
    qtcSetupGradient(opts->customGradient[APPEARANCE_CUSTOM2], GB_3D,3,0.0,0.9,0.5,1.0,1.0,1.0);
    opts->customShades[0]=1.16;
    opts->customShades[1]=1.07;
    opts->customShades[2]=0.9;
    opts->customShades[3]=0.78;
    opts->customShades[4]=0.84;
    opts->customShades[5]=0.75;
    opts->customAlphas[0]=0;
    opts->contrast=7;
    opts->passwordChar=0x25CF;
    opts->gbFactor=DEF_GB_FACTOR;
    opts->highlightFactor=DEFAULT_HIGHLIGHT_FACTOR;
    opts->crHighlight=DEFAULT_CR_HIGHLIGHT_FACTOR;
    opts->splitterHighlight=DEFAULT_SPLITTER_HIGHLIGHT_FACTOR;
    opts->crSize=CR_LARGE_SIZE;
    opts->menuDelay=DEFAULT_MENU_DELAY;
    opts->sliderWidth=DEFAULT_SLIDER_WIDTH;
    opts->selectionAppearance=APPEARANCE_HARSH_GRADIENT;
    opts->fadeLines=true;
    opts->glowProgress=GLOW_NONE;
    opts->round=ROUND_EXTRA;
    opts->gtkButtonOrder=false;
    opts->reorderGtkButtons=false;
    opts->bgndImage.type=IMG_NONE;
    opts->bgndImage.width=opts->bgndImage.height=0;
    opts->bgndImage.onBorder=false;
    opts->bgndImage.pos=PP_TR;
    opts->menuBgndImage.type=IMG_NONE;
    opts->menuBgndImage.width=opts->menuBgndImage.height=0;
    opts->menuBgndImage.onBorder=false;
    opts->menuBgndImage.pos=PP_TR;
    opts->lighterPopupMenuBgnd=DEF_POPUPMENU_LIGHT_FACTOR;
    opts->tabBgnd=DEF_TAB_BGND;
    opts->animatedProgress=false;
    opts->stripedProgress=STRIPE_NONE;
    opts->sliderStyle=SLIDER_PLAIN;
    opts->highlightTab=false;
    opts->colorSelTab=0;
    opts->roundAllTabs=true;
    opts->tabMouseOver=TAB_MO_GLOW;
    opts->embolden=false;
    opts->bgndGrad=GT_HORIZ;
    opts->menuBgndGrad=GT_HORIZ;
    opts->appearance=APPEARANCE_SOFT_GRADIENT;
    opts->tbarBtnAppearance=APPEARANCE_NONE;
    opts->tbarBtnEffect=EFFECT_NONE;
    opts->bgndAppearance=APPEARANCE_FLAT;
    opts->menuBgndAppearance=APPEARANCE_FLAT;
    opts->lvAppearance=APPEARANCE_BEVELLED;
    opts->tabAppearance=APPEARANCE_SOFT_GRADIENT;
    opts->activeTabAppearance=APPEARANCE_SOFT_GRADIENT;
    opts->sliderAppearance=APPEARANCE_SOFT_GRADIENT;
    opts->menubarAppearance=APPEARANCE_FLAT;
    opts->menuitemAppearance=APPEARANCE_FADE;
    opts->toolbarAppearance=APPEARANCE_FLAT;
    opts->progressAppearance=APPEARANCE_DULL_GLASS;
    opts->progressGrooveAppearance=APPEARANCE_INVERTED;
    opts->progressGrooveColor=ECOLOR_DARK;
    opts->grooveAppearance=APPEARANCE_INVERTED;
    opts->sunkenAppearance=APPEARANCE_SOFT_GRADIENT;
    opts->sbarBgndAppearance=APPEARANCE_FLAT;
    opts->tooltipAppearance=APPEARANCE_GRADIENT;
    opts->sliderFill=APPEARANCE_GRADIENT;
    opts->defBtnIndicator=IND_GLOW;
    opts->sliderThumbs=LINE_FLAT;
    opts->handles=LINE_1DOT;
    opts->shadeSliders=SHADE_NONE;
    opts->shadeMenubars=SHADE_NONE;
    opts->shadeCheckRadio=SHADE_NONE;
    opts->sortedLv=SHADE_NONE;
    opts->toolbarBorders=TB_NONE;
    opts->toolbarSeparators=LINE_SUNKEN;
    opts->splitters=LINE_1DOT;
    opts->customMenuTextColor=false;
    opts->coloredMouseOver=MO_GLOW;
    opts->menubarMouseOver=true;
    opts->useHighlightForMenu=false;
    opts->shadeMenubarOnlyWhenActive=false;
    opts->thin=THIN_BUTTONS;
    opts->tbarBtns=TBTN_STANDARD;
    opts->scrollbarType=SCROLLBAR_KDE;
    opts->buttonEffect=EFFECT_SHADOW;
    opts->focus=FOCUS_GLOW;
    opts->lvButton=false;
    opts->lvLines=false; /*LV_NONE;*/
    opts->drawStatusBarFrames=false;
    opts->fillSlider=true;
    opts->roundMbTopOnly=true;
    opts->borderMenuitems=false;
    opts->darkerBorders=false;
    opts->vArrows=true;
    opts->xCheck=false;
    opts->colorMenubarMouseOver=true;
    opts->crButton=true;
    opts->crColor=SHADE_NONE;
    opts->progressColor=SHADE_SELECTED;
    opts->smallRadio=true;
    opts->fillProgress=true;
    opts->comboSplitter=false;
    opts->highlightScrollViews=false;
    opts->etchEntry=false;
    opts->flatSbarButtons=true;
    opts->borderSbarGroove=true;
    opts->borderProgress=true;
    opts->popupBorder=true;
    opts->unifySpinBtns=false;
    opts->unifySpin=true;
    opts->unifyCombo=true;
    opts->borderTab=true;
    opts->borderInactiveTab=false;
    opts->thinSbarGroove=true;
    opts->colorSliderMouseOver=false;
    opts->menuIcons=true;
    opts->forceAlternateLvCols=false;
    opts->invertBotTab=true;
    opts->menubarHiding=HIDE_NONE;
    opts->statusbarHiding=HIDE_NONE;
    opts->boldProgress=true;
    opts->coloredTbarMo=false;
    opts->borderSelection=false;
    opts->square=SQUARE_POPUP_MENUS|SQUARE_TOOLTIPS;
    opts->stripedSbar=false;
    opts->windowDrag=WM_DRAG_NONE;
    opts->shadePopupMenu=false;
    opts->hideShortcutUnderline=false;
    opts->windowBorder=WINDOW_BORDER_ADD_LIGHT_BORDER|WINDOW_BORDER_FILL_TITLEBAR;
    opts->groupBox=FRAME_FADED;
    opts->gbFactor=DEF_GB_FACTOR;
    opts->gbLabel=GB_LBL_BOLD|GB_LBL_OUTSIDE;
#if defined CONFIG_DIALOG
    opts->stdBtnSizes=false;
    opts->titlebarButtons=TITLEBAR_BUTTON_ROUND|TITLEBAR_BUTTON_HOVER_SYMBOL;
    opts->titlebarIcon=TITLEBAR_ICON_NEXT_TO_TITLE;
#endif
    opts->menuStripe=SHADE_NONE;
    opts->menuStripeAppearance=APPEARANCE_DARK_INVERTED;
    opts->shading=Shading::HSL;
    opts->gtkScrollViews=true;
    opts->comboBtn=SHADE_NONE;
    opts->doubleGtkComboArrow=true;
    opts->stdSidebarButtons=false;
    opts->toolbarTabs=false;
    opts->bgndOpacity = opts->dlgOpacity = opts->menuBgndOpacity = 100;
    opts->gtkComboMenus=false;
    opts->noBgndGradientApps=nullptr;
    opts->noBgndOpacityApps=g_strsplit("sonata,totem,vmware,vmplayer",",", -1);;
    opts->noBgndImageApps=nullptr;
    opts->noMenuStripeApps=g_strsplit("gtk",",", -1);
    opts->noMenuBgndOpacityApps=g_strsplit("sonata,totem,vmware,vmplayer,gtk",",", -1);
/*
    opts->setDialogButtonOrder=false;
*/
    opts->customMenubarsColor.red=opts->customMenubarsColor.green=opts->customMenubarsColor.blue=0;
    opts->customSlidersColor.red=opts->customSlidersColor.green=opts->customSlidersColor.blue=0;
    opts->customMenuNormTextColor.red=opts->customMenuNormTextColor.green=opts->customMenuNormTextColor.blue=0;
    opts->customMenuSelTextColor.red=opts->customMenuSelTextColor.green=opts->customMenuSelTextColor.blue=0;
    opts->customCheckRadioColor.red=opts->customCheckRadioColor.green=opts->customCheckRadioColor.blue=0;
    opts->customComboBtnColor.red=opts->customCheckRadioColor.green=opts->customCheckRadioColor.blue=0;
    opts->customMenuStripeColor.red=opts->customMenuStripeColor.green=opts->customMenuStripeColor.blue=0;
    opts->customProgressColor.red=opts->customProgressColor.green=opts->customProgressColor.blue=0;

    opts->mapKdeIcons=true;
    opts->expanderHighlight=DEFAULT_EXPANDER_HIGHLIGHT_FACTOR;
    opts->titlebarAppearance=APPEARANCE_CUSTOM1;
    opts->inactiveTitlebarAppearance=APPEARANCE_CUSTOM1;
    /* Read system config file... */
    static const char *systemFilename = nullptr;

    if (!systemFilename) {
        systemFilename = getSystemConfigFile();
    }
    if (systemFilename) {
        qtcReadConfig(systemFilename, opts, opts);
    }
}
