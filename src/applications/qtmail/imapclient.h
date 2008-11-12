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



#ifndef ImapClient_H
#define ImapClient_H

#include <qstring.h>
#include <qstringlist.h>
#include <qobject.h>
#include <qlist.h>



#include "maillist.h"
#include "account.h"
#include "imapprotocol.h"
#include "client.h"

class ImapClient: public Client
{
    Q_OBJECT

public:
    ImapClient();
    ~ImapClient();
    void newConnection();
    void setAccount(QMailAccount *_account);
    void setSelectedMails(MailList *list, bool connected);
    void quit();
    void checkForNewMessages();
    int newMessageCount();

signals:
    void serverFolders();
    void newMessage(const QMailMessage&);
    void mailTransferred(int);
    void failedList(QStringList &);
    void allMessagesReceived();
    void nonexistentMessage(const QMailId& id);
    void expiredMessages(const QStringList&, const QString& mailbox, bool locationExists);
    void retrievalProgress(const QString&, uint);
    void messageProcessed(const QString&);

public slots:
    void errorHandling(int, QString msg);

protected slots:
    void operationDone(ImapCommand &, OperationState &);
    void mailboxListed(QString &, QString &, QString &);
    void messageFetched(QMailMessage& mail);
    void nonexistentMessage(const QString& uid);
    void downloadSize(int);

private:
    void removeDeletedMailboxes();
    bool nextMailbox();
    void handleSelect();
    void handleUid();
    void handleUidFetch();
    void handleSearch();

    bool messagesToDelete();
    void setNextDeleted();
    void fetchNextMail();
    void searchCompleted();
    void previewCompleted();

private:
    ImapProtocol client;
    QMailAccount *account;
    MailList *mailList;
    QString msgUidl;
    enum transferStatus
    {
        Init, Fetch, Rfc822
    };
    enum SearchStatus
    {
        All,Seen,Unseen
    };
    SearchStatus _searchStatus;
    QStringList _mailboxUidList;

    int status;
    int messageCount, mailSize, mailDropSize;
    bool selected;

    QStringList mailboxNames;
    QMailId internalId;
    QStringList uniqueUidList, expiredUidList, unresolvedUid, delList;
    Mailbox *currentBox;
    uint atCurrentBox;

    QString retrieveUid;
    uint retrieveLength;
    bool tlsEnabled;
};

#endif
