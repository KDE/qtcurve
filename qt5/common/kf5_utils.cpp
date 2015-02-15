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

#include "kf5_utils.h"

#include <qtcurve-utils/qtutils.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QString>
#include <QValidator>
#include <QLabel>
#include <QLineEdit>

namespace QtCurve {

QDialogButtonBox*
createDialogButtonBox(QDialog *dialog)
{
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                          QDialogButtonBox::Cancel);
    auto okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    QObject::connect(qtcSlot(buttonBox, accepted), qtcSlot(dialog, accept));
    QObject::connect(qtcSlot(buttonBox, rejected), qtcSlot(dialog, reject));
    return buttonBox;
}

InputDialog::InputDialog(QWidget *parent, Qt::WindowFlags flags)
    : QDialog(parent),
      m_validator(nullptr)
{
    if (flags != 0)
        setWindowFlags(flags);
    auto l = new QVBoxLayout(this);
    m_label = new QLabel(this);

    m_text = new QLineEdit(this);
    connect(m_text, &QLineEdit::textChanged, this, &InputDialog::checkValid);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                       QDialogButtonBox::Cancel,
                                       Qt::Horizontal, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted,
            this, &InputDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this, &InputDialog::reject);

    l->addWidget(m_label);
    l->addWidget(m_text);
    l->addWidget(m_buttonBox);
}

void
InputDialog::setTitle(const QString &title)
{
    setWindowTitle(title);
}

void
InputDialog::setLabelText(const QString &label)
{
    m_label->setText(label);
}

void
InputDialog::setText(const QString &text)
{
    m_text->setText(text);
}

void
InputDialog::setValidator(QValidator *validator)
{
    m_validator = validator;
    m_text->setValidator(validator);
    checkValid(m_text->text());
}

QString
InputDialog::getLabelText()
{
    return m_label->text();
}

QString
InputDialog::getText()
{
    return m_text->text();
}

void
InputDialog::checkValid(const QString &_text)
{
    if (!m_validator)
        return;
    QString text = QString(_text);
    int pos = 0;
    bool valid = (m_validator->validate(text, pos) == QValidator::Acceptable);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

QString
InputDialog::getText(QWidget *parent, const QString &title,
                     const QString &label, const QString &text,
                     QValidator *validator, bool *ok, Qt::WindowFlags flags)
{
    InputDialog *r = new InputDialog(parent, flags);
    r->setTitle(title);
    r->setLabelText(label);
    r->setText(text);
    r->setValidator(validator);
    bool _ok = r->exec() == QDialog::Accepted;
    if (ok) {
        *ok = _ok;
    }
    if (_ok) {
        return r->getText();
    } else {
        return QString();
    }
}

}
