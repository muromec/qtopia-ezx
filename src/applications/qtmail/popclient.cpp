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

#include "popclient.h"

#include <qtopialog.h>

#include "mailtransport.h"
#include "emailhandler.h"
#include <longstream_p.h>

PopClient::PopClient()
{
    receiving = false;
    headerLimit = 0;
    preview = false;
    transport = 0;
    d = new LongStream();
}

PopClient::~PopClient()
{
    delete d;
    delete transport;
}

void PopClient::newConnection()
{
    if (receiving) {
        qLog(POP) << "transport in use, connection refused";
        return;
    }

    if ( account->mailServer().isEmpty() ) {
        status = Exit;
        emit mailTransferred(0);
        return;
    }

    status = Init;
    uidlList.clear();
    uniqueUidlList.clear();
    unresolvedUidl.clear();
    sizeList.clear();
    receiving = true;
    selected = false;
    awaitingData = false;
    newMessages = 0;
    messageCount = 0;
    internalId = QMailId();
    deleteList.clear();

    if (!transport) {
        // Set up the transport
        transport = new MailTransport("POP");

        connect(transport, SIGNAL(updateStatus(QString)),
                this, SIGNAL(updateStatus(QString)));
        connect(transport, SIGNAL(errorOccurred(int,QString)),
                this, SLOT(errorHandling(int,QString)));
        connect(transport, SIGNAL(readyRead()),
                this, SLOT(incomingData()));
    }

    transport->open(*account);
}

void PopClient::setAccount(QMailAccount *_account)
{
    account = _account;
    lastUidl = account->getUidlList();
}

void PopClient::headersOnly(bool headers, int limit)
{
    preview = headers;
    headerLimit = limit;
}

void PopClient::setSelectedMails(MailList *list, bool connected)
{
    selected = true;
    mailList = list;

    messageCount = 0;
    newMessages = 0;

    if (connected) {
        status = Retr;
        incomingData();
    }
}

void PopClient::errorHandling(int status, QString msg)
{
    if ( receiving ) {
        receiving = false;
        account->setUidlList(lastUidl);
        transport->close();
        emit errorOccurred(status, msg);
    }
}

void PopClient::quit()
{
    if ( status == Exit ) {
        emit mailTransferred(0);
    } else {
        status = Quit;
        incomingData();
    }
}

void PopClient::sendCommand(const QString& cmd)
{
    transport->stream() << cmd << "\r\n" << flush;
    
    if (cmd.length())
        qLog(POP) << "SEND:" << qPrintable(cmd);
}

QString PopClient::readResponse() 
{
    QString response = transport->readLine();

    if (response.length() > 1)
        qLog(POP) << "RECV:" << qPrintable(response.left(response.length() - 2));

    return response;
}

void PopClient::incomingData()
{
    QString response, temp;

    if ( (status != Dele) && (status != Quit) )
        response = readResponse();

    if (status == Init) {
        emit updateStatus(tr("Logging in"));
        transport->stream().setCodec("UTF-8");
        sendCommand("USER " + account->mailUserName());
        status = Pass;
    } else if (status == Pass) {
        if (response[0] != '+') {
            errorHandling(ErrLoginFailed, "");
            return;
        }
        sendCommand("PASS " + account->mailPassword());
        status = Uidl;
    } else if (status == Uidl) {
        if (response[0] != '+') {
            errorHandling(ErrLoginFailed, "");
            return;
        }

        status = Guidl;
        awaitingData = false;
        sendCommand("UIDL");
        return;
    } else if (status == Guidl) {           //get list of uidls

        //means first time in, response should be "+Ok"
        if (! awaitingData) {
            if ( response[0] != '+' ) {
                errorHandling(ErrUnknownResponse, response);
                return;
            }

            awaitingData = true;
            if ( transport->canReadLine() ) {
                response = readResponse();
            } else {
                return;
            }
        }
        uidlList.append( response.mid(0, response.length() - 2) );
        while ( transport->canReadLine() ) {
            response = readResponse();
            uidlList.append( response.mid(0, response.length() - 2) );
        }
        if (response == ".\r\n") {
            uidlList.removeLast();
            status = List;
        } else {
            return;          //more data incoming
        }
    }

    if (status == List) {
        sendCommand("LIST");
        awaitingData = false;
        status = Size;
        return;
    }

    if (status == Size) {
        //means first time in, response should be "+Ok"
        if (!awaitingData) {
            if ( response[0] != '+' ) {
                errorHandling(ErrUnknownResponse, response);
                return;
            }

            awaitingData = true;
            if ( transport->canReadLine() )
                response = readResponse();
            else return;
        }
        sizeList.append( response.mid(0, response.length() - 2) );
        while ( transport->canReadLine() ) {
            response = readResponse();
            sizeList.append( response.mid(0, response.length() - 2) );
        }
        if (response == ".\r\n") {
            sizeList.removeLast();
            status = Retr;
            uidlIntegrityCheck();
        } else {
            return;          //more data incoming
        }
    }

    if (status == Retr) {

        msgNum = nextMsgServerPos();
        if (msgNum != -1) {

            if (selected) {
                // Need twice the size of the mail free, 10kb margin of safety
                const int minFree = mailList->currentSize() * 2 + 1024*10;
                if (!LongStream::freeSpace( "", minFree )) {
                    errorHandling(ErrFileSystemFull, LongStream::errorMessage( "\n" ));
                    status = Exit;
                    return;
                }
            }

            if (!selected) {
                if (messageCount == 1)
                    emit updateStatus( tr("Previewing ") + QString::number( uniqueUidlList.count() ) );
            } else {
                emit updateStatus( tr("Completing %1 / %2").arg( messageCount ).arg( mailList->count() ) );
            }

            temp.setNum(msgNum);
            if (!preview || mailSize <= headerLimit) {
                sendCommand("RETR " + temp);
            } else {                                //only header
                sendCommand("TOP " + temp + " 0");
            }

            status = Ignore;
            return;
        } else {
            status = Acks;
        }
    }

    if (status == Ignore) {
        if (response[0] == '+') {
            message = "";
            d->reset();
            status = Read;
            if (!transport->canReadLine())    //sync. problems
                return;
            response = readResponse();
        } else errorHandling(ErrUnknownResponse, response);
    }

    if (status == Read) {
        message += response;
        d->append( response );
        if (d->status() == LongStream::OutOfSpace) {
            errorHandling(ErrFileSystemFull, LongStream::errorMessage( "\n" ));
            return;
        }

        while ( transport->canReadLine()) {
            response = readResponse();
            message += response;
            message = message.right( 100 );
            d->append( response );
            if (d->status() == LongStream::OutOfSpace) {
                errorHandling(ErrFileSystemFull, LongStream::errorMessage( "\n" ));
                return;
            }
        }

        message = message.right( 100 ); // 100 > 5 need at least 5 chars
        if (message.indexOf("\r\n.\r\n", -5) == -1) {
            // We have an incomplete message 
            if (!retrieveUid.isEmpty() && retrieveLength) {
                uint percentage = qMin<uint>(d->length() * 100 / retrieveLength, 100);
                emit retrievalProgress(retrieveUid, percentage);
            }
            return;
        }

        createMail();

        // in case user jumps out, don't download message on next entry
        if (preview) {
            lastUidl.append(msgUidl);
            account->setUidlList(lastUidl);
        }

        newMessages++;

        // We have now retrieved the entire message
        if (!retrieveUid.isEmpty()) {
            emit messageProcessed(retrieveUid);
            retrieveUid = QString();
        }

        if(!preview || (preview && mailSize <= headerLimit) && account->deleteMail())
            account->deleteMsg(msgUidl,0);

        status = Retr;
        incomingData();         //remember: recursive
        return;
    }

    if (status == Acks) {
        //ok, delete mail, await user input
        if ( account->deleteMail() ) {
            status = Dele;
            awaitingData = false;
            deleteList = account->msgToDelete();
            if ( deleteList.isEmpty() )
                status = Done;
        } else {
            status = Done;
        }
    }

    if (status == Dele) {
        if ( awaitingData ) {
            response = readResponse();
            if ( response[0] != '+' ) {
                errorHandling(ErrUnknownResponse, response);
                return;
            }
        } else {
            qLog(POP) << qPrintable(QString::number( deleteList.count() ) + " messages in mailbox to be deleted");
            emit updateStatus(tr("Removing old messages"));
        }

        if ( deleteList.count() == 0 ) {
            status = Done;
        } else {
            QString str = deleteList.first();
            QString strPos = msgPosFromUidl( str );
            deleteList.removeAll( str );
            account->msgDeleted( str, "" );

            if ( !strPos.isEmpty() ) {
                awaitingData = true;
                qLog(POP) << qPrintable("deleting message at pos: " + strPos);
                sendCommand("DELE " + strPos);
            } else {
                qLog(POP) << qPrintable("unable to delete unlisted message: " + str);
                incomingData();
                return;
            }
        }
    }

    if (status == Done) {
        if ( preview ) {
            emit mailTransferred(newMessages);
            return;
        }

        status = Quit;
    }

    if (status == Quit) {
        status = Exit;
        sendCommand("QUIT");
        return;
    }

    if (status == Exit) {
        transport->close();        //close regardless
        receiving = false;
        emit updateStatus(tr("Communication finished"));

        if (unresolvedUidl.count() > 0 && !preview) {
            emit unresolvedUidlList(unresolvedUidl);
        }

        account->setUidlList(uidlList);
        emit mailTransferred(newMessages);
    }
}

QString PopClient::getUidl(QString uidl)
{
    return uidl.mid( uidl.indexOf(" ") + 1, uidl.length() );
}

QString PopClient::msgPosFromUidl(QString uidl)
{
    QRegExp exactUid("\\b" + uidl + "\\b");
    QStringList list = uidlList.filter(exactUid);
    if ( list.count() != 1 )        //should be only 1 match
        return QString::null;

    QString str = list.join("").trimmed();
    bool ok;
    uint x = str.mid(0, str.indexOf(" ") ).toUInt(&ok);

    if ( ok ) {
        return QString::number(x);              //found current pos of msg
    } else {
        return QString::null;
    }
}

int PopClient::nextMsgServerPos()
{
    int thisMsg = -1;
    QString *mPtr;

    if (preview) {
        if ( messageCount < uniqueUidlList.count() ) {
            const QString& it = uniqueUidlList.at(messageCount);
            messageCount++;
            thisMsg = ( it.left( it.indexOf(" ") ) ).toInt();
            msgUidl = it;
            internalId = QMailId();
        }
    }

    if (selected) {
        if (messageCount == 0) {
            messageCount = 1;
            mPtr = mailList->first();
        } else {
            ++messageCount;
            mPtr = mailList->next();
        }

        QStringList ref;

        // if requested mail is not on server, try to get
        // a new mail from the list
        while ( (mPtr != NULL) && (ref.count() == 0) ) {
            QRegExp exactUid("\\b" + *mPtr + "\\b");
            ref = uidlList.filter(exactUid);

            if (ref.count() != 1) {
                unresolvedUidl.append(*mPtr);
                mPtr = mailList->next();
            } else {
                const QString& it = ref.first();
                thisMsg = ( it.left( it.indexOf(" ") ) ).toInt();
                msgUidl = it;
            }
        }

        if (ref.count() != 1)           //temporary, what if two? should never happen
            return thisMsg;

        internalId = mailList->currentId();
        if ((mPtr != NULL) && !preview) {
            retrieveUid = *mPtr;
            retrieveLength = mailList->currentSize();
        }
    }

    getSize(thisMsg);

    return thisMsg;
}

// get the reported server size from stored list
int PopClient::getSize(int pos)
{
    int sizeRef = 0;
    for ( QStringList::Iterator it = sizeList.begin(); it != sizeList.end(); ++it ) {
        sizeRef = ( (*it).left( (*it).indexOf(" ") ) ).toInt();

        if (sizeRef == pos) {
            mailSize = ( (*it).mid( (*it).indexOf(" ") + 1, (*it).length() )).toInt();
            return mailSize;
        }
    }

    return -1;
}

// checks the list of uidls retrieved from the server by
// comparing them against the last known serverlist.
// if everything is ok, then qtmail is a very happy program
void PopClient::uidlIntegrityCheck()
{
    QString str;

    int pos = 1, size;
    QStringList::Iterator itSize = sizeList.begin();
    for (QStringList::Iterator it = uidlList.begin(); it != uidlList.end(); ++it ) {
        size = getSize(pos);
        str.setNum(size);
        itSize++;
        pos++;
    }

    // create list of new entries that should be downloaded
    if (preview) {
        if ( !account->synchronize() ) {
            lastUidl.clear();
        }

        QStringList previousList;
        QString thisUidl;
        for ( QStringList::Iterator it = uidlList.begin(); it != uidlList.end(); ++it ) {
            thisUidl = getUidl(*it);        //strips pos

            if ( (lastUidl.filter(thisUidl)).count() == 0 ) {
                uniqueUidlList.append(*it);
            } else {
                previousList.append(*it);
            }

        }
        lastUidl = previousList;
        messageCount = 0;
    }
}

void PopClient::createMail()
{
    QMailMessage mail = QMailMessage::fromRfc2822File( d->fileName() );
    mail.setId( internalId );

    bool isComplete = ((!preview ) || ((preview) && (mailSize <= headerLimit)));
    mail.setStatus( QMailMessage::Incoming, true);
    mail.setStatus( QMailMessage::Downloaded, isComplete );

    mail.setSize( mailSize );
    mail.setServerUid( msgUidl.mid( msgUidl.indexOf(" ") + 1, msgUidl.length() ) );

    mail.setFromAccount( account->id() );
    mail.setMessageType(QMailMessage::Email);
    mail.setFromMailbox("");
    emit newMessage(mail);
    d->reset();
}

void PopClient::checkForNewMessages()
{
    // Don't implement this properly yet - emails are currently only DL'd on-demand
    emit allMessagesReceived();
}

int PopClient::newMessageCount()
{
    return 0;
}

