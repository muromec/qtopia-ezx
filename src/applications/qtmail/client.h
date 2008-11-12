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

#ifndef Client_H
#define Client_H

#include <qobject.h>

#include <qtopiaglobal.h>

class QMailAccount;
class MailList;

class Client : public QObject
{
    Q_OBJECT

public:
    Client();
    virtual ~Client();
    virtual void setAccount(QMailAccount *_account);
    virtual void headersOnly(bool headers, int limit);
    virtual void newConnection();
    virtual void setSelectedMails(MailList *list, bool connected);
    virtual void quit();
    virtual bool hasDeleteImmediately() const;
    virtual void deleteImmediately(const QString& serverUid);
    virtual void resetNewMailCount();

signals:
    void errorOccurred(int, QString &);
    void mailSent(int);
    void transmissionCompleted();
    void updateStatus(const QString &);
    void mailTransferred(int);
    void unresolvedUidlList(QStringList &);
    void serverFolders();
    void mailboxSize(int);
    void transferredSize(int);
    void failedList(QStringList &);
    void deviceReady();
};

#endif
