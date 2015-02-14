/*****************************************************************************
 *   Copyright 2010 Craig Drummond <craig.p.drummond@gmail.com>              *
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

#include "imagepropertiesdialog.h"
#include <KLocale>
#include <KUrlRequester>
#include <KFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <common/common.h>

#define MIN_SIZE 16
#define MAX_SIZE 1024
#define DEF_SIZE 256

static void insertPosEntries(QComboBox *combo)
{
    combo->insertItem(PP_TL, i18n("Top left"));
    combo->insertItem(PP_TM, i18n("Top middle"));
    combo->insertItem(PP_TR, i18n("Top right"));
    combo->insertItem(PP_BL, i18n("Bottom left"));
    combo->insertItem(PP_BM, i18n("Bottom middle"));
    combo->insertItem(PP_BR, i18n("Bottom right"));
    combo->insertItem(PP_LM, i18n("Left middle"));
    combo->insertItem(PP_RM, i18n("Right middle"));
    combo->insertItem(PP_CENTRED, i18n("Centred"));
}

CImagePropertiesDialog::CImagePropertiesDialog(const QString &title, QWidget *parent, int props)
                      : KDialog(parent)
                      , properties(props)
{
    QWidget *page = new QWidget(this);

    setButtons(Ok|Cancel);
    setDefaultButton(Ok);
    setupUi(page);
    setMainWidget(page);
    setCaption(i18n("Edit %1", title));
    fileRequester->setMode(KFile::File|KFile::ExistingOnly|KFile::LocalOnly);
    fileRequester->setFilter("image/svg+xml image/png image/jpeg image/bmp image/gif image/xpixmap");

    if(props&SCALE)
    {
        scaleWidth->setRange(MIN_SIZE, MAX_SIZE);
        scaleHeight->setRange(MIN_SIZE, MAX_SIZE);
    }

    if(props&POS)
        insertPosEntries(posCombo);

    scaleControls->setVisible(props&SCALE);
    scaleImage->setVisible(props&SCALE);
    onBorder->setVisible(props&BORDER);
    onBorderLabel->setVisible(props&BORDER);
    posCombo->setVisible(props&POS);
    posLabel->setVisible(props&POS);

    set(QString(), DEF_SIZE, DEF_SIZE, PP_TR, false);
}

bool CImagePropertiesDialog::run()
{
    QString oldFile=fileName();
    int     oldWidth=imgWidth(),
            oldHeight=imgHeight(),
            oldPos=imgPos();
    bool    oldOnBorder=onBorder->isChecked();

    if(QDialog::Accepted==exec())
        return true;

    set(oldFile, oldWidth, oldHeight, oldPos, oldOnBorder);
    return false;
}

void CImagePropertiesDialog::set(const QString &file, int width, int height, int pos, bool onWindowBorder)
{
    if(properties&SCALE)
    {
        scaleImage->setChecked(0!=width || 0!=height);
        scaleWidth->setValue(width<MIN_SIZE || width>MAX_SIZE ? DEF_SIZE : width);
        scaleHeight->setValue(height<MIN_SIZE || height>MAX_SIZE ? DEF_SIZE : height);
    }

    if(properties&BORDER)
        onBorder->setChecked(onWindowBorder);
    if(properties&POS)
        posCombo->setCurrentIndex(pos);

    fileRequester->setUrl(QFile::exists(file) && !QFileInfo(file).isDir() ?
                          QUrl(file) : QUrl());
}

QSize CImagePropertiesDialog::sizeHint() const
{
    return QSize(400, 120);
}
