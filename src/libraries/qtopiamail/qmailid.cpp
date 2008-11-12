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

#include "qmailid.h"

class QMailIdPrivate : public QSharedData
{
public:
    QMailIdPrivate():QSharedData(){};

    quint64 id;
};

/*!
    \class QMailId
    \mainclass
    \preliminary
    \brief The QMailId class defines the 64bit database ID's for message store elements.
    \ingroup messaginglibrary

    The QMailID class represents the message store identifiers for QMailMessages and QMailFolders.
    A QMailId can be either valid or invalid. A message or folder with a valid ID is expected
    to exist on the store and have a quint64 value greater than 0.

    \sa QMailMessage
*/

Q_IMPLEMENT_USER_METATYPE(QMailId);

/*!
   Constructs a new invalid QMailId.
*/

QMailId::QMailId()
{
    d = new QMailIdPrivate();
    d->id = 0;
}

/*!
    Explicitly constructs a new QMailId from a quint64 \a id.
*/

QMailId::QMailId(const quint64& id)
{
    d = new QMailIdPrivate();
    d->id = id;
}

/*!
    Constructs a QMailId that is a copy of the QMailId \a other.
*/

QMailId::QMailId(const QMailId& other)
{
    d = other.d;
}

/*!
    Destroys the QMailId.
*/

QMailId::~QMailId()
{
}

/*!
    Returns \c true if this QMailId is valid or \c false otherwise. A valid
    QMailId is one which is expected to exist on the message store.
*/

bool QMailId::isValid() const
{
    return d->id > 0;
}

/*!
  Returns the quint64 representation of the QMailId.
*/

quint64 QMailId::toULongLong() const
{
    return d->id;
}

/*!
  Returns the QVariant representation of this QMailId.
*/

QMailId::operator QVariant() const
{
    return QVariant::fromValue(*this);
}

/*!
    Returns \c true if this QMailId does not equal the QMailId \a other,
    and \c false otherwise.
*/

bool QMailId::operator!= ( const QMailId & other ) const
{
    return d->id != other.d->id;
}

/*!
    Returns \c true if this QMailId equals the QMailId \a other, and \c false
    otherwise.
*/

bool QMailId::operator== ( const QMailId& other ) const
{
    return d->id == other.d->id;
}

/*!
    Returns \c true if this QMailId is less than the value of \a other, and \c false
    otherwise.
*/
bool QMailId::operator< (const QMailId& other) const
{
    return d->id < other.d->id;
}

/*!
    Assignes the value of this QMailId to the value of the QMailId \a other.
*/

QMailId& QMailId::operator=(const QMailId& other)
{
    d = other.d;
    return *this;
}

/*!
    \fn QMailId::serialize(Stream &stream) const

    Writes the contents of a QMailId to a \a stream.
*/
template <typename Stream> void QMailId::serialize(Stream &stream) const
{
    stream << d->id;
}

/*!
    \fn QMailId::deserialize(Stream &stream)

    Reads the contents of a QMailId from \a stream.
*/
template <typename Stream> void QMailId::deserialize(Stream &stream)
{
    stream >> d->id;
}


