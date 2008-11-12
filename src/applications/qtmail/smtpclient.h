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



#ifndef SmtpClient_H
#define SmtpClient_H

#include <qstring.h>
#include <qobject.h>
#include <qstringlist.h>
#include <qlist.h>
#include <QMailMessage>

#include "account.h"
#include "client.h"

class MailTransport;

struct RawEmail
{
    QString from;
    QStringList to;
    QMailMessage mail;
};

class SmtpClient: public Client
{
    Q_OBJECT

public:
    SmtpClient();
    ~SmtpClient();
    void newConnection();
    bool addMail(const QMailMessage& mail);
    void setAccount(QMailAccount *_account);

signals:
    void mailSent(int);
    void transmissionCompleted();
    void sendProgress(const QMailId&, uint);
    void messageProcessed(const QMailId&);

public slots:
    void sent(qint64);
    void errorHandling(int, QString msg);

protected slots:
    void connected(QMailAccount::EncryptType encryptType);
    void incomingData();
    void authenticate();

private:
    void doSend(bool authenticating = false);
#ifndef QT_NO_OPENSSL
    QString _toBase64(const QString& in) const;
#endif

private:
    enum TransferStatus
    {
        Init,
#ifndef QT_NO_OPENSSL
        StartTLS, TLS, Auth, AuthUser, AuthPass,
#endif
        Login, Pass, Done, From, Recv, MRcv, Data, Body, Quit
    };

    QMailAccount *account;
    TransferStatus status;
    QList<RawEmail> mailList;
    QList<RawEmail>::Iterator mailItr;
    RawEmail* sendMail;
    uint messageLength;
    uint sentLength;
    bool sending, authenticating, success;
    QStringList::Iterator it;
    MailTransport *transport;
};

#endif
