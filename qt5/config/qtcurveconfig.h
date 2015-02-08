/*****************************************************************************
 *   Copyright 2007 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
 *   Copyright 2013 - 2013 Yichao Yu <yyc1992@gmail.com>                     *
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

#ifndef __QTCURVECONFIG_H__
#define __QTCURVECONFIG_H__

#include <common/common.h>

#include <ui_qtcurveconfigbase.h>
#include "ui_stylepreview.h"

#include <kxmlguiwindow.h>

#include <QMap>
#include <QComboBox>

class QComboBox;
class QDoubleSpinBox;
#ifdef QTC_QT4_STYLE_SUPPORT
class CExportThemeDialog;
#endif
class QtCurveConfig;
class QStyle;
class QMdiSubWindow;
class CWorkspace;
class CStylePreview;
class CImagePropertiesDialog;
class K4AboutData;
class KComponentData;
namespace QtCurve {
class KWinConfig;
}

class CGradientPreview: public QWidget {
    Q_OBJECT
public:
    CGradientPreview(QtCurveConfig *c, QWidget *p);
    ~CGradientPreview();

    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    void paintEvent(QPaintEvent *);
    void setGrad(const Gradient &g);

public Q_SLOTS:
    void setColor(const QColor &col);

private:
    QtCurveConfig *cfg;
    QColor        color;
    Gradient      grad;
    QStyle        *style;
};

struct Preset {
    Preset(const Options &o, const QString &f=QString()):
        loaded(true), opts(o), fileName(f) {}
    Preset(const QString &f=QString()): loaded(false), fileName(f) {}

    bool loaded;
    Options opts;
    QString fileName;
};

class CStylePreview: public KXmlGuiWindow, public Ui::StylePreview {
    Q_OBJECT
public:
    CStylePreview(QWidget *parent = 0);
    ~CStylePreview();

    void closeEvent(QCloseEvent *e);
    QSize sizeHint() const;

Q_SIGNALS:
    void closePressed();

private:
    K4AboutData     *aboutData;
    KComponentData *componentData;
};

class QtCurveConfig: public QWidget, private Ui::QtCurveConfigBase {
    Q_OBJECT

public:
    QtCurveConfig(QWidget *parent);
    ~QtCurveConfig() override;

Q_SIGNALS:
    void changed(bool);

public Q_SLOTS:
    void save();
    void defaults();

private Q_SLOTS:
    void gtkButtonOrderChanged();
    void reorderGtkButtonsChanged();
    void roundChanged();
    void exportTheme();
    void crColorChanged();
    void passwordCharClicked();
    void borderProgressChanged();
    void squareProgressChanged();
    void fillProgressChanged();
    void titlebarButtons_customChanged();
    void titlebarButtons_useHoverChanged();
    void menuBgndImageChanged();
    void configureBgndAppearanceFile();
    void configureBgndImageFile();
    void configureMenuBgndAppearanceFile();
    void configureMenuBgndImageFile();
    void groupBoxChanged();
    void exportKDE3();
    void exportQt();
    void menubarTitlebarBlend();
    void updatePreview();
    void previewControlPressed();

public:
    QString getPresetName(const QString &cap, QString label, QString def,
                          QString name=QString());
    void setupStack();
    void setupPresets(const Options &currentStyle, const Options &defaultStyle);
    void setupPreview();
    void setupGradientsTab();
    void setupShadesTab();
    void setupShade(QDoubleSpinBox *w, int shade);
    void setupAlpha(QDoubleSpinBox *w, int alpha);
    void populateShades(const Options &opts);
    bool diffShades(const Options &opts);
    bool haveImages();
    bool diffImages(const Options &opts);
    void setPasswordChar(int ch);
    int  getTitleBarButtonFlags();
    void setOptions(Options &opts);
    void setWidgetOptions(const Options &opts);
    int  getDwtSettingsFlags();
    int  getSquareFlags();
    int  getWindowBorderFlags();
    int  getGroupBoxLabelFlags();
    int  getThinFlags();
    bool diffTitleBarButtonColors(const Options &opts);
    bool settingsChanged(const Options &opts);
    bool settingsChanged();

    QSize sizeHint() const;
    Shading currentShading() const;

private:
    void activeTabAppearanceChanged();
    void addGradStop();
    void bgndAppearanceChanged();
    void bgndImageChanged();
    void borderChanged(int i);
    void borderSbarGrooveChanged();
    void buttonEffectChanged();
    void changeStack();
    void coloredMouseOverChanged();
    void comboBtnChanged();
    void copyGradient(QAction *act);
    void customMenuTextColorChanged();
    void defBtnIndicatorChanged();
    void deletePreset();
    void editItem(QTreeWidgetItem *i, int col);
    void emboldenToggled();
    void exportPreset();
    void focusChanged();
    void gradChanged(int i);
    void importPreset();
    void itemChanged(QTreeWidgetItem *i, int col);
    void menubarHidingChanged();
    void menuBgndAppearanceChanged();
    void menuStripeChanged();
    void progressColorChanged();
    void removeGradStop();
    void savePreset();
    bool savePreset(const QString &name);
    void setPreset();
    void shadeCheckRadioChanged();
    void shadeMenubarsChanged();
    void shadePopupMenuChanged();
    void shadeSlidersChanged();
    void shadingChanged();
    void sliderThumbChanged();
    void sliderWidthChanged();
    void sortedLvChanged();
    void stopSelected();
    void stripedProgressChanged();
    void tabMoChanged();
    void thinSbarGrooveChanged();
    void unifySpinBtnsToggled();
    void unifySpinToggled();
    void updateChanged();
    void updateGradStop();
    void windowBorder_blendChanged();
    void windowBorder_colorTitlebarOnlyChanged();
    void windowBorder_menuColorChanged();

    Options                previewStyle;
    CWorkspace             *workSpace;
    CStylePreview          *stylePreview;
    QMdiSubWindow          *mdiWindow;
    QMap<QString, Preset>  presets;
#ifdef QTC_QT4_STYLE_SUPPORT
    CExportThemeDialog     *exportDialog;
#endif
    CGradientPreview       *gradPreview;
    GradientCont           customGradient;
    QDoubleSpinBox *shadeVals[QTC_NUM_STD_SHADES];
    QDoubleSpinBox *alphaVals[NUM_STD_ALPHAS];
    QString                currentText,
                           defaultText;
    QtCurve::KWinConfig *kwin;
    int                    kwinPage;
    bool                   readyForPreview;
    CImagePropertiesDialog *bgndPixmapDlg,
                           *menuBgndPixmapDlg,
                           *bgndImageDlg,
                           *menuBgndImageDlg;
};

inline Shading
QtCurveConfig::currentShading() const
{
    return (Shading)shading->currentIndex();
}

inline bool
QtCurveConfig::settingsChanged()
{
    return settingsChanged(presets[currentText].opts);
}

#endif
