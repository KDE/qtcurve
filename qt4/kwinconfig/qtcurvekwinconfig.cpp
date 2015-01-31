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

#include <kconfig.h>
#include <klocale.h>
#include <kglobal.h>
#include <kdeversion.h>
#include <QDBusConnection>
#include "config.h"
#include <common/common.h>
#include "qtcurvekwinconfig.h"

static void insertColorEntries(QComboBox *combo)
{
    combo->insertItem(QtCurve::KWin::ShadowConfig::CT_FOCUS, i18n("Focus"));
    combo->insertItem(QtCurve::KWin::ShadowConfig::CT_HOVER, i18n("Hover"));
    combo->insertItem(QtCurve::KWin::ShadowConfig::CT_SELECTION, i18n("Selection Background"));
    combo->insertItem(QtCurve::KWin::ShadowConfig::CT_TITLEBAR, i18n("Titlebar"));
    combo->insertItem(QtCurve::KWin::ShadowConfig::CT_GRAY, i18n("Gray"));
    combo->insertItem(QtCurve::KWin::ShadowConfig::CT_CUSTOM, i18n("Custom:"));
}

static void insertSizeEntries(QComboBox *combo)
{
    combo->insertItem(QtCurve::KWin::QtCurveConfig::BORDER_NONE, i18n("No Border"));
    combo->insertItem(QtCurve::KWin::QtCurveConfig::BORDER_NO_SIDES, i18n("No Side Border"));
    combo->insertItem(QtCurve::KWin::QtCurveConfig::BORDER_TINY, i18n("Tiny"));
    combo->insertItem(QtCurve::KWin::QtCurveConfig::BORDER_NORMAL, i18n("Normal"));
    combo->insertItem(QtCurve::KWin::QtCurveConfig::BORDER_LARGE, i18n("Large"));
    combo->insertItem(QtCurve::KWin::QtCurveConfig::BORDER_VERY_LARGE, i18n("Very Large"));
    combo->insertItem(QtCurve::KWin::QtCurveConfig::BORDER_HUGE, i18n("Huge"));
    combo->insertItem(QtCurve::KWin::QtCurveConfig::BORDER_VERY_HUGE, i18n("Very Huge"));
    combo->insertItem(QtCurve::KWin::QtCurveConfig::BORDER_OVERSIZED, i18n("Oversized"));
}

static void insertShadeEntries(QComboBox *combo)
{
    combo->insertItem(QtCurve::KWin::QtCurveConfig::SHADE_NONE, i18n("None"));
    combo->insertItem(QtCurve::KWin::QtCurveConfig::SHADE_DARK, i18n("Dark"));
    combo->insertItem(QtCurve::KWin::QtCurveConfig::SHADE_LIGHT, i18n("Light"));
    combo->insertItem(QtCurve::KWin::QtCurveConfig::SHADE_SHADOW, i18n("Shadow"));
}

static const char * constDBusService="org.kde.kcontrol.QtCurve";

QtCurveKWinConfig::QtCurveKWinConfig(KConfig *config, QWidget *parent)
                 : QWidget(parent)
                 , m_activeShadows(QPalette::Active)
                 , m_inactiveShadows(QPalette::Inactive)
{
    Q_UNUSED(config);

    KGlobal::locale()->insertCatalog("qtcurve");
    KGlobal::locale()->insertCatalog("kwin_clients");

    if(!QDBusConnection::sessionBus().registerService(constDBusService))
    {
        m_ok=false;
        QBoxLayout *layout=new QBoxLayout(QBoxLayout::TopToBottom, this);
        layout->addWidget(new QLabel(i18n("<h3>Already Open</h3><p>Another QtCurve configuration dialog is already open. "
                                          "Please close the other before proceeding."), this));
    }
    else
    {
        m_ok=true;

        setupUi(this);

        insertSizeEntries(borderSize);
        insertColorEntries(activeShadowColorType);
        insertColorEntries(inactiveShadowColorType);
        insertShadeEntries(outerBorder);
        insertShadeEntries(innerBorder);

        load(0L);
        connect(borderSize, SIGNAL(currentIndexChanged(int)), this, SLOT(sizeChanged()));
        connect(roundBottom, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
        connect(outerBorder, SIGNAL(currentIndexChanged(int)), this, SLOT(outerBorderChanged()));
        connect(innerBorder, SIGNAL(currentIndexChanged(int)), this, SLOT(innerBorderChanged()));
        connect(borderlessMax, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
        connect(titleBarPad, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
        connect(edgePad, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
        titleBarPad->setRange(-5, 10);
        edgePad->setRange(0, 10);
        connect(useShadows, SIGNAL(toggled(bool)), this, SLOT(shadowsChanged()));
        connect(activeShadowSize, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
        connect(activeShadowHOffset, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
        connect(activeShadowVOffset, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
        connect(activeShadowColorType, SIGNAL(currentIndexChanged(int)), this, SLOT(activeShadowColorTypeChanged()));
        connect(activeShadowColor, SIGNAL(changed(const QColor &)), this, SIGNAL(changed()));
        connect(inactiveShadowSize, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
        connect(inactiveShadowHOffset, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
        connect(inactiveShadowVOffset, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
        connect(inactiveShadowColorType, SIGNAL(currentIndexChanged(int)), this, SLOT(inactiveShadowColorTypeChanged()));
        connect(inactiveShadowColor, SIGNAL(changed(const QColor &)), this, SIGNAL(changed()));
        connect(inactiveUsesActiveGradients, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
        activeShadowColorTypeChanged();
        inactiveShadowColorTypeChanged();
        activeShadowSize->setRange(QtCurve::KWin::ShadowConfig::MIN_SIZE,
                                   QtCurve::KWin::ShadowConfig::MAX_SIZE);
        inactiveShadowSize->setRange(QtCurve::KWin::ShadowConfig::MIN_SIZE,
                                     QtCurve::KWin::ShadowConfig::MAX_SIZE);
        activeShadowHOffset->setRange(QtCurve::KWin::ShadowConfig::MIN_OFFSET,
                                      QtCurve::KWin::ShadowConfig::MAX_OFFSET);
        inactiveShadowHOffset->setRange(QtCurve::KWin::ShadowConfig::MIN_OFFSET,
                                        QtCurve::KWin::ShadowConfig::MAX_OFFSET);
        activeShadowVOffset->setRange(QtCurve::KWin::ShadowConfig::MIN_OFFSET,
                                      QtCurve::KWin::ShadowConfig::MAX_OFFSET);
        inactiveShadowVOffset->setRange(QtCurve::KWin::ShadowConfig::MIN_OFFSET,
                                        QtCurve::KWin::ShadowConfig::MAX_OFFSET);
        setShadows();

        // TODO: remove
        grouping->setVisible(false);
        groupingLabel->setVisible(false);
        connect(activeOpacity, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
        connect(inactiveOpacity, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
        connect(opaqueBorder, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    }
}

QtCurveKWinConfig::~QtCurveKWinConfig()
{
    if(m_ok)
        QDBusConnection::sessionBus().unregisterService(constDBusService);
}

void QtCurveKWinConfig::load(const KConfigGroup &)
{
    load(0L);
}

void QtCurveKWinConfig::load(KConfig *c)
{
    if(!m_ok)
        return;

    KConfig *cfg=c ? c : new KConfig("kwinqtcurverc");

    m_activeShadows.load(cfg);
    m_inactiveShadows.load(cfg);
    setShadows();
    QtCurve::KWin::QtCurveConfig config;

    config.load(cfg, c ? KWIN_GROUP : 0L);
    setWidgets(config);
    if(!c)
        delete cfg;
}

void QtCurveKWinConfig::save(KConfigGroup &)
{
    save(0L);
}

void QtCurveKWinConfig::save(KConfig *c)
{
    if(!m_ok)
        return;

    KConfig *cfg=c ? c : new KConfig("kwinqtcurverc");

    QtCurve::KWin::QtCurveConfig config;

    config.setBorderSize((QtCurve::KWin::QtCurveConfig::Size)borderSize->currentIndex());
    config.setRoundBottom(roundBottom->isChecked());
    config.setOuterBorder((QtCurve::KWin::QtCurveConfig::Shade)outerBorder->currentIndex());
    config.setInnerBorder((QtCurve::KWin::QtCurveConfig::Shade)innerBorder->currentIndex());
    config.setBorderlessMax(borderlessMax->isChecked());
    config.setTitleBarPad(titleBarPad->value());
    config.setEdgePad(edgePad->value());

    config.setCustomShadows(useShadows->isChecked());
    if(useShadows->isChecked())
    {
        m_activeShadows.setShadowSize(activeShadowSize->value());
        m_activeShadows.setHorizontalOffset(activeShadowHOffset->value());
        m_activeShadows.setVerticalOffset(activeShadowVOffset->value());
        m_activeShadows.setColorType((QtCurve::KWin::ShadowConfig::ColorType)activeShadowColorType->currentIndex());
        if(QtCurve::KWin::ShadowConfig::CT_CUSTOM==activeShadowColorType->currentIndex())
            m_activeShadows.setColor(activeShadowColor->color());
        m_inactiveShadows.setShadowSize(inactiveShadowSize->value());
        m_inactiveShadows.setHorizontalOffset(inactiveShadowHOffset->value());
        m_inactiveShadows.setVerticalOffset(inactiveShadowVOffset->value());
        m_inactiveShadows.setColorType((QtCurve::KWin::ShadowConfig::ColorType)inactiveShadowColorType->currentIndex());
        m_inactiveShadows.setShadowType(inactiveUsesActiveGradients->isChecked()
                                            ? QtCurve::KWin::ShadowConfig::SH_ACTIVE
                                            : QtCurve::KWin::ShadowConfig::SH_INACTIVE);
        if(QtCurve::KWin::ShadowConfig::CT_CUSTOM==inactiveShadowColorType->currentIndex())
            m_inactiveShadows.setColor(inactiveShadowColor->color());
    }
    else
    {
        m_activeShadows.defaults();
        m_inactiveShadows.defaults();
    }
    m_activeShadows.save(cfg);
    m_inactiveShadows.save(cfg);
    config.setOpacity(activeOpacity->value(), true);
    config.setOpacity(inactiveOpacity->value(), false);
    config.setOpaqueBorder(opaqueBorder->isChecked());
    config.save(cfg, c ? KWIN_GROUP : 0L);
    cfg->sync();
    if(!c)
        delete cfg;
}

void QtCurveKWinConfig::defaults()
{
    if(!m_ok)
        return;

    setWidgets(QtCurve::KWin::QtCurveConfig());
    m_activeShadows.defaults();
    m_inactiveShadows.defaults();
    setShadows();
}

void QtCurveKWinConfig::outerBorderChanged()
{
    if(QtCurve::KWin::QtCurveConfig::SHADE_NONE==outerBorder->currentIndex())
        innerBorder->setCurrentIndex(QtCurve::KWin::QtCurveConfig::SHADE_NONE);
    if(QtCurve::KWin::QtCurveConfig::SHADE_SHADOW==outerBorder->currentIndex())
        useShadows->setChecked(true);
    setWidgetStates();
    emit changed();
}

void QtCurveKWinConfig::innerBorderChanged()
{
    if(QtCurve::KWin::QtCurveConfig::SHADE_NONE!=innerBorder->currentIndex() &&
       QtCurve::KWin::QtCurveConfig::SHADE_NONE==outerBorder->currentIndex())
        outerBorder->setCurrentIndex(innerBorder->currentIndex());
    if(QtCurve::KWin::QtCurveConfig::SHADE_SHADOW==outerBorder->currentIndex())
        useShadows->setChecked(true);
    emit changed();
}

void QtCurveKWinConfig::shadowsChanged()
{
    if(!useShadows->isChecked())
    {
        if(QtCurve::KWin::QtCurveConfig::SHADE_SHADOW==outerBorder->currentIndex())
            outerBorder->setCurrentIndex(QtCurve::KWin::QtCurveConfig::SHADE_DARK);
        if(QtCurve::KWin::QtCurveConfig::SHADE_SHADOW==innerBorder->currentIndex())
            innerBorder->setCurrentIndex(QtCurve::KWin::QtCurveConfig::SHADE_DARK);
    }
    emit changed();
}

void QtCurveKWinConfig::activeShadowColorTypeChanged()
{
    activeShadowColor->setEnabled(QtCurve::KWin::ShadowConfig::CT_CUSTOM==activeShadowColorType->currentIndex());
    if(m_activeShadows.colorType()!=activeShadowColorType->currentIndex())
        emit changed();
}

void QtCurveKWinConfig::inactiveShadowColorTypeChanged()
{
    inactiveShadowColor->setEnabled(QtCurve::KWin::ShadowConfig::CT_CUSTOM==inactiveShadowColorType->currentIndex());
    if(m_inactiveShadows.colorType()!=inactiveShadowColorType->currentIndex())
        emit changed();
}

void QtCurveKWinConfig::sizeChanged()
{
    setWidgetStates();
    emit changed();
}

void QtCurveKWinConfig::setWidgets(const QtCurve::KWin::QtCurveConfig &cfg)
{
    borderSize->setCurrentIndex(cfg.borderSize());
    roundBottom->setChecked(cfg.roundBottom());
    outerBorder->setCurrentIndex(cfg.outerBorder());
    innerBorder->setCurrentIndex(cfg.innerBorder());
    borderlessMax->setChecked(cfg.borderlessMax());
    titleBarPad->setValue(cfg.titleBarPad());
    edgePad->setValue(cfg.edgePad());
    useShadows->setChecked(cfg.customShadows());
    activeOpacity->setValue(cfg.opacity(true));
    inactiveOpacity->setValue(cfg.opacity(false));
    opaqueBorder->setChecked(cfg.opaqueBorder());
    setWidgetStates();
}

void QtCurveKWinConfig::setWidgetStates()
{
    if(QtCurve::KWin::QtCurveConfig::BORDER_NONE==borderSize->currentIndex())
    {
        roundBottom->setEnabled(false);
        roundBottom->setChecked(false);
    }
    else
        roundBottom->setEnabled(true);

    if(QtCurve::KWin::QtCurveConfig::SHADE_NONE==outerBorder->currentIndex() ||
       borderSize->currentIndex()<QtCurve::KWin::QtCurveConfig::BORDER_TINY)
    {
        innerBorder->setEnabled(false);
        innerBorder->setCurrentIndex(QtCurve::KWin::QtCurveConfig::SHADE_NONE);
    }
    else
        innerBorder->setEnabled(true);
}

void QtCurveKWinConfig::setNote(const QString &txt)
{
    noteLabel->setText(txt);
}

void QtCurveKWinConfig::setShadows()
{
    activeShadowSize->setValue(m_activeShadows.shadowSize());
    activeShadowHOffset->setValue(m_activeShadows.horizontalOffset());
    activeShadowVOffset->setValue(m_activeShadows.verticalOffset());
    activeShadowColor->setColor(m_activeShadows.color());
    activeShadowColorType->setCurrentIndex(m_activeShadows.colorType());
    inactiveShadowSize->setValue(m_inactiveShadows.shadowSize());
    inactiveShadowHOffset->setValue(m_inactiveShadows.horizontalOffset());
    inactiveShadowVOffset->setValue(m_inactiveShadows.verticalOffset());
    inactiveShadowColor->setColor(m_inactiveShadows.color());
    inactiveShadowColorType->setCurrentIndex(m_inactiveShadows.colorType());
    inactiveUsesActiveGradients->setChecked(QtCurve::KWin::ShadowConfig::SH_ACTIVE==m_inactiveShadows.shadowType());
}

extern "C"
{
    KDE_EXPORT QObject * allocate_config(KConfig *config, QWidget *parent)
    {
        return new QtCurveKWinConfig(config, parent);
    }
}

#include "qtcurvekwinconfig.moc"
