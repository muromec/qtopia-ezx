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

#include "qmailfolderkey.h"
#include "qmailfolderkey_p.h"

/*!
    \class QMailFolderKey
    \mainclass
    \preliminary
    \brief The QMailFolderKey class defines the parameters used for querying a subset of
    all available mail folders from the mail store.
    \ingroup messaginglibrary

    A QMailFolderKey is composed of a folder property, an optional comparison operator
    and a comparison value. The QMailFolderKey class is used in conjunction with the QMailStore::queryFolders()
    and QMailStore::countFolders() functions to filter results which meet the criteria defined
    by the key.

    QMailFolderKey's can be combined using the logical operators (&), (|) and (~) to
    build more sophisticated queries.

    For example:
    To create a query for all folders named "inbox" or "sms"
    \code
    QMailFolderKey inboxKey(QMailFolderKey::Name,"inbox");
    QMailFolderKey smsKey(QMailFolderKey::Name,"sms");
    QMailIdList results = QMailStore::instance()->queryFolders(inboxKey | smsKey);
    \endcode

    To query all subfolders with name "foo" for a given folder \a parent:
    \code
    \\assuming parent has been retrieved from the mail store.
    QMailFolder parent;
    QMailFolderKey nameKey(QMailFolderKey::Name,"foo");
    QMailFolderKey childKey(QMailFolderKey::ParentId,parent.id());
    QMailIdList results = QMailStore::instance()->queryFolders(nameKey & childKey);
    \endcode

    \sa QMailStore, QMailMessageKey
*/

/*!
    \enum QMailFolderKey::Operand

    Defines the comparison operators that can be used to comapare QMailFolder::Property elements with
    user sepecified values.

    \value LessThan represents the '<' operator.
    \value LessThanEqual represents the '<=' operator.
    \value GreaterThan represents the '>' operator.
    \value GreaterThanEqual represents the '>= operator'.
    \value Equal represents the '=' operator.
    \value NotEqual represents the '!=' operator.
    \value Contains represents an operation in which an associated property is checked to see if it
    contains a provided value. For most property types this will perform a string based check. For
    Status type properties this will perform a check to see if a status flag bit value is set.
*/

/*!
    \enum QMailFolderKey::Property

    This enum type describes the queryable data properties of a QMailFolder.

    \value Id The ID of the folder.
    \value Name The name of the folder.
    \value ParentId the ID of the parent folder for a given folder.
*/

/*!
    Create a QMailFolderKey with specifying matching parameters.

    A default-constructed key (one for which isEmpty() returns true) matches all folders.
    The logical negation of an empty key also matches all folders.

    The result of combining an empty key with a non-empty key is the same as the original
    non-empty key. This is true regardless of whether the combination is formed by a
    logical AND or a logical OR operation.

    The result of combining two empty keys is an empty key.
*/

QMailFolderKey::QMailFolderKey()
{
    d = new QMailFolderKeyPrivate();
}


/*!
    Construct a QMailFolderKey which defines a query parameter where
    QMailFolder::Property \a p is compared using comparison operator
    \a c with a value \a value.
*/

QMailFolderKey::QMailFolderKey(const Property& p, const QVariant& value, const Operand& c)
{
    d = new QMailFolderKeyPrivate();
    QMailFolderKeyPrivate::Argument m;
    m.property = p;
    m.op = c;
    m.valueList.append(value);
    d->arguments.append(m);
}

/*!
    Construct a QMailFolderKey which defines a query parameter where
    folder id's matching those in \a ids are returned.
*/

QMailFolderKey::QMailFolderKey(const QMailIdList& ids)
{
    d = new QMailFolderKeyPrivate();
    QMailFolderKeyPrivate::Argument m;
    m.property = QMailFolderKey::Id;
    m.op = Equal;
    foreach(QMailId id,ids)
        m.valueList.append(id);
    d->arguments.append(m);
}

/*!
    Create a copy of the QMailFolderKey \a other.
*/

QMailFolderKey::QMailFolderKey(const QMailFolderKey& other)
{
    d = other.d;
}


/*!
    Destroys this QMailFolderKey.
*/


QMailFolderKey::~QMailFolderKey()
{
}

/*!
    Returns a key that is the logical NOT of the value of this key.
*/

QMailFolderKey QMailFolderKey::operator~() const
{
    QMailFolderKey k(*this);
    if(!k.isEmpty())
        k.d->negated = !d->negated;
    return k;
}

/*!
    Returns a key that is the logical AND of this key and the value of key \a other.
*/

QMailFolderKey QMailFolderKey::operator&(const QMailFolderKey& other) const
{
    if(isEmpty())
        return other;
    else if(other.isEmpty())
        return *this;

    QMailFolderKey k;
    k.d->logicalOp = QMailFolderKeyPrivate::And;

    if(d->logicalOp != QMailFolderKeyPrivate::Or && !d->negated && other.d->logicalOp != QMailFolderKeyPrivate::Or && !other.d->negated)
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

QMailFolderKey QMailFolderKey::operator|(const QMailFolderKey& other) const
{
    if(isEmpty())
        return other;
    else if(other.isEmpty())
        return *this;

    QMailFolderKey k;
    k.d->logicalOp = QMailFolderKeyPrivate::Or;

    if(d->logicalOp != QMailFolderKeyPrivate::And && 
       !d->negated && 
       other.d->logicalOp != QMailFolderKeyPrivate::And && 
       !other.d->negated)
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

QMailFolderKey& QMailFolderKey::operator&=(const QMailFolderKey& other)
{
    *this = *this & other;
    return *this;
}

/*!
    Performs a logical OR with this key and the key \a other and assigns the result
    to this key.
*/

QMailFolderKey& QMailFolderKey::operator|=(const QMailFolderKey& other) 
{
    *this = *this | other;
    return *this;
}

/*!
    Returns \c true if the value of this key is the same as the key \a other. Returns 
    \c false otherwise.
*/

bool QMailFolderKey::operator==(const QMailFolderKey& other) const
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

bool QMailFolderKey::operator!=(const QMailFolderKey& other) const
{
   return !(*this == other); 
}

/*!
    Assign the value of the QMailFolderKey \a other to this.
*/

QMailFolderKey& QMailFolderKey::operator=(const QMailFolderKey& other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if the key remains empty after default construction; otherwise returns false. 
*/

bool QMailFolderKey::isEmpty() const
{
    return d->logicalOp == QMailFolderKeyPrivate::None &&
        d->negated == false &&
        d->subKeys.isEmpty() && 
        d->arguments.isEmpty();
}


