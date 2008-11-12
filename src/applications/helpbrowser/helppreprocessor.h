/****************************************************************************
**
** This file is part of the Qtopia Opensource Edition Package.
**
** Copyright (C) 2008 Trolltech ASA.
**
** Contact: Qt Extended Information (info@qtextended.org)
**
** This file may be used under the terms of the GNU General Public License
** versions 2.0 as published by the Free Software Foundation and appearing
** in the file LICENSE.GPL included in the packaging of this file.
**
** Please review the following information to ensure GNU General Public
** Licensing requirements will be met:
**     http://www.fsf.org/licensing/licenses/info/GPLv2.html.
**
**
****************************************************************************/
#ifndef HELPPREPROCESSOR_H
#define HELPPREPROCESSOR_H

#include <QObject>
#include <QStringList>
#include <QMap>
#include <QStack>

class HelpPreProcessor : public QObject
{
    Q_OBJECT
public:
    HelpPreProcessor( const QString &file, int maxrecurs=5 );

    QString text();

private:
    QString parse(const QString& filename);

    QString mFile;
    int levels;

    QString iconSize;

    QStack<QStringList> tests;
    QStack<bool> inverts;
    QMap<QString,QString> replace;
};

#endif
