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

#ifndef __QMAILFOLDERKEY_H
#define __QMAILFOLDERKEY_H

#include <QList>
#include <QVariant>
#include <qtopiaglobal.h>
#include <QSharedData>
#include "qmailfolder.h"

class QMailFolderKeyPrivate;

class QTOPIAMAIL_EXPORT QMailFolderKey
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
        Id,
        Name,
        ParentId
    };

public:
    QMailFolderKey();
    QMailFolderKey(const Property& p, const QVariant& value, const Operand& = Equal);
    explicit QMailFolderKey(const QMailIdList& ids);
    QMailFolderKey(const QMailFolderKey& other);
    virtual ~QMailFolderKey();

    QMailFolderKey operator~() const;
    QMailFolderKey operator&(const QMailFolderKey& other) const;
    QMailFolderKey operator|(const QMailFolderKey& other) const;
    QMailFolderKey& operator&=(const QMailFolderKey& other);
    QMailFolderKey& operator|=(const QMailFolderKey& other);

    bool operator==(const QMailFolderKey& other) const;
    bool operator !=(const QMailFolderKey& other) const;

    QMailFolderKey& operator=(const QMailFolderKey& other);

    bool isEmpty() const;

private:
    friend class QMailStore;
    friend class QMailStorePrivate;

private:
    QSharedDataPointer<QMailFolderKeyPrivate> d;

};

#endif
