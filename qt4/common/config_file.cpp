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

#include <qtcurve-utils/dirs.h>
#include <qtcurve-utils/color.h>
#include "common.h"
#include "config_file.h"

#include <qglobal.h>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QSvgRenderer>
#include <QPainter>

#define CONFIG_FILE "stylerc"
#define OLD_CONFIG_FILE "qtcurvestylerc"
#define VERSION_KEY "version"

#define TO_LATIN1(A) A.toLatin1().constData()
static QString
determineFileName(const QString &file)
{
    if(file.startsWith("/"))
        return file;
    return qtcConfDir()+file;
}

static int c2h(char ch)
{
    return (ch>='0' && ch<='9') ? ch-'0' :
           (ch>='a' && ch<='f') ? 10+(ch-'a') :
           (ch>='A' && ch<='F') ? 10+(ch-'A') :
           0;
}

#define ATOH(str) ((c2h(*str)<<4)+c2h(*(str+1)))

void
qtcSetRgb(QColor *col, const char *str)
{
    if (str && strlen(str) > 6) {
        int offset = '#' == str[0] ? 1 : 0;
        col->setRgb(ATOH(&str[offset]), ATOH(&str[offset + 2]),
                    ATOH(&str[offset + 4]));
    } else {
        col->setRgb(0, 0, 0);
    }
}

static bool
loadImage(const QString &file, QtCPixmap *pixmap)
{
    // Need to store filename for config dialog!
    QString f(determineFileName(file));
    pixmap->file=f;
    return pixmap->img.load(f);
}

static EDefBtnIndicator toInd(const char *str, EDefBtnIndicator def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "fontcolor", 9) || 0==memcmp(str, "border", 6))
            return IND_FONT_COLOR;
        if(0==memcmp(str, "none", 4))
            return IND_NONE;
        if(0==memcmp(str, "corner", 6))
            return IND_CORNER;
        if(0==memcmp(str, "colored", 7))
            return IND_COLORED;
        if(0==memcmp(str, "tint", 4))
            return IND_TINT;
        if(0==memcmp(str, "glow", 4))
            return IND_GLOW;
        if(0==memcmp(str, "darken", 6))
            return IND_DARKEN;
        if(0==memcmp(str, "origselected", 12))
            return IND_SELECTED;
    }

    return def;
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
        if(NULL!=pix && APP_ALLOW_STRIPED==allow && 0==memcmp(str, "file", 4) && strlen(str)>9)
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
        bool menuShade, QColor *col)
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

static EFrame toFrame(const char *str, EFrame def)
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

static EAlign toAlign(const char *str, EAlign def)
{
    if(str && 0!=str[0])
    {
        if(0==memcmp(str, "left", 4))
            return ALIGN_LEFT;
        if(0==memcmp(str, "center-full", 11))
            return ALIGN_FULL_CENTER;
        if(0==memcmp(str, "center", 6))
            return ALIGN_CENTER;
        if(0==memcmp(str, "right", 5))
            return ALIGN_RIGHT;
    }
    return def;
}

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

WindowBorders qtcGetWindowBorderSize(bool force)
{
    static WindowBorders def={24, 18, 4, 4};
    static WindowBorders sizes={-1, -1, -1, -1};

    if (-1 == sizes.titleHeight || force) {
        QFile f(qtcConfDir()+QString(BORDER_SIZE_FILE));

        if (f.open(QIODevice::ReadOnly)) {
            QTextStream stream(&f);
            QString     line;

            sizes.titleHeight=stream.readLine().toInt();
            sizes.toolTitleHeight=stream.readLine().toInt();
            sizes.bottom=stream.readLine().toInt();
            sizes.sides=stream.readLine().toInt();
            f.close();
        }
    }
    return sizes.titleHeight<12 ? def : sizes;
}

#ifndef CONFIG_DIALOG

bool qtcBarHidden(const QString &app, const char *prefix)
{
    return QFile::exists(QFile::decodeName(qtcConfDir())+prefix+app);
}

void qtcSetBarHidden(const QString &app, bool hidden, const char *prefix)
{
    if(!hidden)
        QFile::remove(QFile::decodeName(qtcConfDir())+prefix+app);
    else
        QFile(QFile::decodeName(qtcConfDir())+prefix+app).open(QIODevice::WriteOnly);
}

void qtcLoadBgndImage(QtCImage *img)
{
    if(!img->loaded &&
        ( (img->width>16 && img->width<1024 && img->height>16 && img->height<1024) || (0==img->width && 0==img->height)) )
    {
        img->loaded=true;
        img->pixmap.img=QPixmap();
        QString file(determineFileName(img->pixmap.file));

        if(!file.isEmpty())
        {
            bool loaded=false;
            if(0!=img->width && (file.endsWith(".svg", Qt::CaseInsensitive) || file.endsWith(".svgz", Qt::CaseInsensitive)))
            {
                QSvgRenderer svg(file);

                if(svg.isValid())
                {
                    img->pixmap.img=QPixmap(img->width, img->height);
                    img->pixmap.img.fill(Qt::transparent);
                    QPainter painter(&img->pixmap.img);
                    svg.render(&painter);
                    painter.end();
                    loaded=true;
                }
            }
            if(!loaded && img->pixmap.img.load(file) && 0!=img->width &&
               (img->pixmap.img.height()!=img->height || img->pixmap.img.width()!=img->width))
                img->pixmap.img=img->pixmap.img.scaled(img->width, img->height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
    }
}

#endif

static void
checkColor(EShade *s, QColor *c)
{
    if (*s == SHADE_CUSTOM && QtCurve::isBlack(*c)) {
        *s = SHADE_NONE;
    }
}

class QtCConfig {
public:
    QtCConfig(const QString &filename);
    bool ok() const {return values.count() > 0;}
    bool hasKey(const QString &key) {return values.contains(key);}
    QString readEntry(const QString &key, const QString &def=QString::null);
private:
    QMap<QString, QString> values;
};

QtCConfig::QtCConfig(const QString &filename)
{
    QFile f(filename);

    if (f.open(QIODevice::ReadOnly)) {
        QTextStream stream(&f);
        QString     line;

        while(!stream.atEnd())
        {
            line = stream.readLine();
            int pos=line.indexOf('=');
            if(-1!=pos)
                values[line.left(pos)]=line.mid(pos+1);
        }
        f.close();
    }
}

inline QString
QtCConfig::readEntry(const QString &key, const QString &def)
{
    return values.contains(key) ? values[key] : def;
}

inline QString readStringEntry(QtCConfig &cfg, const QString &key)
{
    return cfg.readEntry(key);
}

static int readNumEntry(QtCConfig &cfg, const QString &key, int def)
{
    const QString &val(readStringEntry(cfg, key));

    return val.isEmpty() ? def : val.toInt();
}

static int readVersionEntry(QtCConfig &cfg, const QString &key)
{
    const QString &val(readStringEntry(cfg, key));
    int           major, minor, patch;

    return !val.isEmpty() && 3==sscanf(TO_LATIN1(val), "%d.%d.%d", &major, &minor, &patch)
            ? qtcMakeVersion(major, minor, patch)
            : 0;
}

static bool readBoolEntry(QtCConfig &cfg, const QString &key, bool def)
{
    const QString &val(readStringEntry(cfg, key));

    return val.isEmpty() ? def : (val=="true" ? true : false);
}

static void readDoubleList(QtCConfig &cfg, const char *key, double *list, int count)
{
    QStringList strings(readStringEntry(cfg, key).split(',', QString::SkipEmptyParts));
    bool ok(count==strings.size());

    if (ok) {
        QStringList::ConstIterator it(strings.begin());
        for (int i = 0;i < count && ok;++i, ++it) {
            list[i]=(*it).toDouble(&ok);
        }
    }

    if (!ok && strings.size()) {
        list[0] = 0;
    }
}

#define CFG_READ_COLOR(ENTRY) do {                      \
        QString sVal(cfg.readEntry(#ENTRY));            \
        if (sVal.isEmpty()) {                           \
            opts->ENTRY = def->ENTRY;                   \
        } else {                                        \
            qtcSetRgb(&opts->ENTRY, TO_LATIN1(sVal));   \
        }                                               \
    } while (0)

#define CFG_READ_IMAGE(ENTRY) do {                                      \
        opts->ENTRY.type =                                              \
            toImageType(TO_LATIN1(readStringEntry(cfg, #ENTRY)),        \
                        def->ENTRY.type);                               \
        opts->ENTRY.loaded = false;                                     \
        opts->ENTRY.width = opts->ENTRY.height = 0;                     \
        opts->ENTRY.onBorder = false;                                   \
        opts->ENTRY.pos = PP_TR;                                        \
        if (opts->ENTRY.type == IMG_FILE) {                             \
            QString file(cfg.readEntry(#ENTRY ".file"));                \
            if (!file.isEmpty()) {                                      \
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

#define CFG_READ_STRING_LIST(ENTRY) do {                                \
        QString val = readStringEntry(cfg, #ENTRY);                     \
        Strings set = val.isEmpty() ? Strings() :                       \
            Strings::fromList(val.split(",", QString::SkipEmptyParts)); \
        opts->ENTRY = set.count() || cfg.hasKey(#ENTRY) ? set : def->ENTRY; \
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
                                   def->ENTRY, ALLOW, NULL, false);     \
    } while (0)

#define CFG_READ_APPEARANCE_PIXMAP(ENTRY, ALLOW, PIXMAP, CHECK) do {    \
        opts->ENTRY = toAppearance(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                                   def->ENTRY, ALLOW, PIXMAP, CHECK);   \
    } while (0)

#define CFG_READ_STRIPE(ENTRY) do {                                     \
        opts->ENTRY=toStripe(TO_LATIN1(readStringEntry(cfg, #ENTRY)),   \
                             def->ENTRY);                               \
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

#define CFG_READ_ALIGN(ENTRY) do {                                      \
        opts->ENTRY = toAlign(TO_LATIN1(readStringEntry(cfg, #ENTRY)),  \
                              def->ENTRY);                              \
    } while (0)

#define CFG_READ_TB_ICON(ENTRY) do {                                    \
        opts->ENTRY = toTitlebarIcon(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                                     def->ENTRY);                       \
    } while (0)

#define CFG_READ_GLOW(ENTRY) do {                                       \
        opts->ENTRY = toGlow(TO_LATIN1(readStringEntry(cfg, #ENTRY)),   \
                             def->ENTRY);                               \
    } while (0)

#define CFG_READ_TBAR_BTN(ENTRY) do {                                   \
        opts->ENTRY = toTBarBtn(TO_LATIN1(readStringEntry(cfg, #ENTRY)), \
                                def->ENTRY);                            \
    } while (0)

static void checkAppearance(EAppearance *ap, Options *opts)
{
    if(*ap>=APPEARANCE_CUSTOM1 && *ap<(APPEARANCE_CUSTOM1+NUM_CUSTOM_GRAD))
    {
        if(opts->customGradient.end()==opts->customGradient.find(*ap))
        {
            if(ap==&opts->appearance)
                *ap=APPEARANCE_FLAT;
            else
                *ap=opts->appearance;
        }
    }
}

void qtcDefaultSettings(Options *opts);
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
    checkAppearance(&opts->titlebarButtonAppearance, opts);
    checkAppearance(&opts->selectionAppearance, opts);
    checkAppearance(&opts->dwtAppearance, opts);
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

#if defined CONFIG_DIALOG
    if(opts->expanderHighlight<MIN_HIGHLIGHT_FACTOR || opts->expanderHighlight>MAX_HIGHLIGHT_FACTOR)
        opts->expanderHighlight=DEFAULT_EXPANDER_HIGHLIGHT_FACTOR;
#endif

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
        opts->menuBgndOpacity=100;

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

    if(!(opts->titlebarButtons & TITLEBAR_BUTTON_ROUND))
        opts->titlebarButtonAppearance=MODIFY_AGUA(opts->titlebarButtonAppearance);
    opts->dwtAppearance=MODIFY_AGUA(opts->dwtAppearance);
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

bool qtcReadConfig(const QString &file, Options *opts, Options *defOpts, bool checkImages)
{
    if (file.isEmpty()) {
        const char *env = getenv("QTCURVE_CONFIG_FILE");

        if (env)
            return qtcReadConfig(env, opts, defOpts);

        const char *cfgDir = qtcConfDir();
        if (cfgDir) {
            QString filename(QFile::decodeName(cfgDir)+CONFIG_FILE);

            if (!QFile::exists(filename))
                filename = QFile::decodeName(cfgDir) + "../" OLD_CONFIG_FILE;
            return qtcReadConfig(filename, opts, defOpts);
        }
    } else {
        QtCConfig cfg(file);

        if(cfg.ok())
        {
            int     i;

            opts->version=readVersionEntry(cfg, VERSION_KEY);

            Options newOpts;

            if(defOpts)
                newOpts=*defOpts;
            else
                qtcDefaultSettings(&newOpts);

            Options *def=&newOpts;

            if(opts!=def)
                opts->customGradient=def->customGradient;

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
            } else {
                CFG_READ_INT(windowBorder);
            }

            if (opts->version < qtcMakeVersion(1, 7)) {
                opts->windowBorder |= WINDOW_BORDER_FILL_TITLEBAR;
                def->square = SQUARE_POPUP_MENUS;
            }

            if (opts->version < qtcMakeVersion(1, 4)) {
                opts->square=
                    (readBoolEntry(cfg, "squareLvSelection", def->square&SQUARE_LISTVIEW_SELECTION) ? SQUARE_LISTVIEW_SELECTION : SQUARE_NONE)+
                    (readBoolEntry(cfg, "squareScrollViews", def->square&SQUARE_SCROLLVIEW) ? SQUARE_SCROLLVIEW : SQUARE_NONE)+
                    (readBoolEntry(cfg, "squareProgress", def->square&SQUARE_PROGRESS) ? SQUARE_PROGRESS : SQUARE_NONE)+
                    (readBoolEntry(cfg, "squareEntry", def->square&SQUARE_ENTRY)? SQUARE_ENTRY : SQUARE_NONE);
            } else {
                CFG_READ_INT(square);
            }

            if (opts->version < qtcMakeVersion(1, 7)) {
                def->tbarBtns=TBTN_STANDARD;
                opts->thin=(readBoolEntry(cfg, "thinnerMenuItems", def->thin&THIN_MENU_ITEMS) ? THIN_MENU_ITEMS : 0)+
                           (readBoolEntry(cfg, "thinnerBtns", def->thin&THIN_BUTTONS) ? THIN_BUTTONS : 0);
            }
            else
            {
                CFG_READ_INT(thin);
            }
            if(opts->version<qtcMakeVersion(1, 6))
                opts->square|=SQUARE_TOOLTIPS;
            if(opts->version<qtcMakeVersion(1, 6, 1))
                opts->square|=SQUARE_POPUP_MENUS;
            if(opts->version<qtcMakeVersion(1, 2))
                def->crSize=CR_SMALL_SIZE;
            if(opts!=def)
            {
                opts->customShades[0]=0;
                opts->customAlphas[0]=0;
                if(USE_CUSTOM_SHADES(*def))
                    memcpy(opts->customShades, def->customShades, sizeof(double)*QTC_NUM_STD_SHADES);
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
            if(opts->version<qtcMakeVersion(1, 8))
            {
                opts->tbarBtnAppearance=APPEARANCE_NONE;
                opts->tbarBtnEffect=EFFECT_NONE;
            }
            else
            {
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

            if(APPEARANCE_FLAT==opts->menuBgndAppearance && 0==opts->lighterPopupMenuBgnd && opts->version<qtcMakeVersion(1, 7))
                opts->menuBgndAppearance=APPEARANCE_RAISED;

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
            CFG_READ_SHADE(shadeSliders, true, false, &opts->customSlidersColor);
            CFG_READ_SHADE(shadeMenubars, true, false, &opts->customMenubarsColor);
            CFG_READ_SHADE(shadeCheckRadio, false, false, &opts->customCheckRadioColor);
            CFG_READ_SHADE(sortedLv, true, false, &opts->customSortedLvColor);
            CFG_READ_SHADE(crColor,  true, false, &opts->customCrBgndColor);
            CFG_READ_SHADE(progressColor, false, false, &opts->customProgressColor);
            CFG_READ_APPEARANCE(menubarAppearance, APP_ALLOW_BASIC);
            CFG_READ_APPEARANCE(menuitemAppearance, APP_ALLOW_FADE);
            CFG_READ_APPEARANCE(toolbarAppearance, APP_ALLOW_BASIC);
            CFG_READ_APPEARANCE(selectionAppearance, APP_ALLOW_BASIC);
            CFG_READ_APPEARANCE(dwtAppearance, APP_ALLOW_BASIC);
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
            if(opts->version<qtcMakeVersion(1, 6))
                opts->tooltipAppearance=APPEARANCE_FLAT;
            else
            {
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

            CFG_READ_BOOL(stdBtnSizes);
            CFG_READ_INT(titlebarButtons);
            CFG_READ_TB_ICON(titlebarIcon);
            CFG_READ_BOOL(xbar);
            CFG_READ_INT(dwtSettings);
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
            CFG_READ_ALIGN(titlebarAlignment);
            CFG_READ_EFFECT(titlebarEffect);
            CFG_READ_BOOL(centerTabText);
#if defined CONFIG_DIALOG
            CFG_READ_INT(expanderHighlight);
            CFG_READ_BOOL(mapKdeIcons);
#endif
            CFG_READ_BOOL(gtkButtonOrder);
#if defined CONFIG_DIALOG
            CFG_READ_BOOL(reorderGtkButtons);
#endif
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
            CFG_READ_APPEARANCE(titlebarButtonAppearance, APP_ALLOW_BASIC);
            if(opts->xbar && opts->menubarHiding)
                opts->xbar=false;
            CFG_READ_SHADING(shading);
            CFG_READ_IMAGE(bgndImage);
            CFG_READ_IMAGE(menuBgndImage);
            CFG_READ_STRING_LIST(noMenuStripeApps);
            CFG_READ_STRING_LIST(noBgndGradientApps);
            CFG_READ_STRING_LIST(noBgndOpacityApps);
            CFG_READ_STRING_LIST(noMenuBgndOpacityApps);
            CFG_READ_STRING_LIST(noBgndImageApps);
#ifdef CONFIG_DIALOG
            if(opts->version<qtcMakeVersion(1, 7, 2))
                opts->noMenuBgndOpacityApps << "gtk";
#endif
            CFG_READ_STRING_LIST(menubarApps);
            CFG_READ_STRING_LIST(statusbarApps);
            CFG_READ_STRING_LIST(useQtFileDialogApps);
            CFG_READ_STRING_LIST(windowDragWhiteList);
            CFG_READ_STRING_LIST(windowDragBlackList);
            readDoubleList(cfg, "customShades", opts->customShades, QTC_NUM_STD_SHADES);
            readDoubleList(cfg, "customAlphas", opts->customAlphas, NUM_STD_ALPHAS);

            if (opts->titlebarButtons & TITLEBAR_BUTTON_COLOR ||
                opts->titlebarButtons & TITLEBAR_BUTTON_ICON_COLOR) {
                QStringList cols(readStringEntry(cfg, "titlebarButtonColors").split(',', QString::SkipEmptyParts));
                if(cols.count() && 0==(cols.count()%NUM_TITLEBAR_BUTTONS) && cols.count()<=(NUM_TITLEBAR_BUTTONS*3))
                {
                    QStringList::ConstIterator it(cols.begin()),
                                               end(cols.end());

                    for (int i = 0;it != end;++it, ++i) {
                        QColor col;
                        qtcSetRgb(&col, TO_LATIN1((*it)));
                        opts->titlebarButtonColors[i]=col;
                    }
                    if(cols.count()<(NUM_TITLEBAR_BUTTONS+1))
                        opts->titlebarButtons&=~TITLEBAR_BUTTON_ICON_COLOR;
                }
                else
                {
                    opts->titlebarButtons&=~TITLEBAR_BUTTON_COLOR;
                    opts->titlebarButtons&=~TITLEBAR_BUTTON_ICON_COLOR;
                }
            }

            for(i=APPEARANCE_CUSTOM1; i<(APPEARANCE_CUSTOM1+NUM_CUSTOM_GRAD); ++i)
            {
                QString gradKey;

                gradKey.sprintf("customgradient%d", (i-APPEARANCE_CUSTOM1)+1);

                QStringList vals(readStringEntry(cfg, gradKey)
                                 .split(',', QString::SkipEmptyParts));

                if(vals.size())
                    opts->customGradient.erase((EAppearance)i);

                if(vals.size()>=5)
                {
                    QStringList::ConstIterator it(vals.begin()),
                                               end(vals.end());
                    bool                       ok(true),
                                               haveAlpha(false);
                    Gradient                   grad;
                    int                        j;

                    grad.border=toGradientBorder(TO_LATIN1((*it)), &haveAlpha);
                    ok=vals.size()%(haveAlpha ? 3 : 2);

                    for(++it, j=0; it!=end && ok; ++it, ++j)
                    {
                        double pos=(*it).toDouble(&ok),
                               val=ok ? (*(++it)).toDouble(&ok) : 0.0,
                               alpha=haveAlpha && ok ? (*(++it)).toDouble(&ok) : 1.0;

                        ok=ok && (pos>=0 && pos<=1.0) && (val>=0.0 && val<=2.0) && (alpha>=0.0 && alpha<=1.0);

                        if(ok)
                            grad.stops.insert(GradientStop(pos, val, alpha));
                    }

                    if (ok) {
                        opts->customGradient[(EAppearance)i] = grad;
                        opts->customGradient[(EAppearance)i].stops =
                            grad.stops.fix();
                    }
                }
            }
            qtcCheckConfig(opts);
            return true;
        } else {
            if(defOpts)
                *opts=*defOpts;
            else
                qtcDefaultSettings(opts);
            return true;
        }
    }

    return false;
}

static const char * getSystemConfigFile()
{
    static const char * constFiles[]={ /*"/etc/qt4/" OLD_CONFIG_FILE, "/etc/qt3/" OLD_CONFIG_FILE, "/etc/qt/" OLD_CONFIG_FILE,*/ "/etc/" OLD_CONFIG_FILE, NULL };

    int i;

    for(i=0; constFiles[i]; ++i)
        if(qtcIsRegFile(constFiles[i]))
            return constFiles[i];
    return NULL;
}

void qtcDefaultSettings(Options *opts)
{
    /* Set hard-coded defaults... */
    // Setup titlebar gradients...
    qtcSetupGradient(&opts->customGradient[APPEARANCE_CUSTOM1], GB_3D, 3,
                     0.0, 1.2, 0.5, 1.0, 1.0, 1.0);
    qtcSetupGradient(&opts->customGradient[APPEARANCE_CUSTOM2], GB_3D, 3,
                     0.0, 0.9, 0.5, 1.0, 1.0, 1.0);
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
    opts->dwtAppearance=APPEARANCE_CUSTOM1;
#if defined CONFIG_DIALOG
    opts->reorderGtkButtons=false;
#endif
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
    opts->stdBtnSizes=false;
    opts->titlebarButtons=TITLEBAR_BUTTON_ROUND|TITLEBAR_BUTTON_HOVER_SYMBOL;
    opts->titlebarIcon=TITLEBAR_ICON_NEXT_TO_TITLE;
    opts->menuStripe=SHADE_NONE;
    opts->menuStripeAppearance=APPEARANCE_DARK_INVERTED;
    opts->shading=Shading::HSL;
    opts->gtkScrollViews=true;
    opts->comboBtn=SHADE_NONE;
    opts->doubleGtkComboArrow=true;
    opts->stdSidebarButtons=false;
    opts->toolbarTabs=false;
    opts->bgndOpacity=opts->dlgOpacity=opts->menuBgndOpacity=100;
    opts->gtkComboMenus=false;
    opts->customMenubarsColor.setRgb(0, 0, 0);
    opts->customSlidersColor.setRgb(0, 0, 0);
    opts->customMenuNormTextColor.setRgb(0, 0, 0);
    opts->customMenuSelTextColor.setRgb(0, 0, 0);
    opts->customCheckRadioColor.setRgb(0, 0, 0);
    opts->customComboBtnColor.setRgb(0, 0, 0);
    opts->customMenuStripeColor.setRgb(0, 0, 0);
    opts->customProgressColor.setRgb(0, 0, 0);
    opts->titlebarAlignment=ALIGN_FULL_CENTER;
    opts->titlebarEffect=EFFECT_SHADOW;
    opts->centerTabText=false;
    opts->xbar=false;
    opts->dwtSettings=DWT_BUTTONS_AS_PER_TITLEBAR|DWT_ROUND_TOP_ONLY;
    opts->menubarApps << "smplayer" << "VirtualBox";
    opts->statusbarApps << "kde";
    opts->noMenuBgndOpacityApps << "sonata" << "totem" << "vmware"
                                << "vmplayer" << "gtk";
    opts->noBgndOpacityApps << "smplayer" << "sonata" << "totem" << "vmware"
                            << "vmplayer";
    opts->noMenuStripeApps << "gtk" << "soffice.bin";

#if defined CONFIG_DIALOG
    opts->mapKdeIcons=true;
    opts->expanderHighlight=DEFAULT_EXPANDER_HIGHLIGHT_FACTOR;
#endif
    opts->titlebarAppearance=APPEARANCE_CUSTOM1;
    opts->inactiveTitlebarAppearance=APPEARANCE_CUSTOM1;
    opts->titlebarButtonAppearance=APPEARANCE_GRADIENT;
    /* Read system config file... */
    {
    static const char * systemFilename=NULL;

    if(!systemFilename)
        systemFilename=getSystemConfigFile();

    if(systemFilename)
        qtcReadConfig(systemFilename, opts, opts);
    }
}

#ifdef CONFIG_WRITE
#include <KDE/KConfig>
#include <KDE/KConfigGroup>

static const char*
toStr(EDefBtnIndicator ind)
{
    switch(ind) {
    case IND_NONE:
        return "none";
    case IND_FONT_COLOR:
        return "fontcolor";
    case IND_CORNER:
        return "corner";
    case IND_TINT:
        return "tint";
    case IND_GLOW:
        return "glow";
    case IND_DARKEN:
        return "darken";
    case IND_SELECTED:
        return "origselected";
    default:
        return "colored";
    }
}

static const char *toStr(ELine ind, bool dashes)
{
    switch(ind)
    {
        case LINE_1DOT:
            return "1dot";
        case LINE_DOTS:
            return "dots";
        case LINE_DASHES:
            return dashes ? "dashes" : "none";
        case LINE_NONE:
            return "none";
        case LINE_FLAT:
            return "flat";
        default:
            return "sunken";
    }
}

static const char *toStr(ETBarBorder ind)
{
    switch(ind)
    {
        case TB_DARK:
            return "dark";
        case TB_DARK_ALL:
            return "dark-all";
        case TB_LIGHT_ALL:
            return "light-all";
        case TB_NONE:
            return "none";
        default:
            return "light";
    }
}

static const char *toStr(EMouseOver mo)
{
    switch(mo)
    {
        case MO_COLORED:
            return "colored";
        case MO_COLORED_THICK:
            return "thickcolored";
        case MO_NONE:
            return "none";
        case MO_GLOW:
            return "glow";
        default:
            return "plastik";
    }
}

static QString toStr(EAppearance exp, EAppAllow allow, const QtCPixmap *pix)
{
    switch(exp)
    {
        case APPEARANCE_FLAT:
            return "flat";
        case APPEARANCE_RAISED:
            return "raised";
        case APPEARANCE_DULL_GLASS:
            return "dullglass";
        case APPEARANCE_SHINY_GLASS:
            return "shinyglass";
        case APPEARANCE_AGUA:
            return "agua";
        case APPEARANCE_SOFT_GRADIENT:
            return "soft";
        case APPEARANCE_GRADIENT:
            return "gradient";
        case APPEARANCE_HARSH_GRADIENT:
            return "harsh";
        case APPEARANCE_INVERTED:
            return "inverted";
        case APPEARANCE_DARK_INVERTED:
            return "darkinverted";
        case APPEARANCE_SPLIT_GRADIENT:
            return "splitgradient";
        case APPEARANCE_BEVELLED:
            return "bevelled";
        case APPEARANCE_FILE:
            // When savng, strip users config dir from location.
            return QLatin1String("file:")+
                    (pix->file.startsWith(qtcConfDir())
                        ? pix->file.mid(strlen(qtcConfDir())+1)
                        : pix->file);
        case APPEARANCE_FADE:
            switch(allow)
            {
                case APP_ALLOW_BASIC: // Should not get here!
                case APP_ALLOW_FADE:
                    return "fade";
                case APP_ALLOW_STRIPED:
                    return "striped";
                case APP_ALLOW_NONE:
                    return "none";
            }
        default:
        {
            QString app;

            app.sprintf("customgradient%d", (exp-APPEARANCE_CUSTOM1)+1);
            return app;
        }
    }
}

static QString toStr(const QColor &col)
{
    QString colorStr;

    colorStr.sprintf("#%02X%02X%02X", col.red(), col.green(), col.blue());
    return colorStr;
}

static QString toStr(EShade exp, const QColor &col)
{
    switch(exp)
    {
        default:
        case SHADE_NONE:
            return "none";
        case SHADE_BLEND_SELECTED:
            return "selected";
        case SHADE_CUSTOM:
            return toStr(col);
        case SHADE_SELECTED:
            return "origselected";
        case SHADE_DARKEN:
            return "darken";
        case SHADE_WINDOW_BORDER:
            return "wborder";
    }
}

static const char *toStr(ERound exp)
{
    switch(exp)
    {
        case ROUND_NONE:
            return "none";
        case ROUND_SLIGHT:
            return "slight";
        case ROUND_EXTRA:
            return "extra";
        case ROUND_MAX:
            return "max";
        default:
        case ROUND_FULL:
            return "full";
    }
}

static const char *toStr(EScrollbar sb)
{
    switch(sb)
    {
        case SCROLLBAR_KDE:
            return "kde";
        default:
        case SCROLLBAR_WINDOWS:
            return "windows";
        case SCROLLBAR_PLATINUM:
            return "platinum";
        case SCROLLBAR_NEXT:
            return "next";
        case SCROLLBAR_NONE:
            return "none";
    }
}

static const char *toStr(EFrame sb)
{
    switch(sb)
    {
        case FRAME_NONE:
            return "none";
        case FRAME_PLAIN:
            return "plain";
        case FRAME_LINE:
            return "line";
        case FRAME_SHADED:
            return "shaded";
        case FRAME_FADED:
        default:
            return "faded";
    }
}

static const char *toStr(EEffect e)
{
    switch(e)
    {
        case EFFECT_NONE:
            return "none";
        default:
        case EFFECT_SHADOW:
            return "shadow";
        case EFFECT_ETCH:
            return "etch";
    }
}

inline const char * toStr(bool b) { return b ? "true" : "false"; }

static const char*
toStr(Shading s)
{
    switch (s) {
    case Shading::Simple:
        return "simple";
    default:
    case Shading::HSL:
        return "hsl";
    case Shading::HSV:
        return "hsv";
    case Shading::HCY:
        return "hcy";
    }
}

static const char *toStr(EStripe s)
{
    switch(s)
    {
        default:
        case STRIPE_PLAIN:
            return "plain";
        case STRIPE_NONE:
            return "none";
        case STRIPE_DIAGONAL:
            return "diagonal";
        case STRIPE_FADE:
            return "fade";
    }
}

static const char *toStr(ESliderStyle s)
{
    switch(s)
    {
        case SLIDER_PLAIN:
            return "plain";
        case SLIDER_TRIANGULAR:
            return "triangular";
        case SLIDER_ROUND_ROTATED:
            return "r-round";
        case SLIDER_PLAIN_ROTATED:
            return "r-plain";
        case SLIDER_CIRCULAR:
            return "circular";
        default:
        case SLIDER_ROUND:
            return "round";
    }
}

static const char *toStr(EColor s)
{
    switch(s)
    {
        case ECOLOR_BACKGROUND:
            return "background";
        case ECOLOR_DARK:
            return "dark";
        default:
        case ECOLOR_BASE:
            return "base";
    }
}

static const char *toStr(EFocus f)
{
    switch(f)
    {
        default:
        case FOCUS_STANDARD:
            return "standard";
        case FOCUS_RECTANGLE:
            return "rect";
        case FOCUS_FILLED:
            return "filled";
        case FOCUS_FULL:
            return "full";
        case FOCUS_LINE:
            return "line";
        case FOCUS_GLOW:
            return "glow";
        case FOCUS_NONE:
            return "none";
    }
}

static const char *toStr(ETabMo f)
{
    switch(f)
    {
        default:
        case TAB_MO_BOTTOM:
            return "bot";
        case TAB_MO_TOP:
            return "top";
        case TAB_MO_GLOW:
            return "glow";
    }
}

static const char *toStr(EGradientBorder g)
{
    switch(g)
    {
        case GB_NONE:
            return "none";
        case GB_LIGHT:
            return "light";
        case GB_3D_FULL:
            return "3dfull";
        case GB_SHINE:
            return "shine";
        default:
        case GB_3D:
            return "3d";
    }
}

static const char *toStr(EAlign ind)
{
    switch(ind)
    {
        default:
        case ALIGN_LEFT:
            return "left";
        case ALIGN_CENTER:
            return "center";
        case ALIGN_FULL_CENTER:
            return "center-full";
        case ALIGN_RIGHT:
            return "right";
    }
}

static const char * toStr(ETitleBarIcon icn)
{
    switch(icn)
    {
        case TITLEBAR_ICON_NONE:
            return "none";
        default:
        case TITLEBAR_ICON_MENU_BUTTON:
            return "menu";
        case TITLEBAR_ICON_NEXT_TO_TITLE:
            return "title";
    }
}

static const char * toStr(EGradType gt)
{
    switch(gt)
    {
        case GT_VERT:
            return "vert";
        default:
        case GT_HORIZ:
            return "horiz";
    }
}

#if 0
static const char * toStr(ELvLines lv)
{
    switch(lv)
    {
        case LV_NEW:
            return "new";
        case LV_OLD:
            return "old";
        default:
        case LV_NONE:
            return "none";
    }
}
#endif

static const char * toStr(EImageType lv)
{
    switch(lv)
    {
        default:
        case IMG_NONE:
            return "none";
        case IMG_PLAIN_RINGS:
            return "plainrings";
        case IMG_BORDERED_RINGS:
            return "rings";
        case IMG_SQUARE_RINGS:
            return "squarerings";
        case IMG_FILE:
            return "file";
    }
}

static const char * toStr(EGlow lv)
{
    switch(lv)
    {
        default:
        case GLOW_NONE:
            return "none";
        case GLOW_START:
            return "start";
        case GLOW_MIDDLE:
            return "middle";
        case GLOW_END:
            return "end";
    }
}

static const char * toStr(ETBarBtn tb)
{
    switch(tb)
    {
        default:
        case TBTN_STANDARD:
            return "standard";
        case TBTN_RAISED:
            return "raised";
        case TBTN_JOINED:
            return "joined";
    }
}

#include <QTextStream>
#define CFG config

#define CFG_WRITE_ENTRY(ENTRY) do {                     \
        if (!exportingStyle && def.ENTRY == opts.ENTRY) \
            CFG.deleteEntry(#ENTRY);                    \
        else                                            \
            CFG.writeEntry(#ENTRY, toStr(opts.ENTRY));  \
    } while (0)

#define CFG_WRITE_APPEARANCE_ENTRY(ENTRY, ALLOW) \
    if (!exportingStyle && def.ENTRY==opts.ENTRY) \
        CFG.deleteEntry(#ENTRY); \
    else \
        CFG.writeEntry(#ENTRY, toStr(opts.ENTRY, ALLOW, NULL));

#define CFG_WRITE_APPEARANCE_ENTRY_PIXMAP(ENTRY, ALLOW, PIXMAP) \
    if (!exportingStyle && def.ENTRY==opts.ENTRY) \
        CFG.deleteEntry(#ENTRY); \
    else \
        CFG.writeEntry(#ENTRY, toStr(opts.ENTRY, ALLOW, &opts.PIXMAP));

#define CFG_WRITE_ENTRY_B(ENTRY, B) \
    if (!exportingStyle && def.ENTRY==opts.ENTRY) \
        CFG.deleteEntry(#ENTRY); \
    else \
        CFG.writeEntry(#ENTRY, toStr(opts.ENTRY, B));

#define CFG_WRITE_ENTRY_NUM(ENTRY) \
    if (!exportingStyle && def.ENTRY==opts.ENTRY) \
        CFG.deleteEntry(#ENTRY); \
    else \
        CFG.writeEntry(#ENTRY, opts.ENTRY);

#define CFG_WRITE_SHADE_ENTRY(ENTRY, COL) \
    if (!exportingStyle && def.ENTRY==opts.ENTRY) \
        CFG.deleteEntry(#ENTRY); \
    else \
        CFG.writeEntry(#ENTRY, toStr(opts.ENTRY, opts.COL));

#define CFG_WRITE_IMAGE_ENTRY(ENTRY) \
    if (!exportingStyle && def.ENTRY.type==opts.ENTRY.type) \
        CFG.deleteEntry(#ENTRY); \
    else \
        CFG.writeEntry(#ENTRY, toStr(opts.ENTRY.type)); \
    if(IMG_FILE!=opts.ENTRY.type) \
    { \
        CFG.deleteEntry(#ENTRY ".file"); \
        CFG.deleteEntry(#ENTRY ".width"); \
        CFG.deleteEntry(#ENTRY ".height"); \
        CFG.deleteEntry(#ENTRY ".onBorder"); \
        CFG.deleteEntry(#ENTRY ".pos"); \
    } \
    else \
    { \
        CFG.writeEntry(#ENTRY ".file", opts.ENTRY.pixmap.file); \
        CFG.writeEntry(#ENTRY ".width", opts.ENTRY.width); \
        CFG.writeEntry(#ENTRY ".height", opts.ENTRY.height); \
        CFG.writeEntry(#ENTRY ".onBorder", opts.ENTRY.onBorder); \
        CFG.writeEntry(#ENTRY ".pos", (int)(opts.ENTRY.pos)); \
    }

#define CFG_WRITE_STRING_LIST_ENTRY(ENTRY) \
    if (!exportingStyle && def.ENTRY==opts.ENTRY) \
        CFG.deleteEntry(#ENTRY); \
    else \
        CFG.writeEntry(#ENTRY, QStringList(opts.ENTRY.toList()).join(",")); \

bool qtcWriteConfig(KConfig *cfg, const Options &opts, const Options &def, bool exportingStyle)
{
    if (!cfg) {
        const char *cfgDir=qtcConfDir();

        if (cfgDir) {
            KConfig defCfg(QFile::decodeName(cfgDir) + CONFIG_FILE,
                           KConfig::SimpleConfig);

            if(qtcWriteConfig(&defCfg, opts, def, exportingStyle))
            {
                const char *oldFiles[]={ OLD_CONFIG_FILE, "qtcurve.gtk-icons", 0};

                for(int i=0; oldFiles[i]; ++i)
                {
                    QString oldFileName(QFile::decodeName(cfgDir)+QString("../")+oldFiles[i]);

                    if(QFile::exists(oldFileName))
                        QFile::remove(oldFileName);
                }
            }
        }
    }
    else
    {
        KConfigGroup config(cfg, SETTINGS_GROUP);
        CFG.writeEntry(VERSION_KEY, qtcVersion());
        CFG_WRITE_ENTRY_NUM(passwordChar);
        CFG_WRITE_ENTRY_NUM(gbFactor);
        CFG_WRITE_ENTRY(round);
        CFG_WRITE_ENTRY_NUM(highlightFactor);
        CFG_WRITE_ENTRY_NUM(menuDelay);
        CFG_WRITE_ENTRY_NUM(sliderWidth);
        CFG_WRITE_ENTRY(toolbarBorders);
        CFG_WRITE_APPEARANCE_ENTRY(appearance, APP_ALLOW_BASIC);
        CFG_WRITE_APPEARANCE_ENTRY(tbarBtnAppearance, APP_ALLOW_NONE);
        CFG_WRITE_ENTRY(tbarBtnEffect);
        CFG_WRITE_APPEARANCE_ENTRY_PIXMAP(bgndAppearance, APP_ALLOW_STRIPED,
                                          bgndPixmap);
        CFG_WRITE_ENTRY(bgndGrad);
        CFG_WRITE_ENTRY(menuBgndGrad);
        CFG_WRITE_APPEARANCE_ENTRY_PIXMAP(menuBgndAppearance, APP_ALLOW_STRIPED,
                                          menuBgndPixmap);
        CFG_WRITE_ENTRY(stripedProgress);
        CFG_WRITE_ENTRY(sliderStyle);
        CFG_WRITE_ENTRY(animatedProgress);
        CFG_WRITE_ENTRY_NUM(lighterPopupMenuBgnd);
        CFG_WRITE_ENTRY_NUM(tabBgnd);
        CFG_WRITE_ENTRY(embolden);
        CFG_WRITE_ENTRY(defBtnIndicator);
        CFG_WRITE_ENTRY_B(sliderThumbs, false);
        CFG_WRITE_ENTRY_B(handles, true);
        CFG_WRITE_ENTRY(highlightTab);
        CFG_WRITE_ENTRY_NUM(colorSelTab);
        CFG_WRITE_ENTRY(roundAllTabs);
        CFG_WRITE_ENTRY(tabMouseOver);
        CFG_WRITE_APPEARANCE_ENTRY(menubarAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_APPEARANCE_ENTRY(menuitemAppearance, APP_ALLOW_FADE);
        CFG_WRITE_APPEARANCE_ENTRY(toolbarAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_APPEARANCE_ENTRY(selectionAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_APPEARANCE_ENTRY(dwtAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_ENTRY(titlebarEffect);
        CFG_WRITE_APPEARANCE_ENTRY(menuStripeAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_ENTRY_B(toolbarSeparators, false);
        CFG_WRITE_ENTRY_B(splitters, true);
        CFG_WRITE_ENTRY(customMenuTextColor);
        CFG_WRITE_ENTRY(coloredMouseOver);
        CFG_WRITE_ENTRY(menubarMouseOver);
        CFG_WRITE_ENTRY(useHighlightForMenu);
        CFG_WRITE_ENTRY(shadeMenubarOnlyWhenActive);
        CFG_WRITE_ENTRY_NUM(thin);
        CFG_WRITE_SHADE_ENTRY(shadeSliders, customSlidersColor);
        CFG_WRITE_SHADE_ENTRY(shadeMenubars, customMenubarsColor);
        CFG_WRITE_SHADE_ENTRY(sortedLv, customSortedLvColor);
        CFG_WRITE_ENTRY(customMenuSelTextColor);
        CFG_WRITE_ENTRY(customMenuNormTextColor);
        CFG_WRITE_SHADE_ENTRY(shadeCheckRadio, customCheckRadioColor);
        CFG_WRITE_ENTRY(scrollbarType);
        CFG_WRITE_ENTRY(buttonEffect);
        CFG_WRITE_APPEARANCE_ENTRY(lvAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_APPEARANCE_ENTRY(tabAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_APPEARANCE_ENTRY(activeTabAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_APPEARANCE_ENTRY(sliderAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_APPEARANCE_ENTRY(progressAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_APPEARANCE_ENTRY(progressGrooveAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_APPEARANCE_ENTRY(grooveAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_APPEARANCE_ENTRY(sunkenAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_APPEARANCE_ENTRY(sbarBgndAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_APPEARANCE_ENTRY(tooltipAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_ENTRY(sliderFill);
        CFG_WRITE_ENTRY(progressGrooveColor);
        CFG_WRITE_ENTRY(focus);
        CFG_WRITE_ENTRY(lvButton);
        CFG_WRITE_ENTRY(lvLines);
        CFG_WRITE_ENTRY(drawStatusBarFrames);
        CFG_WRITE_ENTRY(fillSlider);
        CFG_WRITE_ENTRY(roundMbTopOnly);
        CFG_WRITE_ENTRY(borderMenuitems);
        CFG_WRITE_ENTRY(darkerBorders);
        CFG_WRITE_ENTRY(vArrows);
        CFG_WRITE_ENTRY(xCheck);
        CFG_WRITE_ENTRY(groupBox);
        CFG_WRITE_ENTRY_NUM(gbLabel);
        CFG_WRITE_ENTRY(fadeLines);
        CFG_WRITE_ENTRY(glowProgress);
        CFG_WRITE_IMAGE_ENTRY(bgndImage);
        CFG_WRITE_IMAGE_ENTRY(menuBgndImage);
        CFG_WRITE_ENTRY(colorMenubarMouseOver);
        CFG_WRITE_ENTRY_NUM(crHighlight);
        CFG_WRITE_ENTRY(crButton);
        CFG_WRITE_SHADE_ENTRY(crColor, customCrBgndColor);
        CFG_WRITE_SHADE_ENTRY(progressColor, customProgressColor);
        CFG_WRITE_ENTRY(smallRadio);
        CFG_WRITE_ENTRY(fillProgress);
        CFG_WRITE_ENTRY(comboSplitter);
        CFG_WRITE_ENTRY(highlightScrollViews);
        CFG_WRITE_ENTRY(etchEntry);
        CFG_WRITE_ENTRY_NUM(splitterHighlight);
        CFG_WRITE_ENTRY_NUM(expanderHighlight);
        CFG_WRITE_ENTRY_NUM(crSize);
        CFG_WRITE_ENTRY(flatSbarButtons);
        CFG_WRITE_ENTRY(borderSbarGroove);
        CFG_WRITE_ENTRY(borderProgress);
        CFG_WRITE_ENTRY(popupBorder);
        CFG_WRITE_ENTRY(unifySpinBtns);
        CFG_WRITE_ENTRY(unifySpin);
        CFG_WRITE_ENTRY(unifyCombo);
        CFG_WRITE_ENTRY(borderTab);
        CFG_WRITE_ENTRY(borderInactiveTab);
        CFG_WRITE_ENTRY(thinSbarGroove);
        CFG_WRITE_ENTRY(colorSliderMouseOver);
        CFG_WRITE_ENTRY(menuIcons);
        CFG_WRITE_ENTRY(forceAlternateLvCols);
        CFG_WRITE_ENTRY_NUM(square);
        CFG_WRITE_ENTRY(invertBotTab);
        CFG_WRITE_ENTRY_NUM(menubarHiding);
        CFG_WRITE_ENTRY_NUM(statusbarHiding);
        CFG_WRITE_ENTRY(boldProgress);
        CFG_WRITE_ENTRY(coloredTbarMo);
        CFG_WRITE_ENTRY(borderSelection);
        CFG_WRITE_ENTRY(stripedSbar);
        CFG_WRITE_ENTRY_NUM(windowDrag);
        CFG_WRITE_ENTRY(shadePopupMenu);
        CFG_WRITE_ENTRY(hideShortcutUnderline);
        CFG_WRITE_ENTRY_NUM(windowBorder);
        CFG_WRITE_ENTRY(tbarBtns);
        CFG_WRITE_ENTRY(xbar);
        CFG_WRITE_ENTRY_NUM(dwtSettings);
        CFG_WRITE_ENTRY_NUM(bgndOpacity);
        CFG_WRITE_ENTRY_NUM(menuBgndOpacity);
        CFG_WRITE_ENTRY_NUM(dlgOpacity);
        CFG_WRITE_ENTRY(stdBtnSizes);
        CFG_WRITE_ENTRY_NUM(titlebarButtons);
        CFG_WRITE_ENTRY(titlebarIcon);

        if ((opts.titlebarButtons & TITLEBAR_BUTTON_COLOR ||
             opts.titlebarButtons & TITLEBAR_BUTTON_ICON_COLOR) &&
            opts.titlebarButtonColors.size() &&
            !(opts.titlebarButtonColors.size() % NUM_TITLEBAR_BUTTONS)) {
            QString val;
            QTextStream str(&val);
            for (unsigned i = 0;i < opts.titlebarButtonColors.size();i++) {
                auto c = opts.titlebarButtonColors.find((ETitleBarButtons)i);

                if (c != opts.titlebarButtonColors.end()) {
                    if (i) {
                        str << ',';
                    }
                    str << toStr((*c).second);
                }
            }
            CFG.writeEntry("titlebarButtonColors", val);
        } else {
            CFG.deleteEntry("titlebarButtonColors");
        }
        CFG_WRITE_SHADE_ENTRY(menuStripe, customMenuStripeColor);
        CFG_WRITE_SHADE_ENTRY(comboBtn, customComboBtnColor);
        CFG_WRITE_ENTRY(stdSidebarButtons);
        CFG_WRITE_ENTRY(toolbarTabs);
        CFG_WRITE_APPEARANCE_ENTRY(titlebarAppearance, APP_ALLOW_NONE);
        CFG_WRITE_APPEARANCE_ENTRY(inactiveTitlebarAppearance, APP_ALLOW_NONE);
        CFG_WRITE_APPEARANCE_ENTRY(titlebarButtonAppearance, APP_ALLOW_BASIC);
        CFG_WRITE_ENTRY(gtkScrollViews);
        CFG_WRITE_ENTRY(gtkComboMenus);
        CFG_WRITE_ENTRY(doubleGtkComboArrow);
        CFG_WRITE_ENTRY(gtkButtonOrder);
#if defined CONFIG_DIALOG
        CFG_WRITE_ENTRY(reorderGtkButtons);
#endif
        CFG_WRITE_ENTRY(mapKdeIcons);
        CFG_WRITE_ENTRY(shading);
        CFG_WRITE_ENTRY(titlebarAlignment);
        CFG_WRITE_ENTRY(centerTabText);
        CFG_WRITE_STRING_LIST_ENTRY(noBgndGradientApps);
        CFG_WRITE_STRING_LIST_ENTRY(noBgndOpacityApps);
        CFG_WRITE_STRING_LIST_ENTRY(noMenuBgndOpacityApps);
        CFG_WRITE_STRING_LIST_ENTRY(noBgndImageApps);
        CFG_WRITE_STRING_LIST_ENTRY(noMenuStripeApps);
        CFG_WRITE_STRING_LIST_ENTRY(menubarApps);
        CFG_WRITE_STRING_LIST_ENTRY(statusbarApps);
        CFG_WRITE_STRING_LIST_ENTRY(useQtFileDialogApps);

        for(int i=APPEARANCE_CUSTOM1; i<(APPEARANCE_CUSTOM1+NUM_CUSTOM_GRAD); ++i)
        {
            GradientCont::const_iterator cg(opts.customGradient.find((EAppearance)i));
            QString                      gradKey;

            gradKey.sprintf("customgradient%d", (i-APPEARANCE_CUSTOM1)+1);

            if(cg==opts.customGradient.end())
                CFG.deleteEntry(gradKey);
            else
            {
                GradientCont::const_iterator d;

                if(exportingStyle || (d=def.customGradient.find((EAppearance)i))==def.customGradient.end() || !((*d)==(*cg)))
                {
                    QString gradVal;
                    QTextStream str(&gradVal);
                    GradientStopCont stops(cg->second.stops.fix());
                    GradientStopCont::const_iterator it(stops.begin()),
                                                     end(stops.end());
                    bool                             haveAlpha(false);

                    for(; it!=end && !haveAlpha; ++it)
                        if((*it).alpha<1.0)
                            haveAlpha=true;

                    str << toStr((*cg).second.border);
                    if(haveAlpha)
                        str << "-alpha";

                    for(it=stops.begin(); it!=end; ++it)
                        if(haveAlpha)
                            str << ',' << (*it).pos << ',' << (*it).val << ',' << (*it).alpha;
                        else
                            str << ',' << (*it).pos << ',' << (*it).val;
                    CFG.writeEntry(gradKey, gradVal);
                }
                else
                    CFG.deleteEntry(gradKey);
            }
        }

        if(opts.customShades[0]==0 ||
           exportingStyle ||
           opts.customShades[0]!=def.customShades[0] ||
           opts.customShades[1]!=def.customShades[1] ||
           opts.customShades[2]!=def.customShades[2] ||
           opts.customShades[3]!=def.customShades[3] ||
           opts.customShades[4]!=def.customShades[4] ||
           opts.customShades[5]!=def.customShades[5])
        {
            QString     shadeVal;
            QTextStream str(&shadeVal);
            if(0==opts.customShades[0])
                 str << 0;
            else
                for(int i=0; i<QTC_NUM_STD_SHADES; ++i)
                    if(0==i)
                        str << opts.customShades[i];
                    else
                        str << ',' << opts.customShades[i];
            CFG.writeEntry("customShades", shadeVal);
        }
        else
            CFG.deleteEntry("customShades");

        if(opts.customAlphas[0]==0 ||
           exportingStyle ||
           opts.customAlphas[0]!=def.customAlphas[0] ||
           opts.customAlphas[1]!=def.customAlphas[1]) {
            QString     shadeVal;
            QTextStream str(&shadeVal);
            if(0==opts.customAlphas[0])
                 str << 0;
            else
                for(int i=0; i<NUM_STD_ALPHAS; ++i)
                    if(0==i)
                        str << opts.customAlphas[i];
                    else
                        str << ',' << opts.customAlphas[i];
            CFG.writeEntry("customAlphas", shadeVal);
        }
        else
            CFG.deleteEntry("customAlphas");

        // Removed from 1.5 onwards...
        CFG.deleteEntry("colorTitlebarOnly");
        CFG.deleteEntry("titlebarBorder");
        CFG.deleteEntry("titlebarBlend");
        // Removed from 1.4 onwards..
        CFG.deleteEntry("squareLvSelection");
        CFG.deleteEntry("squareScrollViews");
        CFG.deleteEntry("squareProgress");
        CFG.deleteEntry("squareEntry");

        cfg->sync();
        return true;
    }
    return false;
}
#endif
