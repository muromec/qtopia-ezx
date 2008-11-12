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



#ifndef PopClient_H
#define PopClient_H

#include <qstring.h>
#include <qstringlist.h>
#include <qobject.h>
#include <qlist.h>

#include "maillist.h"
#include "account.h"
#include "client.h"

class LongStream;
class MailTransport;

class PopClient: public Client
{
    Q_OBJECT

public:
    PopClient();
    ~PopClient();
    void newConnection();
    void setAccount(QMailAccount *_account);
    void headersOnly(bool headers, int limit);
    void setSelectedMails(MailList *list, bool connected);
    void quit();
    void checkForNewMessages();
    int newMessageCount();

signals:
    void newMessage(const QMailMessage&);
    void unresolvedUidlList(QStringList &);
    void mailTransferred(int);
    void mailboxSize(int);
    void retrievalProgress(const QString&, uint);
    void messageProcessed(const QString&);
    void allMessagesReceived();

public slots:
    void errorHandling(int, QString msg);

protected slots:
    void incomingData();

private:
    int nextMsgServerPos();
    QString getUidl(QString uidl);
    QString msgPosFromUidl(QString uidl);
    int getSize(int pos);
    void uidlIntegrityCheck();
    void createMail();
    void sendCommand(const QString& cmd);
    QString readResponse();

private:
    enum TransferStatus
    {
            Init, Pass, Stat, Mcnt, Read, List, Size, Retr, Acks,
            Quit, Done, Ignore, Dele, Rset, Uidl, Guidl, Exit
    };

    QMailAccount *account;
    TransferStatus status;
    int messageCount, newMessages, mailSize, headerLimit;
    int msgNum;
    QMailId internalId;
    bool receiving, preview, selected;
    bool awaitingData;
    QString message;
    MailList *mailList;

    QString msgUidl;
    QStringList uidlList, uniqueUidlList, unresolvedUidl;
    QStringList sizeList;
    QStringList lastUidl;
    QStringList deleteList;
    LongStream *d;

    MailTransport *transport;

    QString retrieveUid;
    uint retrieveLength;
};

#endif
