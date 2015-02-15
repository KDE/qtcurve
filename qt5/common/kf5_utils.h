/*****************************************************************************
 *   Copyright 2015 - 2015 Yichao Yu <yyc1992@gmail.com>                     *
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

#ifndef __COMMON_KF5_UTILS_H__
#define __COMMON_KF5_UTILS_H__

#include <qtcurve-utils/utils.h>

#include <kiconengine.h>
#include <kiconloader.h>

#include <QStandardPaths>
#include <QDir>
#include <QDialog>

class QDialogButtonBox;
class QValidator;
class QLineEdit;
class QLabel;

namespace QtCurve {

QDialogButtonBox *createDialogButtonBox(QDialog *dialog);

class InputDialog : public QDialog {
public:
    explicit InputDialog(QWidget *parent=nullptr, Qt::WindowFlags=0);

    void setTitle(const QString &title);
    void setLabelText(const QString &label);
    void setText(const QString &text);
    void setValidator(QValidator *validator);

    QString getLabelText();
    QString getText();

    static QString getText(QWidget *parent, const QString &title,
                           const QString &label, const QString &text,
                           QValidator *validator=nullptr,
                           bool *ok=nullptr, Qt::WindowFlags flags=0);

private:
    void checkValid(const QString &text);

private:
    QLabel *m_label;
    QLineEdit *m_text;
    QDialogButtonBox *m_buttonBox;
    QValidator *m_validator;
};

static inline QIcon
loadKIcon(const QString &name)
{
    return QIcon(new KIconEngine(name, KIconLoader::global()));
}

// TODO probably merge with utils/dirs
static inline QString
saveLocation(QStandardPaths::StandardLocation type, const QString &suffix)
{
    QString path = QStandardPaths::writableLocation(type);
    QTC_RET_IF_FAIL(!path.isEmpty(), path);
    path += '/' + suffix;
    QDir().mkpath(path);
    return path;
}

static inline QString
qtcSaveDir()
{
    return saveLocation(QStandardPaths::GenericDataLocation, "QtCurve/");
}

}

#endif
