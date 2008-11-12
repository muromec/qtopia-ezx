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

#ifndef __QMAILMESSAGEKEY_H
#define __QMAILMESSAGEKEY_H

#include <QList>
#include <QVariant>
#include <QSharedData>
#include <QFlags>
#include <qtopiaglobal.h>
#include "qmailmessage.h"

class QMailMessageKeyPrivate;

class QTOPIAMAIL_EXPORT QMailMessageKey
{
public:
    enum Operand
    {
        LessThan,
        LessThanEqual,
        GreaterThan,
        GreaterThanEqual,
        Equal,
        NotEqual,
        Contains
     };

    enum Property
    {
        Id = 0x0001,
        Type = 0x0002,
        ParentFolderId = 0x0004,
        Sender = 0x0008,
        Recipients = 0x0010,
        Subject = 0x0020,
        TimeStamp = 0x0040,
        Status = 0x0080,
        FromAccount = 0x0100,
        FromMailbox = 0x0200,
        ServerUid = 0x0400,
        Size = 0x0800,
    };
    Q_DECLARE_FLAGS(Properties,Property)

public:
    QMailMessageKey();
    QMailMessageKey(const Property& p, const QVariant& value, const Operand& c = Equal);
    explicit QMailMessageKey(const QMailIdList& ids);
    QMailMessageKey(const QMailMessageKey& other);
    virtual ~QMailMessageKey();

    QMailMessageKey operator~() const;
    QMailMessageKey operator&(const QMailMessageKey& other) const;
    QMailMessageKey operator|(const QMailMessageKey& other) const;
    QMailMessageKey& operator&=(const QMailMessageKey& other);
    QMailMessageKey& operator|=(const QMailMessageKey& other);

    bool operator==(const QMailMessageKey& other) const;
    bool operator !=(const QMailMessageKey& other) const;

    QMailMessageKey& operator=(const QMailMessageKey& other);

    bool isEmpty() const;

private:
    friend class QMailStore;
    friend class QMailStorePrivate;

private:
    QSharedDataPointer<QMailMessageKeyPrivate> d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMailMessageKey::Properties)

#endif
