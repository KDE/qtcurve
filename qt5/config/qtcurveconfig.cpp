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
// config
#include "config.h"

// local
#include "qtcurveconfig.h"
#include "imagepropertiesdialog.h"
// TODO
#ifdef QTC_QT4_STYLE_SUPPORT
#  include "exportthemedialog.h"
#endif

// other qt5
#include <kwinconfig/qtcurvekwinconfig.h>
#include <style/qtcurve.h>

// common
#include <common/kf5_utils.h>
#include <common/config_file.h>

// libs
#include <qtcurve-utils/dirs.h>
#include <qtcurve-utils/process.h>
#include <qtcurve-utils/qtutils.h>

// Qt
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QFileInfo>
#include <QBoxLayout>
#include <QTreeWidget>
#include <QPainter>
#include <QSettings>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QTextStream>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QStyleFactory>
#include <QCloseEvent>
#include <QRegExp>
#include <QRegExpValidator>
#include <QMenu>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QStatusBar>

// KDE
#include <klocalizedstring.h>
#include <kactioncollection.h>
#include <kguiitem.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kcharselect.h>
#include <kstandardaction.h>
#include <ktoolbar.h>
#include <kzip.h>
#include <kmimetype.h>
#include <kcolorscheme.h>
#include <ksharedconfig.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kaboutdata.h>

// KDE4 support
#include <KDE/KGlobal>
#include <KDE/KStandardDirs>
#include <KDE/KInputDialog>

// system
#include <mutex>
#include <initializer_list>

#define EXTENSION ".qtcurve"
#define VERSION_WITH_KWIN_SETTINGS qtcMakeVersion(1, 5)

#define THEME_IMAGE_PREFIX "style"
#define BGND_FILE "-bgnd"
#define IMAGE_FILE "-img"
#define MENU_FILE  "-menu"

extern "C" Q_DECL_EXPORT QWidget*
allocate_kstyle_config(QWidget* parent)
{
    return new QtCurveConfig(parent);
}

static QString getExt(const QString &file)
{
    int dotPos=file.lastIndexOf('.');

    return dotPos!=-1 ? file.mid(dotPos) : QString();
}

static inline QString getFileName(const QString &f)
{
    return QFileInfo(f).fileName();
}

static QString getThemeFile(const QString &file)
{
    if (file.startsWith(THEME_IMAGE_PREFIX BGND_FILE)) {
        QString f(QtCurve::confDir() + file);

        if (QFile::exists(f)) {
            return f.replace("//", "/");
        }
    }
    if (!file.startsWith("/")) {
        QString f(QtCurve::qtcSaveDir() + file);
        if (QFile::exists(f)) {
            return f.replace("//", "/");
        }
    }
    return QString(file).replace("//", "/");
}

static void removeFile(const QString &f)
{
    if(QFile::exists(f))
        QFile::remove(f);
}

static void copyFile(const QString &src, const QString &dest)
{
    if(QFile::exists(src))
    {
        // QFile::copy will not overwrite existing files. If destination exists, it needs to be removed first.
        removeFile(dest);
        QFile::copy(src, dest);
    }
}

static QString installThemeFile(const QString &src, const QString &dest)
{
    QString source(getThemeFile(src)),
            name(QLatin1String(THEME_IMAGE_PREFIX)+dest+getExt(source)),
            destination(QtCurve::confDir()+name);

//     printf("INST THM \"%s\" \"%s\"", source.toLatin1().constData(), destination.toLatin1().constData());
    if(source!=destination)
        copyFile(source, destination);

    return name;
}

static QString saveThemeFile(const QString &src, const QString &dest, const QString &themeName)
{
    QString source(getThemeFile(src));
    QString destination =
        QtCurve::qtcSaveDir() + themeName + dest + getExt(source);

    // printf("SAVE THM \"%s\" \"%s\"", source.toLatin1().constData(),
    //        destination.toLatin1().constData());
    if (source != destination)
        copyFile(source, destination);

    return destination;
}

static void removeInstalledThemeFile(const QString &file)
{
    removeFile(QtCurve::confDir()+QLatin1String(THEME_IMAGE_PREFIX)+file);
}

static void
removeThemeImages(const QString &themeFile)
{
    QString themeName(getFileName(themeFile).remove(EXTENSION).replace(' ', '_'));
    QDir dir(QtCurve::qtcSaveDir());
    foreach (const QString &file, dir.entryList()) {
        if (file.startsWith(themeName + BGND_FILE)) {
            QFile::remove(dir.path() + "/" + file);
        }
    }
}

static void setStyleRecursive(QWidget *w, QStyle *s)
{
    if (!w) {
        return;
    }
    w->setStyle(s);
    foreach (QObject *child, w->children()) {
        if (child && child->isWidgetType()) {
            setStyleRecursive((QWidget*)child, s);
        }
    }
}

static const KStandardAction::StandardAction standardAction[] =
{
    KStandardAction::New, KStandardAction::Open, KStandardAction::OpenRecent, KStandardAction::Save, KStandardAction::SaveAs, KStandardAction::Revert, KStandardAction::Close, KStandardAction::Quit,
    KStandardAction::Cut, KStandardAction::Copy, KStandardAction::Paste,
    KStandardAction::ActionNone
};

static QString toString(const QSet<QString> &set)
{
    QStringList list=set.toList();

    qSort(list);
    return list.join(", ");
}

static QSet<QString> toSet(const QString &str)
{
    QStringList           list=str.simplified().split(QRegExp("\\s*,\\s*"), QString::SkipEmptyParts);
    QStringList::Iterator it(list.begin()),
                          end(list.end());

    for(; it!=end; ++it)
        (*it)=(*it).simplified();

    return QSet<QString>::fromList(list);
}

CStylePreview::CStylePreview(QWidget *parent)
             : KXmlGuiWindow(parent)
{
    aboutData = new KAboutData("QtCurve", i18n("QtCurve"), qtcVersion(),
                               i18n("Unified widget style."),
                               KAboutLicense::LicenseKey::LGPL,
                               i18n("(C) Craig Drummond, 2003-2011 & "
                                    "Yichao Yu, 2013-2015"));
    setWindowIcon(QIcon::fromTheme("preferences-desktop-theme"));

    QWidget *main = new QWidget(this);
    setObjectName("QtCurvePreview");
    setupUi(main);
    setCentralWidget(main);
    for (uint i = 0; standardAction[i] != KStandardAction::ActionNone; ++i)
        actionCollection()->addAction(standardAction[i]);
    createGUI();
    statusBar()->setSizeGripEnabled(true);
    toolBar()->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setCaption(i18n("Preview Window"));
}

CStylePreview::~CStylePreview()
{
    delete aboutData;
}

void CStylePreview::closeEvent(QCloseEvent *e)
{
    emit closePressed();
    e->ignore();
}

QSize CStylePreview::sizeHint() const
{
    return QSize(500, 260);
}

class CWorkspace : public QMdiArea
{
    public:

    CWorkspace(QWidget *parent) : QMdiArea(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    QSize sizeHint() const
    {
        return QSize(200, 200);
    }

    void paintEvent(QPaintEvent *)
    {
        QPainter p(viewport());
        p.fillRect(rect(), palette().color(backgroundRole()).dark(110));
    }
};

class CharSelectDialog : public QDialog {
public:
    CharSelectDialog(QWidget *parent, int v)
        : QDialog(parent)
    {
        if (QWidget *win = window()) {
            win->setWindowTitle(i18n("Select Password Character"));
        }
        setModal(true);
        auto mainLayout = new QVBoxLayout(this);
        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                              QDialogButtonBox::Cancel);
        auto page = new QFrame(this);
        auto okButton = buttonBox->button(QDialogButtonBox::Ok);
        okButton->setDefault(true);
        okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
        connect(qtcSlot(buttonBox, accepted), qtcSlot(this, accept));
        connect(qtcSlot(buttonBox, rejected), qtcSlot(this, reject));

        QBoxLayout *layout = new QBoxLayout(QBoxLayout::TopToBottom, page);
        layout->setMargin(0);
        layout->setSpacing(QApplication::style()
                           ->pixelMetric(QStyle::PM_DefaultLayoutSpacing));

        m_selector = new KCharSelect(page, nullptr);
        m_selector->setCurrentChar(QChar(v));
        layout->addWidget(m_selector);

        mainLayout->addWidget(page);
        mainLayout->addWidget(buttonBox);
    }

    int
    currentChar() const
    {
        return m_selector->currentChar().unicode();
    }
private:
    KCharSelect *m_selector;
};

class CGradItem : public QTreeWidgetItem {
public:
    CGradItem(QTreeWidget *p, const QStringList &vals)
        : QTreeWidgetItem(p, vals)
    {
        setFlags(flags()|Qt::ItemIsEditable);
    }

    bool
    operator<(const QTreeWidgetItem &i) const
    {
        return (text(0).toDouble() < i.text(0).toDouble() ||
                (qtcEqual(text(0).toDouble(), i.text(0).toDouble()) &&
                 (text(1).toDouble()<i.text(1).toDouble() ||
                  (qtcEqual(text(1).toDouble(), i.text(1).toDouble()) &&
                   (text(2).toDouble()<i.text(2).toDouble())))));
    }
};

static QStringList
toList(const QString &str)
{
    QStringList lst;
    lst.append(str);
    return lst;
}

class CStackItem : public QTreeWidgetItem {
public:
    CStackItem(QTreeWidget *p, const QString &text, int s)
        : QTreeWidgetItem(p, toList(text)),
          stackId(s)
    {
        if (s == 0) {
            QFont fnt(font(0));

            fnt.setBold(true);
            setFont(0, fnt);
        }
        setTextAlignment(0, Qt::AlignRight);
    }

    bool
    operator<(const QTreeWidgetItem &o) const
    {
        return stackId < ((CStackItem&)o).stackId;
    }
    int
    stack()
    {
        return stackId;
    }
private:
    int stackId;
};

CGradientPreview::CGradientPreview(QtCurveConfig *c, QWidget *p)
                : QWidget(p),
                  cfg(c),
                  style(0L)
{
    // setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    setObjectName("QtCurveConfigDialog-GradientPreview");
}

CGradientPreview::~CGradientPreview()
{
    delete style;
}

QSize CGradientPreview::sizeHint() const
{
    return QSize(64, 24);
}

QSize CGradientPreview::minimumSizeHint() const
{
    return sizeHint();
}

void CGradientPreview::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    if(!style)
        style=QStyleFactory::create("qtcurve");

    if(style)
    {
        QtCurve::Style::PreviewOption styleOpt;

        styleOpt.init(this);

        cfg->setOptions(styleOpt.opts);
        styleOpt.opts.appearance=APPEARANCE_CUSTOM1;
        styleOpt.opts.customGradient[APPEARANCE_CUSTOM1]=grad;
        styleOpt.palette.setColor(QPalette::Button, color);
        styleOpt.state|=QStyle::State_Raised;
        style->drawControl((QStyle::ControlElement)QtCurve::Style::CE_QtC_Preview, &styleOpt, &p, this);
    }
    p.end();
}

void CGradientPreview::setGrad(const Gradient &g)
{
    grad=g;
    repaint();
}

void CGradientPreview::setColor(const QColor &col)
{
    if(col!=color)
    {
        color=col;
        repaint();
    }
}

static QString readEnvPath(const char *env)
{
   const char *path=getenv(env);

   return path ? QFile::decodeName(path) : QString::null;
}

static QString
kdeHome(bool kde3)
{
    static QString kdeHome[2];

    // Execute kde-config to ascertain users KDEHOME
    if (kdeHome[kde3 ? 0 : 1].isEmpty()) {
        size_t len = 0;
        const char *kde_config = kde3 ? "kde-config" : "kde4-config";
        const char *const argv[] = {kde_config, "--localprefix", nullptr};
        char *res = qtcPopenStdout(kde_config, argv, 300, &len);
        if (res) {
            res[len] = '\0';
            kdeHome[kde3 ? 0 : 1] = QFile::decodeName(res).replace("\n", "");
            free(res);
        }
    }

    // Try env vars...
    if (kdeHome[kde3 ? 0 : 1].isEmpty()) {
        kdeHome[kde3 ? 0 : 1] = readEnvPath(getuid() ? "KDEHOME" : "KDEROOTHOME");
        if (kdeHome[kde3 ? 0 : 1].isEmpty()) {
            QDir homeDir(QDir::homePath());
            QString kdeConfDir("/.kde");
            if (!kde3 && homeDir.exists(".kde4"))
                kdeConfDir = QString("/.kde4");
            kdeHome[kde3 ? 0 : 1] = QDir::homePath() + kdeConfDir;
        }
    }
    return kdeHome[kde3 ? 0 : 1];
}

static int toInt(const QString &str)
{
    return str.length()>1 ? str[0].unicode() : 0;
}

static int getHideFlags(const QCheckBox *kbd, const QCheckBox *kwin)
{
    return (kbd->isChecked() ? HIDE_KEYBOARD : HIDE_NONE)+
           (kwin->isChecked() ? HIDE_KWIN : HIDE_NONE);
}

enum ShadeWidget
{
    SW_MENUBAR,
    SW_SLIDER,
    SW_CHECK_RADIO,
    SW_MENU_STRIPE,
    SW_COMBO,
    SW_LV_HEADER,
    SW_CR_BGND,
    SW_PROGRESS
};

static QString uiString(EShade shade, ShadeWidget sw)
{
    switch(shade)
    {
        case SHADE_NONE:
            switch(sw)
            {
                case SW_MENUBAR:
                case SW_PROGRESS:
                    return i18n("Background");
                case SW_COMBO:
                case SW_SLIDER:
                    return i18n("Button");
                case SW_CHECK_RADIO:
                    return i18n("Text");
                case SW_CR_BGND:
                case SW_LV_HEADER:
                case SW_MENU_STRIPE:
                    return i18n("None");
            }
        default:
            return i18n("<unknown>");
        case SHADE_CUSTOM:
            return i18n("Custom:");
        case SHADE_SELECTED:
            return i18n("Selected background");
        case SHADE_BLEND_SELECTED:
            return i18n("Blended selected background");
        case SHADE_DARKEN:
            return SW_MENU_STRIPE==sw ? i18n("Menu background") : i18n("Darken");
        case SHADE_WINDOW_BORDER:
            return i18n("Titlebar");
    }
}

static void insertShadeEntries(QComboBox *combo, ShadeWidget sw)
{
    combo->insertItem(SHADE_NONE, uiString(SHADE_NONE, sw));
    combo->insertItem(SHADE_CUSTOM, uiString(SHADE_CUSTOM, sw));
    combo->insertItem(SHADE_SELECTED, uiString(SHADE_SELECTED, sw));
    if(SW_CHECK_RADIO!=sw) // For check/radio, we dont blend, and dont allow darken
    {
        combo->insertItem(SHADE_BLEND_SELECTED, uiString(SHADE_BLEND_SELECTED, sw));
        if(SW_PROGRESS!=sw)
            combo->insertItem(SHADE_DARKEN, uiString(SHADE_DARKEN, sw));
    }
    if(SW_MENUBAR==sw)
        combo->insertItem(SHADE_WINDOW_BORDER, uiString(SHADE_WINDOW_BORDER, sw));
}

static QString uiString(EAppearance app, EAppAllow allow=APP_ALLOW_BASIC, bool sameAsApp=false)
{
    if(app>=APPEARANCE_CUSTOM1 && app<(APPEARANCE_CUSTOM1+NUM_CUSTOM_GRAD))
        return i18n("Custom gradient %1", (app-APPEARANCE_CUSTOM1)+1);

    switch(app)
    {
        case APPEARANCE_FLAT: return i18n("Flat");
        case APPEARANCE_RAISED: return i18n("Raised");
        case APPEARANCE_DULL_GLASS: return i18n("Dull glass");
        case APPEARANCE_SHINY_GLASS: return i18n("Shiny glass");
        case APPEARANCE_AGUA: return i18n("Agua");
        case APPEARANCE_SOFT_GRADIENT: return i18n("Soft gradient");
        case APPEARANCE_GRADIENT: return i18n("Standard gradient");
        case APPEARANCE_HARSH_GRADIENT: return i18n("Harsh gradient");
        case APPEARANCE_INVERTED: return i18n("Inverted gradient");
        case APPEARANCE_DARK_INVERTED: return i18n("Dark inverted gradient");
        case APPEARANCE_SPLIT_GRADIENT: return i18n("Split gradient");
        case APPEARANCE_BEVELLED: return i18n("Bevelled");
        case APPEARANCE_FADE:
            switch(allow)
            {
                case APP_ALLOW_FADE:
                    return i18n("Fade out (popup menuitems)");
                case APP_ALLOW_STRIPED:
                    return i18n("Striped");
                default:
                case APP_ALLOW_NONE:
                    return sameAsApp ? i18n("Same as general setting") : i18n("None");
            }
        case APPEARANCE_FILE:
            return i18n("Tiled image");
        default:
            return i18n("<unknown>");
    }
}

static void insertAppearanceEntries(QComboBox *combo, EAppAllow allow=APP_ALLOW_BASIC, bool sameAsApp=false)
{
    int max=APP_ALLOW_BASIC==allow
                ? APPEARANCE_FADE
                : APP_ALLOW_STRIPED==allow
                    ? APPEARANCE_FADE+2
                    : APPEARANCE_FADE+1;

    for(int i=APPEARANCE_CUSTOM1; i<max; ++i)
        combo->insertItem(i, uiString((EAppearance)i, allow, sameAsApp));
}

static void insertLineEntries(QComboBox *combo,  bool singleDot, bool dashes)
{
    combo->insertItem(LINE_NONE, i18n("None"));
    combo->insertItem(LINE_SUNKEN, i18n("Sunken lines"));
    combo->insertItem(LINE_FLAT, i18n("Flat lines"));
    combo->insertItem(LINE_DOTS, i18n("Dots"));
    if(singleDot)
    {
        combo->insertItem(LINE_1DOT, i18n("Single dot"));
        if(dashes)
            combo->insertItem(LINE_DASHES, i18n("Dashes"));
    }
}

static void insertDefBtnEntries(QComboBox *combo)
{
    combo->insertItem(IND_CORNER, i18n("Corner indicator"));
    combo->insertItem(IND_FONT_COLOR, i18n("Font color thin border"));
    combo->insertItem(IND_COLORED, i18n("Selected background thick border"));
    combo->insertItem(IND_TINT, i18n("Selected background tinting"));
    combo->insertItem(IND_GLOW, i18n("A slight glow"));
    combo->insertItem(IND_DARKEN, i18n("Darken"));
    combo->insertItem(IND_SELECTED, i18n("Use selected background color"));
    combo->insertItem(IND_NONE, i18n("No indicator"));
}

static void insertScrollbarEntries(QComboBox *combo)
{
    combo->insertItem(SCROLLBAR_KDE, i18n("KDE"));
    combo->insertItem(SCROLLBAR_WINDOWS, i18n("MS Windows"));
    combo->insertItem(SCROLLBAR_PLATINUM, i18n("Platinum"));
    combo->insertItem(SCROLLBAR_NEXT, i18n("NeXT"));
    combo->insertItem(SCROLLBAR_NONE, i18n("No buttons"));
}

static void insertRoundEntries(QComboBox *combo)
{
    combo->insertItem(ROUND_NONE, i18n("Square"));
    combo->insertItem(ROUND_SLIGHT, i18n("Slightly rounded"));
    combo->insertItem(ROUND_FULL, i18n("Fully rounded"));
    combo->insertItem(ROUND_EXTRA, i18n("Extra rounded"));
    combo->insertItem(ROUND_MAX, i18n("Max rounded"));
}

static void insertMouseOverEntries(QComboBox *combo)
{
    combo->insertItem(MO_NONE, i18n("No coloration"));
    combo->insertItem(MO_COLORED, i18n("Color border"));
    combo->insertItem(MO_COLORED_THICK, i18n("Thick color border"));
    combo->insertItem(MO_PLASTIK, i18n("Plastik style"));
    combo->insertItem(MO_GLOW, i18n("Glow"));
}

static void insertToolbarBorderEntries(QComboBox *combo)
{
    combo->insertItem(TB_NONE, i18n("None"));
    combo->insertItem(TB_LIGHT, i18n("Light"));
    combo->insertItem(TB_DARK, i18n("Dark"));
    combo->insertItem(TB_LIGHT_ALL, i18n("Light (all sides)"));
    combo->insertItem(TB_DARK_ALL, i18n("Dark (all sides)"));
}

static void insertEffectEntries(QComboBox *combo, bool sameAsApp=false)
{
    combo->insertItem(EFFECT_NONE, sameAsApp ? i18n("Same as general setting") : i18n("Plain"));
    combo->insertItem(EFFECT_ETCH, i18n("Etched"));
    combo->insertItem(EFFECT_SHADOW, i18n("Shadowed"));
}

static void insertShadingEntries(QComboBox *combo)
{
    combo->insertItem(int(Shading::Simple), i18n("Simple"));
    combo->insertItem(int(Shading::HSL), i18n("Use HSL color space"));
    combo->insertItem(int(Shading::HSV), i18n("Use HSV color space"));
    combo->insertItem(int(Shading::HCY), i18n("Use HCY color space"));
}

static void insertStripeEntries(QComboBox *combo)
{
    combo->insertItem(STRIPE_NONE, i18n("Plain"));
    combo->insertItem(STRIPE_PLAIN, i18n("Stripes"));
    combo->insertItem(STRIPE_DIAGONAL, i18n("Diagonal stripes"));
    combo->insertItem(STRIPE_FADE, i18n("Faded stripes"));
}

static void insertSliderStyleEntries(QComboBox *combo)
{
    combo->insertItem(SLIDER_PLAIN, i18n("Plain"));
    combo->insertItem(SLIDER_ROUND, i18n("Round"));
    combo->insertItem(SLIDER_PLAIN_ROTATED, i18n("Plain - rotated"));
    combo->insertItem(SLIDER_ROUND_ROTATED, i18n("Round - rotated"));
    combo->insertItem(SLIDER_TRIANGULAR, i18n("Triangular"));
    combo->insertItem(SLIDER_CIRCULAR, i18n("Circular"));
}

static void insertEColorEntries(QComboBox *combo)
{
    combo->insertItem(ECOLOR_BASE, i18n("Base color"));
    combo->insertItem(ECOLOR_BACKGROUND, i18n("Background color"));
    combo->insertItem(ECOLOR_DARK, i18n("Darkened background color"));
}

static void insertFocusEntries(QComboBox *combo)
{
    combo->insertItem(FOCUS_STANDARD, i18n("Standard (dotted)"));
    combo->insertItem(FOCUS_RECTANGLE, i18n("Highlight color"));
    combo->insertItem(FOCUS_FULL, i18n("Highlight color (full size)"));
    combo->insertItem(FOCUS_FILLED, i18n("Highlight color, and fill"));
    combo->insertItem(FOCUS_LINE, i18n("Line drawn with highlight color"));
    combo->insertItem(FOCUS_GLOW, i18n("Glow"));
    combo->insertItem(FOCUS_NONE, i18n("Nothing"));
}

static void insertGradBorderEntries(QComboBox *combo)
{
    combo->insertItem(GB_NONE, i18n("No border"));
    combo->insertItem(GB_LIGHT, i18n("Light border"));
    combo->insertItem(GB_3D, i18n("3D border (light only)"));
    combo->insertItem(GB_3D_FULL, i18n("3D border (dark and light)"));
    combo->insertItem(GB_SHINE, i18n("Shine"));
}

static void insertAlignEntries(QComboBox *combo)
{
    combo->insertItem(ALIGN_LEFT, i18n("Left"));
    combo->insertItem(ALIGN_CENTER, i18n("Center (between controls)"));
    combo->insertItem(ALIGN_FULL_CENTER, i18n("Center (full width)"));
    combo->insertItem(ALIGN_RIGHT, i18n("Right"));
}

enum ETitleBarButtonColoration
{
    TITLE_BTN_COL_BACKGROUND,
    TITLE_BTN_COL_BUTTON,
    TITLE_BTN_COL_CUSTOM
};

static void insertTitlebarIconEntries(QComboBox *combo)
{
    combo->insertItem(TITLEBAR_ICON_NONE, i18n("Do not show"));
    combo->insertItem(TITLEBAR_ICON_MENU_BUTTON, i18n("Place on menu button"));
    combo->insertItem(TITLEBAR_ICON_NEXT_TO_TITLE, i18n("Place next to title"));
}

static void insertTabMoEntries(QComboBox *combo)
{
    combo->insertItem(TAB_MO_TOP, i18n("Highlight on top"));
    combo->insertItem(TAB_MO_BOTTOM, i18n("Highlight on bottom"));
    combo->insertItem(TAB_MO_GLOW, i18n("Add a slight glow"));
}

static void insertGradTypeEntries(QComboBox *combo)
{
    combo->insertItem(GT_HORIZ, i18n("Top to bottom"));
    combo->insertItem(GT_VERT, i18n("Left to right"));
}

#if 0
static void insertLvLinesEntries(QComboBox *combo)
{
    combo->insertItem(LV_NONE, i18n("None"));
    combo->insertItem(LV_NEW, i18n("New style (KDE and Gtk2 similar)"));
    combo->insertItem(LV_OLD, i18n("Old style (KDE and Gtk2 different)"));
}
#endif

static void insertImageEntries(QComboBox *combo)
{
    combo->insertItem(IMG_NONE, i18n("None"));
    combo->insertItem(IMG_BORDERED_RINGS, i18n("Bordered rings"));
    combo->insertItem(IMG_PLAIN_RINGS, i18n("Plain rings"));
    combo->insertItem(IMG_SQUARE_RINGS, i18n("Square rings"));
    combo->insertItem(IMG_FILE, i18n("File"));
}

static void insertGlowEntries(QComboBox *combo)
{
    combo->insertItem(GLOW_NONE, i18n("No glow"));
    combo->insertItem(GLOW_START, i18n("Add glow at the start"));
    combo->insertItem(GLOW_MIDDLE, i18n("Add glow in the middle"));
    combo->insertItem(GLOW_END, i18n("Add glow at the end"));
}

static void insertCrSizeEntries(QComboBox *combo)
{
    combo->insertItem(0, i18n("Small (%1 pixels)", CR_SMALL_SIZE));
    combo->insertItem(1, i18n("Large (%1 pixels)", CR_LARGE_SIZE));
}

static void setCrSize(QComboBox *combo, int size)
{
    combo->setCurrentIndex(CR_SMALL_SIZE==size ? 0 : 1);
}

static int getCrSize(QComboBox *combo)
{
    return 0==combo->currentIndex() ? CR_SMALL_SIZE : CR_LARGE_SIZE;
}

static void insertDragEntries(QComboBox *combo)
{
    combo->insertItem(WM_DRAG_NONE, i18n("Titlebar only"));
    combo->insertItem(WM_DRAG_MENUBAR, i18n("Titlebar and menubar"));
    combo->insertItem(WM_DRAG_MENU_AND_TOOLBAR, i18n("Titlebar, menubar, and toolbars"));
    combo->insertItem(WM_DRAG_ALL, i18n("All empty areas"));
}


static void insertFrameEntries(QComboBox *combo)
{
    combo->insertItem(FRAME_NONE, i18n("No border"));
    combo->insertItem(FRAME_PLAIN, i18n("Standard frame border"));
    combo->insertItem(FRAME_LINE, i18n("Single separator line"));
    combo->insertItem(FRAME_SHADED, i18n("Shaded background"));
    combo->insertItem(FRAME_FADED, i18n("Faded background"));
}

enum EGBLabelVPos
{
    GBV_OUTSIDE,
    GBV_STANDARD,
    GBV_INSIDE
};

static void insertGbLabelEntries(QComboBox *combo)
{
    combo->insertItem(GBV_OUTSIDE, i18n("Outside frame"));
    combo->insertItem(GBV_STANDARD, i18n("On frame"));
    combo->insertItem(GBV_INSIDE, i18n("Inside frame"));
}

static void insertTBarBtnEntries(QComboBox *combo)
{
    combo->insertItem(TBTN_STANDARD, i18n("Standard (auto-raise)"));
    combo->insertItem(TBTN_RAISED, i18n("Raised"));
    combo->insertItem(TBTN_JOINED, i18n("Raised and joined"));
}

QtCurveConfig::QtCurveConfig(QWidget *parent)
             : QWidget(parent),
               workSpace(nullptr),
               stylePreview(nullptr),
               mdiWindow(nullptr),
#ifdef QTC_QT4_STYLE_SUPPORT
               exportDialog(nullptr),
#endif
               gradPreview(nullptr),
               readyForPreview(false)
{
    setupUi(this);
    setObjectName("QtCurveConfigDialog");
    titleLabel->setText(QString("QtCurve %1 - (C) Craig Drummond, 2003-2010 & "
                                "Yichao Yu, 2013-2015").arg(qtcVersion()));
    insertShadeEntries(shadeSliders, SW_SLIDER);
    insertShadeEntries(shadeMenubars, SW_MENUBAR);
    insertShadeEntries(shadeCheckRadio, SW_CHECK_RADIO);
    insertShadeEntries(menuStripe, SW_MENU_STRIPE);
    insertShadeEntries(comboBtn, SW_COMBO);
    insertShadeEntries(sortedLv, SW_LV_HEADER);
    insertShadeEntries(crColor, SW_CR_BGND);
    insertShadeEntries(progressColor, SW_PROGRESS);
    insertAppearanceEntries(appearance);
    insertAppearanceEntries(menubarAppearance);
    insertAppearanceEntries(toolbarAppearance);
    insertAppearanceEntries(lvAppearance);
    insertAppearanceEntries(sliderAppearance);
    insertAppearanceEntries(tabAppearance);
    insertAppearanceEntries(activeTabAppearance);
    insertAppearanceEntries(progressAppearance);
    insertAppearanceEntries(progressGrooveAppearance);
    insertAppearanceEntries(grooveAppearance);
    insertAppearanceEntries(sunkenAppearance);
    insertAppearanceEntries(menuitemAppearance, APP_ALLOW_FADE);
    insertAppearanceEntries(menuBgndAppearance, APP_ALLOW_STRIPED);
    insertAppearanceEntries(titlebarAppearance, APP_ALLOW_NONE);
    insertAppearanceEntries(inactiveTitlebarAppearance, APP_ALLOW_NONE);
    insertAppearanceEntries(titlebarButtonAppearance);
    insertAppearanceEntries(selectionAppearance);
    insertAppearanceEntries(menuStripeAppearance);
    insertAppearanceEntries(sbarBgndAppearance);
    insertAppearanceEntries(sliderFill);
    insertAppearanceEntries(bgndAppearance, APP_ALLOW_STRIPED);
    insertAppearanceEntries(dwtAppearance);
    insertAppearanceEntries(tooltipAppearance);
    insertAppearanceEntries(tbarBtnAppearance, APP_ALLOW_NONE, true);
    insertLineEntries(handles, true, true);
    insertLineEntries(sliderThumbs, true, false);
    insertLineEntries(toolbarSeparators, false, false);
    insertLineEntries(splitters, true, true);
    insertDefBtnEntries(defBtnIndicator);
    insertScrollbarEntries(scrollbarType);
    insertRoundEntries(round);
    insertMouseOverEntries(coloredMouseOver);
    insertToolbarBorderEntries(toolbarBorders);
    insertEffectEntries(buttonEffect);
    insertEffectEntries(tbarBtnEffect, true);
    insertShadingEntries(shading);
    insertStripeEntries(stripedProgress);
    insertSliderStyleEntries(sliderStyle);
    insertEColorEntries(progressGrooveColor);
    insertFocusEntries(focus);
    insertGradBorderEntries(gradBorder);
    insertAlignEntries(titlebarAlignment);
    insertEffectEntries(titlebarEffect);
    insertTitlebarIconEntries(titlebarIcon);
    insertTabMoEntries(tabMouseOver);
    insertGradTypeEntries(bgndGrad);
    insertGradTypeEntries(menuBgndGrad);
    //insertLvLinesEntries(lvLines);
    insertImageEntries(bgndImage);
    insertImageEntries(menuBgndImage);
    insertGlowEntries(glowProgress);
    insertCrSizeEntries(crSize);
    insertDragEntries(windowDrag);
    insertFrameEntries(groupBox);
    insertGbLabelEntries(gbLabel_textPos);
    insertTBarBtnEntries(tbarBtns);

    highlightFactor->setRange(MIN_HIGHLIGHT_FACTOR, MAX_HIGHLIGHT_FACTOR);
    highlightFactor->setValue(DEFAULT_HIGHLIGHT_FACTOR);

    crHighlight->setRange(MIN_HIGHLIGHT_FACTOR, MAX_HIGHLIGHT_FACTOR);
    crHighlight->setValue(DEFAULT_CR_HIGHLIGHT_FACTOR);

    splitterHighlight->setRange(MIN_HIGHLIGHT_FACTOR, MAX_HIGHLIGHT_FACTOR);
    splitterHighlight->setValue(DEFAULT_SPLITTER_HIGHLIGHT_FACTOR);

    lighterPopupMenuBgnd->setRange(MIN_LIGHTER_POPUP_MENU, MAX_LIGHTER_POPUP_MENU);
    lighterPopupMenuBgnd->setValue(DEF_POPUPMENU_LIGHT_FACTOR);

    expanderHighlight->setRange(MIN_HIGHLIGHT_FACTOR, MAX_HIGHLIGHT_FACTOR);
    expanderHighlight->setValue(DEFAULT_EXPANDER_HIGHLIGHT_FACTOR);

    menuDelay->setRange(MIN_MENU_DELAY, MAX_MENU_DELAY);
    menuDelay->setValue(DEFAULT_MENU_DELAY);

    gbFactor->setRange(MIN_GB_FACTOR, MAX_GB_FACTOR);
    gbFactor->setValue(DEF_GB_FACTOR);

    bgndOpacity->setRange(0, 100);
    bgndOpacity->setSingleStep(5);
    bgndOpacity->setValue(100);
    dlgOpacity->setRange(0, 100);
    dlgOpacity->setSingleStep(5);
    dlgOpacity->setValue(100);
    menuBgndOpacity->setRange(0, 100);
    menuBgndOpacity->setSingleStep(5);
    menuBgndOpacity->setValue(100);

    sliderWidth->setRange(MIN_SLIDER_WIDTH, MAX_SLIDER_WIDTH);
    sliderWidth->setSingleStep(2);
    sliderWidth->setValue(DEFAULT_SLIDER_WIDTH);
    sliderWidth->setSuffix(i18n(" pixels"));

    tabBgnd->setRange(MIN_TAB_BGND, MAX_TAB_BGND);
    tabBgnd->setValue(DEF_TAB_BGND);

    colorSelTab->setRange(MIN_COLOR_SEL_TAB_FACTOR, MAX_COLOR_SEL_TAB_FACTOR);
    colorSelTab->setValue(DEF_COLOR_SEL_TAB_FACTOR);

    stopPosition->setValue(0);
    stopValue->setValue(100);
    stopAlpha->setValue(100);

    bgndPixmapDlg=new CImagePropertiesDialog(i18n("Background"), this, CImagePropertiesDialog::BASIC);
    menuBgndPixmapDlg=new CImagePropertiesDialog(i18n("Menu Background"), this, CImagePropertiesDialog::BASIC);
    bgndImageDlg=new CImagePropertiesDialog(i18n("Background Image"), this,
                                            CImagePropertiesDialog::POS|
                                            CImagePropertiesDialog::SCALE|
                                            CImagePropertiesDialog::BORDER);
    menuBgndImageDlg=new CImagePropertiesDialog(i18n("Menu Image"), this,
                                                CImagePropertiesDialog::POS|
                                                CImagePropertiesDialog::SCALE);

    for (auto *w: {lighterPopupMenuBgnd, tabBgnd, menuDelay, crHighlight,
                expanderHighlight, colorSelTab, highlightFactor, bgndOpacity,
                dlgOpacity, menuBgndOpacity, splitterHighlight, gbFactor}) {
        connect(qtcSlot(w, valueChanged, (int)), qtcSlot(this, updateChanged));
    }

    for (auto *w: {menuStripeAppearance, bgndGrad, menuBgndGrad,
                toolbarBorders, handles, appearance, menubarAppearance,
                toolbarAppearance, lvAppearance, sliderAppearance,
                tabAppearance, toolbarSeparators, splitters, sliderStyle,
                glowProgress, crSize, progressAppearance,
                progressGrooveAppearance, grooveAppearance, sunkenAppearance,
                progressGrooveColor, menuitemAppearance, titlebarAppearance,
                inactiveTitlebarAppearance, titlebarButtonAppearance,
                selectionAppearance, scrollbarType, windowDrag,
                sbarBgndAppearance, sliderFill, dwtAppearance,
                tooltipAppearance, gbLabel_textPos, titlebarAlignment,
                titlebarEffect, titlebarIcon, tbarBtns, tbarBtnAppearance,
                tbarBtnEffect}) {
        connect(qtcSlot(w, currentIndexChanged, (int)),
                qtcSlot(this, updateChanged));
    }

    for (auto *w: std::initializer_list<QAbstractButton*>{
            animatedProgress, highlightTab, fillSlider, stripedSbar,
                roundMbTopOnly, statusbarHiding_keyboard, statusbarHiding_kwin,
                darkerBorders, comboSplitter, unifyCombo, vArrows, xCheck,
                crButton, roundAllTabs, borderTab, borderInactiveTab,
                invertBotTab, doubleGtkComboArrow, stdSidebarButtons,
                toolbarTabs, centerTabText, borderMenuitems, popupBorder,
                windowBorder_fill, windowBorder_separator, lvLines, lvButton,
                drawStatusBarFrames, menubarMouseOver,
                shadeMenubarOnlyWhenActive, thin_menuitems, thin_buttons,
                thin_frames, hideShortcutUnderline, gtkScrollViews,
                highlightScrollViews, etchEntry, flatSbarButtons,
                colorSliderMouseOver, windowBorder_addLightBorder,
                dwtBtnAsPerTitleBar, dwtColAsPerTitleBar,
                dwtIconColAsPerTitleBar, dwtFontAsPerTitleBar,
                dwtTextAsPerTitleBar, dwtEffectAsPerTitleBar, dwtRoundTopOnly,
                smallRadio, gtkComboMenus, mapKdeIcons, colorMenubarMouseOver,
                useHighlightForMenu, gbLabel_bold, gbLabel_centred, fadeLines,
                menuIcons, stdBtnSizes, forceAlternateLvCols, boldProgress,
                coloredTbarMo, borderSelection, squareEntry, squareLvSelection,
                squareScrollViews, squareFrame, squareTabFrame, squareSlider,
                squareScrollbarSlider, squareWindows, squareTooltips,
                squarePopupMenus, titlebarButtons_button,
                titlebarButtons_customIcon, titlebarButtons_noFrame,
                titlebarButtons_round, titlebarButtons_hoverFrame,
                titlebarButtons_hoverSymbol, titlebarButtons_hoverSymbolFull,
                titlebarButtons_sunkenBackground, titlebarButtons_arrowMinMax,
                titlebarButtons_colorOnMouseOver,
                titlebarButtons_colorSymbolsOnly, titlebarButtons_colorInactive,
                titlebarButtons_hideOnInactiveWindow}) {
        connect(qtcSlot(w, toggled), qtcSlot(this, updateChanged));
    }

    for (auto *w: {customComboBtnColor, customSortedLvColor,
                customProgressColor, customMenuStripeColor,
                customCheckRadioColor, customSlidersColor, customMenubarsColor,
                customMenuSelTextColor, customMenuNormTextColor,
                customCrBgndColor, titlebarButtons_colorClose,
                titlebarButtons_colorMin, titlebarButtons_colorMax,
                titlebarButtons_colorKeepAbove, titlebarButtons_colorKeepBelow,
                titlebarButtons_colorHelp, titlebarButtons_colorMenu,
                titlebarButtons_colorShade, titlebarButtons_colorAllDesktops,
                titlebarButtons_colorCloseIcon, titlebarButtons_colorMinIcon,
                titlebarButtons_colorMaxIcon,
                titlebarButtons_colorKeepAboveIcon,
                titlebarButtons_colorKeepBelowIcon,
                titlebarButtons_colorHelpIcon, titlebarButtons_colorMenuIcon,
                titlebarButtons_colorShadeIcon,
                titlebarButtons_colorAllDesktopsIcon,
                titlebarButtons_colorCloseInactiveIcon,
                titlebarButtons_colorMinInactiveIcon,
                titlebarButtons_colorMaxInactiveIcon,
                titlebarButtons_colorKeepAboveInactiveIcon,
                titlebarButtons_colorKeepBelowInactiveIcon,
                titlebarButtons_colorHelpInactiveIcon,
                titlebarButtons_colorMenuInactiveIcon,
                titlebarButtons_colorShadeInactiveIcon,
                titlebarButtons_colorAllDesktopsInactiveIcon}) {
        connect(qtcSlot(w, changed), qtcSlot(this, updateChanged));
    }

    for (auto *w: {noBgndGradientApps, noBgndOpacityApps,
                noMenuBgndOpacityApps, noBgndImageApps, useQtFileDialogApps,
                menubarApps, statusbarApps, noMenuStripeApps}) {
        connect(qtcSlot(w, editingFinished), qtcSlot(this, updateChanged));
    }

    connect(qtcSlot(sliderWidth, valueChanged, (int)),
            qtcSlot(this, sliderWidthChanged));
    connect(qtcSlot(menuStripe, currentIndexChanged, (int)),
            qtcSlot(this, menuStripeChanged));
    connect(qtcSlot(round, currentIndexChanged, (int)),
            qtcSlot(this, roundChanged));
    connect(qtcSlot(sliderThumbs, currentIndexChanged, (int)),
            qtcSlot(this, sliderThumbChanged));
    connect(qtcSlot(customMenuTextColor, toggled),
            qtcSlot(this, customMenuTextColorChanged));
    connect(qtcSlot(stripedProgress, currentIndexChanged, (int)),
            qtcSlot(this, stripedProgressChanged));
    connect(qtcSlot(embolden, toggled), qtcSlot(this, emboldenToggled));
    connect(qtcSlot(defBtnIndicator, currentIndexChanged, (int)),
            qtcSlot(this, defBtnIndicatorChanged));
    connect(qtcSlot(activeTabAppearance, currentIndexChanged, (int)),
            qtcSlot(this, activeTabAppearanceChanged));
    connect(qtcSlot(menubarHiding_keyboard, toggled),
            qtcSlot(this, menubarHidingChanged));
    connect(qtcSlot(menubarHiding_kwin, toggled),
            qtcSlot(this, menubarHidingChanged));
    connect(qtcSlot(comboBtn, currentIndexChanged, (int)),
            qtcSlot(this, comboBtnChanged));
    connect(qtcSlot(sortedLv, currentIndexChanged, (int)),
            qtcSlot(this, sortedLvChanged));
    connect(qtcSlot(unifySpinBtns, toggled),
            qtcSlot(this, unifySpinBtnsToggled));
    connect(qtcSlot(unifySpin, toggled), qtcSlot(this, unifySpinToggled));
    connect(qtcSlot(tabMouseOver, currentIndexChanged, (int)),
            qtcSlot(this, tabMoChanged));
    connect(qtcSlot(shadePopupMenu, toggled),
            qtcSlot(this, shadePopupMenuChanged));
    connect(qtcSlot(progressColor, currentIndexChanged, (int)),
            qtcSlot(this, progressColorChanged));
    connect(qtcSlot(menuBgndAppearance, currentIndexChanged, (int)),
            qtcSlot(this, menuBgndAppearanceChanged));
    connect(qtcSlot(windowBorder_colorTitlebarOnly, toggled),
            qtcSlot(this, windowBorder_colorTitlebarOnlyChanged));
    connect(qtcSlot(windowBorder_blend, toggled),
            qtcSlot(this, windowBorder_blendChanged));
    connect(qtcSlot(windowBorder_menuColor, toggled),
            qtcSlot(this, windowBorder_menuColorChanged));
    connect(qtcSlot(shadeCheckRadio, currentIndexChanged, (int)),
            qtcSlot(this, shadeCheckRadioChanged));
    connect(qtcSlot(focus, currentIndexChanged, (int)),
            qtcSlot(this, focusChanged));
    connect(qtcSlot(buttonEffect, currentIndexChanged, (int)),
            qtcSlot(this, buttonEffectChanged));
    connect(qtcSlot(coloredMouseOver, currentIndexChanged, (int)),
            qtcSlot(this, coloredMouseOverChanged));
    connect(qtcSlot(shadeSliders, currentIndexChanged, (int)),
            qtcSlot(this, shadeSlidersChanged));
    connect(qtcSlot(shadeMenubars, currentIndexChanged, (int)),
            qtcSlot(this, shadeMenubarsChanged));
    connect(qtcSlot(shading, currentIndexChanged, (int)),
            qtcSlot(this, shadingChanged));
    connect(qtcSlot(borderSbarGroove, toggled),
            qtcSlot(this, borderSbarGrooveChanged));
    connect(qtcSlot(thinSbarGroove, toggled),
            qtcSlot(this, thinSbarGrooveChanged));
    connect(qtcSlot(bgndAppearance, currentIndexChanged, (int)),
            qtcSlot(this, bgndAppearanceChanged));
    connect(qtcSlot(bgndImage, currentIndexChanged, (int)),
            qtcSlot(this, bgndImageChanged));

    // TODO
    connect(menuBgndImage, SIGNAL(currentIndexChanged(int)), SLOT(menuBgndImageChanged()));
    connect(crColor, SIGNAL(currentIndexChanged(int)), SLOT(crColorChanged()));
    connect(gtkButtonOrder, SIGNAL(toggled(bool)), SLOT(gtkButtonOrderChanged()));
    connect(reorderGtkButtons, SIGNAL(toggled(bool)), SLOT(reorderGtkButtonsChanged()));
    connect(passwordChar, SIGNAL(clicked()), SLOT(passwordCharClicked()));
    connect(groupBox, SIGNAL(currentIndexChanged(int)), SLOT(groupBoxChanged()));
    connect(titlebarButtons_custom, SIGNAL(toggled(bool)), SLOT(titlebarButtons_customChanged()));
    connect(titlebarButtons_useHover, SIGNAL(toggled(bool)), SLOT(titlebarButtons_useHoverChanged()));

    connect(borderProgress, SIGNAL(toggled(bool)), SLOT(borderProgressChanged()));
    connect(fillProgress, SIGNAL(toggled(bool)), SLOT(fillProgressChanged()));
    connect(squareProgress, SIGNAL(toggled(bool)), SLOT(squareProgressChanged()));

    menubarBlend->setIcon(QtCurve::loadKIcon("configure"));
    connect(menubarBlend, SIGNAL(clicked(bool)), SLOT(menubarTitlebarBlend()));
    connect(previewControlButton, SIGNAL(clicked(bool)), SLOT(previewControlPressed()));

    connect(bgndAppearance_btn, SIGNAL(clicked(bool)), SLOT(configureBgndAppearanceFile()));
    connect(bgndImage_btn, SIGNAL(clicked(bool)), SLOT(configureBgndImageFile()));
    connect(menuBgndAppearance_btn, SIGNAL(clicked(bool)), SLOT(configureMenuBgndAppearanceFile()));
    connect(menuBgndImage_btn, SIGNAL(clicked(bool)), SLOT(configureMenuBgndImageFile()));

    bgndAppearance_btn->setAutoRaise(true);
    bgndAppearance_btn->setVisible(false);
    bgndAppearance_btn->setIcon(QtCurve::loadKIcon("configure"));
    bgndImage_btn->setAutoRaise(true);
    bgndImage_btn->setVisible(false);
    bgndImage_btn->setIcon(QtCurve::loadKIcon("configure"));
    menuBgndAppearance_btn->setAutoRaise(true);
    menuBgndAppearance_btn->setVisible(false);
    menuBgndAppearance_btn->setIcon(QtCurve::loadKIcon("configure"));
    menuBgndImage_btn->setAutoRaise(true);
    menuBgndImage_btn->setVisible(false);
    menuBgndImage_btn->setIcon(QtCurve::loadKIcon("configure"));

    setupStack();

    if (kwin->ok()) {
        Options currentStyle;
        Options defaultStyle;

        kwin->load(nullptr);
        qtcDefaultSettings(&defaultStyle);
        if (!qtcReadConfig(nullptr, &currentStyle, &defaultStyle))
            currentStyle = defaultStyle;

        previewStyle = currentStyle;
        setupShadesTab();
        setWidgetOptions(currentStyle);

        setupGradientsTab();
        setupPresets(currentStyle, defaultStyle);
        setupPreview();
        readyForPreview = true;
        updatePreview();
    } else {
        stack->setCurrentIndex(kwinPage);
        titleLabel->setVisible(false);
        stackList->setVisible(false);
    }
    // KMainWindow dereferences KGlobal when it closes. When KGlobal's refs get
    // to 0 it quits! ...running kcmshell4 style does not seem to increase ref
    // count of KGlobal - therefore we do it here - otherwse kcmshell4 would
    // exit immediately after QtCurve's config dialog was closed :-(
    if (QCoreApplication::applicationName() == QLatin1String("kcmshell")) {
        static std::once_flag ref_flag;
        std::call_once(ref_flag, [] {
        // TODO figure out if it is still necessary
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                KGlobal::ref();
#pragma GCC diagnostic pop
            });
    }
}

QtCurveConfig::~QtCurveConfig()
{
    // Remove QTCURVE_PREVIEW_CONFIG setting, so that main kcmstyle preview
    // does not revert to default settings!
    qputenv(QTCURVE_PREVIEW_CONFIG, "");
    previewFrame->hide();
    previewFrame->setParent(0);
    delete previewFrame;
    if (!mdiWindow) {
        delete stylePreview;
    }
}

QSize QtCurveConfig::sizeHint() const
{
    return QSize(700, 500);
}

void QtCurveConfig::save()
{
    if (!kwin->ok())
        return;

    Options opts = presets[currentText].opts;

    setOptions(opts);

    if (IMG_FILE == opts.bgndImage.type) {
        opts.bgndImage.pixmap.file = installThemeFile(bgndImageDlg->fileName(),
                                                      BGND_FILE IMAGE_FILE);
    } else {
        removeInstalledThemeFile(BGND_FILE IMAGE_FILE);
    }
    if (APPEARANCE_FILE == opts.bgndAppearance) {
        opts.bgndPixmap.file = installThemeFile(bgndPixmapDlg->fileName(),
                                                BGND_FILE);
    } else {
        removeInstalledThemeFile(BGND_FILE);
    }
    if(IMG_FILE==opts.menuBgndImage.type)
    {
        opts.menuBgndImage.pixmap.file=installThemeFile(menuBgndImageDlg->fileName(), BGND_FILE MENU_FILE IMAGE_FILE);
//         printf("SAVE MENU:%s\n", opts.menuBgndImage.pixmap.file.toLatin1().constData());
    }
    else
        removeInstalledThemeFile(BGND_FILE MENU_FILE IMAGE_FILE);
    if(APPEARANCE_FILE==opts.menuBgndAppearance)
        opts.menuBgndPixmap.file=installThemeFile(menuBgndPixmapDlg->fileName(), BGND_FILE MENU_FILE);
    else
        removeInstalledThemeFile(BGND_FILE MENU_FILE);

    qtcWriteConfig(nullptr, opts, presets[defaultText].opts);

    // This is only read by KDE3...
    KConfig      k3globals(kdeHome(true)+"/share/config/kdeglobals", KConfig::CascadeConfig);
    KConfigGroup kde(&k3globals, "KDE");

    if(opts.gtkButtonOrder)
        kde.writeEntry("ButtonLayout", 2);
    else
        kde.deleteEntry("ButtonLayout");

    kwin->save(0L);
    // If using QtCurve window decoration, get this to update...
    KConfig kwin("kwinrc", KConfig::CascadeConfig);
    KConfigGroup style(&kwin, "Style");

    if (style.readEntry("PluginLib", QString()) == "kwin3_qtcurve") {
        QDBusConnection::sessionBus().send(
            QDBusMessage::createSignal("/KWin", "org.kde.KWin",
                                       "reloadConfig"));
    }

    // Remove QTCURVE_PREVIEW_CONFIG setting, so that main kcmstyle
    // preview does not revert to default settings!
    qputenv(QTCURVE_PREVIEW_CONFIG, "");
}

void QtCurveConfig::defaults()
{
    if(!kwin->ok())
        return;

    int index=-1;

    for(int i=0; i<presetsCombo->count() && -1==index; ++i)
        if(presetsCombo->itemText(i)==defaultText)
            index=i;

    presetsCombo->setCurrentIndex(index);
    setPreset();
    kwin->defaults();
}

void QtCurveConfig::emboldenToggled()
{
    if(!embolden->isChecked() && IND_NONE==defBtnIndicator->currentIndex())
        defBtnIndicator->setCurrentIndex(IND_TINT);
    updateChanged();
}

void QtCurveConfig::defBtnIndicatorChanged()
{
    if(IND_NONE==defBtnIndicator->currentIndex() && !embolden->isChecked())
        embolden->setChecked(true);
    else if(IND_GLOW==defBtnIndicator->currentIndex() && EFFECT_NONE==buttonEffect->currentIndex())
        buttonEffect->setCurrentIndex(EFFECT_SHADOW);

    if(IND_COLORED==defBtnIndicator->currentIndex() && round->currentIndex()>ROUND_FULL)
        round->setCurrentIndex(ROUND_FULL);

    updateChanged();
}

void QtCurveConfig::buttonEffectChanged()
{
    if(EFFECT_NONE==buttonEffect->currentIndex())
    {
        if(IND_GLOW==defBtnIndicator->currentIndex())
            defBtnIndicator->setCurrentIndex(IND_TINT);
        if(MO_GLOW==coloredMouseOver->currentIndex())
            coloredMouseOver->setCurrentIndex(MO_PLASTIK);
    }

    updateChanged();
}

void QtCurveConfig::coloredMouseOverChanged()
{
    if(MO_GLOW==coloredMouseOver->currentIndex() &&
       EFFECT_NONE==buttonEffect->currentIndex())
        buttonEffect->setCurrentIndex(EFFECT_SHADOW);

    updateChanged();
}

void QtCurveConfig::shadeSlidersChanged()
{
    customSlidersColor->setEnabled(SHADE_CUSTOM==shadeSliders->currentIndex());
    updateChanged();
    if(gradPreview)
        gradPreview->repaint();
}

void QtCurveConfig::shadeMenubarsChanged()
{
    customMenubarsColor->setEnabled(SHADE_CUSTOM==shadeMenubars->currentIndex());
    customMenuNormTextColor->setEnabled(SHADE_WINDOW_BORDER!=shadeMenubars->currentIndex());
    customMenuSelTextColor->setEnabled(SHADE_WINDOW_BORDER!=shadeMenubars->currentIndex());
    customMenuTextColor->setEnabled(SHADE_WINDOW_BORDER!=shadeMenubars->currentIndex());
    shadeMenubarOnlyWhenActive->setEnabled(SHADE_WINDOW_BORDER!=shadeMenubars->currentIndex());
    if(SHADE_WINDOW_BORDER==shadeMenubars->currentIndex())
        windowBorder_menuColor->setChecked(false);
    updateChanged();
}

void QtCurveConfig::shadeCheckRadioChanged()
{
    customCheckRadioColor->setEnabled(SHADE_CUSTOM==shadeCheckRadio->currentIndex());
    updateChanged();
}

void QtCurveConfig::customMenuTextColorChanged()
{
    customMenuNormTextColor->setEnabled(SHADE_WINDOW_BORDER!=shadeMenubars->currentIndex() && customMenuTextColor->isChecked());
    customMenuSelTextColor->setEnabled(SHADE_WINDOW_BORDER!=shadeMenubars->currentIndex() && customMenuTextColor->isChecked());
    updateChanged();
}

void QtCurveConfig::menuStripeChanged()
{
    customMenuStripeColor->setEnabled(SHADE_CUSTOM==menuStripe->currentIndex());
    menuStripeAppearance->setEnabled(SHADE_NONE!=menuStripe->currentIndex());
    updateChanged();
}

void QtCurveConfig::shadePopupMenuChanged()
{
    lighterPopupMenuBgnd->setEnabled(!shadePopupMenu->isChecked());
}

void QtCurveConfig::progressColorChanged()
{
    customProgressColor->setEnabled(SHADE_CUSTOM==progressColor->currentIndex());
    updateChanged();
}

void QtCurveConfig::comboBtnChanged()
{
    customComboBtnColor->setEnabled(SHADE_CUSTOM==comboBtn->currentIndex());
    updateChanged();
}

void QtCurveConfig::sortedLvChanged()
{
    customSortedLvColor->setEnabled(SHADE_CUSTOM==sortedLv->currentIndex());
    updateChanged();
}

void QtCurveConfig::crColorChanged()
{
    customCrBgndColor->setEnabled(SHADE_CUSTOM==crColor->currentIndex());
    updateChanged();
}

void QtCurveConfig::stripedProgressChanged()
{
    bool allowAnimation=STRIPE_NONE!=stripedProgress->currentIndex() &&
                        STRIPE_FADE!=stripedProgress->currentIndex();

    animatedProgress->setEnabled(allowAnimation);
    if(animatedProgress->isChecked() && !allowAnimation)
        animatedProgress->setChecked(false);
    updateChanged();
}

void QtCurveConfig::activeTabAppearanceChanged()
{
    int  current(activeTabAppearance->currentIndex());
    bool disableCol(APPEARANCE_FLAT==current || APPEARANCE_RAISED==current);

    if(colorSelTab->value() && disableCol)
        colorSelTab->setValue(MIN_COLOR_SEL_TAB_FACTOR);
    colorSelTab->setEnabled(!disableCol);
    updateChanged();
}

void QtCurveConfig::tabMoChanged()
{
    if(TAB_MO_GLOW==tabMouseOver->currentIndex())
        roundAllTabs->setChecked(true);
    roundAllTabs->setEnabled(TAB_MO_GLOW!=tabMouseOver->currentIndex());
    roundAllTabs_false->setEnabled(TAB_MO_GLOW!=tabMouseOver->currentIndex());
    updateChanged();
}

void QtCurveConfig::shadingChanged()
{
    updateChanged();
    if(gradPreview)
        gradPreview->repaint();
}

void QtCurveConfig::passwordCharClicked()
{
    int              cur(toInt(passwordChar->text()));
    CharSelectDialog dlg(this, cur);

    if(QDialog::Accepted==dlg.exec() && dlg.currentChar()!=cur)
    {
        setPasswordChar(dlg.currentChar());
        updateChanged();
    }
}

void QtCurveConfig::unifySpinBtnsToggled()
{
    if(unifySpinBtns->isChecked())
        unifySpin->setChecked(false);
    unifySpin->setDisabled(unifySpinBtns->isChecked());
    updateChanged();
}

void QtCurveConfig::unifySpinToggled()
{
    if(unifySpin->isChecked())
        unifySpinBtns->setChecked(false);
    unifySpinBtns->setDisabled(unifySpin->isChecked());
    updateChanged();
}

void QtCurveConfig::sliderThumbChanged()
{
    if(LINE_NONE!=sliderThumbs->currentIndex() && sliderWidth->value()<DEFAULT_SLIDER_WIDTH)
        sliderWidth->setValue(DEFAULT_SLIDER_WIDTH);
    updateChanged();
}

void QtCurveConfig::sliderWidthChanged()
{
    if(0==sliderWidth->value()%2)
        sliderWidth->setValue(sliderWidth->value()+1);

    if(LINE_NONE!=sliderThumbs->currentIndex() && sliderWidth->value()<DEFAULT_SLIDER_WIDTH)
        sliderThumbs->setCurrentIndex(LINE_NONE);
    updateChanged();
}

void QtCurveConfig::menubarHidingChanged()
{
    updateChanged();
}

void QtCurveConfig::windowBorder_colorTitlebarOnlyChanged()
{
    if(!windowBorder_colorTitlebarOnly->isChecked())
        windowBorder_blend->setChecked(false);
    updateChanged();
}

void QtCurveConfig::windowBorder_blendChanged()
{
    if(windowBorder_blend->isChecked())
    {
        windowBorder_colorTitlebarOnly->setChecked(true);
        windowBorder_menuColor->setChecked(false);
    }
    updateChanged();
}

void QtCurveConfig::windowBorder_menuColorChanged()
{
    if(windowBorder_menuColor->isChecked())
    {
        windowBorder_colorTitlebarOnly->setChecked(false);
        if(SHADE_WINDOW_BORDER==shadeMenubars->currentIndex())
            shadeMenubars->setCurrentIndex(SHADE_NONE);
    }
    updateChanged();
}

void QtCurveConfig::thinSbarGrooveChanged()
{
    if(thinSbarGroove->isChecked())
        borderSbarGroove->setChecked(true);
    updateChanged();
}

void QtCurveConfig::borderSbarGrooveChanged()
{
    if(!borderSbarGroove->isChecked())
        thinSbarGroove->setChecked(false);
    updateChanged();
}

void QtCurveConfig::borderProgressChanged()
{
    if(!borderProgress->isChecked())
    {
        squareProgress->setChecked(true);
        fillProgress->setChecked(true);
    }
    updateChanged();
}

void QtCurveConfig::squareProgressChanged()
{
    if(!fillProgress->isChecked() || !squareProgress->isChecked())
        borderProgress->setChecked(true);
    updateChanged();
}

void QtCurveConfig::fillProgressChanged()
{
    if(!fillProgress->isChecked() || !squareProgress->isChecked())
        borderProgress->setChecked(true);
    updateChanged();
}

void QtCurveConfig::titlebarButtons_customChanged()
{
    if(titlebarButtons_custom->isChecked())
        titlebarButtons_useHover->setChecked(false);
    updateChanged();
}

void QtCurveConfig::titlebarButtons_useHoverChanged()
{
    if(titlebarButtons_useHover->isChecked())
        titlebarButtons_custom->setChecked(false);
    updateChanged();
}

void QtCurveConfig::bgndAppearanceChanged()
{
    if(APPEARANCE_STRIPED==bgndAppearance->currentIndex())
        bgndGrad->setCurrentIndex(GT_HORIZ);
    bgndGrad->setEnabled(APPEARANCE_STRIPED!=bgndAppearance->currentIndex() && APPEARANCE_FILE!=bgndAppearance->currentIndex());
    bgndAppearance_btn->setVisible(APPEARANCE_FILE==bgndAppearance->currentIndex());
    updateChanged();
}

void QtCurveConfig::bgndImageChanged()
{
    bgndImage_btn->setVisible(IMG_FILE==bgndImage->currentIndex());
    updateChanged();
}

void QtCurveConfig::menuBgndAppearanceChanged()
{
    if(APPEARANCE_STRIPED==menuBgndAppearance->currentIndex())
        menuBgndGrad->setCurrentIndex(GT_HORIZ);
    menuBgndGrad->setEnabled(APPEARANCE_STRIPED!=menuBgndAppearance->currentIndex() && APPEARANCE_FILE!=menuBgndAppearance->currentIndex());
    menuBgndAppearance_btn->setVisible(APPEARANCE_FILE==menuBgndAppearance->currentIndex());
    updateChanged();
}

void QtCurveConfig::menuBgndImageChanged()
{
    menuBgndImage_btn->setVisible(IMG_FILE==menuBgndImage->currentIndex());
    updateChanged();
}

void QtCurveConfig::configureBgndAppearanceFile()
{
    if(bgndPixmapDlg->run())
        updateChanged();
}

void QtCurveConfig::configureBgndImageFile()
{
    if(bgndImageDlg->run())
        updateChanged();
}

void QtCurveConfig::configureMenuBgndAppearanceFile()
{
    if(menuBgndPixmapDlg->run())
        updateChanged();
}

void QtCurveConfig::configureMenuBgndImageFile()
{
    if(menuBgndImageDlg->run())
        updateChanged();
}

void QtCurveConfig::groupBoxChanged()
{
    gbFactor->setEnabled(FRAME_SHADED==groupBox->currentIndex() || FRAME_FADED==groupBox->currentIndex());
    updateChanged();
}

void QtCurveConfig::setupStack()
{
    int i=0;
    CStackItem *first=new CStackItem(stackList, i18n("Presets and Preview"), i++);
    new CStackItem(stackList, i18n("General"), i++);
    new CStackItem(stackList, i18n("Rounding"), i++);
    new CStackItem(stackList, i18n("Opacity"), i++);
    new CStackItem(stackList, i18n("Group Boxes"), i++);
    new CStackItem(stackList, i18n("Combos"), i++);
    new CStackItem(stackList, i18n("Spin Buttons"), i++);
    new CStackItem(stackList, i18n("Splitters"), i++);
    new CStackItem(stackList, i18n("Sliders and Scrollbars"), i++);
    new CStackItem(stackList, i18n("Progressbars"), i++);
    new CStackItem(stackList, i18n("Default Button"),i++);
    new CStackItem(stackList, i18n("Mouse-over"), i++);
    new CStackItem(stackList, i18n("Item Views"), i++);
    new CStackItem(stackList, i18n("Scrollviews"), i++);
    new CStackItem(stackList, i18n("Tabs"), i++);
    new CStackItem(stackList, i18n("Checks and Radios"), i++);
    new CStackItem(stackList, i18n("Windows"), i++);

    kwin = new QtCurve::KWinConfig(nullptr, this);
    kwinPage = i;

    if (kwin->ok()) {
        kwin->setNote(i18n("<p><b>NOTE:</b><i>The settings here affect the borders drawn around application windows and dialogs - and "
                           "not internal (or MDI) windows. Therefore, these settings will <b>not</b> be reflected in the Preview "
                           "page.</i></p>"));
        connect(qtcSlot(kwin, changed), qtcSlot(this, updateChanged));
    }
    stack->insertWidget(i, kwin);
    new CStackItem(stackList, i18n("Window Manager"), i++);

    new CStackItem(stackList, i18n("Window buttons"), i++);
    new CStackItem(stackList, i18n("Window button colors"), i++);
    new CStackItem(stackList, i18n("Menubars"), i++);
    new CStackItem(stackList, i18n("Popup menus"), i++);
    new CStackItem(stackList, i18n("Toolbars"), i++);
    new CStackItem(stackList, i18n("Statusbars"), i++);
    new CStackItem(stackList, i18n("Dock windows"), i++);
    new CStackItem(stackList, i18n("Advanced Settings"), i++);
    new CStackItem(stackList, i18n("Applications"), i++);
    new CStackItem(stackList, i18n("Legacy"), i++);
    new CStackItem(stackList, i18n("Custom Gradients"), i++);
    new CStackItem(stackList, i18n("Custom Shades"), i++);

    stackList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    stackList->setSelectionMode(QAbstractItemView::SingleSelection);
    first->setSelected(true);
    connect(qtcSlot(stackList, itemSelectionChanged),
            qtcSlot(this, changeStack));
}

void
QtCurveConfig::setupPresets(const Options &currentStyle,
                            const Options &defaultStyle)
{
    // TODO custom filter and figure out what directory to use.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    QStringList files(KGlobal::dirs()->findAllResources("data", "QtCurve/*" EXTENSION, KStandardDirs::NoDuplicates));
#pragma GCC diagnostic pop

    files.sort();

    QStringList::Iterator it(files.begin()),
                          end(files.end());

    KGuiItem::assign(saveButton, KGuiItem(i18n("Save"), "document-save"));
    KGuiItem::assign(deleteButton, KGuiItem(i18n("Delete"), "edit-delete"));
    KGuiItem::assign(importButton, KGuiItem(i18n("Import..."),
                                            "document-import"));
    KGuiItem::assign(exportButton, KGuiItem(i18n("Export..."),
                                            "document-export"));

    deleteButton->setEnabled(false);

    currentText=i18n("(Current)");
    defaultText=i18n("(Default)");
    presets[currentText]=Preset(currentStyle);
    presets[defaultText]=Preset(defaultStyle);
    for (;it != end;++it) {
        QString name(getFileName(*it).remove(EXTENSION).replace('_', ' '));

        if (!name.isEmpty() && name != currentText && name != defaultText) {
            presetsCombo->insertItem(0, name);
            presets[name] = Preset(*it);
        }
    }

    presetsCombo->insertItem(0, currentText);
    presetsCombo->insertItem(0, defaultText);
    presetsCombo->model()->sort(0);
    connect(qtcSlot(presetsCombo, currentIndexChanged, (int)),
            qtcSlot(this, setPreset));
    connect(qtcSlot(saveButton, clicked), qtcSlot(this, savePreset, ()));
    connect(qtcSlot(deleteButton, clicked), qtcSlot(this, deletePreset));
    connect(qtcSlot(importButton, clicked), qtcSlot(this, importPreset));
    connect(qtcSlot(exportButton, clicked), qtcSlot(this, exportPreset));

    int index = -1;

    for (int i = 0;i < presetsCombo->count() && -1 == index;++i) {
        if (presetsCombo->itemText(i) == currentText) {
            index = i;
        }
    }

    presetsCombo->setCurrentIndex(index);
    setPreset();
}

void QtCurveConfig::setupPreview()
{
    QVBoxLayout *layout = new QVBoxLayout(previewFrame);

    workSpace = new CWorkspace(previewFrame);
    layout->setMargin(0);
    layout->addWidget(workSpace);

    previewControlPressed();
}

void QtCurveConfig::changeStack()
{
    CStackItem *item=(CStackItem *)(stackList->currentItem());

    if(item && !item->isSelected())
        item->setSelected(true);

    if(item)
    {
        if(0==item->stack() && settingsChanged(previewStyle))
            updatePreview();
        stack->setCurrentIndex(item->stack());
    }
}

void QtCurveConfig::gradChanged(int i)
{
    GradientCont::const_iterator it(customGradient.find((EAppearance)i));

    gradStops->clear();

    if(it!=customGradient.end())
    {
        gradPreview->setGrad((*it).second);
        gradBorder->setCurrentIndex((*it).second.border);

        GradientStopCont::const_iterator git((*it).second.stops.begin()),
                                         gend((*it).second.stops.end());
        CGradItem                        *first=0L;

        gradStops->blockSignals(true);
        for(; git!=gend; ++git)
        {
            QStringList details;

            details << QString().setNum((*git).pos*100.0)
                    << QString().setNum((*git).val*100.0)
                    << QString().setNum((*git).alpha*100.0);
            CGradItem *grad=new CGradItem(gradStops, details);
            if(!first)
                first=grad;
        }
        gradStops->blockSignals(false);
        gradStops->sortItems(0, Qt::AscendingOrder);
        if(first)
            gradStops->setCurrentItem(first);
    }
    else
    {
        gradPreview->setGrad(Gradient());
        gradBorder->setCurrentIndex(GB_3D);
    }

    gradBorder->setEnabled(NUM_CUSTOM_GRAD!=i);
}

void QtCurveConfig::borderChanged(int i)
{
    GradientCont::iterator it=customGradient.find((EAppearance)gradCombo->currentIndex());
    if(it!=customGradient.end())
    {
        (*it).second.border=(EGradientBorder)i;
        gradPreview->setGrad((*it).second);
        emit changed(true);
    }
}

static double prev=-1.0;

void QtCurveConfig::editItem(QTreeWidgetItem *i, int col)
{
    bool   ok;
    prev=i->text(col).toDouble(&ok);
    if(!ok)
        prev=-1.0;

    gradStops->editItem(i, col);
}

void QtCurveConfig::itemChanged(QTreeWidgetItem *i, int col)
{
    bool   ok;
    double val=i->text(col).toDouble(&ok)/100.0;

    if(prev<0 || (ok && qtcEqual(val, prev)))
        return;

    if(!ok || ((0==col || 2==col) && (val<0.0 || val>1.0)) || (1==col && (val<0.0 || val>2.0)))
        i->setText(col, QString().setNum(prev));
    else
    {
        double other=i->text(col ? 0 : 1).toDouble(&ok)/100.0;

        GradientCont::iterator it=customGradient.find((EAppearance)gradCombo->currentIndex());

        if(it!=customGradient.end())
        {
            (*it).second.stops.erase(GradientStop(0==col ? prev : other, 1==col ? prev : other, 2==col ? prev : other));
            (*it).second.stops.insert(GradientStop(0==col ? val : other, 1==col ? val : other, 2==col ? val : other));
            gradPreview->setGrad((*it).second);
            i->setText(col, QString().setNum(val*100.0));
            emit changed(true);
        }
    }
}

void QtCurveConfig::addGradStop()
{
    GradientCont::iterator cg=customGradient.find((EAppearance)gradCombo->currentIndex());

    if(cg==customGradient.end())
    {
        Gradient cust;

        cust.border=(EGradientBorder)gradBorder->currentIndex();
        cust.stops.insert(GradientStop(stopPosition->value()/100.0, stopValue->value()/100.0, stopAlpha->value()/100.0));
        customGradient[(EAppearance)gradCombo->currentIndex()]=cust;
        gradChanged(gradCombo->currentIndex());
        emit changed(true);
    }
    else
    {
        GradientStopCont::const_iterator it((*cg).second.stops.begin()),
                                         end((*cg).second.stops.end());
        double                           pos(stopPosition->value()/100.0),
                                         val(stopValue->value()/100.0),
                                         alpha(stopAlpha->value()/100.0);

        for(; it!=end; ++it)
            if(qtcEqual(pos, (*it).pos))
            {
                if(qtcEqual(val, (*it).val) && qtcEqual(alpha, (*it).alpha))
                    return;
                else
                {
                    (*cg).second.stops.erase(it);
                    break;
                }
            }

        unsigned int b4=(*cg).second.stops.size();
        (*cg).second.stops.insert(GradientStop(pos, val, alpha));
        if((*cg).second.stops.size()!=b4)
        {
            gradPreview->setGrad((*cg).second);

            QStringList details;

            details << QString().setNum(pos*100.0)
                    << QString().setNum(val*100.0)
                    << QString().setNum(alpha*100.0);

            QTreeWidgetItem *i=new CGradItem(gradStops, details);

            gradStops->setCurrentItem(i);
            gradStops->sortItems(0, Qt::AscendingOrder);
        }
    }
}

void QtCurveConfig::removeGradStop()
{
    QTreeWidgetItem *cur=gradStops->currentItem();

    if(cur)
    {
        QTreeWidgetItem *next=gradStops->itemBelow(cur);

        if(!next)
            next=gradStops->itemAbove(cur);

        GradientCont::iterator it=customGradient.find((EAppearance)gradCombo->currentIndex());

        if(it!=customGradient.end())
        {
            bool   ok;
            double pos=cur->text(0).toDouble(&ok)/100.0,
                   val=cur->text(1).toDouble(&ok)/100.0,
                   alpha=cur->text(2).toDouble(&ok)/100.0;

            (*it).second.stops.erase(GradientStop(pos, val, alpha));
            gradPreview->setGrad((*it).second);
            emit changed(true);

            delete cur;
            if(next)
                gradStops->setCurrentItem(next);
        }
    }
}

void QtCurveConfig::updateGradStop()
{
    QTreeWidgetItem *i=gradStops->selectedItems().size() ? *(gradStops->selectedItems().begin()) : 0L;

    GradientCont::iterator cg=customGradient.find((EAppearance)gradCombo->currentIndex());

    if(i)
    {
        double curPos=i->text(0).toDouble()/100.0,
               curVal=i->text(1).toDouble()/100.0,
               curAlpha=i->text(2).toDouble()/100.0,
               newPos(stopPosition->value()/100.0),
               newVal(stopValue->value()/100.0),
               newAlpha(stopAlpha->value()/100.0);

        if(!qtcEqual(newPos, curPos) || !qtcEqual(newVal, curVal) || !qtcEqual(newAlpha, curAlpha))
        {
            (*cg).second.stops.erase(GradientStop(curPos, curVal, curAlpha));
            (*cg).second.stops.insert(GradientStop(newPos, newVal, newAlpha));

            i->setText(0, QString().setNum(stopPosition->value()));
            i->setText(1, QString().setNum(stopValue->value()));
            i->setText(2, QString().setNum(stopAlpha->value()));
            gradPreview->setGrad((*cg).second);
            emit changed(true);
        }
    }
    else
        addGradStop();
}

void QtCurveConfig::stopSelected()
{
    QTreeWidgetItem *i=gradStops->selectedItems().size() ? *(gradStops->selectedItems().begin()) : 0L;

    removeButton->setEnabled(i);
    updateButton->setEnabled(i);

    if(i)
    {
        stopPosition->setValue(i->text(0).toInt());
        stopValue->setValue(i->text(1).toInt());
        stopAlpha->setValue(i->text(2).toInt());
    }
    else
    {
        stopPosition->setValue(0);
        stopValue->setValue(100);
        stopAlpha->setValue(100);
    }
}

void QtCurveConfig::exportKDE3()
{
    if(KMessageBox::Yes==KMessageBox::questionYesNo(this, i18n("Export your current KDE4 color palette, and font, so "
                                                               "that they can be used by KDE3 applications?")))
    {
        QString      kde3Home(kdeHome(true));
        KConfig      k3globals(kde3Home+"/share/config/kdeglobals", KConfig::NoGlobals);
        KConfigGroup general(&k3globals, "General");
        KConfigGroup wm(&k3globals, "WM");

        general.writeEntry("alternateBackground", palette().color(QPalette::Active, QPalette::AlternateBase));
        general.writeEntry("background", palette().color(QPalette::Active, QPalette::Window));
        general.writeEntry("buttonBackground", palette().color(QPalette::Active, QPalette::Button));
        general.writeEntry("buttonForeground", palette().color(QPalette::Active, QPalette::ButtonText));
        general.writeEntry("foreground", palette().color(QPalette::Active, QPalette::WindowText));
        general.writeEntry("selectBackground", palette().color(QPalette::Active, QPalette::Highlight));
        general.writeEntry("selectForeground", palette().color(QPalette::Active, QPalette::HighlightedText));
        general.writeEntry("windowBackground", palette().color(QPalette::Active, QPalette::Base));
        general.writeEntry("windowForeground", palette().color(QPalette::Active, QPalette::Text));
        general.writeEntry("linkColor", palette().color(QPalette::Active, QPalette::Link));
        general.writeEntry("visitedLinkColor", palette().color(QPalette::Active, QPalette::LinkVisited));

        if (kdeHome(false)!=kde3Home) {
            KConfigGroup k4General(KSharedConfig::openConfig(), "General");
            KConfigGroup k4wm(KSharedConfig::openConfig(), "WM");

            // Mainly for K3B...
            wm.writeEntry("activeBackground", k4wm.readEntry("activeBackground",
                                                             palette().color(QPalette::Active, QPalette::Window)));
            wm.writeEntry("activeForeground", k4wm.readEntry("activeForeground",
                                                             palette().color(QPalette::Active, QPalette::WindowText)));
            wm.writeEntry("inactiveBackground", k4wm.readEntry("inactiveBackground",
                                                               palette().color(QPalette::Inactive, QPalette::Window)));
            wm.writeEntry("inactiveForeground", k4wm.readEntry("inactiveForeground",
                                                               palette().color(QPalette::Inactive, QPalette::WindowText)));
            // Font settings...
            general.writeEntry("font", k4General.readEntry("font", font()));
            general.writeEntry("fixed", k4General.readEntry("fixed", font()));
            general.writeEntry("desktopFont", k4General.readEntry("desktopFont", font()));
            general.writeEntry("taskbarFont", k4General.readEntry("taskbarFont", font()));
            general.writeEntry("toolBarFont", k4General.readEntry("toolBarFont", font()));
        }
    }
}

void QtCurveConfig::exportQt()
{
    if(KMessageBox::Yes==KMessageBox::questionYesNo(this, i18n("Export your current KDE4 color palette, and font, so "
                                                               "that they can be used by pure-Qt3 applications?")))
    {
        KConfig        cfg(QDir::homePath()+"/.qt/qtrc", KConfig::NoGlobals);
        KConfigGroup   gen(&cfg, "General");
        KConfigGroup   pal(&cfg, "Palette");
        KConfigGroup   kde(&cfg, "KDE");
        const QPalette &p=palette();
        int            i;
        QStringList    act,
                       inact,
                       dis;
        QString        sep("^e");

        QPalette::ColorRole roles[]={QPalette::Foreground,
                                     QPalette::Button,
                                     QPalette::Light,
                                     QPalette::Midlight,
                                     QPalette::Dark,
                                     QPalette::Mid,
                                     QPalette::Text,
                                     QPalette::BrightText,
                                     QPalette::ButtonText,
                                     QPalette::Base,
                                     QPalette::Background,
                                     QPalette::Shadow,
                                     QPalette::Highlight,
                                     QPalette::HighlightedText,
                                     QPalette::Link,
                                     QPalette::LinkVisited,
                                     QPalette::NColorRoles
                                    };

        for (i = 0; roles[i] != QPalette::NColorRoles; i++)
        {
            act << p.color(QPalette::Active, roles[i]).name();
            inact << p.color(QPalette::Inactive, roles[i]).name();
            dis << p.color(QPalette::Disabled, roles[i]).name();
        }

        KConfigGroup k4General(KSharedConfig::openConfig(), "General");
        gen.writeEntry("font", k4General.readEntry("font", font()));
        gen.writeEntry("font", font());
        pal.writeEntry("active", act.join(sep));
        pal.writeEntry("disabled", dis.join(sep));
        pal.writeEntry("inactive", inact.join(sep));
        kde.writeEntry("contrast", QSettings(QLatin1String("Trolltech")).value("/Qt/KDE/contrast", 7).toInt());
    }
}

void QtCurveConfig::menubarTitlebarBlend()
{
    if(KMessageBox::Yes==KMessageBox::questionYesNo(this,
        i18n("<p>Set the following config items so that window titlebar and menubars appear blended?</p>"
             "<ul><li>Menubar, titlebar, and inactive titlebar gradient to \"%1\"</li>"
             "<li>Disable \"Blend titlebar color into background color\"</li>"
             "<li>Set menubar coloration to \"%2\"</li>"
             "<li>Extend window dragging into menubar</li>",
             uiString((EAppearance)menubarAppearance->currentIndex()),
             uiString(SHADE_WINDOW_BORDER, SW_MENUBAR))))
    {
        titlebarAppearance->setCurrentIndex(menubarAppearance->currentIndex());
        inactiveTitlebarAppearance->setCurrentIndex(menubarAppearance->currentIndex());
        windowBorder_blend->setChecked(false);
        windowBorder_fill->setChecked(true);
        shadeMenubars->setCurrentIndex(SHADE_WINDOW_BORDER);
        if(windowDrag->currentIndex()<WM_DRAG_MENUBAR)
            windowDrag->setCurrentIndex(WM_DRAG_MENUBAR);
    }
}

void QtCurveConfig::updatePreview()
{
    if(!readyForPreview)
        return;

    setOptions(previewStyle);

    qputenv(QTCURVE_PREVIEW_CONFIG, mdiWindow ? QTCURVE_PREVIEW_CONFIG : QTCURVE_PREVIEW_CONFIG_FULL);
    QStyle *style = QStyleFactory::create("qtcurve");
    qputenv(QTCURVE_PREVIEW_CONFIG, "");
    if (!style)
        return;

    // Very hacky way to pass preview options to style!!!
    QtCurve::Style::PreviewOption styleOpt;
    styleOpt.opts=previewStyle;

    style->drawControl((QStyle::ControlElement)QtCurve::Style::CE_QtC_SetOptions, &styleOpt, 0L, this);

    setStyleRecursive(mdiWindow ? (QWidget *)previewFrame : (QWidget *)stylePreview, style);
}

static const char * constGradValProp="qtc-grad-val";

void QtCurveConfig::copyGradient(QAction *act)
{
    int            val=act->property(constGradValProp).toInt();
    const Gradient *copy=nullptr;

    if(val>=APPEARANCE_CUSTOM1 && val <(APPEARANCE_CUSTOM1+NUM_CUSTOM_GRAD))
    {
        // Custom gradient!
        if(val!=gradCombo->currentIndex())
        {
            GradientCont::const_iterator grad(customGradient.find((EAppearance)val));

            if(grad!=customGradient.end())
                copy=&((*grad).second);
        }
    }
    else
        copy=qtcGetGradient((EAppearance)val, &previewStyle);

    if(copy)
    {
        customGradient[(EAppearance)gradCombo->currentIndex()]=*copy;
        gradChanged(gradCombo->currentIndex());
        emit changed(true);
    }
}

void QtCurveConfig::previewControlPressed()
{
    if(mdiWindow)
    {
        previewControlButton->setText(i18n("Reattach"));
        workSpace->removeSubWindow(stylePreview);
        if(stylePreview)
            stylePreview->deleteLater();
        mdiWindow->deleteLater();
        mdiWindow=0L;
        stylePreview = new CStylePreview(this);
        stylePreview->show();
    }
    else
    {
        if(stylePreview)
            stylePreview->deleteLater();
        stylePreview = new CStylePreview;
        mdiWindow = workSpace->addSubWindow(stylePreview, Qt::Window);
        mdiWindow->move(4, 4);
        mdiWindow->showMaximized();
        previewControlButton->setText(i18n("Detach"));
    }
    connect(qtcSlot(stylePreview, closePressed),
            qtcSlot(this, previewControlPressed));
    updatePreview();
}

void QtCurveConfig::setupGradientsTab()
{
    QMenu *menu=new QMenu(copyGradientButton);

    for(int i=0; i<appearance->count(); ++i)
        menu->addAction(appearance->itemText(i))->setProperty(constGradValProp, i);

    for(int i=APPEARANCE_CUSTOM1; i<(APPEARANCE_CUSTOM1+NUM_CUSTOM_GRAD); ++i)
        gradCombo->insertItem(i-APPEARANCE_CUSTOM1, i18n("Custom gradient %1", (i-APPEARANCE_CUSTOM1)+1));

    gradCombo->setCurrentIndex(APPEARANCE_CUSTOM1);

    copyGradientButton->setIcon(QtCurve::loadKIcon("edit-copy"));
    copyGradientButton->setToolTip(i18n("Copy settings from another gradient"));
    copyGradientButton->setMenu(menu);
    copyGradientButton->setPopupMode(QToolButton::InstantPopup);
    connect(qtcSlot(menu, triggered), qtcSlot(this, copyGradient));

    gradPreview=new CGradientPreview(this, previewWidgetContainer);
    QBoxLayout *layout=new QBoxLayout(QBoxLayout::TopToBottom, previewWidgetContainer);
    layout->addWidget(gradPreview);
    layout->setMargin(0);
    layout->setSpacing(0);
    QColor col(palette().color(QPalette::Active, QPalette::Button));
    previewColor->setColor(col);
    gradPreview->setColor(col);
    gradChanged(APPEARANCE_CUSTOM1);
    KGuiItem::assign(addButton, KGuiItem(i18n("Add"), "list-add"));
    KGuiItem::assign(removeButton, KGuiItem(i18n("Remove"), "list-remove"));
    KGuiItem::assign(updateButton, KGuiItem(i18n("Update"), "dialog-ok"));

    stopPosition->setRange(0, 100);
    stopPosition->setSingleStep(5);
    stopValue->setRange(0, 200);
    stopValue->setSingleStep(5);
    stopAlpha->setRange(0, 100);
    stopAlpha->setSingleStep(5);
    removeButton->setEnabled(false);
    updateButton->setEnabled(false);
    connect(qtcSlot(gradCombo, currentIndexChanged, (int)),
            qtcSlot(this, gradChanged));
    connect(qtcSlot(gradBorder, currentIndexChanged, (int)),
            qtcSlot(this, borderChanged));
    connect(qtcSlot(previewColor, changed),
            qtcSlot(gradPreview, setColor));
    connect(qtcSlot(gradStops, itemDoubleClicked), qtcSlot(this, editItem));
    connect(qtcSlot(gradStops, itemChanged), qtcSlot(this, itemChanged));
    connect(qtcSlot(addButton, clicked), qtcSlot(this, addGradStop));
    connect(qtcSlot(removeButton, clicked), qtcSlot(this, removeGradStop));
    connect(qtcSlot(updateButton, clicked), qtcSlot(this, updateGradStop));
    connect(qtcSlot(gradStops, itemSelectionChanged),
            qtcSlot(this, stopSelected));
}

void QtCurveConfig::setupShadesTab()
{
    int shade(0);

    setupShade(shade0, shade++);
    setupShade(shade1, shade++);
    setupShade(shade2, shade++);
    setupShade(shade3, shade++);
    setupShade(shade4, shade++);
    setupShade(shade5, shade++);
    connect(qtcSlot(customShading, toggled), qtcSlot(this, updateChanged));
    shade=0;
    setupAlpha(alpha0, shade++);
    setupAlpha(alpha1, shade++);
    connect(qtcSlot(customAlphas, toggled), qtcSlot(this, updateChanged));
}

void QtCurveConfig::setupShade(QDoubleSpinBox *w, int shade)
{
    w->setRange(0.0, 2.0);
    w->setSingleStep(0.05);
    connect(qtcSlot(w, valueChanged, (double)), qtcSlot(this, updateChanged));
    shadeVals[shade] = w;
}

void QtCurveConfig::setupAlpha(QDoubleSpinBox *w, int alpha)
{
    w->setRange(0.0, 1.0);
    w->setSingleStep(0.05);
    connect(qtcSlot(w, valueChanged, (double)), qtcSlot(this, updateChanged));
    alphaVals[alpha] = w;
}

void QtCurveConfig::populateShades(const Options &opts)
{
    int contrast = (QSettings(QLatin1String("Trolltech"))
                    .value("/Qt/KDE/contrast", 7).toInt());

    if(contrast<0 || contrast>10)
        contrast=7;

    customShading->setChecked(USE_CUSTOM_SHADES(opts));

    for(int i=0; i<QTC_NUM_STD_SHADES; ++i)
        shadeVals[i]->setValue(USE_CUSTOM_SHADES(opts) ? opts.customShades[i] :
                               qtc_intern_shades[shading->currentIndex() ==
                                                 int(Shading::Simple) ? 1 : 0]
                               [contrast][i]);

    customAlphas->setChecked(USE_CUSTOM_ALPHAS(opts));
    alphaVals[0]->setValue(USE_CUSTOM_ALPHAS(opts) ? opts.customAlphas[0] : ETCH_TOP_ALPHA);
    alphaVals[1]->setValue(USE_CUSTOM_ALPHAS(opts) ? opts.customAlphas[1] : ETCH_BOTTOM_ALPHA);
}

bool QtCurveConfig::diffShades(const Options &opts)
{
    if( (!USE_CUSTOM_SHADES(opts) && customShading->isChecked()) ||
        (USE_CUSTOM_SHADES(opts) && !customShading->isChecked()) )
        return true;

    if(customShading->isChecked())
    {
        for(int i=0; i<QTC_NUM_STD_SHADES; ++i)
            if(!qtcEqual(shadeVals[i]->value(), opts.customShades[i]))
                return true;
    }

    if( (!USE_CUSTOM_ALPHAS(opts) && customAlphas->isChecked()) ||
        (USE_CUSTOM_ALPHAS(opts) && !customAlphas->isChecked()) )
        return true;

    if(customAlphas->isChecked())
    {
        for(int i=0; i<NUM_STD_ALPHAS; ++i)
            if(!qtcEqual(alphaVals[i]->value(), opts.customAlphas[i]))
                return true;
    }
    return false;
}

bool QtCurveConfig::haveImages()
{
    return IMG_FILE==bgndImage->currentIndex() ||
           IMG_FILE==menuBgndImage->currentIndex() ||
           APPEARANCE_FILE==bgndAppearance->currentIndex() ||
           APPEARANCE_FILE==menuBgndAppearance->currentIndex();
}

bool QtCurveConfig::diffImages(const Options &opts)
{
    return (IMG_FILE==bgndImage->currentIndex() &&
                ( getThemeFile(bgndImageDlg->fileName())!=getThemeFile(opts.bgndImage.pixmap.file) ||
                  bgndImageDlg->imgWidth()!=opts.bgndImage.width ||
                  bgndImageDlg->imgHeight()!=opts.bgndImage.height ||
                  bgndImageDlg->onWindowBorder()!=opts.bgndImage.onBorder ||
                  bgndImageDlg->imgPos()!=opts.bgndImage.pos ) ) ||
           (IMG_FILE==menuBgndImage->currentIndex() &&
                ( getThemeFile(menuBgndImageDlg->fileName())!=getThemeFile(opts.menuBgndImage.pixmap.file) ||
                  menuBgndImageDlg->imgWidth()!=opts.menuBgndImage.width ||
                  menuBgndImageDlg->imgHeight()!=opts.menuBgndImage.height ||
                  menuBgndImageDlg->imgPos()!=opts.menuBgndImage.pos ) ) ||
           (APPEARANCE_FILE==bgndAppearance->currentIndex() &&
                (getThemeFile(bgndPixmapDlg->fileName())!=getThemeFile(opts.bgndPixmap.file))) ||
           (APPEARANCE_FILE==menuBgndAppearance->currentIndex() &&
                (getThemeFile(menuBgndPixmapDlg->fileName())!=getThemeFile(opts.menuBgndPixmap.file)));
}

void QtCurveConfig::setPasswordChar(int ch)
{
    QString     str;
    QTextStream s(&str);

    s.setIntegerBase(16);
    s << QChar(ch) << " (" << ch << ')';
    passwordChar->setText(str);
}

void QtCurveConfig::updateChanged()
{
    // Check if we have a floating preview!
    if(!mdiWindow && settingsChanged(previewStyle))
        updatePreview();

    if (settingsChanged())
        emit changed(true);
}

void QtCurveConfig::gtkButtonOrderChanged()
{
    if(gtkButtonOrder->isChecked())
        reorderGtkButtons->setChecked(false);
    updateChanged();
}

void QtCurveConfig::reorderGtkButtonsChanged()
{
    if(reorderGtkButtons->isChecked())
        gtkButtonOrder->setChecked(false);
    updateChanged();
}

void QtCurveConfig::focusChanged()
{
    if(ROUND_MAX==round->currentIndex() && FOCUS_LINE!=focus->currentIndex() && !(EFFECT_NONE!=buttonEffect->currentIndex() && FOCUS_GLOW==focus->currentIndex()))
        round->setCurrentIndex(ROUND_EXTRA);
    updateChanged();
}

void QtCurveConfig::roundChanged()
{
    if (ROUND_MAX == round->currentIndex() &&
        FOCUS_LINE != focus->currentIndex() &&
        !(EFFECT_NONE != buttonEffect->currentIndex() &&
          FOCUS_GLOW == focus->currentIndex())) {
        focus->setCurrentIndex(EFFECT_NONE == buttonEffect->currentIndex() ?
                               FOCUS_LINE : FOCUS_GLOW);
    }

    if(round->currentIndex()>ROUND_FULL && IND_COLORED==defBtnIndicator->currentIndex())
        defBtnIndicator->setCurrentIndex(IND_TINT);
    updateChanged();
}

void QtCurveConfig::setPreset()
{
    readyForPreview=false;
    Preset &p(presets[presetsCombo->currentText()]);

    if(!p.loaded)
        qtcReadConfig(p.fileName, &p.opts, &presets[defaultText].opts, false);

    setWidgetOptions(p.opts);

    if(defaultText==presetsCombo->currentText())
        kwin->defaults();
    else if(currentText==presetsCombo->currentText())
        kwin->load(nullptr);
    else if(p.opts.version>=VERSION_WITH_KWIN_SETTINGS)
    {
        KConfig cfg(p.fileName, KConfig::SimpleConfig);

        if(cfg.hasGroup(KWIN_GROUP))
            kwin->load(&cfg);
    }

    readyForPreview=true;
    if (settingsChanged(previewStyle))
        updatePreview();
    if (settingsChanged())
        emit changed(true);

    deleteButton->setEnabled(currentText!=presetsCombo->currentText() &&
                             defaultText!=presetsCombo->currentText() &&
                             0==presets[presetsCombo->currentText()].fileName.indexOf(QDir::homePath()));
    gradChanged(gradCombo->currentIndex());
}

void QtCurveConfig::savePreset()
{
    QString name=getPresetName(i18n("Save Preset"), i18n("Please enter a name for the preset:"),
                               currentText==presetsCombo->currentText() || defaultText==presetsCombo->currentText()
                                ? i18n("New preset")
                                : 0==presets[presetsCombo->currentText()].fileName.indexOf(QDir::homePath())
                                    ? presetsCombo->currentText()
                                    : i18n("%1 New", presetsCombo->currentText()));

    if(!name.isEmpty() && !savePreset(name))
        KMessageBox::error(this, i18n("Sorry, failed to save preset"));
}

bool QtCurveConfig::savePreset(const QString &name)
{
    if(!kwin->ok())
        return false;

    QString fname=QString(name).replace(' ', '_');
    QString dir(QtCurve::qtcSaveDir());

    KConfig cfg(dir+fname+EXTENSION, KConfig::NoGlobals);
    Options opts;

    setOptions(opts);

    if(IMG_FILE==opts.bgndImage.type)
        opts.bgndImage.pixmap.file=saveThemeFile(bgndImageDlg->fileName(), BGND_FILE IMAGE_FILE, fname);
    if(APPEARANCE_FILE==opts.bgndAppearance)
        opts.bgndPixmap.file=saveThemeFile(bgndPixmapDlg->fileName(), BGND_FILE, fname);
    if(IMG_FILE==opts.menuBgndImage.type)
        opts.menuBgndImage.pixmap.file=saveThemeFile(menuBgndImageDlg->fileName(), BGND_FILE MENU_FILE IMAGE_FILE, fname);
    if(APPEARANCE_FILE==opts.menuBgndAppearance)
        opts.menuBgndPixmap.file=saveThemeFile(menuBgndPixmapDlg->fileName(), BGND_FILE MENU_FILE, fname);

    if(qtcWriteConfig(&cfg, opts, presets[defaultText].opts, true))
    {
        kwin->save(&cfg);

        QMap<QString, Preset>::iterator it(presets.find(name)),
                                        end(presets.end());

        presets[name]=Preset(opts, dir+fname+EXTENSION);
        if(it==end)
        {
            presetsCombo->insertItem(0, name);
            presetsCombo->model()->sort(0);
            int index=-1;

            for(int i=0; i<presetsCombo->count() && -1==index; ++i)
                if(presetsCombo->itemText(i)==name)
                    index=i;

            presetsCombo->setCurrentIndex(index);
            setPreset();
        }

        return true;
    }

    return false;
}

QString
QtCurveConfig::getPresetName(const QString &cap, QString label, QString def,
                             QString name)
{
    QRegExp exp("\\w+[^\\0042\\0044\\0045\\0046\\0047\\0052\\0057\\0077\\0137\\0140]*");
    QRegExpValidator validator(exp, this);

    while (true) {
        if (name.isEmpty()) {
            name = KInputDialog::getText(cap, label, def, 0, this, &validator);
        }

        if (!name.isEmpty()) {
            name = name.replace('\"', ' ')
                .replace('$', ' ')
                .replace('%', ' ')
                .replace('&', ' ')
                .replace('\'', ' ')
                .replace('*', ' ')
                .replace('/', ' ')
                .replace('?', ' ')
                .replace('_', ' ')
                .replace('`', ' ')
                .simplified();

            if (name == currentText || name == defaultText) {
                label = i18n("<p>You cannot use the name \"%1\".</p>"
                             "<p>Please enter a different name:<p>", name);
                def = i18n("%1 New", name);
                name = QString();
            } else {
                QMap<QString, Preset>::iterator it(presets.find(name));

                if (it != presets.end()) {
                    if ((*it).fileName.indexOf(QDir::homePath()) != 0) {
                        label = i18n("<p>A system defined preset named "
                                     "\"%1\" already exists.</p>"
                                     "<p>Please enter a new name:<p>", name);
                        def = i18n("%1 New", name);
                        name = QString();
                    } else if (name == presetsCombo->currentText() ||
                               KMessageBox::warningYesNo(
                                   this, i18n("<p>A preset named \"%1\" "
                                              "already exists.</p><p>Do you "
                                              "wish to overwrite this?<p>",
                                              name)) == KMessageBox::Yes) {
                        return name;
                    } else {
                        label = i18n("<p>Please enter a new name:<p>");
                        def = i18n("%1 New", name);
                        name = QString();
                    }
                } else {
                    return name;
                }
            }
        } else {
            return QString();
        }
    }

    return QString();
}

void QtCurveConfig::deletePreset()
{
    if(KMessageBox::Yes==KMessageBox::warningYesNo(this, i18n("<p>Are you sure you wish to delete:</p><p><b>%1</b></p>",
                                                              presetsCombo->currentText())))
    {
        if(QFile::remove(presets[presetsCombo->currentText()].fileName))
        {
            removeThemeImages(presets[presetsCombo->currentText()].fileName);
            presets.remove(presetsCombo->currentText());
            presetsCombo->removeItem(presetsCombo->currentIndex());
        }
        else
            KMessageBox::error(this, i18n("<p>Sorry, failed to remove the preset file:</p><p><i>%1</i></p>",
                                          presets[presetsCombo->currentText()].fileName));
    }
}

void QtCurveConfig::importPreset()
{
    QString file(KFileDialog::getOpenFileName(
                     QUrl(), i18n("*" EXTENSION "|QtCurve Settings Files\n"
                                  THEME_PREFIX "*" THEME_SUFFIX
                                  "|QtCurve KDE Theme Files"), this));

    if (!file.isEmpty()) {
        KMimeType::Ptr mimeType=KMimeType::findByFileContent(file);;
        bool           compressed(mimeType && !mimeType->is("text/plain"));
        QString        fileName(getFileName(file)),
                       baseName(fileName.remove(EXTENSION).replace(' ', '_')),
                       name(QString(baseName).replace('_', ' '));
        Options        opts;

        if (name.isEmpty())
            KMessageBox::error(this, i18n("<p>Sorry, failed to load file.</p><p><i>Empty preset name?</i></p>"));
        else if (name == currentText || name == defaultText) {
            KMessageBox::error(this, i18n("<p>Sorry, failed to load file.</p><p><i>Cannot have a preset named "
                                          "\"%1\"</i></p>", name));
        } else {
            QString qtcFile(file);
            KZip *zip = compressed ? new KZip(file) : 0L;
            QTemporaryDir *tmpDir = nullptr;
            if (compressed) {
                qtcFile = QString();
                if (!zip->open(QIODevice::ReadOnly))
                    KMessageBox::error(this, i18n("Sorry, failed to open compressed file."));
                else {
                    const KArchiveDirectory *zipDir=zip->directory();

                    if (zipDir) {
                        tmpDir = new QTemporaryDir(
                            QDir::tempPath() + "/qtcurve");
                        tmpDir->setAutoRemove(false); // true);
                        zipDir->copyTo(tmpDir->path(), false);

                        // Find settings file...
                        QDir dir(tmpDir->path());
                        foreach (const QString &file, dir.entryList()) {
                            if (file.endsWith(EXTENSION)) {
                                qtcFile = dir.path() + "/" + file;
                            }
                        }
                        if (qtcFile.isEmpty())
                            KMessageBox::error(this, i18n("Invalid compressed settings file.\n(Could not locate settings file.)"));
                    } else {
                        KMessageBox::error(this, i18n("Invalid compressed settings file.\n(Could not list ZIP contents.)"));
                    }
                }
            }

            if (!qtcFile.isEmpty()) {
                if (qtcReadConfig(qtcFile, &opts, &presets[defaultText].opts,
                                  false)) {
                    name = getPresetName(i18n("Import Preset"), QString(),
                                         name, name);
                    if (!name.isEmpty()) {
                        QString qtcDir(QtCurve::qtcSaveDir());
                        name=name.replace(' ', '_');

                        if (compressed && tmpDir) {
                            // printf("IMPORT\n");
                            if (opts.bgndImage.type == IMG_FILE) {
                                auto &bgndFile = opts.bgndImage.pixmap.file;
                                QString fileName(name + BGND_FILE IMAGE_FILE +
                                                 getExt(bgndFile));
                                saveThemeFile(tmpDir->path() + '/' +
                                              getFileName(bgndFile),
                                              BGND_FILE IMAGE_FILE, name);
                                bgndFile = fileName;
                            }
                            if (opts.menuBgndImage.type == IMG_FILE) {
                                auto &menuBgndFile =
                                    opts.menuBgndImage.pixmap.file;
                                QString fileName(name +
                                                 BGND_FILE MENU_FILE
                                                 IMAGE_FILE +
                                                 getExt(menuBgndFile));
                                saveThemeFile(tmpDir->path() + '/' +
                                              getFileName(menuBgndFile),
                                              BGND_FILE MENU_FILE IMAGE_FILE,
                                              name);
                                menuBgndFile = fileName;
                            }
                            if (opts.bgndAppearance == APPEARANCE_FILE) {
                                auto &bgndFile = opts.bgndPixmap.file;
                                QString fileName(name + BGND_FILE +
                                                 getExt(bgndFile));
                                bgndFile = fileName;
                                saveThemeFile(tmpDir->path() + '/' +
                                              getFileName(bgndFile), BGND_FILE,
                                              name);
                            }
                            if (opts.menuBgndAppearance == APPEARANCE_FILE) {
                                auto &menuBgndFile = opts.menuBgndPixmap.file;
                                QString fileName(name + BGND_FILE MENU_FILE +
                                                 getExt(menuBgndFile));
                                saveThemeFile(tmpDir->path() + '/' +
                                              getFileName(menuBgndFile),
                                              BGND_FILE MENU_FILE, name);
                                menuBgndFile = fileName;
                            }
                        }

                        readyForPreview = false;
                        setWidgetOptions(opts);
                        savePreset(name);

                        // Load kwin options - if present
                        KConfig cfg(qtcFile, KConfig::SimpleConfig);

                        if(cfg.hasGroup(KWIN_GROUP))
                        {
                            KConfigGroup grp(&cfg, SETTINGS_GROUP);
                            QStringList  ver(grp.readEntry("version", QString()).split('.'));

                            if(3==ver.count() && qtcMakeVersion(ver[0].toInt(), ver[1].toInt())>=VERSION_WITH_KWIN_SETTINGS)
                                kwin->load(&cfg);
                        }
                        readyForPreview=true;
                        updatePreview();
                    }
                }
                else
                    KMessageBox::error(this, i18n("Sorry, failed to load file."));
            }

            delete zip;
            delete tmpDir;
        }
    }
}

void QtCurveConfig::exportPreset()
{
#ifdef QTC_QT4_STYLE_SUPPORT
    switch(KMessageBox::questionYesNoCancel(this, i18n("<p>In which format would you like to export the QtCurve "
                                                       "settings?<ul><li><i>QtCurve settings file</i> -"
                                                       " a file to be imported via this config dialog.</li>"
                                                       "<li><i>Standalone theme</i> - a style that user\'s can "
                                                       "select from the KDE style panel.</li></ul></p>"),
                                            i18n("Export Settings"),
                                            KGuiItem(i18n("QtCurve Settings File")), KGuiItem(i18n("Standalone Theme"))))
    {
        case KMessageBox::No:
            exportTheme();
        case KMessageBox::Cancel:
            return;
        case KMessageBox::Yes:
            break;
    }
#endif

    bool    compressed=haveImages();
    QString file(KFileDialog::getSaveFileName(
                     QUrl(), i18n("*" EXTENSION "|QtCurve Settings Files"),
                     this));

    if (!file.isEmpty()) {
        KZip *zip(compressed ? new KZip(file) : 0L);
        bool rv(true);

        if(zip && !zip->open(QIODevice::WriteOnly))
            rv=false;

        if (rv) {
            QTemporaryFile *temp(compressed ? new QTemporaryFile() : nullptr);

            if (temp && !temp->open())
                rv = false;

            if (rv) {
                KConfig cfg(compressed ? temp->fileName() : file,
                            KConfig::NoGlobals);
                Options opts;
                QString bgndImageName, menuBgndImageName, bgndPixmapName, menuBgndPixmapName;
                QString themeName(getFileName(file).remove(EXTENSION).replace(' ', '_'));

                setOptions(opts);

                if(compressed)
                {
//                     printf("EXPORT\n");
                    if(IMG_FILE==opts.bgndImage.type)
                    {
//                         printf("BGND: \"%s\"\n", opts.bgndImage.pixmap.file.toLatin1().constData());
                        bgndImageName=getThemeFile(opts.bgndImage.pixmap.file);
                        opts.bgndImage.pixmap.file=themeName+BGND_FILE IMAGE_FILE+getExt(bgndImageName);
//                         printf("BGND: \"%s\" \"%s\"\n", bgndImageName.toLatin1().constData(), opts.bgndImage.pixmap.file.toLatin1().constData());
                    }
                    if(IMG_FILE==opts.menuBgndImage.type)
                    {
//                         printf("MENU: \"%s\"\n", opts.menuBgndImage.pixmap.file.toLatin1().constData());
                        menuBgndImageName=getThemeFile(opts.menuBgndImage.pixmap.file);
                        opts.menuBgndImage.pixmap.file=themeName+BGND_FILE MENU_FILE IMAGE_FILE+getExt(menuBgndImageName);
//                         printf("MENU: \"%s\" \"%s\"\n", menuBgndImageName.toLatin1().constData(), opts.menuBgndImage.pixmap.file.toLatin1().constData());
                    }
                    if(APPEARANCE_FILE==opts.bgndAppearance)
                    {
                        bgndPixmapName=getThemeFile(opts.bgndPixmap.file);
                        opts.bgndPixmap.file=themeName+BGND_FILE+getExt(bgndPixmapName);
                    }
                    if(APPEARANCE_FILE==opts.menuBgndAppearance)
                    {
                        menuBgndPixmapName=getThemeFile(opts.menuBgndPixmap.file);
                        opts.menuBgndPixmap.file=themeName+BGND_FILE MENU_FILE+getExt(menuBgndPixmapName);
                    }
                }

                rv=qtcWriteConfig(&cfg, opts, presets[defaultText].opts, true);
                if(rv)
                    kwin->save(&cfg);
                if(rv && compressed)
                {
                    zip->addLocalFile(temp->fileName(), themeName+EXTENSION);
                    if(!bgndImageName.isEmpty())
                        zip->addLocalFile(bgndImageName, opts.bgndImage.pixmap.file);
                    if(!menuBgndImageName.isEmpty())
                        zip->addLocalFile(menuBgndImageName, opts.menuBgndImage.pixmap.file);
                    if(!bgndPixmapName.isEmpty())
                        zip->addLocalFile(bgndPixmapName, opts.bgndPixmap.file);
                    if(!menuBgndPixmapName.isEmpty())
                        zip->addLocalFile(menuBgndPixmapName, opts.menuBgndPixmap.file);
                    zip->close();
                }
            }

            if (temp) {
                temp->setAutoRemove(true);
                delete temp;
            }
        }
        if (!rv) {
            KMessageBox::error(this, i18n("Could not write to file:\n%1", file));
        }
        if (zip) {
            delete zip;
        }
    }
}

void QtCurveConfig::exportTheme()
{
#ifdef QTC_QT4_STYLE_SUPPORT
    if(!exportDialog)
        exportDialog=new CExportThemeDialog(this);

    Options opts;

    setOptions(opts);
    exportDialog->run(opts);
#endif
}

int QtCurveConfig::getTitleBarButtonFlags()
{
    int titlebarButtons=0;

    if(titlebarButtons_button->isChecked())
        titlebarButtons+=TITLEBAR_BUTTON_STD_COLOR;
    if(titlebarButtons_custom->isChecked())
        titlebarButtons+=TITLEBAR_BUTTON_COLOR;
    if(titlebarButtons_customIcon->isChecked())
        titlebarButtons+=TITLEBAR_BUTTON_ICON_COLOR;
    if(titlebarButtons_noFrame->isChecked())
        titlebarButtons+=TITLEBAR_BUTTON_NO_FRAME;
    if(titlebarButtons_round->isChecked())
        titlebarButtons+=TITLEBAR_BUTTON_ROUND;
    if(titlebarButtons_hoverFrame->isChecked())
        titlebarButtons+=TITLEBAR_BUTTON_HOVER_FRAME;
    if(titlebarButtons_hoverSymbol->isChecked())
        titlebarButtons+=TITLEBAR_BUTTON_HOVER_SYMBOL;
    if(titlebarButtons_hoverSymbolFull->isChecked())
        titlebarButtons+=TITLEBAR_BUTTON_HOVER_SYMBOL_FULL;
    if(titlebarButtons_colorOnMouseOver->isChecked())
        titlebarButtons+=TITLEBAR_BUTTON_COLOR_MOUSE_OVER;
    if(titlebarButtons_colorInactive->isChecked())
        titlebarButtons+=TITLEBAR_BUTTON_COLOR_INACTIVE;
    if(titlebarButtons_colorSymbolsOnly->isChecked())
        titlebarButtons+=TITLEBAR_BUTTON_COLOR_SYMBOL;
    if(titlebarButtons_sunkenBackground->isChecked())
        titlebarButtons+=TITLEBAR_BUTTON_SUNKEN_BACKGROUND;
    if(titlebarButtons_arrowMinMax->isChecked())
        titlebarButtons+=TITLEBAR_BUTTOM_ARROW_MIN_MAX;
    if(titlebarButtons_hideOnInactiveWindow->isChecked())
        titlebarButtons+=TITLEBAR_BUTTOM_HIDE_ON_INACTIVE_WINDOW;
    if(titlebarButtons_useHover->isChecked())
        titlebarButtons+=TITLEBAR_BUTTON_USE_HOVER_COLOR;
    return titlebarButtons;
}

int QtCurveConfig::getGroupBoxLabelFlags()
{
    int flags=0;
    if(gbLabel_bold->isChecked())
        flags+=GB_LBL_BOLD;
    if(gbLabel_centred->isChecked())
        flags+=GB_LBL_CENTRED;
    switch(gbLabel_textPos->currentIndex())
    {
        case GBV_INSIDE:
            flags+=GB_LBL_INSIDE;
            break;
        case GBV_OUTSIDE:
            flags+=GB_LBL_OUTSIDE;
        default:
            break;
    }
    return flags;
}

void QtCurveConfig::setOptions(Options &opts)
{
    opts.round=(ERound)round->currentIndex();
    opts.toolbarBorders=(ETBarBorder)toolbarBorders->currentIndex();
    opts.appearance=(EAppearance)appearance->currentIndex();
    opts.focus=(EFocus)focus->currentIndex();
    opts.lvLines=lvLines->isChecked();
    opts.lvButton=lvButton->isChecked();
    opts.drawStatusBarFrames=drawStatusBarFrames->isChecked();
    opts.buttonEffect=(EEffect)buttonEffect->currentIndex();
    opts.coloredMouseOver=(EMouseOver)coloredMouseOver->currentIndex();
    opts.menubarMouseOver=menubarMouseOver->isChecked();
    opts.shadeMenubarOnlyWhenActive=shadeMenubarOnlyWhenActive->isChecked();
    opts.thin=getThinFlags();
    opts.animatedProgress=animatedProgress->isChecked();
    opts.stripedProgress=(EStripe)stripedProgress->currentIndex();
    opts.lighterPopupMenuBgnd=lighterPopupMenuBgnd->value();
    opts.tabBgnd=tabBgnd->value();
    opts.menuDelay=menuDelay->value();
    opts.sliderWidth=sliderWidth->value();
    opts.menuStripe=(EShade)menuStripe->currentIndex();
    opts.customMenuStripeColor=customMenuStripeColor->color();
    opts.menuStripeAppearance=(EAppearance)menuStripeAppearance->currentIndex();
    opts.bgndGrad=(EGradType)bgndGrad->currentIndex();
    opts.menuBgndGrad=(EGradType)menuBgndGrad->currentIndex();
    opts.embolden=embolden->isChecked();
    opts.scrollbarType=(EScrollbar)scrollbarType->currentIndex();
    opts.defBtnIndicator=(EDefBtnIndicator)defBtnIndicator->currentIndex();
    opts.sliderThumbs=(ELine)sliderThumbs->currentIndex();
    opts.handles=(ELine)handles->currentIndex();
    opts.highlightTab=highlightTab->isChecked();
    opts.shadeSliders=(EShade)shadeSliders->currentIndex();
    opts.shadeMenubars=(EShade)shadeMenubars->currentIndex();
    opts.menubarAppearance=(EAppearance)menubarAppearance->currentIndex();
    opts.toolbarAppearance=(EAppearance)toolbarAppearance->currentIndex();
    opts.lvAppearance=(EAppearance)lvAppearance->currentIndex();
    opts.sliderAppearance=(EAppearance)sliderAppearance->currentIndex();
    opts.tabAppearance=(EAppearance)tabAppearance->currentIndex();
    opts.activeTabAppearance=(EAppearance)activeTabAppearance->currentIndex();
    opts.toolbarSeparators=(ELine)toolbarSeparators->currentIndex();
    opts.splitters=(ELine)splitters->currentIndex();
    opts.customSlidersColor=customSlidersColor->color();
    opts.customMenubarsColor=customMenubarsColor->color();
    opts.highlightFactor=highlightFactor->value();
    opts.customMenuNormTextColor=customMenuNormTextColor->color();
    opts.customMenuSelTextColor=customMenuSelTextColor->color();
    opts.customMenuTextColor=customMenuTextColor->isChecked();
    opts.fillSlider=fillSlider->isChecked();
    opts.stripedSbar=stripedSbar->isChecked();
    opts.sliderStyle=(ESliderStyle)sliderStyle->currentIndex();
    opts.roundMbTopOnly=roundMbTopOnly->isChecked();
    opts.menubarHiding=getHideFlags(menubarHiding_keyboard, menubarHiding_kwin);
    opts.statusbarHiding=getHideFlags(statusbarHiding_keyboard, statusbarHiding_kwin);
    opts.fillProgress=fillProgress->isChecked();
    opts.glowProgress=(EGlow)glowProgress->currentIndex();
    opts.darkerBorders=darkerBorders->isChecked();
    opts.comboSplitter=comboSplitter->isChecked();
    opts.comboBtn=(EShade)comboBtn->currentIndex();
    opts.customComboBtnColor=customComboBtnColor->color();
    opts.sortedLv=(EShade)sortedLv->currentIndex();
    opts.customSortedLvColor=customSortedLvColor->color();
    opts.unifySpinBtns=unifySpinBtns->isChecked();
    opts.unifySpin=unifySpin->isChecked();
    opts.unifyCombo=unifyCombo->isChecked();
    opts.vArrows=vArrows->isChecked();
    opts.xCheck=xCheck->isChecked();
    opts.hideShortcutUnderline=hideShortcutUnderline->isChecked();
    opts.crHighlight=crHighlight->value();
    opts.expanderHighlight=expanderHighlight->value();
    opts.crButton=crButton->isChecked();
    opts.colorSelTab=colorSelTab->value();
    opts.roundAllTabs=roundAllTabs->isChecked();
    opts.borderTab=borderTab->isChecked();
    opts.borderInactiveTab=borderInactiveTab->isChecked();
    opts.invertBotTab=invertBotTab->isChecked();
    opts.doubleGtkComboArrow=doubleGtkComboArrow->isChecked();
    opts.tabMouseOver=(ETabMo)tabMouseOver->currentIndex();
    opts.stdSidebarButtons=stdSidebarButtons->isChecked();
    opts.toolbarTabs=toolbarTabs->isChecked();
    opts.centerTabText=centerTabText->isChecked();
    opts.borderMenuitems=borderMenuitems->isChecked();
    opts.shadePopupMenu=shadePopupMenu->isChecked();
    opts.popupBorder=popupBorder->isChecked();
    opts.progressAppearance=(EAppearance)progressAppearance->currentIndex();
    opts.progressColor=(EShade)progressColor->currentIndex();
    opts.customProgressColor=customProgressColor->color();
    opts.progressGrooveAppearance=(EAppearance)progressGrooveAppearance->currentIndex();
    opts.grooveAppearance=(EAppearance)grooveAppearance->currentIndex();
    opts.sunkenAppearance=(EAppearance)sunkenAppearance->currentIndex();
    opts.progressGrooveColor=(EColor)progressGrooveColor->currentIndex();
    opts.menuitemAppearance=(EAppearance)menuitemAppearance->currentIndex();
    opts.menuBgndAppearance=(EAppearance)menuBgndAppearance->currentIndex();
    opts.titlebarAppearance=(EAppearance)titlebarAppearance->currentIndex();
    opts.inactiveTitlebarAppearance=(EAppearance)inactiveTitlebarAppearance->currentIndex();
    opts.titlebarButtonAppearance=(EAppearance)titlebarButtonAppearance->currentIndex();
    opts.windowBorder=getWindowBorderFlags();
    opts.selectionAppearance=(EAppearance)selectionAppearance->currentIndex();
    opts.shadeCheckRadio=(EShade)shadeCheckRadio->currentIndex();
    opts.customCheckRadioColor=customCheckRadioColor->color();
    opts.shading=(Shading)shading->currentIndex();
    opts.gtkScrollViews=gtkScrollViews->isChecked();
    opts.highlightScrollViews=highlightScrollViews->isChecked();
    opts.etchEntry=etchEntry->isChecked();
    opts.flatSbarButtons=flatSbarButtons->isChecked();
    opts.borderSbarGroove=borderSbarGroove->isChecked();
    opts.thinSbarGroove=thinSbarGroove->isChecked();
    opts.colorSliderMouseOver=colorSliderMouseOver->isChecked();
    opts.windowDrag=windowDrag->currentIndex();
    opts.sbarBgndAppearance=(EAppearance)sbarBgndAppearance->currentIndex();
    opts.sliderFill=(EAppearance)sliderFill->currentIndex();
    opts.bgndAppearance=(EAppearance)bgndAppearance->currentIndex();
    opts.bgndImage.type=(EImageType)bgndImage->currentIndex();
    opts.bgndOpacity=bgndOpacity->value();
    opts.dlgOpacity=dlgOpacity->value();
    opts.menuBgndImage.type=(EImageType)menuBgndImage->currentIndex();
    opts.menuBgndOpacity=menuBgndOpacity->value();
    opts.dwtAppearance=(EAppearance)dwtAppearance->currentIndex();
    opts.tooltipAppearance=(EAppearance)tooltipAppearance->currentIndex();
    opts.crColor=(EShade)crColor->currentIndex();
    opts.customCrBgndColor=customCrBgndColor->color();
    opts.smallRadio=smallRadio->isChecked();
    opts.splitterHighlight=splitterHighlight->value();
    opts.gtkComboMenus=gtkComboMenus->isChecked();
    opts.gtkButtonOrder=gtkButtonOrder->isChecked();
    opts.reorderGtkButtons=reorderGtkButtons->isChecked();
    opts.mapKdeIcons=mapKdeIcons->isChecked();
    opts.passwordChar=toInt(passwordChar->text());
    opts.groupBox=(EFrame)groupBox->currentIndex();
    opts.gbFactor=gbFactor->value();
    opts.customGradient=customGradient;
    opts.colorMenubarMouseOver=colorMenubarMouseOver->isChecked();
    opts.useHighlightForMenu=useHighlightForMenu->isChecked();
    opts.gbLabel=getGroupBoxLabelFlags();
    opts.fadeLines=fadeLines->isChecked();
    opts.menuIcons=menuIcons->isChecked();
    opts.stdBtnSizes=stdBtnSizes->isChecked();
    opts.boldProgress=boldProgress->isChecked();
    opts.coloredTbarMo=coloredTbarMo->isChecked();
    opts.tbarBtns=(ETBarBtn)tbarBtns->currentIndex();
    opts.tbarBtnAppearance=(EAppearance)tbarBtnAppearance->currentIndex();
    opts.tbarBtnEffect=(EEffect)tbarBtnEffect->currentIndex();
    opts.borderSelection=borderSelection->isChecked();
    opts.forceAlternateLvCols=forceAlternateLvCols->isChecked();
    opts.titlebarAlignment=(EAlign)titlebarAlignment->currentIndex();
    opts.titlebarEffect=(EEffect)titlebarEffect->currentIndex();
    opts.titlebarIcon=(ETitleBarIcon)titlebarIcon->currentIndex();
    opts.dwtSettings=getDwtSettingsFlags();
    opts.crSize=getCrSize(crSize);
    opts.square=getSquareFlags();

    opts.borderProgress=borderProgress->isChecked();

    if(customShading->isChecked())
    {
        for(int i=0; i<QTC_NUM_STD_SHADES; ++i)
            opts.customShades[i]=shadeVals[i]->value();
    }
    else
        opts.customShades[0]=0;

    if(customAlphas->isChecked())
    {
        for(int i=0; i<NUM_STD_ALPHAS; ++i)
            opts.customAlphas[i]=alphaVals[i]->value();
    }
    else
        opts.customAlphas[0]=0;

    opts.titlebarButtons=getTitleBarButtonFlags();
    opts.titlebarButtonColors.clear();
    if(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR || opts.titlebarButtons&TITLEBAR_BUTTON_ICON_COLOR)
    {
        opts.titlebarButtonColors[TITLEBAR_CLOSE]=titlebarButtons_colorClose->color();
        opts.titlebarButtonColors[TITLEBAR_MIN]=titlebarButtons_colorMin->color();
        opts.titlebarButtonColors[TITLEBAR_MAX]=titlebarButtons_colorMax->color();
        opts.titlebarButtonColors[TITLEBAR_KEEP_ABOVE]=titlebarButtons_colorKeepAbove->color();
        opts.titlebarButtonColors[TITLEBAR_KEEP_BELOW]=titlebarButtons_colorKeepBelow->color();
        opts.titlebarButtonColors[TITLEBAR_HELP]=titlebarButtons_colorHelp->color();
        opts.titlebarButtonColors[TITLEBAR_MENU]=titlebarButtons_colorMenu->color();
        opts.titlebarButtonColors[TITLEBAR_SHADE]=titlebarButtons_colorShade->color();
        opts.titlebarButtonColors[TITLEBAR_ALL_DESKTOPS]=titlebarButtons_colorAllDesktops->color();

        if(opts.titlebarButtons&TITLEBAR_BUTTON_ICON_COLOR)
        {
            int offset=NUM_TITLEBAR_BUTTONS;
            opts.titlebarButtonColors[TITLEBAR_CLOSE+offset]=titlebarButtons_colorCloseIcon->color();
            opts.titlebarButtonColors[TITLEBAR_MIN+offset]=titlebarButtons_colorMinIcon->color();
            opts.titlebarButtonColors[TITLEBAR_MAX+offset]=titlebarButtons_colorMaxIcon->color();
            opts.titlebarButtonColors[TITLEBAR_KEEP_ABOVE+offset]=titlebarButtons_colorKeepAboveIcon->color();
            opts.titlebarButtonColors[TITLEBAR_KEEP_BELOW+offset]=titlebarButtons_colorKeepBelowIcon->color();
            opts.titlebarButtonColors[TITLEBAR_HELP+offset]=titlebarButtons_colorHelpIcon->color();
            opts.titlebarButtonColors[TITLEBAR_MENU+offset]=titlebarButtons_colorMenuIcon->color();
            opts.titlebarButtonColors[TITLEBAR_SHADE+offset]=titlebarButtons_colorShadeIcon->color();
            opts.titlebarButtonColors[TITLEBAR_ALL_DESKTOPS+offset]=titlebarButtons_colorAllDesktopsIcon->color();
            offset+=NUM_TITLEBAR_BUTTONS;
            opts.titlebarButtonColors[TITLEBAR_CLOSE+offset]=titlebarButtons_colorCloseInactiveIcon->color();
            opts.titlebarButtonColors[TITLEBAR_MIN+offset]=titlebarButtons_colorMinInactiveIcon->color();
            opts.titlebarButtonColors[TITLEBAR_MAX+offset]=titlebarButtons_colorMaxInactiveIcon->color();
            opts.titlebarButtonColors[TITLEBAR_KEEP_ABOVE+offset]=titlebarButtons_colorKeepAboveInactiveIcon->color();
            opts.titlebarButtonColors[TITLEBAR_KEEP_BELOW+offset]=titlebarButtons_colorKeepBelowInactiveIcon->color();
            opts.titlebarButtonColors[TITLEBAR_HELP+offset]=titlebarButtons_colorHelpInactiveIcon->color();
            opts.titlebarButtonColors[TITLEBAR_MENU+offset]=titlebarButtons_colorMenuInactiveIcon->color();
            opts.titlebarButtonColors[TITLEBAR_SHADE+offset]=titlebarButtons_colorShadeInactiveIcon->color();
            opts.titlebarButtonColors[TITLEBAR_ALL_DESKTOPS+offset]=titlebarButtons_colorAllDesktopsInactiveIcon->color();
        }
    }

    opts.noBgndGradientApps=toSet(noBgndGradientApps->text());
    opts.noBgndOpacityApps=toSet(noBgndOpacityApps->text());
    opts.noMenuBgndOpacityApps=toSet(noMenuBgndOpacityApps->text());
    opts.noBgndImageApps=toSet(noBgndImageApps->text());
    opts.useQtFileDialogApps=toSet(useQtFileDialogApps->text());
    opts.menubarApps=toSet(menubarApps->text());
    opts.statusbarApps=toSet(statusbarApps->text());
    opts.noMenuStripeApps=toSet(noMenuStripeApps->text());

    if(IMG_FILE==opts.bgndImage.type)
    {
        opts.bgndImage.pixmap.file=getThemeFile(bgndImageDlg->fileName());
//         printf("SET OP BGND:%s\n", opts.menuBgndImage.pixmap.file.toLatin1().constData());
        opts.bgndImage.width=bgndImageDlg->imgWidth();
        opts.bgndImage.height=bgndImageDlg->imgHeight();
        opts.bgndImage.onBorder=bgndImageDlg->onWindowBorder();
        opts.bgndImage.pos=(EPixPos)bgndImageDlg->imgPos();
        opts.bgndImage.loaded=false;
    }
    if(APPEARANCE_FILE==opts.bgndAppearance)
    {
        opts.bgndPixmap.file=getThemeFile(bgndPixmapDlg->fileName());
        if((&opts) == (&previewStyle))
            opts.bgndPixmap.img=QPixmap(opts.bgndPixmap.file);
    }
    if(IMG_FILE==opts.menuBgndImage.type)
    {
        opts.menuBgndImage.pixmap.file=getThemeFile(menuBgndImageDlg->fileName());
//         printf("SET OP MENU:%s\n", opts.menuBgndImage.pixmap.file.toLatin1().constData());
        opts.menuBgndImage.width=menuBgndImageDlg->imgWidth();
        opts.menuBgndImage.height=menuBgndImageDlg->imgHeight();
        opts.menuBgndImage.onBorder=false; // Not used!!!
        opts.menuBgndImage.pos=(EPixPos)menuBgndImageDlg->imgPos();
        opts.menuBgndImage.loaded=false;
    }
    if(APPEARANCE_FILE==opts.menuBgndAppearance)
    {
        opts.menuBgndPixmap.file=getThemeFile(menuBgndPixmapDlg->fileName());
        if((&opts) == (&previewStyle))
            opts.menuBgndPixmap.img=QPixmap(opts.menuBgndPixmap.file);
    }
}

static QColor getColor(const TBCols &cols, int btn, int offset=0, const QColor &def=Qt::black)
{
    TBCols::const_iterator it=cols.find(btn+(NUM_TITLEBAR_BUTTONS*offset));

    return cols.end()==it ? def : (*it).second;
}

void QtCurveConfig::setWidgetOptions(const Options &opts)
{
    round->setCurrentIndex(opts.round);
    scrollbarType->setCurrentIndex(opts.scrollbarType);
    lighterPopupMenuBgnd->setValue(opts.lighterPopupMenuBgnd);
    tabBgnd->setValue(opts.tabBgnd);
    menuDelay->setValue(opts.menuDelay);
    sliderWidth->setValue(opts.sliderWidth);
    menuStripe->setCurrentIndex(opts.menuStripe);
    customMenuStripeColor->setColor(opts.customMenuStripeColor);
    menuStripeAppearance->setCurrentIndex(opts.menuStripeAppearance);
    bgndGrad->setCurrentIndex(opts.bgndGrad);
    menuBgndGrad->setCurrentIndex(opts.menuBgndGrad);
    toolbarBorders->setCurrentIndex(opts.toolbarBorders);
    sliderThumbs->setCurrentIndex(opts.sliderThumbs);
    handles->setCurrentIndex(opts.handles);
    appearance->setCurrentIndex(opts.appearance);
    focus->setCurrentIndex(opts.focus);
    lvLines->setChecked(opts.lvLines);
    lvButton->setChecked(opts.lvButton);
    drawStatusBarFrames->setChecked(opts.drawStatusBarFrames);
    buttonEffect->setCurrentIndex(opts.buttonEffect);
    coloredMouseOver->setCurrentIndex(opts.coloredMouseOver);
    menubarMouseOver->setChecked(opts.menubarMouseOver);
    shadeMenubarOnlyWhenActive->setChecked(opts.shadeMenubarOnlyWhenActive);
    thin_menuitems->setChecked(opts.thin&THIN_MENU_ITEMS);
    thin_buttons->setChecked(opts.thin&THIN_BUTTONS);
    thin_frames->setChecked(opts.thin&THIN_FRAMES);
    animatedProgress->setChecked(opts.animatedProgress);
    stripedProgress->setCurrentIndex(opts.stripedProgress);
    embolden->setChecked(opts.embolden);
    defBtnIndicator->setCurrentIndex(opts.defBtnIndicator);
    highlightTab->setChecked(opts.highlightTab);
    menubarAppearance->setCurrentIndex(opts.menubarAppearance);
    toolbarAppearance->setCurrentIndex(opts.toolbarAppearance);
    lvAppearance->setCurrentIndex(opts.lvAppearance);
    sliderAppearance->setCurrentIndex(opts.sliderAppearance);
    tabAppearance->setCurrentIndex(opts.tabAppearance);
    activeTabAppearance->setCurrentIndex(opts.activeTabAppearance);
    toolbarSeparators->setCurrentIndex(opts.toolbarSeparators);
    splitters->setCurrentIndex(opts.splitters);
    shadeSliders->setCurrentIndex(opts.shadeSliders);
    shadeMenubars->setCurrentIndex(opts.shadeMenubars);
    highlightFactor->setValue(opts.highlightFactor);
    customSlidersColor->setColor(opts.customSlidersColor);
    customMenubarsColor->setColor(opts.customMenubarsColor);
    customMenuNormTextColor->setColor(opts.customMenuNormTextColor);
    customMenuSelTextColor->setColor(opts.customMenuSelTextColor);
    customMenuTextColor->setChecked(opts.customMenuTextColor);

    customSlidersColor->setEnabled(SHADE_CUSTOM==opts.shadeSliders);
    customMenubarsColor->setEnabled(SHADE_CUSTOM==opts.shadeMenubars);
    customMenuNormTextColor->setEnabled(customMenuTextColor->isChecked());
    customMenuSelTextColor->setEnabled(customMenuTextColor->isChecked());
    customCheckRadioColor->setEnabled(SHADE_CUSTOM==opts.shadeCheckRadio);
    customMenuStripeColor->setEnabled(SHADE_CUSTOM==opts.menuStripe);
    menuStripeAppearance->setEnabled(SHADE_NONE!=opts.menuStripe);

    animatedProgress->setEnabled(STRIPE_NONE!=stripedProgress->currentIndex() &&
                                 STRIPE_FADE!=stripedProgress->currentIndex());

    fillSlider->setChecked(opts.fillSlider);
    stripedSbar->setChecked(opts.stripedSbar);
    sliderStyle->setCurrentIndex(opts.sliderStyle);
    roundMbTopOnly->setChecked(opts.roundMbTopOnly);
    menubarHiding_keyboard->setChecked(opts.menubarHiding&HIDE_KEYBOARD);
    menubarHiding_kwin->setChecked(opts.menubarHiding&HIDE_KWIN);
    statusbarHiding_keyboard->setChecked(opts.statusbarHiding&HIDE_KEYBOARD);
    statusbarHiding_kwin->setChecked(opts.statusbarHiding&HIDE_KWIN);
    fillProgress->setChecked(opts.fillProgress);
    glowProgress->setCurrentIndex(opts.glowProgress);
    darkerBorders->setChecked(opts.darkerBorders);
    comboSplitter->setChecked(opts.comboSplitter);
    comboBtn->setCurrentIndex(opts.comboBtn);
    customComboBtnColor->setColor(opts.customComboBtnColor);
    sortedLv->setCurrentIndex(opts.sortedLv);
    customSortedLvColor->setColor(opts.customSortedLvColor);
    unifySpinBtns->setChecked(opts.unifySpinBtns);
    unifySpin->setChecked(opts.unifySpin);
    unifyCombo->setChecked(opts.unifyCombo);
    vArrows->setChecked(opts.vArrows);
    xCheck->setChecked(opts.xCheck);
    xCheck_false->setChecked(!opts.xCheck);
    hideShortcutUnderline->setChecked(opts.hideShortcutUnderline);
    crHighlight->setValue(opts.crHighlight);
    expanderHighlight->setValue(opts.expanderHighlight);
    crButton->setChecked(opts.crButton);
    colorSelTab->setValue(opts.colorSelTab);
    roundAllTabs->setChecked(opts.roundAllTabs);
    roundAllTabs_false->setChecked(!opts.roundAllTabs);
    borderTab->setChecked(opts.borderTab);
    borderInactiveTab->setChecked(opts.borderInactiveTab);
    invertBotTab->setChecked(opts.invertBotTab);
    doubleGtkComboArrow->setChecked(opts.doubleGtkComboArrow);
    borderTab_false->setChecked(!opts.borderTab);
    borderInactiveTab_false->setChecked(!opts.borderInactiveTab);
    invertBotTab_false->setChecked(!opts.invertBotTab);
    tabMouseOver->setCurrentIndex(opts.tabMouseOver);
    stdSidebarButtons->setChecked(opts.stdSidebarButtons);
    toolbarTabs->setChecked(opts.toolbarTabs);
    toolbarTabs_false->setChecked(!opts.toolbarTabs);
    centerTabText->setChecked(opts.centerTabText);
    centerTabText_false->setChecked(!opts.centerTabText);
    borderMenuitems->setChecked(opts.borderMenuitems);
    shadePopupMenu->setChecked(opts.shadePopupMenu);
    popupBorder->setChecked(opts.popupBorder);
    progressAppearance->setCurrentIndex(opts.progressAppearance);
    progressColor->setCurrentIndex(opts.progressColor);
    customProgressColor->setColor(opts.customProgressColor);
    progressGrooveAppearance->setCurrentIndex(opts.progressGrooveAppearance);
    grooveAppearance->setCurrentIndex(opts.grooveAppearance);
    sunkenAppearance->setCurrentIndex(opts.sunkenAppearance);
    progressGrooveColor->setCurrentIndex(opts.progressGrooveColor);
    menuitemAppearance->setCurrentIndex(opts.menuitemAppearance);
    menuBgndAppearance->setCurrentIndex(opts.menuBgndAppearance);
    titlebarAppearance->setCurrentIndex(opts.titlebarAppearance);
    inactiveTitlebarAppearance->setCurrentIndex(opts.inactiveTitlebarAppearance);
    titlebarButtonAppearance->setCurrentIndex(opts.titlebarButtonAppearance);
    windowBorder_colorTitlebarOnly->setChecked(opts.windowBorder&WINDOW_BORDER_COLOR_TITLEBAR_ONLY);
    windowBorder_blend->setChecked(opts.windowBorder&WINDOW_BORDER_BLEND_TITLEBAR);
    windowBorder_fill->setChecked(opts.windowBorder&WINDOW_BORDER_FILL_TITLEBAR);
    windowBorder_menuColor->setChecked(opts.windowBorder&WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR);
    windowBorder_separator->setChecked(opts.windowBorder&WINDOW_BORDER_SEPARATOR);
    selectionAppearance->setCurrentIndex(opts.selectionAppearance);
    shadeCheckRadio->setCurrentIndex(opts.shadeCheckRadio);
    customCheckRadioColor->setColor(opts.customCheckRadioColor);
    colorMenubarMouseOver->setChecked(opts.colorMenubarMouseOver);
    useHighlightForMenu->setChecked(opts.useHighlightForMenu);
    gbLabel_bold->setChecked(opts.gbLabel&GB_LBL_BOLD);
    gbLabel_centred->setChecked(opts.gbLabel&GB_LBL_CENTRED);
    gbLabel_textPos->setCurrentIndex(opts.gbLabel&GB_LBL_INSIDE
                                        ? GBV_INSIDE
                                        : opts.gbLabel&GB_LBL_OUTSIDE
                                            ? GBV_OUTSIDE
                                            : GBV_STANDARD);
    fadeLines->setChecked(opts.fadeLines);
    menuIcons->setChecked(opts.menuIcons);
    stdBtnSizes->setChecked(opts.stdBtnSizes);
    boldProgress->setChecked(opts.boldProgress);
    boldProgress_false->setChecked(!opts.boldProgress);
    coloredTbarMo->setChecked(opts.coloredTbarMo);
    coloredTbarMo_false->setChecked(!opts.coloredTbarMo);
    tbarBtns->setCurrentIndex(opts.tbarBtns);
    tbarBtnAppearance->setCurrentIndex(opts.tbarBtnAppearance);
    tbarBtnEffect->setCurrentIndex(opts.tbarBtnEffect);
    borderSelection->setChecked(opts.borderSelection);
    forceAlternateLvCols->setChecked(opts.forceAlternateLvCols);
    titlebarAlignment->setCurrentIndex(opts.titlebarAlignment);
    titlebarEffect->setCurrentIndex(opts.titlebarEffect);
    titlebarIcon->setCurrentIndex(opts.titlebarIcon);

    shading->setCurrentIndex(int(opts.shading));
    gtkScrollViews->setChecked(opts.gtkScrollViews);
    highlightScrollViews->setChecked(opts.highlightScrollViews);
    etchEntry->setChecked(opts.etchEntry);
    flatSbarButtons->setChecked(opts.flatSbarButtons);
    borderSbarGroove->setChecked(opts.borderSbarGroove);
    thinSbarGroove->setChecked(opts.thinSbarGroove);
    colorSliderMouseOver->setChecked(opts.colorSliderMouseOver);
    windowBorder_addLightBorder->setChecked(opts.windowBorder&WINDOW_BORDER_ADD_LIGHT_BORDER);
    windowDrag->setCurrentIndex(opts.windowDrag);
    sbarBgndAppearance->setCurrentIndex(opts.sbarBgndAppearance);
    sliderFill->setCurrentIndex(opts.sliderFill);
    bgndAppearance->setCurrentIndex(opts.bgndAppearance);
    bgndImage->setCurrentIndex(opts.bgndImage.type);
    bgndOpacity->setValue(opts.bgndOpacity);
    dlgOpacity->setValue(opts.dlgOpacity);
    menuBgndImage->setCurrentIndex(opts.menuBgndImage.type);
    menuBgndOpacity->setValue(opts.menuBgndOpacity);
    dwtAppearance->setCurrentIndex(opts.dwtAppearance);
    tooltipAppearance->setCurrentIndex(opts.tooltipAppearance);
    dwtBtnAsPerTitleBar->setChecked(opts.dwtSettings&DWT_BUTTONS_AS_PER_TITLEBAR);
    dwtColAsPerTitleBar->setChecked(opts.dwtSettings&DWT_COLOR_AS_PER_TITLEBAR);
    dwtIconColAsPerTitleBar->setChecked(opts.dwtSettings&DWT_ICON_COLOR_AS_PER_TITLEBAR);
    dwtFontAsPerTitleBar->setChecked(opts.dwtSettings&DWT_FONT_AS_PER_TITLEBAR);
    dwtTextAsPerTitleBar->setChecked(opts.dwtSettings&DWT_TEXT_ALIGN_AS_PER_TITLEBAR);
    dwtEffectAsPerTitleBar->setChecked(opts.dwtSettings&DWT_EFFECT_AS_PER_TITLEBAR);
    dwtRoundTopOnly->setChecked(opts.dwtSettings&DWT_ROUND_TOP_ONLY);
    crColor->setCurrentIndex(opts.crColor);
    customCrBgndColor->setColor(opts.customCrBgndColor);
    smallRadio->setChecked(opts.smallRadio);
    smallRadio_false->setChecked(!opts.smallRadio);
    splitterHighlight->setValue(opts.splitterHighlight);
    gtkComboMenus->setChecked(opts.gtkComboMenus);
    gtkButtonOrder->setChecked(opts.gtkButtonOrder);
    reorderGtkButtons->setChecked(opts.reorderGtkButtons);
    mapKdeIcons->setChecked(opts.mapKdeIcons);
    setPasswordChar(opts.passwordChar);
    groupBox->setCurrentIndex(opts.groupBox);
    gbFactor->setValue(opts.gbFactor);
    customGradient=opts.customGradient;
    gradCombo->setCurrentIndex(APPEARANCE_CUSTOM1);
    borderProgress->setChecked(opts.borderProgress);

    setCrSize(crSize, opts.crSize);

    squareLvSelection->setChecked(opts.square&SQUARE_LISTVIEW_SELECTION);
    squareScrollViews->setChecked(opts.square&SQUARE_SCROLLVIEW);
    squareEntry->setChecked(opts.square&SQUARE_ENTRY);
    squareProgress->setChecked(opts.square&SQUARE_PROGRESS);
    squareFrame->setChecked(opts.square&SQUARE_FRAME);
    squareTabFrame->setChecked(opts.square&SQUARE_TAB_FRAME);
    squareSlider->setChecked(opts.square&SQUARE_SLIDER);
    squareScrollbarSlider->setChecked(opts.square&SQUARE_SB_SLIDER);
    squareWindows->setChecked(opts.square&SQUARE_WINDOWS);
    squareTooltips->setChecked(opts.square&SQUARE_TOOLTIPS);
    squarePopupMenus->setChecked(opts.square&SQUARE_POPUP_MENUS);

    if(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR)
    {
        titlebarButtons_colorClose->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_CLOSE));
        titlebarButtons_colorMin->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_MIN));
        titlebarButtons_colorMax->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_MAX));
        titlebarButtons_colorKeepAbove->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_KEEP_ABOVE));
        titlebarButtons_colorKeepBelow->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_KEEP_BELOW));
        titlebarButtons_colorHelp->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_HELP));
        titlebarButtons_colorMenu->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_MENU));
        titlebarButtons_colorShade->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_SHADE));
        titlebarButtons_colorAllDesktops->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_ALL_DESKTOPS));
    }
    else
    {
        QColor col(palette().color(QPalette::Active, QPalette::Button));

        titlebarButtons_colorClose->setColor(col);
        titlebarButtons_colorMin->setColor(col);
        titlebarButtons_colorMax->setColor(col);
        titlebarButtons_colorKeepAbove->setColor(col);
        titlebarButtons_colorKeepBelow->setColor(col);
        titlebarButtons_colorHelp->setColor(col);
        titlebarButtons_colorMenu->setColor(col);
        titlebarButtons_colorShade->setColor(col);
        titlebarButtons_colorAllDesktops->setColor(col);
    }

    if(opts.titlebarButtons&TITLEBAR_BUTTON_ICON_COLOR)
    {
        titlebarButtons_colorCloseIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_CLOSE, 1));
        titlebarButtons_colorMinIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_MIN, 1));
        titlebarButtons_colorMaxIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_MAX, 1));
        titlebarButtons_colorKeepAboveIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_KEEP_ABOVE, 1));
        titlebarButtons_colorKeepBelowIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_KEEP_BELOW, 1));
        titlebarButtons_colorHelpIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_HELP, 1));
        titlebarButtons_colorMenuIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_MENU, 1));
        titlebarButtons_colorShadeIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_SHADE, 1));
        titlebarButtons_colorAllDesktopsIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_ALL_DESKTOPS, 1));

        titlebarButtons_colorCloseInactiveIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_CLOSE, 2));
        titlebarButtons_colorMinInactiveIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_MIN, 2));
        titlebarButtons_colorMaxInactiveIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_MAX, 2));
        titlebarButtons_colorKeepAboveInactiveIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_KEEP_ABOVE, 2));
        titlebarButtons_colorKeepBelowInactiveIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_KEEP_BELOW, 2));
        titlebarButtons_colorHelpInactiveIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_HELP, 2));
        titlebarButtons_colorMenuInactiveIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_MENU, 2));
        titlebarButtons_colorShadeInactiveIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_SHADE, 2));
        titlebarButtons_colorAllDesktopsInactiveIcon->setColor(getColor(opts.titlebarButtonColors, TITLEBAR_ALL_DESKTOPS, 2));
    } else {
        QColor col = KColorScheme(QPalette::Active, KColorScheme::Window)
            .foreground(KColorScheme::ActiveText).color();
        titlebarButtons_colorCloseIcon->setColor(col);
        titlebarButtons_colorMinIcon->setColor(col);
        titlebarButtons_colorMaxIcon->setColor(col);
        titlebarButtons_colorKeepAboveIcon->setColor(col);
        titlebarButtons_colorKeepBelowIcon->setColor(col);
        titlebarButtons_colorHelpIcon->setColor(col);
        titlebarButtons_colorMenuIcon->setColor(col);
        titlebarButtons_colorShadeIcon->setColor(col);
        titlebarButtons_colorAllDesktopsIcon->setColor(col);

        col = KColorScheme(QPalette::Inactive, KColorScheme::Window)
            .foreground().color();
        titlebarButtons_colorCloseInactiveIcon->setColor(col);
        titlebarButtons_colorMinInactiveIcon->setColor(col);
        titlebarButtons_colorMaxInactiveIcon->setColor(col);
        titlebarButtons_colorKeepAboveInactiveIcon->setColor(col);
        titlebarButtons_colorKeepBelowInactiveIcon->setColor(col);
        titlebarButtons_colorHelpInactiveIcon->setColor(col);
        titlebarButtons_colorMenuInactiveIcon->setColor(col);
        titlebarButtons_colorShadeInactiveIcon->setColor(col);
        titlebarButtons_colorAllDesktopsInactiveIcon->setColor(col);
    }

    titlebarButtons_button->setChecked(opts.titlebarButtons&TITLEBAR_BUTTON_STD_COLOR);
    titlebarButtons_custom->setChecked(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR);
    titlebarButtons_customIcon->setChecked(opts.titlebarButtons&TITLEBAR_BUTTON_ICON_COLOR);
    titlebarButtons_noFrame->setChecked(opts.titlebarButtons&TITLEBAR_BUTTON_NO_FRAME);
    titlebarButtons_round->setChecked(opts.titlebarButtons&TITLEBAR_BUTTON_ROUND);
    titlebarButtons_hoverFrame->setChecked(opts.titlebarButtons&TITLEBAR_BUTTON_HOVER_FRAME);
    titlebarButtons_hoverSymbol->setChecked(opts.titlebarButtons&TITLEBAR_BUTTON_HOVER_SYMBOL);
    titlebarButtons_hoverSymbolFull->setChecked(opts.titlebarButtons&TITLEBAR_BUTTON_HOVER_SYMBOL_FULL);
    titlebarButtons_colorOnMouseOver->setChecked(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR_MOUSE_OVER);
    titlebarButtons_colorInactive->setChecked(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR_INACTIVE);
    titlebarButtons_colorSymbolsOnly->setChecked(opts.titlebarButtons&TITLEBAR_BUTTON_COLOR_SYMBOL);
    titlebarButtons_sunkenBackground->setChecked(opts.titlebarButtons&TITLEBAR_BUTTON_SUNKEN_BACKGROUND);
    titlebarButtons_arrowMinMax->setChecked(opts.titlebarButtons&TITLEBAR_BUTTOM_ARROW_MIN_MAX);
    titlebarButtons_hideOnInactiveWindow->setChecked(opts.titlebarButtons&TITLEBAR_BUTTOM_HIDE_ON_INACTIVE_WINDOW);
    titlebarButtons_useHover->setChecked(opts.titlebarButtons&TITLEBAR_BUTTON_USE_HOVER_COLOR);

    populateShades(opts);

    noBgndGradientApps->setText(toString(opts.noBgndGradientApps));
    noBgndOpacityApps->setText(toString(opts.noBgndOpacityApps));
    noMenuBgndOpacityApps->setText(toString(opts.noMenuBgndOpacityApps));
    noBgndImageApps->setText(toString(opts.noBgndImageApps));
    useQtFileDialogApps->setText(toString(opts.useQtFileDialogApps));
    menubarApps->setText(toString(opts.menubarApps));
    statusbarApps->setText(toString(opts.statusbarApps));
    noMenuStripeApps->setText(toString(opts.noMenuStripeApps));

    bgndImageDlg->set(getThemeFile(opts.bgndImage.pixmap.file), opts.bgndImage.width, opts.bgndImage.height,
                      opts.bgndImage.pos, opts.bgndImage.onBorder);
//     printf("SET WO BGND \"%s\"  \"%s\"", opts.bgndImage.pixmap.file.toLatin1().constData(), bgndImageDlg->fileName().toLatin1().constData());
    bgndPixmapDlg->set(getThemeFile(opts.bgndPixmap.file));
    menuBgndImageDlg->set(getThemeFile(opts.menuBgndImage.pixmap.file), opts.menuBgndImage.width, opts.menuBgndImage.height,
                          opts.menuBgndImage.pos);
//     printf("SET WO MENU \"%s\"  \"%s\"", opts.menuBgndImage.pixmap.file.toLatin1().constData(), menuBgndImageDlg->fileName().toLatin1().constData());
    menuBgndPixmapDlg->set(getThemeFile(opts.menuBgndPixmap.file));
}

int QtCurveConfig::getDwtSettingsFlags()
{
    int dwt(0);

    if(dwtBtnAsPerTitleBar->isChecked())
        dwt|=DWT_BUTTONS_AS_PER_TITLEBAR;
    if(dwtColAsPerTitleBar->isChecked())
        dwt|=DWT_COLOR_AS_PER_TITLEBAR;
    if(dwtIconColAsPerTitleBar->isChecked())
        dwt|=DWT_ICON_COLOR_AS_PER_TITLEBAR;
    if(dwtFontAsPerTitleBar->isChecked())
        dwt|=DWT_FONT_AS_PER_TITLEBAR;
    if(dwtTextAsPerTitleBar->isChecked())
        dwt|=DWT_TEXT_ALIGN_AS_PER_TITLEBAR;
    if(dwtEffectAsPerTitleBar->isChecked())
        dwt|=DWT_EFFECT_AS_PER_TITLEBAR;
    if(dwtRoundTopOnly->isChecked())
        dwt|=DWT_ROUND_TOP_ONLY;
    return dwt;
}

int QtCurveConfig::getSquareFlags()
{
    int square(0);

    if(squareEntry->isChecked())
        square|=SQUARE_ENTRY;
    if(squareProgress->isChecked())
        square|=SQUARE_PROGRESS;
    if(squareScrollViews->isChecked())
        square|=SQUARE_SCROLLVIEW;
    if(squareLvSelection->isChecked())
        square|=SQUARE_LISTVIEW_SELECTION;
    if(squareFrame->isChecked())
        square|=SQUARE_FRAME;
    if(squareTabFrame->isChecked())
        square|=SQUARE_TAB_FRAME;
    if(squareSlider->isChecked())
        square|=SQUARE_SLIDER;
    if(squareScrollbarSlider->isChecked())
        square|=SQUARE_SB_SLIDER;
    if(squareWindows->isChecked())
        square|=SQUARE_WINDOWS;
    if(squareTooltips->isChecked())
        square|=SQUARE_TOOLTIPS;
    if(squarePopupMenus->isChecked())
        square|=SQUARE_POPUP_MENUS;
    return square;
}

int QtCurveConfig::getWindowBorderFlags()
{
    int flags(0);

    if(windowBorder_colorTitlebarOnly->isChecked())
        flags|=WINDOW_BORDER_COLOR_TITLEBAR_ONLY;
    if(windowBorder_menuColor->isChecked())
        flags|=WINDOW_BORDER_USE_MENUBAR_COLOR_FOR_TITLEBAR;
    if(windowBorder_addLightBorder->isChecked())
        flags|=WINDOW_BORDER_ADD_LIGHT_BORDER;
    if(windowBorder_blend->isChecked())
        flags|=WINDOW_BORDER_BLEND_TITLEBAR;
    if(windowBorder_separator->isChecked())
        flags|=WINDOW_BORDER_SEPARATOR;
    if(windowBorder_fill->isChecked())
        flags|=WINDOW_BORDER_FILL_TITLEBAR;
    return flags;
}

int QtCurveConfig::getThinFlags()
{
    int flags(0);

    if(thin_buttons->isChecked())
        flags|=THIN_BUTTONS;
    if(thin_menuitems->isChecked())
        flags|=THIN_MENU_ITEMS;
    if(thin_frames->isChecked())
        flags|=THIN_FRAMES;
    return flags;
}

bool QtCurveConfig::diffTitleBarButtonColors(const Options &opts)
{
    return (titlebarButtons_custom->isChecked() &&
            ( titlebarButtons_colorClose->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_CLOSE) ||
              titlebarButtons_colorMin->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_MIN) ||
              titlebarButtons_colorMax->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_MAX) ||
              titlebarButtons_colorKeepAbove->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_KEEP_ABOVE) ||
              titlebarButtons_colorKeepBelow->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_KEEP_BELOW) ||
              titlebarButtons_colorHelp->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_HELP) ||
              titlebarButtons_colorMenu->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_MENU) ||
              titlebarButtons_colorShade->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_SHADE) ||
              titlebarButtons_colorAllDesktops->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_ALL_DESKTOPS))) ||
            (titlebarButtons_customIcon->isChecked() &&
            ( titlebarButtons_colorCloseIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_CLOSE, 1) ||
              titlebarButtons_colorMinIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_MIN, 1) ||
              titlebarButtons_colorMaxIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_MAX, 1) ||
              titlebarButtons_colorKeepAboveIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_KEEP_ABOVE, 1) ||
              titlebarButtons_colorKeepBelowIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_KEEP_BELOW, 1) ||
              titlebarButtons_colorHelpIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_HELP, 1) ||
              titlebarButtons_colorMenuIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_MENU, 1) ||
              titlebarButtons_colorShadeIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_SHADE, 1) ||
              titlebarButtons_colorAllDesktopsIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_ALL_DESKTOPS, 1) ||
              titlebarButtons_colorCloseInactiveIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_CLOSE, 2) ||
              titlebarButtons_colorMinInactiveIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_MIN, 2) ||
              titlebarButtons_colorMaxInactiveIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_MAX, 2) ||
              titlebarButtons_colorKeepAboveInactiveIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_KEEP_ABOVE, 2) ||
              titlebarButtons_colorKeepBelowInactiveIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_KEEP_BELOW, 2) ||
              titlebarButtons_colorHelpInactiveIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_HELP, 2) ||
              titlebarButtons_colorMenuInactiveIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_MENU, 2) ||
              titlebarButtons_colorShadeInactiveIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_SHADE, 2) ||
              titlebarButtons_colorAllDesktopsInactiveIcon->color()!=getColor(opts.titlebarButtonColors, TITLEBAR_ALL_DESKTOPS, 2)));
}

bool QtCurveConfig::settingsChanged(const Options &opts)
{
    return round->currentIndex()!=opts.round ||
         toolbarBorders->currentIndex()!=opts.toolbarBorders ||
         appearance->currentIndex()!=(int)opts.appearance ||
         focus->currentIndex()!=(int)opts.focus ||
         lvLines->isChecked()!=opts.lvLines ||
         lvButton->isChecked()!=opts.lvButton ||
         drawStatusBarFrames->isChecked()!=opts.drawStatusBarFrames ||
         buttonEffect->currentIndex()!=(EEffect)opts.buttonEffect ||
         coloredMouseOver->currentIndex()!=(int)opts.coloredMouseOver ||
         menubarMouseOver->isChecked()!=opts.menubarMouseOver ||
         shadeMenubarOnlyWhenActive->isChecked()!=opts.shadeMenubarOnlyWhenActive ||
         getThinFlags()!=opts.thin ||
         animatedProgress->isChecked()!=opts.animatedProgress ||
         stripedProgress->currentIndex()!=opts.stripedProgress ||
         lighterPopupMenuBgnd->value()!=opts.lighterPopupMenuBgnd ||
         tabBgnd->value()!=opts.tabBgnd ||
         menuDelay->value()!=opts.menuDelay ||
         sliderWidth->value()!=opts.sliderWidth ||
         menuStripe->currentIndex()!=opts.menuStripe ||
         menuStripeAppearance->currentIndex()!=opts.menuStripeAppearance ||
         bgndGrad->currentIndex()!=opts.bgndGrad ||
         menuBgndGrad->currentIndex()!=opts.menuBgndGrad ||
         embolden->isChecked()!=opts.embolden ||
         fillSlider->isChecked()!=opts.fillSlider ||
         stripedSbar->isChecked()!=opts.stripedSbar ||
         sliderStyle->currentIndex()!=opts.sliderStyle ||
         roundMbTopOnly->isChecked()!=opts.roundMbTopOnly ||
         getHideFlags(menubarHiding_keyboard, menubarHiding_kwin)!=opts.menubarHiding ||
         getHideFlags(statusbarHiding_keyboard, statusbarHiding_kwin)!=opts.statusbarHiding ||
         fillProgress->isChecked()!=opts.fillProgress ||
         glowProgress->currentIndex()!=opts.glowProgress ||
         darkerBorders->isChecked()!=opts.darkerBorders ||
         comboSplitter->isChecked()!=opts.comboSplitter ||
         comboBtn->currentIndex()!=(int)opts.comboBtn ||
         sortedLv->currentIndex()!=(int)opts.sortedLv ||
         unifySpinBtns->isChecked()!=opts.unifySpinBtns ||
         unifySpin->isChecked()!=opts.unifySpin ||
         unifyCombo->isChecked()!=opts.unifyCombo ||
         vArrows->isChecked()!=opts.vArrows ||
         xCheck->isChecked()!=opts.xCheck ||
         hideShortcutUnderline->isChecked()!=opts.hideShortcutUnderline ||
         crHighlight->value()!=opts.crHighlight ||
         expanderHighlight->value()!=opts.expanderHighlight ||
         crButton->isChecked()!=opts.crButton ||
         colorSelTab->value()!=opts.colorSelTab ||
         roundAllTabs->isChecked()!=opts.roundAllTabs ||
         borderTab->isChecked()!=opts.borderTab ||
         borderInactiveTab->isChecked()!=opts.borderInactiveTab ||
         invertBotTab->isChecked()!=opts.invertBotTab ||
         doubleGtkComboArrow->isChecked()!=opts.doubleGtkComboArrow ||
         tabMouseOver->currentIndex()!=opts.tabMouseOver ||
         stdSidebarButtons->isChecked()!=opts.stdSidebarButtons ||
         toolbarTabs->isChecked()!=opts.toolbarTabs ||
         centerTabText->isChecked()!=opts.centerTabText ||
         borderMenuitems->isChecked()!=opts.borderMenuitems ||
         shadePopupMenu->isChecked()!=opts.shadePopupMenu ||
         popupBorder->isChecked()!=opts.popupBorder ||
         defBtnIndicator->currentIndex()!=(int)opts.defBtnIndicator ||
         sliderThumbs->currentIndex()!=(int)opts.sliderThumbs ||
         handles->currentIndex()!=(int)opts.handles ||
         scrollbarType->currentIndex()!=(int)opts.scrollbarType ||
         highlightTab->isChecked()!=opts.highlightTab ||
         shadeSliders->currentIndex()!=(int)opts.shadeSliders ||
         shadeMenubars->currentIndex()!=(int)opts.shadeMenubars ||
         shadeCheckRadio->currentIndex()!=(int)opts.shadeCheckRadio ||
         menubarAppearance->currentIndex()!=opts.menubarAppearance ||
         toolbarAppearance->currentIndex()!=opts.toolbarAppearance ||
         lvAppearance->currentIndex()!=opts.lvAppearance ||
         sliderAppearance->currentIndex()!=opts.sliderAppearance ||
         tabAppearance->currentIndex()!=opts.tabAppearance ||
         activeTabAppearance->currentIndex()!=opts.activeTabAppearance ||
         progressAppearance->currentIndex()!=opts.progressAppearance ||
         progressColor->currentIndex()!=opts.progressColor ||
         progressGrooveAppearance->currentIndex()!=opts.progressGrooveAppearance ||
         grooveAppearance->currentIndex()!=opts.grooveAppearance ||
         sunkenAppearance->currentIndex()!=opts.sunkenAppearance ||
         progressGrooveColor->currentIndex()!=opts.progressGrooveColor ||
         menuitemAppearance->currentIndex()!=opts.menuitemAppearance ||
         menuBgndAppearance->currentIndex()!=opts.menuBgndAppearance ||
         titlebarAppearance->currentIndex()!=opts.titlebarAppearance ||
         inactiveTitlebarAppearance->currentIndex()!=opts.inactiveTitlebarAppearance ||
         titlebarButtonAppearance->currentIndex()!=opts.titlebarButtonAppearance ||
         selectionAppearance->currentIndex()!=opts.selectionAppearance ||
         toolbarSeparators->currentIndex()!=opts.toolbarSeparators ||
         splitters->currentIndex()!=opts.splitters ||
         colorMenubarMouseOver->isChecked()!=opts.colorMenubarMouseOver ||
         useHighlightForMenu->isChecked()!=opts.useHighlightForMenu ||
         getGroupBoxLabelFlags()!=opts.gbLabel ||
         fadeLines->isChecked()!=opts.fadeLines ||
         menuIcons->isChecked()!=opts.menuIcons ||
         stdBtnSizes->isChecked()!=opts.stdBtnSizes ||
         boldProgress->isChecked()!=opts.boldProgress ||
         coloredTbarMo->isChecked()!=opts.coloredTbarMo ||
         tbarBtns->currentIndex()!=opts.tbarBtns ||
         tbarBtnAppearance->currentIndex()!=opts.tbarBtnAppearance ||
         tbarBtnEffect->currentIndex()!=opts.tbarBtnEffect ||
         borderSelection->isChecked()!=opts.borderSelection ||
         forceAlternateLvCols->isChecked()!=opts.forceAlternateLvCols ||
         titlebarAlignment->currentIndex()!=opts.titlebarAlignment ||
         titlebarEffect->currentIndex()!=opts.titlebarEffect ||
         titlebarIcon->currentIndex()!=opts.titlebarIcon ||
         getCrSize(crSize)!=opts.crSize ||
         borderProgress->isChecked()!=opts.borderProgress ||
         shading->currentIndex()!=(int)opts.shading ||
         gtkScrollViews->isChecked()!=opts.gtkScrollViews ||
         highlightScrollViews->isChecked()!=opts.highlightScrollViews ||
         etchEntry->isChecked()!=opts.etchEntry ||
         flatSbarButtons->isChecked()!=opts.flatSbarButtons ||
         borderSbarGroove->isChecked()!=opts.borderSbarGroove ||
         thinSbarGroove->isChecked()!=opts.thinSbarGroove ||
         colorSliderMouseOver->isChecked()!=opts.colorSliderMouseOver ||
         getWindowBorderFlags()!=opts.windowBorder ||
         windowDrag->currentIndex()!=opts.windowDrag ||
         sbarBgndAppearance->currentIndex()!=opts.sbarBgndAppearance ||
         sliderFill->currentIndex()!=opts.sliderFill ||
         bgndAppearance->currentIndex()!=opts.bgndAppearance ||
         bgndImage->currentIndex()!=opts.bgndImage.type ||
         bgndOpacity->value()!=opts.bgndOpacity ||
         dlgOpacity->value()!=opts.dlgOpacity ||
         menuBgndImage->currentIndex()!=opts.menuBgndImage.type ||
         menuBgndOpacity->value()!=opts.menuBgndOpacity ||
         dwtAppearance->currentIndex()!=opts.dwtAppearance ||
         tooltipAppearance->currentIndex()!=opts.tooltipAppearance ||
         crColor->currentIndex()!=opts.crColor ||
         smallRadio->isChecked()!=opts.smallRadio ||
         splitterHighlight->value()!=opts.splitterHighlight ||
         gtkComboMenus->isChecked()!=opts.gtkComboMenus ||
         gtkButtonOrder->isChecked()!=opts.gtkButtonOrder ||
         reorderGtkButtons->isChecked()!=opts.reorderGtkButtons ||
         mapKdeIcons->isChecked()!=opts.mapKdeIcons ||
         groupBox->currentIndex()!=opts.groupBox ||
         ((FRAME_SHADED==opts.groupBox || FRAME_FADED==opts.groupBox) && gbFactor->value()!=opts.gbFactor) ||

         toInt(passwordChar->text())!=opts.passwordChar ||
         highlightFactor->value()!=opts.highlightFactor ||
         getTitleBarButtonFlags()!=opts.titlebarButtons ||

         getDwtSettingsFlags()!=opts.dwtSettings ||
         getSquareFlags()!=opts.square ||

         diffTitleBarButtonColors(opts) ||

         customMenuTextColor->isChecked()!=opts.customMenuTextColor ||
         (SHADE_CUSTOM==opts.shadeSliders &&
               customSlidersColor->color()!=opts.customSlidersColor) ||
         (SHADE_CUSTOM==opts.shadeMenubars &&
               customMenubarsColor->color()!=opts.customMenubarsColor) ||
         (SHADE_CUSTOM==opts.shadeCheckRadio &&
               customCheckRadioColor->color()!=opts.customCheckRadioColor) ||
         (customMenuTextColor->isChecked() &&
               customMenuNormTextColor->color()!=opts.customMenuNormTextColor) ||
         (customMenuTextColor->isChecked() &&
               customMenuSelTextColor->color()!=opts.customMenuSelTextColor) ||
         (SHADE_CUSTOM==opts.menuStripe &&
               customMenuStripeColor->color()!=opts.customMenuStripeColor) ||
         (SHADE_CUSTOM==opts.comboBtn &&
               customComboBtnColor->color()!=opts.customComboBtnColor) ||
         (SHADE_CUSTOM==opts.sortedLv &&
               customSortedLvColor->color()!=opts.customSortedLvColor) ||
         (SHADE_CUSTOM==opts.crColor &&
               customCrBgndColor->color()!=opts.customCrBgndColor) ||
         (SHADE_CUSTOM==opts.progressColor &&
               customProgressColor->color()!=opts.customProgressColor) ||

         customGradient!=opts.customGradient ||

         toSet(noBgndGradientApps->text())!=opts.noBgndGradientApps ||
         toSet(noBgndOpacityApps->text())!=opts.noBgndOpacityApps ||
         toSet(noMenuBgndOpacityApps->text())!=opts.noMenuBgndOpacityApps ||
         toSet(noBgndImageApps->text())!=opts.noBgndImageApps ||
         toSet(useQtFileDialogApps->text())!=opts.useQtFileDialogApps ||
         toSet(menubarApps->text())!=opts.menubarApps ||
         toSet(statusbarApps->text())!=opts.statusbarApps ||
         toSet(noMenuStripeApps->text())!=opts.noMenuStripeApps ||

         diffShades(opts) ||

         diffImages(opts);
}

#include "qtcurveconfig.moc"
