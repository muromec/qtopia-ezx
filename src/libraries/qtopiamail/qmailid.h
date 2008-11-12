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

#ifndef __QMAILID_H
#define __QMAILID_H

#include <QString>
#include <QVariant>
#include <qtopiaglobal.h>
#include <QSharedData>
#include <qtopiaipcmarshal.h>

class QMailIdPrivate;

class QTOPIAMAIL_EXPORT QMailId
{
public:
    QMailId();
    explicit QMailId(const quint64& id);
    QMailId(const QMailId& other);
    virtual ~QMailId();

    bool isValid() const;

    operator QVariant() const;

    bool operator!=( const QMailId& other ) const;
    bool operator==( const QMailId& other ) const;
    QMailId& operator=(const QMailId& other);
    bool operator <(const QMailId& other) const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    quint64 toULongLong() const;

private:
    QSharedDataPointer<QMailIdPrivate> d;

};

Q_DECLARE_USER_METATYPE(QMailId);

typedef QList<QMailId> QMailIdList;

#endif //QMAILID_H
