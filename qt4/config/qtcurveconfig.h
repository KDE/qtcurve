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

#ifndef __QTCURVECONFIG_H__
#define __QTCURVECONFIG_H__

#include <ui_qtcurveconfigbase.h>
#include "ui_stylepreview.h"
#include <QMap>
#include <QComboBox>
#include <KDE/KXmlGuiWindow>

#include <common/common.h>

#include <memory>

class QComboBox;
class KDoubleNumInput;
#ifdef QTC_QT4_STYLE_SUPPORT
class CExportThemeDialog;
#endif
class QtCurveConfig;
class QStyle;
class QMdiSubWindow;
class CWorkspace;
class CStylePreview;
class CImagePropertiesDialog;
class KAboutData;
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
    std::unique_ptr<KAboutData> m_aboutData;
    std::unique_ptr<KComponentData> m_componentData;
};

class QtCurveConfig : public QWidget, private Ui::QtCurveConfigBase {
    Q_OBJECT
public:
    QtCurveConfig(QWidget *parent);
    ~QtCurveConfig() override;

    QSize sizeHint() const override;
    Shading currentShading() const
    {
        return (Shading)shading->currentIndex();
    }

Q_SIGNALS:
    void changed(bool);

public Q_SLOTS:
    void save();
    void defaults();

private Q_SLOTS:
    void setPreset();
    void updateChanged();
    void gtkButtonOrderChanged();
    void reorderGtkButtonsChanged();
    void focusChanged();
    void roundChanged();
    void savePreset();
    void deletePreset();
    void importPreset();
    void exportPreset();
    void exportTheme();
    void emboldenToggled();
    void defBtnIndicatorChanged();
    void buttonEffectChanged();
    void coloredMouseOverChanged();
    void shadeSlidersChanged();
    void shadeMenubarsChanged();
    void shadeCheckRadioChanged();
    void customMenuTextColorChanged();
    void menuStripeChanged();
    void shadePopupMenuChanged();
    void progressColorChanged();
    void comboBtnChanged();
    void sortedLvChanged();
    void crColorChanged();
    void stripedProgressChanged();
    void shadingChanged();
    void activeTabAppearanceChanged();
    void tabMoChanged();
    void passwordCharClicked();
    void unifySpinBtnsToggled();
    void unifySpinToggled();
    void sliderThumbChanged();
    void sliderWidthChanged();
    void menubarHidingChanged();
    void xbarChanged();
    void windowBorder_colorTitlebarOnlyChanged();
    void windowBorder_blendChanged();
    void windowBorder_menuColorChanged();
    void thinSbarGrooveChanged();
    void borderSbarGrooveChanged();
    void borderProgressChanged();
    void squareProgressChanged();
    void fillProgressChanged();
    void titlebarButtons_customChanged();
    void titlebarButtons_useHoverChanged();
    void bgndAppearanceChanged();
    void bgndImageChanged();
    void menuBgndAppearanceChanged();
    void menuBgndImageChanged();
    void configureBgndAppearanceFile();
    void configureBgndImageFile();
    void configureMenuBgndAppearanceFile();
    void configureMenuBgndImageFile();
    void groupBoxChanged();
    void changeStack();
    void gradChanged(int i);
    void borderChanged(int i);
    void editItem(QTreeWidgetItem *i, int col);
    void itemChanged(QTreeWidgetItem *i, int col);
    void addGradStop();
    void removeGradStop();
    void updateGradStop();
    void stopSelected();
    void exportKDE3();
    void exportQt();
    void menubarTitlebarBlend();
    void updatePreview();
    void copyGradient(QAction *act);
    void previewControlPressed();

public:
    bool savePreset(const QString &name);
    QString getPresetName(const QString &cap, QString label, QString def,
                          QString name=QString());
    void setupStack();
    void setupPresets(const Options &currentStyle, const Options &defaultStyle);
    void setupPreview();
    void setupGradientsTab();
    void setupShadesTab();
    void setupShade(KDoubleNumInput *w, int shade);
    void setupAlpha(KDoubleNumInput *w, int alpha);
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
    bool
    settingsChanged()
    {
        return settingsChanged(presets[currentText].opts);
    }

private:
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
    KDoubleNumInput        *shadeVals[QTC_NUM_STD_SHADES],
                           *alphaVals[NUM_STD_ALPHAS];
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

#endif
