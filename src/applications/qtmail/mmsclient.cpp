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

#include "mmsclient.h"
#include "mmscomms.h"
#ifdef MMSCOMMS_HTTP
# include "mmscomms_http.h"
#endif
#include "maillist.h"
#include "mmsmessage.h"
#include <QHttp>
#include <QUrl>
#include <QDSAction>
#include <QDSActionRequest>
#include <QDSData>
#include <QDSServices>
#include <QDSServiceInfo>
#include <QBuffer>

#include <qtopiaapplication.h>
#include <qtopiaipcenvelope.h>
#include <qtopialog.h>
#include <qmailcodec.h>
#include <qmailtimestamp.h>

#include <QtopiaAbstractService>
#include <QNetworkDevice>
#include <QtopiaNetwork>
#include <QWapAccount>
#include <QMailMessage>
#include <QDrmContentPlugin>

QString getPath(const QString& fn, bool isdir=false);


int MmsClient::txnId = 1;

MmsClient::MmsClient()
    : account(0), comms(0), mailList(0), quitRecv(false), 
      networkReference(false), networkDevice(0), networkStatus(QtopiaNetworkInterface::Unknown)
{
    connect(&raiseTimer, SIGNAL(timeout()), this, SLOT(raiseFailure()));
    connect(&inactivityTimer, SIGNAL(timeout()), this, SLOT(networkDormant()));

    QDrmContentPlugin::initialize();
}

MmsClient::~MmsClient()
{
    raiseTimer.stop();
    inactivityTimer.stop();

    if (networkReference) 
    {
        qLog(Messaging) << "dtor stopInterface:" << mmsInterfaceName();
        QtopiaNetwork::stopInterface(mmsInterfaceName());
    }
}

void MmsClient::setAccount(QMailAccount *_account)
{
    if (account == _account)
        return;
    if (comms && comms->isActive()) {
        qWarning("MMS connection is already use.");
        return;
    }

    account = _account;

    delete comms;

#ifdef MMSCOMMS_HTTP
    qLog(Messaging) << "Using MMSCommsHttp reference implementation";
    comms = new MmsCommsHttp(account, this);
#else
# error "No MMS comms implementation supplied"
#endif
    connect(comms, SIGNAL(notificationInd(MMSMessage)),
            this, SLOT(notificationInd(MMSMessage)));
    connect(comms, SIGNAL(deliveryInd(MMSMessage)),
            this, SLOT(deliveryInd(MMSMessage)));
    connect(comms, SIGNAL(sendConf(MMSMessage)),
            this, SLOT(sendConf(MMSMessage)));
    connect(comms, SIGNAL(retrieveConf(MMSMessage,int)),
            this, SLOT(retrieveConf(MMSMessage,int)));
    connect(comms, SIGNAL(statusChange(QString)),
            this, SLOT(statusChange(QString)));
    connect(comms, SIGNAL(error(int,QString)),
            this, SLOT(commsError(int,QString)));
    connect(comms, SIGNAL(transferSize(int)),
            this, SLOT(transferSize(int)));
    connect(comms, SIGNAL(transfersComplete()),
            this, SLOT(transfersComplete()));

    qLog(Messaging) << "MmsClient: Using network config:" << networkConfig();

    QString interface(mmsInterfaceName());
    if (!interface.isEmpty()) 
    {
        qLog(Messaging) << "MmsClient: Using data account:" << interface;

        networkDevice = new QNetworkDevice(interface, this);
        if (networkDevice->error() == QtopiaNetworkInterface::NoError)
        {
            connect(networkDevice, SIGNAL(stateChanged(QtopiaNetworkInterface::Status,bool)),
                    this, SLOT(networkStatusChange(QtopiaNetworkInterface::Status,bool)));
        }
        else 
        {
            qLog(Messaging) << "MmsClient: Error on networkDevice:" << networkDevice->errorString();
        }
    }
}

void MmsClient::newConnection()
{
    messagesSent = 0;
    sendNextMessage();
    quitRecv = false;
}

void MmsClient::quit()
{
    if (comms && comms->isActive()) {
        quitRecv = true;
    } else {
        emit mailTransferred(0);
    }
}

bool MmsClient::addMail(const QMailMessage &mail)
{
    MMSMessage mms = convertToMms(mail);
    QWapAccount acc( networkConfig() );
    switch(acc.mmsSenderVisibility() ) {
        case QWapAccount::SenderHidden:
            mms.addField("X-Mms-Sender-Visibility", "Hide");
            break;
        case QWapAccount::SenderVisible:
            mms.addField("X-Mms-Sender-Visibility", "Show");
            break;
        default:
            break;
    }
    int exp = acc.mmsExpiry();
    if (exp)
        mms.addField("X-Mms-Expiry", exp*3600);

    outgoing.append(mms);
    return true;
}

void MmsClient::sendNextMessage()
{
    if (!outgoing.count())
        return;
    
    // Ensure that the network is up
    if (raiseNetwork())
    {
        transferNextMessage();
    }
    else
    {
        // If it's borked, we want to respond appropriately
        if (networkStatus == QtopiaNetworkInterface::Unavailable)
        {
            emit updateStatus(tr("Network Fault"));
            
            QString msg(tr("Invalid network interface"));
            emit errorOccurred(0, msg);
        }
        else
        {
            const int raisePeriod = 10 * 1000;
            raiseTimer.setInterval(raisePeriod);
            raiseTimer.start();
        }
    }
}

void MmsClient::transferNextMessage()
{
    if (!outgoing.count())
        return;
    
    qLog(Messaging) << "Sending MMS message";
    MMSMessage &msg = outgoing.first();
    sentMessages.insert(msg.txnId(), msg.messageId());
    sendMessage(msg);

    const int inactivityPeriod = 20 * 1000;
    inactivityTimer.setInterval(inactivityPeriod);
    inactivityTimer.start();
}

void MmsClient::sendConf(const MMSMessage &msg)
{
    QString txnId(msg.txnId());
    qLog(Messaging) << "Received send conf, txn id" << (txnId.isEmpty() ? QString("unknown") : txnId);

    if (!txnId.isEmpty()) {
        QMap<QString, QMailId>::iterator it = sentMessages.find(txnId);
        if (it != sentMessages.end()) {
            // Report this message as processed regardless of transmission success
            emit messageProcessed(sentMessages[txnId]);

            sentMessages.erase(it);
            sentLength = 0;
            messageLength = 0;
        } else {
            qWarning() << "MMS: Cannot process unknown message:" << txnId;
        }
    }

    const QWspField *field = msg.field("X-Mms-Response-Status");
    if (field && field->value != "Ok") {
        // ### handle send error
        errorHandling(0, field->value);
    } else {
        messagesSent++;
        outgoing.removeFirst();
        if (outgoing.count()) {
            sendNextMessage();
        } else {
            emit mailSent(messagesSent);
            emit transmissionCompleted();
        }
    }
}

static QMailMessage messageFromHeaders(const QList<QWspField>& headers)
{
    QString text;
    QList<QWspField>::ConstIterator it = headers.begin(), end = headers.end();
    for ( ; it != end; ++it) {
        text += (*it).name + ": " + (*it).value + QMailMessage::CRLF;
    }

    return QMailMessage::fromRfc2822(text.toLatin1());
}

void MmsClient::notificationInd(const MMSMessage &mmsMsg)
{
    const QWspField *field = mmsMsg.field("X-Mms-Transaction-Id");
    if (!field) {
        qWarning("Invalid m-notification-ind");
        return;
    }
    QString txnId = field->value;

    field = mmsMsg.field("X-Mms-Content-Location");
    if (!field) {
        qWarning("Invalid m-notification-ind");
        return;
    }
    QString location = field->value;

    int size = 0;
    field = mmsMsg.field("X-Mms-Message-Size");
    if (field)
        size = field->value.toInt();

    internalId = QMailId();

    QMailMessage mail(messageFromHeaders(mmsMsg.headers()));
    mail.setId(internalId);
    mail.setFrom(QMailAddress(decodeRecipient(mail.from().toString())));        // fix phone numbers
    mail.setMessageType(QMailMessage::Mms);
    mail.setSize(size);
    mail.setDate(QMailTimeStamp::currentDateTime());
    mail.setServerUid(location);
    mail.setStatus(QMailMessage::Downloaded, false);
    mail.setStatus(QMailMessage::Incoming, true);
    mail.setFromAccount(account->id());
    mail.setFromMailbox(QString());
    mail.setHeaderField("X-Mms-Transaction-Id", txnId);

    emit newMessage(mail);

    emit allMessagesReceived();
}

void MmsClient::sendNotifyResp(const QMailMessage &notification, const QString &status)
{
    MMSMessage resp;
    resp.setType(MMSMessage::MNotifyResp);
    resp.addField("X-Mms-MMS-Version", "1.0");
    QString field = notification.headerFieldText("X-Mms-Transaction-Id");
    resp.addField("X-Mms-Transaction-Id", field.mid(field.indexOf(':')+1));
    resp.addField("X-Mms-Status", status);
    QWapAccount acc( networkConfig() );
    if ( !acc.mmsDeliveryReport() )
        resp.addField("X-Mms-Report-Allowed", "No");

    sendMessage(resp);
}

void MmsClient::resetNewMailCount()
{
    QSettings mailconf("Trolltech", "qtmail");
    mailconf.beginGroup("MMS");
    if (mailconf.value("newMmsCount").toInt()) {
        mailconf.setValue("newMmsCount", 0);
        QtopiaIpcEnvelope e("QPE/System", "newMmsCount(int)");
        e << static_cast<int>(0);
    }
}

int MmsClient::unreadMmsCount()
{
    QSettings mailconf("Trolltech", "qtmail");
    return mailconf.value("MMS/newMmsCount").toInt();
}

void MmsClient::setSelectedMails(MailList *list, bool connected)
{
    Q_UNUSED(connected);

    mailList = list;
    messagesRecv = 0;
    getNextMessage();
}

void MmsClient::getNextMessage()
{
    if (!mailList)
        return;
    QString *msg;

    msg = (messagesRecv == 0 ? mailList->first() : mailList->next());
    if (msg) {
        internalId = mailList->currentId();
        QUrl url(*msg);
        comms->retrieveMessage(url);
    } else {
        emit mailTransferred(messagesRecv);
        mailList = 0;
    }
}

void MmsClient::retrieveConf(const MMSMessage &msg, int size)
{
    messagesRecv++;
    sendAcknowledge(msg);
    
    emit newMessage(convertToEmail(msg, size));

    getNextMessage();
}

void MmsClient::sendAcknowledge(const MMSMessage &retr)
{
    MMSMessage resp;
    resp.setType(MMSMessage::MAckowledgeInd);
    resp.addField("X-Mms-MMS-Version", "1.0");
    const QWspField *field = retr.field("X-Mms-Transaction-Id");
    if (field)
        resp.addField("X-Mms-Transaction-Id", field->value);
    QWapAccount acc( networkConfig() );
    if ( !acc.mmsDeliveryReport() )
        resp.addField("X-Mms-Report-Allowed", "No");

    sendMessage(resp);
}

void MmsClient::deliveryInd(const MMSMessage &mmsMsg)
{
    internalId = QMailId();

    QMailMessage mail(messageFromHeaders(mmsMsg.headers()));
    mail.setId(internalId);
    mail.setStatus(QMailMessage::Downloaded, true);
    mail.setStatus(QMailMessage::Incoming, true);
    mail.setFromAccount(account->id());
    mail.setFromMailbox("");

    emit newMessage(mail);
}

void MmsClient::transferSize(int done)
{
    if ((sentMessages.count() == 1) && messageLength) {
        const QMailId& id = (*sentMessages.begin());

        sentLength += done;
        uint percentage = qMin<uint>(sentLength * 100 / messageLength, 100);
        emit sendProgress(id, percentage);
    }
}

void MmsClient::statusChange(const QString &status)
{
    emit updateStatus(status);
}

void MmsClient::commsError(int code, const QString &str)
{
    QString tmp(str);
    emit updateStatus(tr("Error occurred"));
    emit errorOccurred(code, tmp);
}

void MmsClient::transfersComplete()
{
    if (quitRecv) {
        emit mailTransferred(0);
        quitRecv = false;
    }
}

static bool online(QtopiaNetworkInterface::Status status)
{
    return ((status == QtopiaNetworkInterface::Up) || (status == QtopiaNetworkInterface::Pending) || (status == QtopiaNetworkInterface::Demand));
}

void MmsClient::networkStatusChange(QtopiaNetworkInterface::Status status, bool)
{
    const QtopiaNetworkInterface::Status unavailableStatus(QtopiaNetworkInterface::Unavailable);

    bool nowOnline(online(status) && !online(networkStatus));
    bool nowUnavailable((status == unavailableStatus) && (networkStatus != unavailableStatus));

    networkStatus = status;

    if (nowOnline || nowUnavailable)
    {
        raiseTimer.stop();

        if (nowOnline && outgoing.count())
        {
            transferNextMessage();
        }
        else if (nowUnavailable && networkReference)
        {
            emit updateStatus(tr("Network Fault"));

            QString msg(tr("Invalid network interface"));
            emit errorOccurred(0, msg);
        }
    }
}

void MmsClient::networkDormant()
{
    if (networkReference) 
    {
        networkReference = false;

        QString interface(mmsInterfaceName());
        qLog(Messaging) << "stopInterface:" << interface;
        QtopiaNetwork::stopInterface(interface);
    }
}

void MmsClient::errorHandling(int code, QString msg)
{
    if (!comms || !comms->isActive())
        return;

    emit updateStatus(tr("Error occurred"));
    emit errorOccurred(code, msg);
    if (comms)
        comms->clearRequests(); //clear after error emit
}

QMailMessage MmsClient::convertToEmail(const MMSMessage &mms, int size)
{
    QMailMessage msg(messageFromHeaders(mms.headers()));

    // Default to mixed multipart - so we don't assume dependenccies between parts
    QMailMessagePartContainer::MultipartType multipartType = QMailMessagePartContainer::MultipartMixed;

    // Determine whether this message is composed of dependant parts
    const QWspField *f = mms.field("Content-Type");
    if (f) {
        // Add the original C-T, since we will need to change it to store the message
        msg.appendHeaderField("X-qtmail-internal-original-content-type", f->value);

        if (f->value.contains("application/vnd.wap.multipart.related"))
            multipartType = QMailMessagePartContainer::MultipartRelated;
    }

    f = mms.field("Message-ID");
    if (f)
        msg.setServerUid(f->value);

    // Now add the parts
    for (int i = 0; i < mms.messagePartCount(); i++) {
        const QWspPart &mmsPart = mms.messagePart(i);

        QByteArray data;
        QString type;

        const QWspField *f = mmsPart.header("Content-Type");
        if (f) {
            type = f->value.trimmed();

            int semicolon = type.indexOf( ';' );
            if( semicolon != -1 )
                type.truncate( semicolon );

            static const QStringList serviceAttributes = QStringList() << QLatin1String( "drm" ) 
                                                                      << QLatin1String( "handle" );

            // See if we need to extract this data
            QDSServices services( type, QLatin1String( "*" ), serviceAttributes );
            if( !services.isEmpty() )
            {
                QDSServiceInfo service = services.first();
                QDSAction action( service );

                if( action.exec( QDSData( mmsPart.data(), QMimeType(type) ) ) == QDSAction::CompleteData )
                {
                    QDSData responseData(action.responseData());
                    data = responseData.data();
                    size += data.size() - mmsPart.data().size();
                    type = responseData.type().id();
                }
                else
                {
                    // We can't handle this part...
                    continue;
                }
            }
            else
            {
                data = mmsPart.data();
            }
        }

        QMailMessageContentDisposition disposition(QMailMessageContentDisposition::Inline);
        QMailMessageContentType contentType(type.toLatin1());

        QMailMessagePart part = QMailMessagePart::fromData(data, disposition, contentType, 
                                                           QMailMessageBody::Base64, QMailMessageBody::RequiresEncoding);

        f = mmsPart.header("Content-ID");
        if (f)
            part.setContentID(f->value);

        f = mmsPart.header("Content-Location");
        if (f)
            part.setContentLocation(f->value);

        msg.appendPart(part);
    }

    // Meta-data
    msg.setMessageType(QMailMessage::Mms);
    msg.setId(internalId);
    msg.setMultipartType(multipartType);
    msg.setSize(size);
    msg.setStatus(QMailMessage::Downloaded, true);
    msg.setStatus(QMailMessage::Incoming, true);
    msg.setFromAccount(account->id());
    msg.setFromMailbox(QString());

    return msg;
}

MMSMessage MmsClient::convertToMms(const QMailMessage &mail)
{
    QString id = "00000" + QString::number(txnId);
    id = id.right(5);
    txnId++;

    MMSMessage mms;
    mms.setType(MMSMessage::MSendReq);
    mms.setTxnId(id);
    mms.setMessageId(mail.id());

    mms.addField("X-Mms-MMS-Version", "1.0");

    QString date(mail.date().isNull() ? QDateTime::currentDateTime().toString() : mail.date().toString());
    mms.addField("Date", date);

    mms.addField("From", QString());   // i.e. insert-an-address token
    if (!mail.subject().isEmpty())
        mms.addField("Subject", mail.subject());

    foreach (const QMailAddress& address, mail.to())
        mms.addField("To", encodeRecipient(address.toString()));
    foreach (const QMailAddress& address, mail.cc())
        mms.addField("Cc", encodeRecipient(address.toString()));
    foreach (const QMailAddress& address, mail.bcc())
        mms.addField("Bcc", encodeRecipient(address.toString()));

    addField(mms, mail, "X-Mms-Delivery-Report");
    addField(mms, mail, "X-Mms-Read-Reply");

    if (mail.multipartType() == QMailMessage::MultipartRelated)
        mms.addField("Content-Type", "application/vnd.wap.multipart.related; type=application/smil; start=<presentation-part>");
    else // We'll treat everything else as multipart/mixed
        mms.addField("Content-Type", "application/vnd.wap.multipart.mixed");

    if (mail.partCount() == 0 && mail.hasBody()) {
        // Add the message body as the only part
        QWspPart mmsPart;
        mmsPart.addHeader("Content-Type", mail.contentType().content());
        mmsPart.addHeader("Content-ID", QString("CID-0"));
        mmsPart.addHeader("Date", date);

        QByteArray body = mail.body().data(QMailMessageBody::Decoded);
        mmsPart.setData(body.data(), body.length());
        mms.addMessagePart(mmsPart);
    } else {
        // Add each part to the message
        for (uint i = 0; i < mail.partCount(); i++) {
            const QMailMessagePart &mailPart = mail.partAt(i);
            QMailMessageContentType type = mailPart.body().contentType();

            QWspPart mmsPart;
            mmsPart.addHeader("Content-Type", type.content());
            if (type.content() == "application/smil") {
                mmsPart.addHeader("Content-ID", "<presentation-part>");
            } else if (!mailPart.contentID().isEmpty()) {
                mmsPart.addHeader("Content-ID", mailPart.contentID());
            } else {
                mmsPart.addHeader("Content-ID", QString("CID-%1").arg(i));
            }
                mmsPart.addHeader("Date", date);
            if (!mailPart.contentLocation().isEmpty()) {
                mmsPart.addHeader("Content-Location", mailPart.contentLocation());
                mmsPart.addHeader("X-Wap-Content-URI", "http://orig.host/" + mailPart.contentLocation());
            }

            QByteArray decoded = mailPart.body().data(QMailMessageBody::Decoded);
            mmsPart.setData(decoded.data(), decoded.length());
            mms.addMessagePart(mmsPart);
        }
    }

    return mms;
}

void MmsClient::addField(MMSMessage &mms, const QMailMessage& mail, const QString &field)
{
    QString f = mail.headerFieldText(field);
    if (!f.isNull()) {
        int colon = f.indexOf(':');
        if (colon) {
            f = f.mid(colon+1);
            if (f.length() && f[0].isSpace())
                f = f.mid(1);
            if (!f.isEmpty())
                mms.addField(field, f);
        }
    }
}

QString MmsClient::encodeRecipient(const QString &address)
{
    QMailAddress addr(address);
    if (addr.isPhoneNumber())
        return addr.minimalPhoneNumber().append("/TYPE=PLMN");

    return address;
}

QString MmsClient::decodeRecipient(const QString &address)
{
    int pos = address.indexOf("/TYPE=");
    if (pos > 0)
        return address.left(pos);

    return address;
}

QString MmsClient::networkConfig() const
{
    Q_ASSERT(account);
    QString accountstr = account->networkConfig();
    return accountstr;
}

QString MmsClient::mmsInterfaceName() const
{
    QString config(networkConfig());
    if (config.isEmpty())
        return QString();

    QWapAccount acc(networkConfig());
    return acc.dataInterface();
}

bool MmsClient::raiseNetwork()
{
    if (!networkReference) {
        QString interface(mmsInterfaceName());
        if (!interface.isEmpty()) {
            // Register our interest in the network
            qLog(Messaging) << "startInterface:" << interface;
            QtopiaNetwork::startInterface(interface);

            networkReference = true;
            networkStatus = networkDevice->state();
        } else {
            // No network required?  We'll attempt comms anyway
            networkReference = false;
            networkStatus = QtopiaNetworkInterface::Up;
        }
    }

    return online(networkStatus);
}

void MmsClient::raiseFailure()
{
    emit updateStatus(tr("Network Fault"));
    
    QString msg(tr("Unable to start network interface"));
    emit errorOccurred(0, msg);
}

void MmsClient::checkForNewMessages()
{
    // We never have un-retrieved MMS messages; we either already have their ID 
    // from the notification, or we don't know about them at all
    emit allMessagesReceived();
}

int MmsClient::newMessageCount()
{
    return 0;
}

void MmsClient::pushMmsMessage(const QDSActionRequest& request)
{
    // Extract the MMS message from the push data
    QByteArray pushData(request.requestData().data());
    QBuffer buffer(&pushData);
    buffer.open(QIODevice::ReadOnly);

    MMSMessage mmsMsg;
    mmsMsg.decode(&buffer);
    if (mmsMsg.type() == MMSMessage::MNotificationInd)
        emit notificationInd(mmsMsg);
    else if (mmsMsg.type() == MMSMessage::MDeliveryInd)
        emit deliveryInd(mmsMsg);
}

void MmsClient::closeWhenDone()
{
    quitRecv = true;
}

void MmsClient::sendMessage(MMSMessage& msg)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    msg.encode(&buffer);
    buffer.close();

    messageLength = data.length();
    sentLength = 0;

    comms->sendMessage(msg, data);
}

