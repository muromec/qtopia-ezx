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



#ifndef MAILLIST_H
#define MAILLIST_H

#include <qobject.h>
#include <qstring.h>
#include <qlist.h>
#include <qtopia/mail/qmailid.h>


struct dList{
    QString serverId;
    uint size;
    QMailId internalId;
    QString fromBox;
};

class MailList : public QObject
{
    Q_OBJECT

public:
    void clear();
    int count();
    int size();
    int currentSize();
    QMailId currentId();
    QString currentMailbox();

    QString* first();
    QString* next();
    void sizeInsert(QString serverId, uint size, QMailId id, QString box);
    void append(QString serverId, uint size, QMailId id, QString box);
    void moveFront(QString serverId);
    bool remove(QString serverId);

private:
    QList<dList*> sortedList;
    uint currentPos;
};

#endif
