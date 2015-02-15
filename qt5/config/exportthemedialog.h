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

#ifndef __EXPORT_THEME_DIALOG_H__
#define __EXPORT_THEME_DIALOG_H__

#include <common/common.h>

#include <QDialog>

class KUrlRequester;
class QLineEdit;

class CExportThemeDialog: public QDialog {
    Q_OBJECT
public:
    CExportThemeDialog(QWidget *parent);

    void run(const Options &o);
    QSize sizeHint() const override;

private:
    void accept() override;

    QLineEdit *themeName;
    QLineEdit *themeComment;
    KUrlRequester *themeUrl;
    Options opts;
};

#endif
