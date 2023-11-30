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

#include "config.h"

#include "qtcurvekwinconfig.h"

#include <common/common.h>

#include <qtcurve-utils/qtutils.h>

#include <KConfigCore/KConfig>

#include <QDBusConnection>

namespace QtCurve {

using namespace KWin;

static void
insertColorEntries(QComboBox *combo)
{
    combo->insertItem(ShadowConfig::CT_FOCUS, i18n("Focus"));
    combo->insertItem(ShadowConfig::CT_HOVER, i18n("Hover"));
    combo->insertItem(ShadowConfig::CT_SELECTION, i18n("Selection Background"));
    combo->insertItem(ShadowConfig::CT_TITLEBAR, i18n("Titlebar"));
    combo->insertItem(ShadowConfig::CT_GRAY, i18n("Gray"));
    combo->insertItem(ShadowConfig::CT_CUSTOM, i18n("Custom:"));
}

static void insertSizeEntries(QComboBox *combo)
{
    combo->insertItem(KWin::QtCurveConfig::BORDER_NONE, i18n("No Border"));
    combo->insertItem(KWin::QtCurveConfig::BORDER_NO_SIDES, i18n("No Side Border"));
    combo->insertItem(KWin::QtCurveConfig::BORDER_TINY, i18n("Tiny"));
    combo->insertItem(KWin::QtCurveConfig::BORDER_NORMAL, i18n("Normal"));
    combo->insertItem(KWin::QtCurveConfig::BORDER_LARGE, i18n("Large"));
    combo->insertItem(KWin::QtCurveConfig::BORDER_VERY_LARGE, i18n("Very Large"));
    combo->insertItem(KWin::QtCurveConfig::BORDER_HUGE, i18n("Huge"));
    combo->insertItem(KWin::QtCurveConfig::BORDER_VERY_HUGE, i18n("Very Huge"));
    combo->insertItem(KWin::QtCurveConfig::BORDER_OVERSIZED, i18n("Oversized"));
}

static void insertShadeEntries(QComboBox *combo)
{
    combo->insertItem(KWin::QtCurveConfig::SHADE_NONE, i18n("None"));
    combo->insertItem(KWin::QtCurveConfig::SHADE_DARK, i18n("Dark"));
    combo->insertItem(KWin::QtCurveConfig::SHADE_LIGHT, i18n("Light"));
    combo->insertItem(KWin::QtCurveConfig::SHADE_SHADOW, i18n("Shadow"));
}

static const char *constDBusService = "org.kde.kcontrol.QtCurve";

KWinConfig::KWinConfig(KConfig *config, QWidget *parent)
                 : QWidget(parent)
                 , m_activeShadows(QPalette::Active)
                 , m_inactiveShadows(QPalette::Inactive)
{
    Q_UNUSED(config);

    if (!QDBusConnection::sessionBus().registerService(constDBusService)) {
        m_ok = false;
        QBoxLayout *layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
        layout->addWidget(new QLabel(i18n("<h3>Already Open</h3><p>Another "
                                          "QtCurve configuration dialog is "
                                          "already open. Please close the "
                                          "other before proceeding."), this));
    } else {
        m_ok = true;

        setupUi(this);

        insertSizeEntries(borderSize);
        insertColorEntries(activeShadowColorType);
        insertColorEntries(inactiveShadowColorType);
        insertShadeEntries(outerBorder);
        insertShadeEntries(innerBorder);

        load(nullptr);
        connect(qtcSlot(borderSize, currentIndexChanged, (int)),
                qtcSlot(this, sizeChanged));
        connect(qtcSlot(roundBottom, toggled), qtcSlot(this, changed));
        connect(qtcSlot(outerBorder, currentIndexChanged, (int)),
                qtcSlot(this, outerBorderChanged));
        connect(qtcSlot(innerBorder, currentIndexChanged, (int)),
                qtcSlot(this, innerBorderChanged));
        connect(qtcSlot(borderlessMax, toggled), qtcSlot(this, changed));
        connect(qtcSlot(titleBarPad, valueChanged, (int)),
                qtcSlot(this, changed));
        connect(qtcSlot(edgePad, valueChanged, (int)), qtcSlot(this, changed));
        titleBarPad->setRange(-5, 10);
        edgePad->setRange(0, 10);
        connect(qtcSlot(useShadows, toggled), qtcSlot(this, shadowsChanged));
        connect(qtcSlot(activeShadowSize, valueChanged, (int)),
                qtcSlot(this, changed));
        connect(qtcSlot(activeShadowHOffset, valueChanged, (int)),
                qtcSlot(this, changed));
        connect(qtcSlot(activeShadowVOffset, valueChanged, (int)),
                qtcSlot(this, changed));
        connect(qtcSlot(activeShadowColorType, currentIndexChanged, (int)),
                qtcSlot(this, activeShadowColorTypeChanged));
        connect(qtcSlot(activeShadowColor, changed), qtcSlot(this, changed));
        connect(qtcSlot(inactiveShadowSize, valueChanged, (int)),
                qtcSlot(this, changed));
        connect(qtcSlot(inactiveShadowHOffset, valueChanged, (int)),
                qtcSlot(this, changed));
        connect(qtcSlot(inactiveShadowVOffset, valueChanged, (int)),
                qtcSlot(this, changed));
        connect(qtcSlot(inactiveShadowColorType, currentIndexChanged, (int)),
                qtcSlot(this, inactiveShadowColorTypeChanged));
        connect(qtcSlot(inactiveShadowColor, changed), qtcSlot(this, changed));
        connect(qtcSlot(inactiveUsesActiveGradients, toggled),
                qtcSlot(this, changed));
        activeShadowColorTypeChanged();
        inactiveShadowColorTypeChanged();
        activeShadowSize->setRange(ShadowConfig::MIN_SIZE,
                                   ShadowConfig::MAX_SIZE);
        inactiveShadowSize->setRange(ShadowConfig::MIN_SIZE,
                                     ShadowConfig::MAX_SIZE);
        activeShadowHOffset->setRange(ShadowConfig::MIN_OFFSET,
                                      ShadowConfig::MAX_OFFSET);
        inactiveShadowHOffset->setRange(ShadowConfig::MIN_OFFSET,
                                        ShadowConfig::MAX_OFFSET);
        activeShadowVOffset->setRange(ShadowConfig::MIN_OFFSET,
                                      ShadowConfig::MAX_OFFSET);
        inactiveShadowVOffset->setRange(ShadowConfig::MIN_OFFSET,
                                        ShadowConfig::MAX_OFFSET);
        setShadows();

        // TODO: remove
        grouping->setVisible(false);
        groupingLabel->setVisible(false);
        connect(qtcSlot(activeOpacity, valueChanged, (int)),
                qtcSlot(this, changed));
        connect(qtcSlot(inactiveOpacity, valueChanged, (int)),
                qtcSlot(this, changed));
        connect(qtcSlot(opaqueBorder, toggled), qtcSlot(this, changed));
    }
}

KWinConfig::~KWinConfig()
{
    if (m_ok) {
        QDBusConnection::sessionBus().unregisterService(constDBusService);
    }
}

void KWinConfig::load(KConfig *c)
{
    if (!m_ok)
        return;

    KConfig *cfg=c ? c : new KConfig("kwinqtcurverc");

    m_activeShadows.load(cfg);
    m_inactiveShadows.load(cfg);
    setShadows();
    KWin::QtCurveConfig config;

    config.load(cfg, c ? KWIN_GROUP : nullptr);
    setWidgets(config);
    if(!c)
        delete cfg;
}

void KWinConfig::save(KConfig *c)
{
    if (!m_ok)
        return;

    KConfig *cfg=c ? c : new KConfig("kwinqtcurverc");

    KWin::QtCurveConfig config;

    config.setBorderSize((KWin::QtCurveConfig::Size)borderSize->currentIndex());
    config.setRoundBottom(roundBottom->isChecked());
    config.setOuterBorder((KWin::QtCurveConfig::Shade)outerBorder->currentIndex());
    config.setInnerBorder((KWin::QtCurveConfig::Shade)innerBorder->currentIndex());
    config.setBorderlessMax(borderlessMax->isChecked());
    config.setTitleBarPad(titleBarPad->value());
    config.setEdgePad(edgePad->value());

    config.setCustomShadows(useShadows->isChecked());
    if(useShadows->isChecked())
    {
        m_activeShadows.setShadowSize(activeShadowSize->value());
        m_activeShadows.setHorizontalOffset(activeShadowHOffset->value());
        m_activeShadows.setVerticalOffset(activeShadowVOffset->value());
        m_activeShadows.setColorType((ShadowConfig::ColorType)activeShadowColorType->currentIndex());
        if(ShadowConfig::CT_CUSTOM==activeShadowColorType->currentIndex())
            m_activeShadows.setColor(activeShadowColor->color());
        m_inactiveShadows.setShadowSize(inactiveShadowSize->value());
        m_inactiveShadows.setHorizontalOffset(inactiveShadowHOffset->value());
        m_inactiveShadows.setVerticalOffset(inactiveShadowVOffset->value());
        m_inactiveShadows.setColorType((ShadowConfig::ColorType)inactiveShadowColorType->currentIndex());
        m_inactiveShadows.setShadowType(inactiveUsesActiveGradients->isChecked()
                                            ? ShadowConfig::SH_ACTIVE
                                            : ShadowConfig::SH_INACTIVE);
        if(ShadowConfig::CT_CUSTOM==inactiveShadowColorType->currentIndex())
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
    config.save(cfg, c ? KWIN_GROUP : nullptr);
    cfg->sync();
    if(!c)
        delete cfg;
}

void KWinConfig::defaults()
{
    if (!m_ok)
        return;

    setWidgets(KWin::QtCurveConfig());
    m_activeShadows.defaults();
    m_inactiveShadows.defaults();
    setShadows();
}

void KWinConfig::outerBorderChanged()
{
    if(KWin::QtCurveConfig::SHADE_NONE==outerBorder->currentIndex())
        innerBorder->setCurrentIndex(KWin::QtCurveConfig::SHADE_NONE);
    if(KWin::QtCurveConfig::SHADE_SHADOW==outerBorder->currentIndex())
        useShadows->setChecked(true);
    setWidgetStates();
    emit changed();
}

void KWinConfig::innerBorderChanged()
{
    if(KWin::QtCurveConfig::SHADE_NONE!=innerBorder->currentIndex() &&
       KWin::QtCurveConfig::SHADE_NONE==outerBorder->currentIndex())
        outerBorder->setCurrentIndex(innerBorder->currentIndex());
    if(KWin::QtCurveConfig::SHADE_SHADOW==outerBorder->currentIndex())
        useShadows->setChecked(true);
    emit changed();
}

void KWinConfig::shadowsChanged()
{
    if(!useShadows->isChecked())
    {
        if(KWin::QtCurveConfig::SHADE_SHADOW==outerBorder->currentIndex())
            outerBorder->setCurrentIndex(KWin::QtCurveConfig::SHADE_DARK);
        if(KWin::QtCurveConfig::SHADE_SHADOW==innerBorder->currentIndex())
            innerBorder->setCurrentIndex(KWin::QtCurveConfig::SHADE_DARK);
    }
    emit changed();
}

void KWinConfig::activeShadowColorTypeChanged()
{
    activeShadowColor->setEnabled(ShadowConfig::CT_CUSTOM==activeShadowColorType->currentIndex());
    if(m_activeShadows.colorType()!=activeShadowColorType->currentIndex())
        emit changed();
}

void KWinConfig::inactiveShadowColorTypeChanged()
{
    inactiveShadowColor->setEnabled(ShadowConfig::CT_CUSTOM==inactiveShadowColorType->currentIndex());
    if(m_inactiveShadows.colorType()!=inactiveShadowColorType->currentIndex())
        emit changed();
}

void KWinConfig::sizeChanged()
{
    setWidgetStates();
    emit changed();
}

void KWinConfig::setWidgets(const KWin::QtCurveConfig &cfg)
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

void KWinConfig::setWidgetStates()
{
    if(KWin::QtCurveConfig::BORDER_NONE==borderSize->currentIndex())
    {
        roundBottom->setEnabled(false);
        roundBottom->setChecked(false);
    }
    else
        roundBottom->setEnabled(true);

    if(KWin::QtCurveConfig::SHADE_NONE==outerBorder->currentIndex() ||
       borderSize->currentIndex()<KWin::QtCurveConfig::BORDER_TINY)
    {
        innerBorder->setEnabled(false);
        innerBorder->setCurrentIndex(KWin::QtCurveConfig::SHADE_NONE);
    }
    else
        innerBorder->setEnabled(true);
}

void KWinConfig::setNote(const QString &txt)
{
    noteLabel->setText(txt);
}

void KWinConfig::setShadows()
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
    inactiveUsesActiveGradients->setChecked(ShadowConfig::SH_ACTIVE==m_inactiveShadows.shadowType());
}

}

extern "C" Q_DECL_EXPORT QObject*
allocate_config(KConfig *config, QWidget *parent)
{
    return new QtCurve::KWinConfig(config, parent);
}

#include "qtcurvekwinconfig.moc"
