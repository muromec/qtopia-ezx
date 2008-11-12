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

#include "qmailfolder.h"
#include "qmailstore.h"

class QMailFolderPrivate : public QSharedData
{
public:
    QMailFolderPrivate() : QSharedData() {};

    QMailId id;
    QString name;
    QMailId parentFolderId;
};

/*!
    \class QMailFolder
    \mainclass
    \preliminary
    \brief The QMailFolder class represents a folder for mail messages on the message store.
    \ingroup messaginglibrary

    A QMailFolder acts as a named aggregate for \c QMailMessage's, other \c QMailFolder's or both, thus forming
    a tree-like structure in the message store.

    \sa QMailMessage
*/

QMailFolder::QMailFolder()
{
    d = new QMailFolderPrivate();
}

/*!
  Convienience constructor that creates a root \c QMailFolder by loading the data from the message store as
  specified by the QMailId \a id. If the folder does not exist in the message store, then this object
  will create an empty and invalid QMailFolder.
*/

QMailFolder::QMailFolder(const QMailId& id)
{
    *this = QMailStore::instance()->folder(id);
}


/*!
  Creates a root \c QMailFolder object with name \a name and \c parent folder ID \a parentFolderId.
  If the parent folder ID is invalid, this folder will be a root folder when added into the message store.
*/

QMailFolder::QMailFolder(const QString& name, const QMailId& parentFolderId)
{
    d = new QMailFolderPrivate();
    d->name = name;
    d->parentFolderId = parentFolderId;
}


/*!
  Creates a copy of the \c QMailFolder object \a other.
 */

QMailFolder::QMailFolder(const QMailFolder& other)
{
    d = other.d;
}


/*!
  Destroys the \c QMailFolder object.
*/

QMailFolder::~QMailFolder()
{
}

/*!
  Returns the \c ID of the \c QMailFolder object. A \c QMailFolder with an invalid ID
  is one which does not yet exist on the message store.
*/

QMailId QMailFolder::id() const
{
    return d->id;
}

/*!
  Returns \c true if the folder is a root folder or \c false otherwise. A root folder
  is one which has not parent.
*/

bool QMailFolder::isRoot() const
{
    return (!d->parentFolderId.isValid());
}

/*!
  Returns the name of the folder.
*/
QString QMailFolder::name() const
{
    return d->name;
}

/*!
  Sets the name of this folder to \a name.
*/

void QMailFolder::setName(const QString& name)
{
    d->name = name;
}

/*!
  Returns the ID of the parent folder. This folder is a root folder if the parent
  ID is invalid.
*/

QMailId QMailFolder::parentId() const
{
    return d->parentFolderId;
}

/*!
 Sets the parent folder id to \a id. \bold{Warning}: it is the responsibility of the
 application to make sure that no circular folder refernces are created.
*/

void QMailFolder::setParentId(const QMailId& id)
{
    d->parentFolderId = id;
}

/*!
  Assigns the value of the \c QMailFolder object \a other to this.
 */

QMailFolder& QMailFolder::operator=(const QMailFolder& other)
{
    d = other.d;
    return  *this;
}

/*!
  Sets the ID of this folder to \a id
*/

void QMailFolder::setId(const QMailId& id)
{
    d->id = id;
}

