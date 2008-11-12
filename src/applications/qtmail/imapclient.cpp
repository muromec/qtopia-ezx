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



#include "imapclient.h"
#include "emailhandler.h"
#include <longstream_p.h>

ImapClient::ImapClient()
{
    connect(&client, SIGNAL(finished(ImapCommand&,OperationState&)),
            this, SLOT(operationDone(ImapCommand&,OperationState&)) );
    connect(&client, SIGNAL(mailboxListed(QString&,QString&,QString&)),
            this, SLOT(mailboxListed(QString&,QString&,QString&)) );
    connect(&client, SIGNAL(messageFetched(QMailMessage&)),
            this, SLOT(messageFetched(QMailMessage&)) );
    connect(&client, SIGNAL(nonexistentMessage(QString)),
            this, SLOT(nonexistentMessage(QString)) );
    connect(&client, SIGNAL(downloadSize(int)),
            this, SLOT(downloadSize(int)) );
    connect(&client, SIGNAL(updateStatus(QString)),
            this, SIGNAL(updateStatus(QString)) );
    connect(&client, SIGNAL(connectionError(int,QString)),
            this, SLOT(errorHandling(int,QString)) );
}

ImapClient::~ImapClient()
{
}

void ImapClient::newConnection()
{
    if ( client.connected() ) {
        qLog(IMAP) << "socket in use, connection refused";
        return;
    }

    if ( account->mailServer().isEmpty() ) {
        emit mailTransferred(0);
        return;
    }

    status = Init;

    unresolvedUid.clear();
    selected = false;
    tlsEnabled = false;
    messageCount = 0;

    atCurrentBox = 0;
    internalId = QMailId();

    client.open( *account );
}

void ImapClient::operationDone(ImapCommand &command, OperationState &state)
{
    if ( state != OpOk ) {
        switch ( command ) {
            case IMAP_UIDStore:
            {
                // Couldn't set a flag, ignore as we can stil continue
                qLog(IMAP) << "could not store message flag";
                break;
            }

            case IMAP_Login:
            {
                errorHandling(ErrLoginFailed, client.lastError() );
                return;
            }

            case IMAP_Full:
            {
                errorHandling(ErrFileSystemFull, client.lastError() );
                return;
            }

            default:        //default = all critical messages
            {
                errorHandling(ErrUnknownResponse, client.lastError() );
                return;
            }
        }
    }

    switch( command ) {
        case IMAP_Init:
        {
            // Cannot be emitted in 4.3 due to string freeze:
            //emit updateStatus( tr("Checking capabilities" ) );
            client.capability();
            break;
        }
        case IMAP_Capability:
        {
#ifndef QT_NO_OPENSSL
            if (!tlsEnabled && (account->mailEncryption() == QMailAccount::Encrypt_TLS)) {
                if (client.supportsCapability("STARTTLS")) {
                    // Cannot be emitted in 4.3 due to string freeze:
                    //emit updateStatus( tr("Starting TLS" ) );
                    client.startTLS();
                    break;
                } else {
                    // TODO: request user direction
                    qWarning() << "No TLS support - continuing unencrypted";
                }
            }
#endif
            emit updateStatus( tr("Logging in" ) );
            client.login(account->mailUserName(), account->mailPassword());
            break;
        }
        case IMAP_StartTLS:
        {
            // We are now in TLS mode
            tlsEnabled = true;

            // Check capabilities for encrypted mode
            client.capability();
            break;
        }
        case IMAP_Login:
        {
            emit updateStatus( tr("Retrieving folders") );
            mailboxNames.clear();

            if ( selected ) {
                fetchNextMail();    //get selected messages only
            } else {
                client.list(account->baseFolder(), "*");        //account->baseFolder() == root folder
            }
            break;
        }
        case IMAP_List:
        {
            removeDeletedMailboxes();
            emit serverFolders();

            // Could be no mailbox has been selected to be stored locally
            if ( !nextMailbox() ) {
//              errorHandling(ErrUnknownResponse, tr("No Mailbox on Server") );
                emit mailTransferred( 0 );
                return;
            }

            emit updateStatus( tr("Checking ") + currentBox->displayName() );
            client.select( currentBox->pathName() );
            break;
        }
        case IMAP_Select:
        {
            handleSelect();
            break;
        }
        case IMAP_UIDSearch:
        {
            handleSearch();
            break;
        }
        case IMAP_UIDFetch:
        {
            handleUidFetch();
            break;
        }

        case IMAP_UIDStore:
        {
            setNextDeleted();
            break;
        }

        case IMAP_Expunge:
        {
            // Deleted messages, we can handle UID now
            handleUid();
            break;
        }

        case IMAP_Logout:
        {
            emit mailTransferred( 0 );
            return;

            break;
        }

        case IMAP_Full:
        {
            qFatal( "Logic error, IMAP_Full" );
            break;
        }

    }
}

void ImapClient::handleUidFetch()
{
    if (status == Fetch) {    //getting headers
        //currentBox->setServerUid( client.mailboxUidList() );
        currentBox->setServerUid( _mailboxUidList );

        if (nextMailbox()) {
            emit updateStatus( tr("Checking ") + currentBox->displayName() );
            client.select( currentBox->pathName() );
        } else {
            previewCompleted();
            return;
        }
    } else if (status == Rfc822) {    //getting complete messages
        fetchNextMail();
        return;
    }
}

void ImapClient::handleSearch()
{
    switch(_searchStatus)
    {
    case All:
    {
      //deal with possbily buggy SEARCH command implementations

        if (client.mailboxUidList().count() == 0 &&
            client.mailboxUidList().count() != client.exists()) {
            qLog(IMAP) << "Inconsistent UID SEARCH result. Trying SEEN/UNSEEN";
            _searchStatus = Seen;
            client.uidSearch(MFlag_Seen);
        } else {
            _mailboxUidList = client.mailboxUidList();

            searchCompleted();
        }
        break;
    }
    case Seen:
    {
        _mailboxUidList = client.mailboxUidList();
        _searchStatus = Unseen;

        client.uidSearch(MFlag_Unseen);
        break;
    }
    case Unseen:
    {
        _mailboxUidList += client.mailboxUidList();
        _searchStatus = All; //reset

        searchCompleted();
        break;
    }
    default:
        qLog(IMAP) << "Unknown search status";
    }
}

void ImapClient::searchCompleted()
{
    uniqueUidList = currentBox->getNewUids( _mailboxUidList );
    expiredUidList = currentBox->getExpiredUids( _mailboxUidList );

    if (!expiredUidList.isEmpty())
        emit expiredMessages( expiredUidList, currentBox->pathName(), true );

    if (!messagesToDelete())
        handleUid();
}

void ImapClient::handleUid()
{
    if (uniqueUidList.count() > 0) {
        emit updateStatus( tr("Previewing ") + QString::number( uniqueUidList.count() ) );

        status = Fetch;
        client.uidFetch(uniqueUidList.first(), uniqueUidList.last(),
                        F_Uid | F_Rfc822_Size | F_Rfc822_Header );

    } else if (nextMailbox()) {
        emit updateStatus( tr("Checking ") + currentBox->displayName() );
        client.select( currentBox->pathName() );
    } else {
        previewCompleted();
    }

//    currentBox->setServerUid( client.mailboxUidList() );
}

bool ImapClient::messagesToDelete()
{
    if (account->deleteMail()) {
        delList = currentBox->msgToDelete();
        if (delList.count() == 0)
            return false;

        setNextDeleted();
        return true;
    }

    return false;
}

void ImapClient::setNextDeleted()
{
    if (delList.count() > 0) {
        msgUidl = delList.first();
        delList.removeAll( msgUidl );
        //precaution, don't download a header to a message we mark as deleted
        uniqueUidList.removeAll( msgUidl );

        emit updateStatus( tr("Deleting message %1").arg(msgUidl) );
        currentBox->msgDeleted( msgUidl );
        client.uidStore(msgUidl, MFlag_Deleted );
    } else {
        // All messages flagged as deleted, expunge them
        client.expunge();
    }
}

void ImapClient::handleSelect()
{
    /*  We arrive here when we want to get a single mail, need to change
        folder, and the correct folder has been selected.  Isn't async. prog. fun? */
    if (status == Rfc822) {
        emit updateStatus( tr("Completing %1 / %2").arg(messageCount).arg(mailList->count()) );
        client.uidFetch( msgUidl, msgUidl, F_Uid | F_Rfc822_Size | F_Rfc822 );
        return;
    }

    if (client.exists() > 0) {
        _searchStatus = All;
        client.uidSearch(1, client.exists() );  //get all uids just to be safe
    } else if (nextMailbox()) {
        emit updateStatus( tr("Checking ") + currentBox->displayName() );
        client.select( currentBox->pathName() );
    } else {    //last box checked
        previewCompleted();
    }
}

bool ImapClient::nextMailbox()
{
    bool found = false;

    while (!found) {
        if (atCurrentBox >= static_cast<uint>(account->mailboxes().count()))
            return false;

        currentBox = account->mailboxes().at(atCurrentBox);
        if (currentBox == NULL) {
            return false;
        }

        found = currentBox->localCopy();
        atCurrentBox++;
    }

    return true;
}

void ImapClient::fetchNextMail()
{
    QString *mPtr;

    if (messageCount == 0) {
        messageCount = 1;
        mPtr = mailList->first();
    } else {
        mPtr = mailList->next();
        messageCount++;
    }

    // Need twice the size of the mail free, extra 10kb is a margin of safety
    if (!LongStream::freeSpace( "", mailList->currentSize() * 2 + 1024*10 )) {
        errorHandling(ErrFileSystemFull, client.lastError() );
        return;
    }

    if (mPtr == NULL) {
        emit mailTransferred( messageCount );
        return;
    }

    status = Rfc822;

    // Get next message in queue, but verify it's still on the server
    currentBox = account->getMailboxRefByMsgUid(*mPtr, mailList->currentMailbox() );
    while ((currentBox == NULL) && (mPtr != NULL)) {
        qLog(Messaging) << "ImapClient::fetchNextMail: cant find uid" << qPrintable(*mPtr);
        unresolvedUid.append(*mPtr);
        mPtr = mailList->next();
        messageCount++;
        if (mPtr != NULL)
            currentBox = account->getMailboxRefByMsgUid(*mPtr, mailList->currentMailbox() );
    }

    if (currentBox != NULL) {
        internalId = mailList->currentId();
        retrieveUid = *mPtr;
        retrieveLength = mailList->currentSize();

        if ((currentBox->pathName()) == client.selected()) {
            emit updateStatus( tr("Completing %1 / %2").arg(messageCount).arg(mailList->count()) );
            msgUidl = *mPtr;
            client.uidFetch( msgUidl, msgUidl, F_Uid | F_Rfc822_Size | F_Rfc822 );
            return;

        } else {
            msgUidl = *mPtr;
            client.select( currentBox->pathName() );
            return;
        }
    } else {
	if (!mPtr)
	    errorHandling(ErrUnknownResponse, tr( "Message not found" ) );
        emit mailTransferred( messageCount );
    }
}

/*  Mailboxes retrived from the server goes here.  If the INBOX mailbox
    is new, it means it is the first time the account is used.  Set the
    local copy flag to on so that the user can get messages without
    needing to configure it first.
*/
void ImapClient::mailboxListed(QString &flags, QString &del, QString &name)
{
    Mailbox *box;
    QStringList list = name.split(del);
    QString item;

    for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
        item += *it;
        if ( ( box = account->getMailboxRef(item) ) == NULL ) {
            box = new Mailbox(account, flags, del, item);
            box->setLocalCopy( true );
            account->addBox(box);
        }

        item += del;
    }

    mailboxNames.append(name);
}

void ImapClient::messageFetched(QMailMessage& mail)
{     //set some other parameters

    mail.setFromAccount( account->id() );
    mail.setFromMailbox( currentBox->pathName() );

    if ( status == Fetch ) {
        mail.setId( QMailId());
        mail.setStatus(QMailMessage::Downloaded, false);
    } else {
        mail.setId( internalId );
        mail.setStatus(QMailMessage::Downloaded, true);
    }

    emit newMessage(mail);

    if (!retrieveUid.isEmpty()) {
        emit messageProcessed(retrieveUid);
        retrieveUid = QString();
    }
}

void ImapClient::nonexistentMessage(const QString& uid)
{
    if (uid == msgUidl) {
        emit nonexistentMessage(internalId);
    } else {
        emit updateStatus(tr("Error occurred"));

        QString errorText(tr("Message deleted from server"));
        emit errorOccurred(ErrNonexistentMessage, errorText);
    }
}

void ImapClient::setAccount(QMailAccount *_account)
{
    account = _account;
}

void ImapClient::errorHandling(int status, QString msg)
{
    if ( client.connected() ) {
        client.close();
    }

    emit updateStatus(tr("Error occurred"));
    emit errorOccurred(status, msg);
}

void ImapClient::quit()
{
    emit updateStatus( tr("Logging out") );
    client.logout();
}

void ImapClient::setSelectedMails(MailList *list, bool connected)
{
    selected = true;
    mailList = list;

    messageCount = 0;
    mailDropSize = 0;

    if ( connected ) {
        fetchNextMail();
    }
}

/*  removes any mailboxes form the client list which no longer is
    registered on the server.  Note that this will not apply to
    mailboxes which are newly created by the user.
*/
void ImapClient::removeDeletedMailboxes()
{
    QList<Mailbox*> nonexistent;
    foreach (Mailbox* box, account->mailboxes()) {
        if ( !box->userCreated() ) {
            bool exists = false;
            for (QStringList::Iterator it = mailboxNames.begin();
                 it != mailboxNames.end(); ++it) {
                if ( *it == box->pathName() ) {
                    exists = true;
                } else if ( *it == box->nameChanged() ) {
                    box->changeName(*it, false);
                    exists = true;
                }
            }
            if (!exists) {
                nonexistent.append(box);
            }
        }
    }

    foreach (Mailbox* box, nonexistent) {
        // Any messages in this box should be removed also
        QStringList expiredUidList = box->getExpiredUids(QStringList());
        if (!expiredUidList.isEmpty())
            emit expiredMessages( expiredUidList, box->pathName(), false );

        account->removeBox(box);
    }
}

void ImapClient::checkForNewMessages()
{
    // Don't implement this properly yet - emails are currently only DL'd on-demand
    emit allMessagesReceived();
}

int ImapClient::newMessageCount()
{
    return 0;
}

void ImapClient::previewCompleted()
{
    emit mailTransferred(0);
}

void ImapClient::downloadSize(int length)
{
    if (!retrieveUid.isEmpty() && retrieveLength) {
        uint percentage = qMin<uint>(length * 100 / retrieveLength, 100);
        emit retrievalProgress(retrieveUid, percentage);
    }
}

