/*****************************************************************************
 *   Copyright 2010 Craig Drummond <craig.p.drummond@gmail.com>              *
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

#ifndef __IMAGE_PROPERTIES_DIALOG_H__
#define __IMAGE_PROPERTIES_DIALOG_H__

#include <kdialog.h>
#include <kurlrequester.h>

#include <QCheckBox>
#include <QComboBox>

#include "ui_imageproperties.h"

class CImagePropertiesDialog : public KDialog,  public Ui::ImageProperties {
public:
    enum {
        BASIC  = 0x00,
        POS    = 0x01,
        SCALE  = 0x02,
        BORDER = 0x04
    };

    CImagePropertiesDialog(const QString &title, QWidget *parent, int props);

    bool  run();
    void  set(const QString &file, int width=-1, int height=-1, int pos=1, bool onWindowBorder=false);
    QSize sizeHint() const;

    QString fileName()       { return fileRequester->url().toLocalFile(); }
    int     imgWidth()       { return (properties&SCALE) && scaleImage->isChecked() ? scaleWidth->value() : 0; }
    int     imgHeight()      { return (properties&SCALE) && scaleImage->isChecked() ? scaleHeight->value() : 0; }
    int     imgPos()         { return (properties&POS) ? posCombo->currentIndex() : 0; }
    bool    onWindowBorder() { return (properties&BORDER) && onBorder->isChecked(); }

    private:

    int properties;
};

#endif
