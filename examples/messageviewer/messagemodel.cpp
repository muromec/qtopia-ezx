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

#include "messagemodel.h"
#include "messagedelegate.h"
#include <QIcon>
#include <QMailId>
#include <QMailMessage>
#include <QMailMessageKey>
#include <QMailMessageSortKey>
#include <QMailStore>
#include <QPhoneNumber>
#include <QStandardItem>
#include <QTimeString>
#include <QtopiaApplication>


class MessageItem : public QStandardItem
{
public:
    explicit MessageItem(const QMailId& id);
    virtual ~MessageItem();

    QMailId messageId() const;

private:
    QMailId id;
};

MessageItem::MessageItem(const QMailId& id)
    : QStandardItem(), id(id)
{
    static QIcon sentMessageIcon(":icon/qtmail/sendmail");
    static QIcon receivedMessageIcon(":icon/qtmail/getmail");
    static QIcon smsIcon(":icon/txt");
    static QIcon mmsIcon(":icon/multimedia");
    static QIcon emailIcon(":icon/email");

    // Load the header for this message
    QMailMessage message(id, QMailMessage::Header);

    // Determine the properties we want to display
    QIcon* messageIcon = &smsIcon;
    if (message.messageType() == QMailMessage::Mms)
        messageIcon = &mmsIcon;
    if (message.messageType() == QMailMessage::Email)
        messageIcon = &emailIcon;

    bool sent(message.status() & QMailMessage::Outgoing);

    QDateTime messageTime(message.date().toLocalTime());

    QString action(qApp->translate("MessageViewer", sent ? "Sent" : "Received"));
    QString date(QTimeString::localMD(messageTime.date()));
    QString time(QTimeString::localHM(messageTime.time(), QTimeString::Short));
    QString sublabel(QString("%1 %2 %3").arg(action).arg(date).arg(time));

    // Configure this item
    setIcon(sent ? sentMessageIcon : receivedMessageIcon);
    setText(message.subject());
    setData(sublabel, MessageDelegate::SubLabelRole);
    setData(*messageIcon, MessageDelegate::SecondaryDecorationRole);
}

MessageItem::~MessageItem()
{
}

QMailId MessageItem::messageId() const
{
    return id;
}


MessageModel::MessageModel(QObject *parent)
    : QStandardItemModel(parent)
{
}

MessageModel::~MessageModel()
{
}

void MessageModel::setContact(const QContact& contact)
{
    clear();

    // Locate messages whose sender is this contact
    QMailMessageKey msgsFrom;

    // Locate messages whose recipients list contains this contact
    QMailMessageKey msgsTo;

    // Match on any of contact's phone numbers
    foreach(const QString& number, contact.phoneNumbers().values()) {
        msgsFrom |= QMailMessageKey(QMailMessageKey::Sender, number);
        msgsTo |= QMailMessageKey(QMailMessageKey::Recipients, number, QMailMessageKey::Contains);
    }

    // Match on any of contact's email addresses
    foreach(const QString& address, contact.emailList()) {
        msgsFrom |= QMailMessageKey(QMailMessageKey::Sender, address);
        msgsTo |= QMailMessageKey(QMailMessageKey::Recipients, address, QMailMessageKey::Contains);
    }

    // Sort messages by timestamp, newest to oldest
    QMailMessageSortKey sort(QMailMessageSortKey::TimeStamp, Qt::DescendingOrder);

    // Fetch the messages matching either of our queries, and return them sorted
    QMailIdList matches(QMailStore::instance()->queryMessages(msgsFrom | msgsTo, sort));

    // Add each returned message to our data model
    foreach (const QMailId& id, matches)
        appendRow(new MessageItem(id));
}

QMailId MessageModel::messageId(const QModelIndex& index)
{
    if (index.isValid())
        if (MessageItem* item = static_cast<MessageItem*>(itemFromIndex(index)))
            return item->messageId();

    return QMailId();
}

