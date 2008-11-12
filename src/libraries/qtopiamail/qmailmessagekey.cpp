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

#include "qmailmessagekey.h"
#include "qmailmessagekey_p.h"

/*!
    \class QMailMessageKey
    \mainclass
    \preliminary
    \brief The QMailMessageKey class defines the parameters used for querying a subset of
    all available mail messages from the mail store.
    \ingroup messaginglibrary

    A QMailMessageKey is composed of a message property, an optional comparison operator
    and a comparison value. The QMailMessageKey class is used in conjunction with the QMailStore::queryMessages()
    and QMailStore::countMessages() functions to filter results which meet the criteria defined
    by the key.

    QMailMessageKey's can be combined using the logical operators (&), (|) and (~) to
    create more refined queries.

    For example:
    To create a query for all message's sent from "joe@user.com" with subject "meeting":
    \code
    QMailMessageKey subjectKey(QMailMessageKey::Subject,"meeting");
    QMailMessageKey senderKey(QMailMessageKey::Sender,"joe@user.com")
    QMailIdList results = QMailStore::instance()->queryMessages(subjectKey & senderKey);
    \endcode

    To query all unread messages from a specific folder \a parent:
    \code
    \\assuming parent has been retrieved from the mail store.
    QMailFolder parent;
    QMailMessageKey parentFolderKey(QMailMessageKey::ParentFolderId,parent.id())
    QMailMessageKey readKey(QMailMessageKey::Status,QMailMessage::Read,QMailMessageKey::Contains);
    QMailIdList results = QMailStore::instance()->queryMessages(parentFolderKey & ~readKey);
    \endcode

    \sa QMailStore, QMailFolderKey
*/

/*!
    \enum QMailMessageKey::Operand

    Defines the comparison operators that can be used to comapare QMailMessage::Property elements with
    user sepecified values.

    \value LessThan represents the '<' operator.
    \value LessThanEqual represents the '<=' operator.
    \value GreaterThan represents the '>' operator.
    \value GreaterThanEqual represents the '>= operator'.
    \value Equal represents the '=' operator.
    \value NotEqual represents the '!=' operator.
    \value Contains represents an operation in which an associated property is checked to see if it
    contains a provided value. For most property types this will perform a string based check. For
    Status type properties this will perform a check to see if a status bit value is set.
*/

/*!
    \enum QMailMessageKey::Property

    This enum type describes the data query properties of a QMailMessage.

    \value Id The ID of the message.
    \value Type The type of the message.
    \value ParentFolderId The parent folder ID this message is contained in.
    \value Sender The message sender address string.
    \value Recipients The message recipient address string.
    \value Subject The message subject string.
    \value TimeStamp The message timestamp
    \value Status The message status flags.
    \value FromAccount The name of the account the mesasge was downloaded from.
    \value FromMailbox The imap mailbox the message was downloaded from. 
    \value ServerUid The IMAP server UID of the message.
    \value Size The size of the message.
*/

/*!
    Create a QMailFolderKey with specifying matching parameters.

    A default-constructed key (one for which isEmpty() returns true) matches all messages. 
    The logical negation of an empty key also matches all messages.

    The result of combining an empty key with a non-empty key is the same as the original 
    non-empty key. This is true regardless of whether the combination is formed by a 
    logical AND or a logical OR operation.

    The result of combining two empty keys is an empty key.
*/

QMailMessageKey::QMailMessageKey()
{
    d = new QMailMessageKeyPrivate;
}

/*!
    Construct a QMailMessageKey which defines a query parameter where
    QMailMessage::Property \a p is compared using comparison operator
    \a c with a value \a value.
*/

QMailMessageKey::QMailMessageKey(const Property& p, const QVariant& value, const Operand& c)
{
    d = new QMailMessageKeyPrivate;
    QMailMessageKeyPrivate::Argument m;
    m.property = p;
    m.op = c;
    m.valueList.append(value);
    d->arguments.append(m);

}

/*!
    Construct a QMailMessageKey which defines a query parameter where
    message id's matching those in \a idList are returned.
*/

QMailMessageKey::QMailMessageKey(const QMailIdList& idList)
{
    d = new QMailMessageKeyPrivate;
    QMailMessageKeyPrivate::Argument m;
    m.property = QMailMessageKey::Id;
    m.op = Equal;
    foreach(QMailId id,idList)
        m.valueList.append(id);
    d->arguments.append(m);
}

/*!
    Create a copy of the QMailMessageKey \a other.
*/

QMailMessageKey::QMailMessageKey(const QMailMessageKey& other)
{
    d = other.d;
}


/*!
    Destroys the QMailMessageKey
*/

QMailMessageKey::~QMailMessageKey()
{
}

/*!
    Returns true if the key remains empty after default construction; otherwise returns false.
*/

bool QMailMessageKey::isEmpty() const
{
    return d->logicalOp == QMailMessageKeyPrivate::None &&
        d->negated == false &&
        d->subKeys.isEmpty() && 
        d->arguments.isEmpty();
}

/*!
    Returns a key that is the logical NOT of the value of this key.
*/

QMailMessageKey QMailMessageKey::operator~() const
{
    QMailMessageKey k(*this);
    if(!isEmpty())
        k.d->negated = !d->negated;
    return k;
}

/*!
    Returns a key that is the logical AND of this key and the value of key \a other.
*/

QMailMessageKey QMailMessageKey::operator&(const QMailMessageKey& other) const
{
    if (isEmpty())
        return other;
    else if (other.isEmpty())
        return *this;

    QMailMessageKey k;
    k.d->logicalOp = QMailMessageKeyPrivate::And;

    if(d->logicalOp != QMailMessageKeyPrivate::Or && !d->negated && other.d->logicalOp != QMailMessageKeyPrivate::Or && !other.d->negated)
    {
        k.d->subKeys = d->subKeys + other.d->subKeys;
        k.d->arguments = d->arguments + other.d->arguments;
    }
    else
    {
        k.d->subKeys.append(*this);
        k.d->subKeys.append(other); 
    }
    return k;            
}

/*!
    Returns a key that is the logical OR of this key and the value of key \a other.
*/

QMailMessageKey QMailMessageKey::operator|(const QMailMessageKey& other) const
{
    if (isEmpty())
        return other;
    else if (other.isEmpty())
        return *this;

    QMailMessageKey k;
    k.d->logicalOp = QMailMessageKeyPrivate::Or;
    if(d->logicalOp != QMailMessageKeyPrivate::And && !d->negated && other.d->logicalOp != QMailMessageKeyPrivate::And && !other.d->negated)
    {
        k.d->subKeys = d->subKeys + other.d->subKeys;
        k.d->arguments = d->arguments + other.d->arguments;
    }
    else
    {
        k.d->subKeys.append(*this);    
        k.d->subKeys.append(other);
    }

    return k;
}

/*!
    Performs a logical AND with this key and the key \a other and assigns the result
    to this key.
*/

QMailMessageKey& QMailMessageKey::operator&=(const QMailMessageKey& other)
{
    *this = *this & other;
    return *this;
}

/*!
    Performs a logical OR with this key and the key \a other and assigns the result
    to this key.
*/

QMailMessageKey& QMailMessageKey::operator|=(const QMailMessageKey& other) 
{
    *this = *this | other;
    return *this;
}

/*!
    Returns \c true if the value of this key is the same as the key \a other. Returns 
    \c false otherwise.
*/

bool QMailMessageKey::operator==(const QMailMessageKey& other) const
{
    return d->negated == other.d->negated &&
           d->logicalOp == other.d->logicalOp &&
           d->subKeys == other.d->subKeys && 
           d->arguments == other.d->arguments;
}

/*!
    Returns \c true if the value of this key is not the same as the key \a other. Returns
    \c false otherwise.
*/

bool QMailMessageKey::operator!=(const QMailMessageKey& other) const
{
   return !(*this == other); 
}

/*!
    Assign the value of the QMailMessageKey \a other to this.
*/

QMailMessageKey& QMailMessageKey::operator=(const QMailMessageKey& other)
{
    d = other.d;
    return *this;
}

