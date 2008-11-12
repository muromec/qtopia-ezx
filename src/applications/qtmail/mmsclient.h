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


#ifndef MMSCLIENT_H
#define MMSCLIENT_H

#include <quuid.h>
#include <qlist.h>
#include <qobject.h>
#include "account.h"
#include "client.h"
#include <QMailMessage>

#include <QTimer>
#include <QtopiaNetworkInterface>

class QMailAccount;
class MailList;
class MMSMessage;
class MmsComms;
class QHttp;
class QHttpResponseHeader;
class QHttpRequestHeader;
class QWspPart;
class QNetworkDevice;
class QDSActionRequest;

class MmsClient: public Client
{
    Q_OBJECT
public:
    MmsClient();
    ~MmsClient();

    void newConnection();
    void quit();
    void setSelectedMails(MailList *list, bool connected);
    bool addMail(const QMailMessage &);
    void setAccount(QMailAccount *_account);
    void sendNotifyResp(const QMailMessage &, const QString &status);
    void resetNewMailCount();
    int unreadMmsCount();
    void checkForNewMessages();
    int newMessageCount();
    void pushMmsMessage(const QDSActionRequest& request);
    void closeWhenDone();

signals:
    void errorOccurred(int, QString &);
    void updateStatus(const QString &);
    void mailTransferred(int);
    void mailSent(int);
    void transmissionCompleted();
    void newMessage(const QMailMessage&);
    void allMessagesReceived();
    void sendProgress(const QMailId&, uint);
    void messageProcessed(const QMailId&);

public slots:
    void errorHandling(int, QString msg);

private slots:
    void notificationInd(const MMSMessage &msg);
    void deliveryInd(const MMSMessage &msg);
    void sendConf(const MMSMessage &msg);
    void retrieveConf(const MMSMessage &msg, int);
    void statusChange(const QString &status);
    void commsError(int code, const QString &msg);
    void transferSize(int);
    void transfersComplete();
    void transferNextMessage();
    void raiseFailure();
    void networkStatusChange(QtopiaNetworkInterface::Status, bool);
    void networkDormant();

private:
    void sendMessage(MMSMessage&);
    void sendAcknowledge(const MMSMessage &);
    void sendNextMessage();
    void getNextMessage();
    QMailMessage convertToEmail(const MMSMessage &, int size);
    MMSMessage convertToMms(const QMailMessage &mail);
    void addField(MMSMessage &mms, const QMailMessage& mail, const QString &field);
    QString encodeRecipient(const QString &r);
    QString decodeRecipient(const QString &r);
    QString networkConfig() const;
    QString mmsInterfaceName() const;
    bool raiseNetwork();

private:
    QMailAccount *account;
    MmsComms *comms;
    MailList *mailList;
    QList<MMSMessage> outgoing;
    QMailId internalId;
    int messagesSent;
    int messagesRecv;
    bool quitRecv;
    bool networkReference;
    QNetworkDevice* networkDevice;
    QtopiaNetworkInterface::Status networkStatus;
    QTimer raiseTimer;
    QTimer inactivityTimer;
    QMap<QString, QMailId> sentMessages;
    uint messageLength;
    uint sentLength;

    static int txnId;
};

#endif
