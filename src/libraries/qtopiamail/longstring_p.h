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
#ifndef LONGSTRING_H
#define LONGSTRING_H

#include <QString>
#include <QByteArray>
#include <QFile>
#include <QSharedDataPointer>
#include <qtopiaglobal.h>

class QDataStream;
class QTextStream;

class LongStringPrivate;

class QTOPIAMAIL_EXPORT LongString
{
public:
    LongString();
    LongString(const LongString &other);
    LongString(const QByteArray &ba);
    LongString(const QString &fileName);
    virtual ~LongString();

    LongString &operator=(const LongString &);

    bool isEmpty() const;
    int length() const;

    int indexOf(const QByteArray &ba, int from = 0) const;

    LongString mid(int i, int len = -1) const;
    LongString left(int len) const;
    LongString right(int len) const;

    // WARNING the QByteArray returned may become invalid when this LongString
    // is destroyed
    const QByteArray toQByteArray() const;

    QDataStream* dataStream() const;
    QTextStream* textStream() const;

private:
    QSharedDataPointer<LongStringPrivate> d;
};

#endif
