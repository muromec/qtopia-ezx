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

#ifndef __QMAILFOLDER_H
#define __QMAILFOLDER_H

#include "qmailid.h"
#include <QString>
#include <QList>
#include <QSharedData>
#include <qtopiaglobal.h>

class QMailFolderPrivate;

class QTOPIAMAIL_EXPORT QMailFolder
{
public:
    QMailFolder(const QMailId& id);
    QMailFolder(const QString& name, const QMailId& parentFolderId = QMailId());
    QMailFolder(const QMailFolder& other);
    virtual ~QMailFolder();

    QMailId id() const;

    QString name() const;
    void setName(const QString& name);

    QMailId parentId() const;
    void setParentId(const QMailId& id);

    bool isRoot() const;

    QMailFolder& operator=(const QMailFolder& other);

private:
    QMailFolder();
    void setId(const QMailId& id);

private:
    friend class QMailStore;
    friend class QMailStorePrivate;

private:
    QSharedDataPointer<QMailFolderPrivate> d;

};

typedef QList<QMailFolder> QMailFolderList;

#endif //QMAILFOLDER_H

