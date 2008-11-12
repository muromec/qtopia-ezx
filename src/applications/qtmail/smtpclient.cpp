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

#include "smtpclient.h"

#include <QTextCodec>

#include <qmailaddress.h>
#include <qmailcodec.h>
#include <qtopiaapplication.h>

#include "mailtransport.h"
#include "emailhandler.h"

SmtpClient::SmtpClient()
{
    sending = false;
    authenticating = false;
    account = 0;
    transport = 0;
    sendMail = 0;
}

SmtpClient::~SmtpClient()
{
    delete transport;
}

void SmtpClient::setAccount(QMailAccount *_account)
{
    account = _account;
}

void SmtpClient::newConnection()
{
    qLog(SMTP) << "newConnection" << flush;
    if (sending) {
        qWarning("transport in use, connection refused");
        return;
    }

    if ( !account || account->smtpServer().isEmpty() ) {
        qWarning("Tried to send an email without an account and/or SMTP server");
        status = Done;
        emit mailSent(-1);
        return;
    }

    if (!transport) {
        // Set up the transport
        transport = new MailTransport("SMTP");

        connect(transport, SIGNAL(updateStatus(QString)),
                this, SIGNAL(updateStatus(QString)));
        connect(transport, SIGNAL(errorOccurred(int,QString)),
                this, SLOT(errorHandling(int,QString)));
    }

    // authenticate with POP first?
    bool authenticateFirst = ( !account->mailServer().isEmpty() && 
                               (account->smtpAuthentication() == QMailAccount::Auth_INCOMING) );
    doSend(authenticateFirst);
}

void SmtpClient::doSend(bool authenticating)
{
    status = Init;
    sending = true;
    this->authenticating = authenticating;

    if (authenticating)
    {
        qLog(SMTP) << "Authenticating" << flush;
        connect(transport, SIGNAL(readyRead()),
                this, SLOT(authenticate()));

        qLog(SMTP) << "Open connection" << flush;
        transport->open(account->mailServer(), account->mailPort(), account->mailEncryption());
    }
    else
    {
        qLog(SMTP) << "Not authenticating" << flush;
        connect(transport, SIGNAL(connected(QMailAccount::EncryptType)),
                this, SLOT(connected(QMailAccount::EncryptType)));
        connect(transport, SIGNAL(readyRead()),
                this, SLOT(incomingData()));
        connect(transport, SIGNAL(bytesWritten(qint64)),
                this, SLOT(sent(qint64)));

        qLog(SMTP) << "Open connection" << flush;
        transport->open(account->smtpServer(), account->smtpPort(), account->smtpEncryption());
    }
}

#ifndef QT_NO_OPENSSL
QString SmtpClient::_toBase64(const QString& in) const
{
    // We really should QByteArray::toBase64 here, because we don't want embedded line breaks
    QMailBase64Codec codec(QMailBase64Codec::Text);
    QByteArray encoded = codec.encode(in, "ISO-8859-1");
    return QString::fromLatin1(encoded.constData(), encoded.length());
}
#endif

bool SmtpClient::addMail(const QMailMessage& mail)
{
    QStringList sendTo;

    foreach (const QMailAddress& address, mail.recipients()) {
        // Only send to mail addresses
        if (address.isEmailAddress())
            sendTo.append(address.address());
    }

    if (sendTo.isEmpty())
        return false;

    QString userName;
    if (account)
        userName = "<" + account->emailAddress() + ">";

    RawEmail rawmail;
    if ( mail.replyTo().isNull() )
        rawmail.from = "<" + mail.from().address() + ">";
    else
        rawmail.from = userName;
    rawmail.to = sendTo;
    rawmail.mail = mail;

    mailList.append(rawmail);
    return true;
}

void SmtpClient::connected(QMailAccount::EncryptType encryptType)
{
    if (account->smtpEncryption() == encryptType) {
        qLog(SMTP) << "Connected" << flush;
        emit updateStatus(tr("Connected"));
    }

#ifndef QT_NO_OPENSSL
    if (status == TLS)
    {
        transport->stream() << "EHLO qtmail\r\n" << flush;
        if ((account->smtpAuthentication() == QMailAccount::Auth_LOGIN) ||
            (account->smtpAuthentication() == QMailAccount::Auth_PLAIN))
            status = Auth;
        else
            status = From;
    }
#endif
}

void SmtpClient::errorHandling(int errorCode, QString msg)
{
    if ( !sending )
        return;

    transport->close();
    qLog(SMTP) << "Closed connection" << flush;
    
    // C

    if (authenticating) {
	disconnect(transport, SIGNAL(readyRead()),
		   this, SLOT(authenticate()));
        //socket fail: pop may be down, but try sending anyway?
        qWarning() << "POP/IMAP authentication unavailable";

        if (errorCode == ErrCancel) {
            mailList.clear();
            sendMail = 0;
            sending = false;
            emit errorOccurred(errorCode, msg);
        } else {
            doSend();
        }
        return;
    } else {
        disconnect(transport, SIGNAL(connected(QMailAccount::EncryptType)),
		   this, SLOT(connected(QMailAccount::EncryptType)));
        disconnect(transport, SIGNAL(readyRead()),
		   this, SLOT(incomingData()));
        disconnect(transport, SIGNAL(bytesWritten(qint64)),
               this, SLOT(sent(qint64)));
    }

    if (msg.isEmpty())
        msg = tr("Error occurred");

    mailList.clear();
    sendMail = 0;
    sending = false;
    emit updateStatus(msg);
    emit errorOccurred(errorCode, msg);
}

void SmtpClient::sent(qint64 size)
{
    if (sendMail && messageLength) {
        sentLength += size;
        uint percentage = qMin<uint>(sentLength * 100 / messageLength, 100);
        emit sendProgress(sendMail->mail.id(), percentage);
    }
}

void SmtpClient::authenticate()
{
    qLog(SMTP) << "Authenticate begin" << flush;
    QTextStream& stream = transport->stream();
    QString response = transport->readLine();
    qLog(SMTP) << "Authenticate response " << response.left(response.length() - 2) << flush;

    if ( account->accountType() == QMailAccount::IMAP ) {
        if (status == Init ) {
            qLog(SMTP) << "Authenticating IMAP Init" << flush;
            status = Done;
            qLog(SMTP) << "sent:" << "A01 LOGIN " + account->mailUserName() + " <password hidden>" << flush;
            stream << "A01 LOGIN " + account->mailUserName() + " " + account->mailPassword() + "\r\n" << flush;
            qLog(SMTP) << "Authenticate end" << flush;
            return;
        } else if ( status == Done ) {
            qLog(SMTP) << "Authenticating IMAP Done" << flush;
            QString rsp = response.mid(3, response.indexOf(" ") ).trimmed();
            if ( rsp.toUpper() != "OK") {
                qLog(SMTP) << "Authentication failed:" << rsp << flush;
                errorHandling(ErrLoginFailed, "");
                qLog(SMTP) << "Authenticate end" << flush;
                return;
            }

            status = Quit;
            qLog(SMTP) << "sent:" << "A02 LOGOUT" << flush;
            stream << "A02 LOGOUT\r\n" << flush;
            qLog(SMTP) << "Authenticate end" << flush;
            return;
        } else if ( status == Quit ) {
            qLog(SMTP) << "Authenticating IMAP Quit" << flush;
            transport->close();
            qLog(SMTP) << "Closed connection" << flush;
            disconnect(transport, SIGNAL(readyRead()), this, SLOT(authenticate()));
            doSend();
            qLog(SMTP) << "Authenticate end" << flush;
            return;
        }
    }

    if (status == Init) {
        status = Pass;
        qLog(SMTP) << "Authenticating POP sent:" << "USER " << account->mailUserName() << flush;
        stream << "USER " << account->mailUserName() << "\r\n" <<flush;
        return;
    } else if (status == Pass) {
        if (response[0] != '+') {
            errorHandling(ErrLoginFailed, "");
            return;
        }

        status = Done;
        qLog(SMTP) << "Authenticating POP sent:" << "PASS <password hidden>" << flush;
        stream << "PASS " << account->mailPassword() << "\r\n" << flush;
        return;
    } else if (status == Done) {
        if (response[0] != '+') {
            errorHandling(ErrLoginFailed, "");
            return;
        }

        status = Quit;
        qLog(SMTP) << "Authenticating POP sent:" << "QUIT" << flush;
        stream << "QUIT\r\n" << flush;
        return;
    } else if ( status == Quit ) {
        transport->close();
        qLog(SMTP) << "Closed connection" << flush;
        disconnect(transport, SIGNAL(readyRead()), this, SLOT(authenticate()));
        doSend();
    }
}

void SmtpClient::incomingData()
{
    QString response;
    QString line;

    if (!transport->canReadLine())
        return;

    do
    {
        line = transport->readLine();
        response += line;
    }
    while (transport->canReadLine() && line.length() != 0);
    qLog(SMTP) << "response:" << response.left(response.length() - 2) << flush;

    QTextStream& stream = transport->stream();

    switch (status) {
    case Init:  {
        if (response[0] == '2') {
            status = From;
            mailItr = mailList.begin();
#ifndef QT_NO_OPENSSL
            if ((account->smtpAuthentication() == QMailAccount::Auth_LOGIN ||
                 account->smtpAuthentication() == QMailAccount::Auth_PLAIN) ||
                account->smtpEncryption() == QMailAccount::Encrypt_TLS)
            {
                qLog(SMTP) << "Init: sent:" << "EHLO qtmail" << flush;
                stream << "EHLO qtmail\r\n" << flush;
                if (account->smtpEncryption() == QMailAccount::Encrypt_TLS)
                    status = StartTLS;
                else
                    status = Auth;
            }
            else
#endif
            {
                qLog(SMTP) << "Init: sent:" << "HELO qtmail" << flush;
                stream << "HELO qtmail\r\n" << flush;
            }
        } 
        else 
            errorHandling(ErrUnknownResponse, response);
        break;
    }
#ifndef QT_NO_OPENSSL
    case StartTLS:
    {
        if (line[0] == '2')
        {
            qLog(SMTP) << "StartTLS: sent:" << "STARTTLS" << flush;
            stream << "STARTTLS\r\n" << flush;
            status = TLS;
        } 
        else 
            errorHandling(ErrUnknownResponse,response);
        break;
    }
    case TLS:
    {
        if (line[0] == '2')
        {
            // Switch into encrypted mode
            transport->switchToEncrypted();
        }
        else 
            errorHandling(ErrUnknownResponse,response);
        break;
    }

    case Auth:
    {
        if (line[0] == '2')
        {
            if (account->smtpAuthentication() == QMailAccount::Auth_LOGIN)
            {
                qLog(SMTP) << "Auth: sent:" << "AUTH LOGIN " << flush;
                stream << "AUTH LOGIN \r\n" << flush;
                status = AuthUser;
            }
            else if (account->smtpAuthentication() == QMailAccount::Auth_PLAIN)
            {
                QString temp = account->smtpUsername() + '\0' + account->smtpUsername() + '\0' + account->smtpPassword();
                temp = _toBase64(temp);
                qLog(SMTP) << "Auth: sent:" << "AUTH PLAIN " << "<Base64 username/password hidden>" << flush;
                stream << "AUTH PLAIN " << temp << "\r\n" << flush;
                status = From;
            }
        }
        else 
            errorHandling(ErrUnknownResponse, response);
        break;
    }

    case AuthUser:
    {

        if (line[0] == '3')
        {
            qLog(SMTP) << "AuthUser: sent:" << _toBase64(account->smtpUsername()) << flush;
            stream << _toBase64(account->smtpUsername()) << "\r\n" << flush;
            status = AuthPass;
        }
        else
            errorHandling(ErrUnknownResponse, response);
        break;
    }
    case AuthPass:
    {
        if (line[0] == '3')
        {
            qLog(SMTP) << "AuthPass: sent:" << "<Base64 password hidden>" << flush;
            stream << _toBase64(account->smtpPassword()) << "\r\n" << flush;
            status = From;
        }
        else
            errorHandling(ErrUnknownResponse, response);
        break;
    }
#endif

    case Login:
    case Pass:
    case Done:  {
        // Supposed to be unused here - handled by authentication
        qWarning() << "incomingData - Unexpected status value: " << status;
        break;
    }

    case From:  {
        if (sendMail) {
            // The last send operation is complete
            emit messageProcessed(sendMail->mail.id());
            sendMail = 0;
        }

        if (response[0] == '2') {
            qLog(SMTP) << "From: sent:" << "MAIL FROM: " << mailItr->from << flush;
            stream << "MAIL FROM: " << mailItr->from << "\r\n" << flush;
            status = Recv;
        } 
        else 
            errorHandling(ErrUnknownResponse, response);
        break;
    }
    case Recv:  {
        if (response[0] == '2') {
            it = mailItr->to.begin();
            if (it == mailItr->to.end())
                errorHandling(ErrUnknownResponse, "no recipients");
            qLog(SMTP) << "Recv: sent:" << "RCPT TO: <" << *it << '>' << flush;
            stream << "RCPT TO: <" << *it << ">\r\n" << flush;
            status = MRcv;
        } 
        else 
            errorHandling(ErrUnknownResponse, response);
        break;
    }
    case MRcv:  {
        if (response[0] == '2') {
            it++;
            if ( it != mailItr->to.end() ) {
                qLog(SMTP) << "MRcv: sent:" << "RCPT TO: <" << *it << '>' << flush;
                stream << "RCPT TO: <" << *it << ">\r\n" << flush;
                break;
            } else  {
                status = Data;
            }
        } else {
            errorHandling(ErrUnknownResponse, response);
            break;
        }
    }
    case Data:  {
        if (response[0] == '2') {
            qLog(SMTP) << "Data: sent:" << "DATA" << flush;
            stream << "DATA\r\n" << flush;
            status = Body;
            emit updateStatus(tr( "Sending: %1").arg(mailItr->mail.subject()) );
        } 
        else 
            errorHandling(ErrUnknownResponse, response);
        break;
    }
    case Body:  {
        if (response[0] == '3' || response[0] == '2') {
            QByteArray rfcData = mailItr->mail.toRfc2822(QMailMessage::TransmissionFormat);

            sendMail = &(*mailItr);
            messageLength = rfcData.length();
            sentLength = 0;

            qLog(SMTP) << "Body: sent:" << "<rfcData omitted for brevity>" << flush;
            stream << rfcData;
            qLog(SMTP) << "Body: sent:" << "." << flush;
            stream << "\r\n.\r\n" << flush;

            mailItr++;
            if (mailItr != mailList.end()) {
                status = From;
            } else {
                status = Quit;
            }
        } else
            errorHandling(ErrUnknownResponse, response);
        break;
    }
    case Quit:  {
        if (sendMail) {
            // The last send operation is complete
            emit messageProcessed(sendMail->mail.id());
            sendMail = 0;
        }

        if ( response[0] == '2' || response[0] == '3' ) {
            qLog(SMTP) << "Quit: sent:" << "QUIT" << flush;
            stream << "QUIT\r\n" << flush;
            status = Done;
            int count = mailList.count();
            emit updateStatus( tr("Sent %1 messages").arg(count) );
            mailList.clear();
            sending = false;
            transport->close();
            qLog(SMTP) << "Closed connection" << flush;
            disconnect(transport, SIGNAL(connected(QMailAccount::EncryptType)),
		       this, SLOT(connected(QMailAccount::EncryptType)));
            disconnect(transport, SIGNAL(readyRead()),
		       this, SLOT(incomingData()));
            disconnect(transport, SIGNAL(bytesWritten(qint64)),
		       this, SLOT(sent(qint64)));
            emit mailSent(count);
        } else errorHandling(ErrUnknownResponse, response);
        break;
    }
    }
}
