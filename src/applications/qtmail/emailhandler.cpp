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


#include "emailhandler.h"
#ifndef QTOPIA_NO_MMS
#include "mmsclient.h"
#endif

#include <QFileInfo>
#include <QApplication>
#include <QMessageBox>
#include <QString>
#include <longstream_p.h>
#include <qtopialog.h>
#include <qmailaddress.h>

EmailHandler::EmailHandler()
{
    LongStream::cleanupTempFiles();

    mailAccount = 0;
    smtpAccount = 0;
    smsAccount = 0;
    mmsAccount = 0;

    receiving = false;
    totalSendSize = 0;
    totalRetrievalSize = 0;

    smtpClient = new SmtpClient();
    connectClient(smtpClient, Sending, SIGNAL(smtpError(int,QString&)));

#ifndef QTOPIA_NO_SMS
    smsClient = new SmsClient();
    connectClient(smsClient, Sending|Receiving|AsyncArrival, SIGNAL(smsError(int,QString&)));
#endif

#ifndef QTOPIA_NO_MMS
    mmsClient = new MmsClient();
    connectClient(mmsClient, Sending|Receiving|AsyncArrival, SIGNAL(mmsError(int,QString&)));
#endif

    popClient = new PopClient();
    connectClient(popClient, Receiving|SyncRetrieval, SIGNAL(popError(int,QString&)));

    imapClient = new ImapClient();
    connectClient(imapClient, Receiving|SyncRetrieval|AsyncDeletion, SIGNAL(popError(int,QString&)));
}

EmailHandler::~EmailHandler()
{
    delete smtpClient;
#ifndef QTOPIA_NO_SMS
    delete smsClient;
#endif
#ifndef QTOPIA_NO_MMS
    delete mmsClient;
#endif
    delete popClient;
    delete imapClient;
}

int EmailHandler::unreceivedSmsCount()
{
#ifndef QTOPIA_NO_SMS
    return smsClient->unreceivedSmsCount();
#else
    return 0;
#endif
}

int EmailHandler::unreadSmsCount()
{
#ifndef QTOPIA_NO_SMS
    return smsClient->unreadSmsCount();
#else
    return 0;
#endif
}

bool EmailHandler::smsReadyToDelete() const
{
#ifndef QTOPIA_NO_SMS
    return smsClient->readyToDelete();
#else
    return false;
#endif
}

void EmailHandler::synchroniseClients()
{
#ifndef QTOPIA_NO_SMS
    if (!unsynchronised.contains(smsClient))
        unsynchronised.append(smsClient);
#endif
#ifndef QTOPIA_NO_MMS
    if (!unsynchronised.contains(mmsClient))
        unsynchronised.append(mmsClient);
#endif
    /* At this time, we don't have asynchronous arrival of email
    if (!unsynchronised.contains(popClient))
        unsynchronised.append(popClient);
    if (!unsynchronised.contains(imapClient))
        unsynchronised.append(imapClient);
    */

#ifndef QTOPIA_NO_SMS
    smsClient->checkForNewMessages();
#endif
#ifndef QTOPIA_NO_MMS
    mmsClient->checkForNewMessages();
#endif
    /*
    popClient->checkForNewMessages();
    imapClient->checkForNewMessages();
    */
}

#ifdef DEBUG_CLIENT_SIGNALS
void EmailHandler::reportErrorOccurred(int n, QString& s) { qLog(Messaging) << static_cast<Client*>(sender()) << ": errorOccurred(" << n << ',' << s << ')'; }
void EmailHandler::reportMailSent(int n) { qLog(Messaging) << static_cast<Client*>(sender()) << ": mailSent(" << n << ')'; }
void EmailHandler::reportTransmissionCompleted() { qLog(Messaging) << static_cast<Client*>(sender()) << ": transmissionCompleted()"; }
void EmailHandler::reportNonexistentMessage(const QMailId& id) { qLog(Messaging) << static_cast<Client*>(sender()) << ": nonexistentMessage(" << id.toULongLong() << ')'; }
void EmailHandler::reportUpdateStatus(const QString& s) { qLog(Messaging) << static_cast<Client*>(sender()) << ": updateStatus(" << s << ')'; }
void EmailHandler::reportNewMessage(const QMailMessage& m) { qLog(Messaging) << static_cast<Client*>(sender()) << ": newMessage(" << m.subject() << ')'; }
void EmailHandler::reportAllMessagesReceived() { qLog(Messaging) << static_cast<Client*>(sender()) << ": allMessagesReceived()"; }
void EmailHandler::reportMailTransferred(int n) { qLog(Messaging) << static_cast<Client*>(sender()) << ": mailTransferred(" << n << ')'; }
void EmailHandler::reportMailboxSize(int n) { qLog(Messaging) << static_cast<Client*>(sender()) << ": mailboxSize(" << n << ')'; }
void EmailHandler::reportSendProgress(const QMailId& id, uint percentage) { qLog(Messaging) << static_cast<Client*>(sender()) << ": sendProgess(" << id.toULongLong() << ',' << percentage << ')'; }
void EmailHandler::reportRetrievalProgress(const QString& uid, uint percentage) { qLog(Messaging) << static_cast<Client*>(sender()) << ": retrievalProgess(" << uid << ',' << percentage << ')'; }
void EmailHandler::reportMessageProcessed(const QMailId& id) { qLog(Messaging) << static_cast<Client*>(sender()) << ": messageProcessed(" << id.toULongLong() << ')'; }
void EmailHandler::reportMessageProcessed(const QString& uid) { qLog(Messaging) << static_cast<Client*>(sender()) << ": messageProcessed(" << uid << ')'; }
void EmailHandler::reportExpiredMessages(const QStringList& l, const QString& m, bool b) { qLog(Messaging) << static_cast<Client*>(sender()) << ": expiredMessages(" << l << ',' << m << ',' << b << ')'; }

#define DEBUG_CONNECT(a,b,c,d) connect(a,b,c,d)
#else
#define DEBUG_CONNECT(a,b,c,d)
#endif

void EmailHandler::connectClient(Client *client, int type, QString sigName)
{
    DEBUG_CONNECT(client, SIGNAL(errorOccurred(int,QString&)), this, SLOT(reportErrorOccurred(int,QString&)));
    connect(client, SIGNAL(errorOccurred(int,QString&)),sigName.toAscii() );
    DEBUG_CONNECT(client, SIGNAL(mailSent(int)), this, SLOT(reportMailSent(int)) );
    connect(client, SIGNAL(mailSent(int)), this, SIGNAL(mailSent(int)) );
    DEBUG_CONNECT(client, SIGNAL(transmissionCompleted()), this, SLOT(reportTransmissionCompleted()) );
    connect(client, SIGNAL(transmissionCompleted()), this, SIGNAL(transmissionCompleted()) );

    if (type & Receiving)
    {
        if (type & AsyncArrival) {
            /* Actually, we don't want to get a synchronised signal until we
               explicitly ask for it, with synchroniseClients()...
            unsynchronised.append(client);
            */
        }
        if (type & AsyncDeletion) {
            DEBUG_CONNECT(client, SIGNAL(nonexistentMessage(QMailId)), this, SLOT(reportNonexistentMessage(QMailId)) );
            connect(client, SIGNAL(nonexistentMessage(QMailId)), this, SIGNAL(nonexistentMessage(QMailId)) );

            DEBUG_CONNECT(client, SIGNAL(expiredMessages(QStringList, QString, bool)), this, SLOT(reportExpiredMessages(QStringList, QString, bool)) );
            connect(client, SIGNAL(expiredMessages(QStringList, QString, bool)), this, SIGNAL(expiredMessages(QStringList, QString, bool)) );
        }
        if (type & SyncRetrieval) {
            DEBUG_CONNECT(client, SIGNAL(retrievalProgress(QString, uint)), this, SLOT(reportRetrievalProgress(QString, uint)) );
            connect(client, SIGNAL(retrievalProgress(QString, uint)), this, SLOT(retrievalProgress(QString, uint)) );
            DEBUG_CONNECT(client, SIGNAL(messageProcessed(QString)), this, SLOT(reportMessageProcessed(QString)) );
            connect(client, SIGNAL(messageProcessed(QString)), this, SLOT(messageProcessed(QString)) );
        }

        DEBUG_CONNECT(client, SIGNAL(updateStatus(QString)), this, SLOT(reportUpdateStatus(QString)) );
        connect(client, SIGNAL(updateStatus(QString)), this, SLOT(receiveStatusChange(QString)) );
        DEBUG_CONNECT(client, SIGNAL(newMessage(QMailMessage)), this, SLOT(reportNewMessage(QMailMessage)) );
        connect(client, SIGNAL(newMessage(QMailMessage)), this, SIGNAL(mailArrived(QMailMessage)) );
        DEBUG_CONNECT(client, SIGNAL(allMessagesReceived()), this, SLOT(reportAllMessagesReceived()) );
        connect(client, SIGNAL(allMessagesReceived()), this, SLOT(messagesReceived()) );
    }
    if (type & Sending)
    {
        DEBUG_CONNECT(client, SIGNAL(updateStatus(QString)), this, SLOT(reportUpdateStatus(QString)) );
        connect(client, SIGNAL(updateStatus(QString)), this, SLOT(sendStatusChange(QString)) );
        DEBUG_CONNECT(client, SIGNAL(sendProgress(QMailId, uint)), this, SLOT(reportSendProgress(QMailId, uint)) );
        connect(client, SIGNAL(sendProgress(QMailId, uint)), this, SLOT(sendProgress(QMailId, uint)) );
        DEBUG_CONNECT(client, SIGNAL(messageProcessed(QMailId)), this, SLOT(reportMessageProcessed(QMailId)) );
        connect(client, SIGNAL(messageProcessed(QMailId)), this, SLOT(messageProcessed(QMailId)) );
    }

    DEBUG_CONNECT(client, SIGNAL(mailTransferred(int)), this, SLOT(reportMailTransferred(int)) );
    connect(client, SIGNAL(mailTransferred(int)), this, SIGNAL(mailTransferred(int)) );
    connect(client, SIGNAL(unresolvedUidlList(QStringList&)), this, SLOT(unresolvedUidl(QStringList&)) );
    connect(client, SIGNAL(serverFolders()), this, SIGNAL(serverFolders()) );

    //relaying size information
    DEBUG_CONNECT(client, SIGNAL(mailboxSize(int)), this, SLOT(reportMailboxSize(int)) );
    connect(client, SIGNAL(mailboxSize(int)), this, SIGNAL(mailboxSize(int)) );
    connect(client, SIGNAL(failedList(QStringList&)), this, SIGNAL(failedList(QStringList&)) );
}

void EmailHandler::sendMail(QList<QMailMessage>& mailList)
{
    bool errorOccurred = false;
    uint smtpCount = 0;
    uint smsCount = 0;
    uint mmsCount = 0;

    // We shouldn't have anything left in our send list...
    if (!sendSize.isEmpty()) {
        foreach (const QMailId& id, sendSize.keys())
            qWarning() << "Message" << id.toULongLong() << "still in send map...";

        sendSize.clear();
    }

    for (int i = 0; i < mailList.count(); i++) {

        // Get the complete message
        QMailMessage currentMail(mailList[i].id(),QMailMessage::HeaderAndBody);

        int mailRecipients = 0;
        int phoneRecipients = 0;

        foreach (const QMailAddress& address, currentMail.recipients()){
            if (address.isEmailAddress())
                ++mailRecipients;
            else if (address.isPhoneNumber())
                ++phoneRecipients;
        }

        if ( !mailRecipients && !phoneRecipients ) {
            QString temp = tr("No recipients specified for mail with subject:\n"
                              "%1\nNO mail has been sent.").arg( currentMail.subject() );
            QMessageBox::warning(qApp->activeWindow(), tr("Mail encoding error"), temp, tr("OK"));
            errorOccurred = true;
        } else {
            //mms message
#ifndef QTOPIA_NO_MMS
            if (currentMail.messageType() == QMailMessage::Mms) {
                if (mmsClient->addMail(currentMail)) {
                    ++mmsCount;
                    sendSize.insert(currentMail.id(), currentMail.indicativeSize());
                } else {
                    errorOccurred = true;
                }
                continue;
            }
#endif

#ifndef QTOPIA_NO_SMS
            // sms message
            if (currentMail.messageType() == QMailMessage::Sms
                && phoneRecipients > 0)
            {
                if (smsClient->addMail(currentMail)) {
                    ++smsCount;
                    sendSize.insert(currentMail.id(), currentMail.indicativeSize());
                } else {
                    errorOccurred = true;
                }
            }
#endif

            if (errorOccurred)
                continue;

            //email message
            if (mailRecipients == 0)
                continue;

            if (smtpCount == 0) {
                // We need the account already set to addMail correctly
                smtpClient->setAccount(smtpAccount);
            }
            if (smtpClient->addMail(currentMail)) {
                ++smtpCount;
                sendSize.insert(currentMail.id(), currentMail.indicativeSize());
            } else {
                errorOccurred = true;
            }
        }
    }

    if (!errorOccurred && ((smtpCount + smsCount + mmsCount) > 0)) {
        if (smtpCount)
            smtpClient->newConnection();
#ifndef QTOPIA_NO_SMS
        if (smsCount)
            smsClient->newConnection();
#endif
#ifndef QTOPIA_NO_MMS
        if (mmsCount)
            mmsClient->newConnection();
#endif

        // Calculate the total idicative size of the messages we're sending
        uint totalSize = 0;
        foreach (uint size, sendSize.values())
            totalSize += size;

        totalSendSize = 0;
        emit mailSendSize(totalSize);
    } else {
        emit mailSent(-1);
    }

#ifndef QTOPIA_NO_SMS
    if (!errorOccurred && (smsCount == 0))
        smsClient->clearList();
#endif
}

void EmailHandler::sendProgress(const QMailId& id, uint percentage)
{
    QMap<QMailId, uint>::const_iterator it = sendSize.find(id);
    if (it != sendSize.end()) {
        if (percentage > 100)
            percentage = 100;

        // Update the progress figure to count the sent portion of this message
        uint partialSize = (*it) * percentage / 100;
        emit transferredSize(totalSendSize + partialSize);
    } else {
        qWarning() << "Message" << id.toULongLong() << "not present in send map...";
    }
}

void EmailHandler::retrievalProgress(const QString& uid, uint percentage)
{
    QMap<QString, uint>::const_iterator it = retrievalSize.find(uid);
    if (it != retrievalSize.end()) {
        if (percentage > 100)
            percentage = 100;

        // Update the progress figure to count the sent portion of this message
        uint partialSize = (*it) * percentage / 100;
        emit downloadedSize(totalRetrievalSize + partialSize);
    } else {
        qWarning() << "Message" << uid << "not present in retrieval map...";
    }
}

void EmailHandler::messageProcessed(const QMailId& id)
{
    QMap<QMailId, uint>::iterator it = sendSize.find(id);
    if (it != sendSize.end()) {
        // Update the progress figure
        totalSendSize += *it;
        emit transferredSize(totalSendSize);

        sendSize.erase(it);
    } else {
        qWarning() << "Message" << id.toULongLong() << "not present in send map...";
    }
}

void EmailHandler::messageProcessed(const QString& uid)
{
    QMap<QString, uint>::iterator it = retrievalSize.find(uid);
    if (it != retrievalSize.end()) {
        // Update the progress figure
        totalRetrievalSize += *it;
        emit downloadedSize(totalRetrievalSize);

        retrievalSize.erase(it);
    } else {
        qWarning() << "Message" << uid << "not present in retrieval map...";
    }
}

void EmailHandler::setSmtpAccount(QMailAccount *account)
{
    smtpAccount = account;
}

void EmailHandler::setMailAccount(QMailAccount *account)
{
    mailAccount = account;
}

void EmailHandler::setSmsAccount(QMailAccount *account)
{
    smsAccount = account;
#ifndef QTOPIA_NO_SMS
    smsClient->setAccount( account );
#endif
}

void EmailHandler::setMmsAccount(QMailAccount *account)
{
    mmsAccount = account;
#ifndef QTOPIA_NO_MMS
    mmsClient->setAccount(account);
#endif
}

void EmailHandler::getMailHeaders()
{
    Client *client = clientFromAccount(mailAccount);
    if ( mailAccount->accountType() == QMailAccount::POP )
        receiving = true; // maybe this should be unconditional

    if (client) {
        client->setAccount(mailAccount);
        client->headersOnly(true, 2000);        //less than 2000, download all
        client->newConnection();
    }
}

void EmailHandler::getMailByList(MailList *mailList, bool newConnection)
{
    // We shouldn't have anything left in our retrieval list...
    if (!retrievalSize.isEmpty()) {
        foreach (const QString& uid, retrievalSize.keys())
            qWarning() << "Message" << uid << "still in retrieve map...";

        retrievalSize.clear();
    }

    if ((mailList->count() == 0)
        || !mailAccount){       //should not occur though
        emit mailTransferred(0);
        return;
    }
    receiving = true;

    Client *client = clientFromAccount(mailAccount);

    if (client) {
        client->headersOnly(false, 0);

        if (newConnection) {
            client->setAccount(mailAccount);
            client->newConnection();
        }

        uint totalSize = 0;

        QString* item = mailList->first();
        while (item) {
            // Find the indicative size of these messages
            QMailMessage message(mailList->currentId(), QMailMessage::Header);
            uint size = message.indicativeSize();

            retrievalSize.insert(*item, size);
            totalSize += size;

            item = mailList->next();
        }

        // Emit the total size we will receive
        totalRetrievalSize = 0;
        emit mailboxSize(totalSize);

        client->setSelectedMails(mailList, !newConnection);
    }
}

void EmailHandler::popQuit()
{
    Client *client = clientFromAccount(mailAccount);
    if (client)
        client->quit();

    receiving = false;
}

void EmailHandler::acceptMail(const QMailMessage& mail, bool closeAfterSend)
{
#ifndef QTOPIA_NO_MMS
    if (mmsClient && mail.messageType() == QMailMessage::Mms) {
        mmsClient->sendNotifyResp(mail, "Deferred");
    }

    if (closeAfterSend)
        mmsClient->closeWhenDone();
#else
    Q_UNUSED(mail);
    Q_UNUSED(closeAfterSend);
#endif
}

void EmailHandler::rejectMail(const QMailMessage& mail)
{
#ifndef QTOPIA_NO_MMS
    if (mmsClient && mail.messageType() == QMailMessage::Mms) {
        mmsClient->sendNotifyResp(mail, "Rejected");
    }
#else
    Q_UNUSED(mail);
#endif
}

void EmailHandler::unresolvedUidl(QStringList &list)
{
    QString user = mailAccount->id();

    emit unresolvedUidlList(user, list);
}

void EmailHandler::mailRead(const QMailMessage& mail)
{
#ifndef QTOPIA_NO_SMS
    smsClient->mailRead( mail );
#else
    Q_UNUSED(mail);
#endif
}

void EmailHandler::cancel()
{
    QString msg = tr( "Cancelled by user" );

    receiving = false;

    /* all clients handles this call regardless of whether
        they are actually in use or not (disregarded) */
    popClient->errorHandling(ErrCancel, msg);
    imapClient->errorHandling(ErrCancel, msg);
    smtpClient->errorHandling(ErrCancel, msg);
#ifndef QTOPIA_NO_SMS
    smsClient->errorHandling(ErrCancel, msg);
#endif
#ifndef QTOPIA_NO_MMS
    mmsClient->errorHandling(ErrCancel, msg);
#endif
}

Client* EmailHandler::clientFromAccount(const QMailAccount *account) const
{
    if ( !account )
	return 0;
    if ( account->accountType() == QMailAccount::POP )
        return popClient;
#ifndef QTOPIA_NO_MMS
    else if ( account->accountType() == QMailAccount::MMS)
        return mmsClient;
#endif
#ifndef QTOPIA_NO_SMS
    else if ( account->accountType() == QMailAccount::SMS)
        return smsClient;
#endif
    else if ( account->accountType() == QMailAccount::IMAP)
        return imapClient;
    return 0;
}

QMailAccount* EmailHandler::accountFromClient(const Client *client) const
{
    if ( client == popClient || client == imapClient )
        return mailAccount;
#ifndef QTOPIA_NO_MMS
    else if ( client == mmsClient )
        return mmsAccount;
#endif
#ifndef QTOPIA_NO_SMS
    else if ( client == smsClient )
        return smsAccount;
#endif
    else if ( client == smtpClient )
        return smtpAccount;
    return 0;
}

void EmailHandler::messagesReceived()
{
    if (const Client* client = static_cast<const Client*>(sender())) {
        if (unsynchronised.contains(client)) {
            unsynchronised.removeAll(client);

            if (unsynchronised.isEmpty()) {
                qLog(Messaging) << "Messaging clients synchronised";
                emit allMessagesReceived();
            }
        }
    }
}

int EmailHandler::newMessageCount()
{
    QSettings mailconf("Trolltech", "qtmail");
    mailconf.beginGroup("SystemMessages");
    int count = mailconf.value("newSystemCount").toInt();

#ifndef QTOPIA_NO_SMS
    count += smsClient->unreadSmsCount();
#endif
#ifndef QTOPIA_NO_MMS
    count += mmsClient->unreadMmsCount();
#endif

    return count;
}

void EmailHandler::pushMmsMessage(const QDSActionRequest& request)
{
#ifndef QTOPIA_NO_MMS
    mmsClient->pushMmsMessage(request);
#else
    Q_UNUSED(request);
#endif
}

void EmailHandler::receiveStatusChange(const QString& status)
{
    if (const Client* client = static_cast<const Client*>(sender()))
        emit updateReceiveStatus(client, status);
}

void EmailHandler::sendStatusChange(const QString& status)
{
    if (const Client* client = static_cast<const Client*>(sender()))
        emit updateSendStatus(client, status);
}

