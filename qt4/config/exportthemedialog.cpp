/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
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

#include "exportthemedialog.h"
#include <klocale.h>
#include <kurlrequester.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <QDir>
#include <QGridLayout>
#include <QLabel>
#ifdef Q_OS_MAC
#include "common/config_file.h"
#else
#include "config_file.h"
#endif //Q_OS_MAC

CExportThemeDialog::CExportThemeDialog(QWidget *parent)
                  : KDialog(parent)
{
    QWidget     *page = new QWidget(this);
    QGridLayout *layout = new QGridLayout(page);

    setButtons(Ok|Cancel);
    setDefaultButton(Ok);
    setCaption(i18n("Export Theme"));
    layout->setSpacing(spacingHint());
    layout->setMargin(0);
    layout->addWidget(new QLabel(i18n("Name:"), page), 0, 0);
    layout->addWidget(new QLabel(i18n("Comment:"), page), 1, 0);
    layout->addWidget(new QLabel(i18n("Destination folder:"), page), 2, 0);
    layout->addWidget(themeName=new QLineEdit(page), 0, 1);
    layout->addWidget(themeComment=new QLineEdit(i18n("QtCurve based theme"), page), 1, 1);
    layout->addWidget(themeUrl=new KUrlRequester(page), 2, 1);
    layout->addItem(new QSpacerItem(2, 2, QSizePolicy::Minimum, QSizePolicy::Expanding), 3, 1);

    themeUrl->setMode(KFile::Directory|KFile::ExistingOnly|KFile::LocalOnly);
    themeUrl->lineEdit()->setReadOnly(true);
    themeUrl->setUrl(QDir::homePath());
    setMainWidget(page);
}

void CExportThemeDialog::run(const Options &o)
{
    opts=o;
    exec();
}

QSize CExportThemeDialog::sizeHint() const
{
    return QSize(400, 120);
}

void CExportThemeDialog::slotButtonClicked(int button)
{
    if(Ok==button)
    {
        QString name(themeName->text().trimmed().toLower());

        if(name.isEmpty())
            KMessageBox::error(this, i18n("Name is empty!"));
        else
        {
            QString fileName(themeUrl->url().path()+"/"THEME_PREFIX+name+".themerc");

            KConfig cfg(fileName, KConfig::SimpleConfig);
            bool    rv(cfg.isConfigWritable(false));

            if(rv)
            {
                cfg.group("Misc").writeEntry("Name", themeName->text().trimmed());
                cfg.group("Misc").writeEntry("Comment", themeComment->text());
                cfg.group("KDE").writeEntry("WidgetStyle", THEME_PREFIX+name);

                rv=qtcWriteConfig(&cfg, opts, opts, true);
            }

            if(rv)
            {
                QDialog::accept();
                KMessageBox::information(this, i18n("Successfully created:\n%1", fileName));
            }
            else
                KMessageBox::error(this, i18n("Failed to create file: %1", fileName));
        }
    }
    else
        QDialog::reject();
}
