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

// Needed to give friend access to the function defined by Q_GLOBAL_STATIC
#define QMAILSTOREINSTANCE_DEFINED_HERE
#include "qmailstore.h"

#include "qmailstore_p.h"
#include "qmailfolder.h"
#include "qmailmessage.h"
#include "qmailmessagekey.h"
#include "qmailmessagesortkey.h"
#include "qmailfolderkey.h"
#include "qmailfoldersortkey.h"
#include "qmailid.h"
#include "qmailtimestamp.h"
#include <qtopiasql.h>
#include <QDebug>
#include <QContent>
#include <qtopialog.h>
#include <QSqlQuery>
#include <QSqlDriver>
#include <QSqlRecord>
#include <algorithm>

/*!
    \class QMailStore
    \mainclass
    \preliminary
    \brief The QMailStore class represents the main interface for storage and retrieval
    of messages and folders on the message store.

    \ingroup messaginglibrary

    The QMailStore class is accessed through a singleton interface and provides functions
    for adding, updating and deleting of QMailFolders and QMailMessages on the message store.

    QMailStore also provides functions for querying and counting of QMailFolders and QMailMessages
    when used in conjunction with QMailMessageKey and QMailFolderKey classes.

    \sa QMailMessage, QMailFolder, QMailMessageKey, QMailFolderKey
*/

/*!
    Constructs a new QMailStore object and opens the message store database.
*/


QMailStore::QMailStore()
{
    //create the db and sql interfaces
    d = new QMailStorePrivate();
    d->database = QtopiaSql::instance()->applicationSpecificDatabase("qtmail");

    connect(d,SIGNAL(messagesAdded(const QMailIdList&)),
            this,
            SIGNAL(messagesAdded(const QMailIdList&)));

    connect(d,SIGNAL(messagesRemoved(const QMailIdList&)),
            this,
            SIGNAL(messagesRemoved(const QMailIdList&)));

    connect(d,SIGNAL(messagesUpdated(const QMailIdList&)),
            this,
            SIGNAL(messagesUpdated(const QMailIdList&)));

    connect(d,SIGNAL(foldersAdded(const QMailIdList&)),
            this,
            SIGNAL(foldersAdded(const QMailIdList&)));

    connect(d,SIGNAL(foldersUpdated(const QMailIdList&)),
            this,
            SIGNAL(foldersUpdated(const QMailIdList&)));

    connect(d,SIGNAL(foldersRemoved(const QMailIdList&)),
            this,
            SIGNAL(foldersRemoved(const QMailIdList&)));


}

/*!
    Destroys this QMailStore object.
*/

QMailStore::~QMailStore()
{
    delete d; d = 0;
}

/*!
    Adds a new QMailFolder object \a a into the message store, performing
    respective integrity checks. Returns \c true if the operation
    completed successfully, \c false otherwise.
*/

bool QMailStore::addFolder(QMailFolder* a)
{
    d->database.transaction();

    //check that the parent folder actually exists

    if(a->parentId().isValid() && !d->folderExists(a->parentId().toULongLong()))
    {
        d->database.rollback();
        qLog(Messaging) << "Parent folder does not exist";
        return false;
    }

    //insert the qmailfolder

    QSqlQuery query = d->simpleQuery("INSERT INTO mailfolders (name,parentid) VALUES (?,?)",
                                     "MailFolder insertion query",
                                     QVariantList() << a->name()
                                                    << a->parentId().toULongLong());

    if(query.lastError().type() != QSqlError::NoError || !d->database.commit())
    {
        d->database.rollback();
        return false;
    }

    //set the insert id

    quint64 newId = query.lastInsertId().toULongLong();
    QMailId insertId(newId);
    a->setId(insertId);

    //synchronize

    QMailIdList ids;
    ids.append(insertId);
    d->notifyFoldersChange(QMailStorePrivate::Added,ids);
    emit foldersAdded(ids);

    return true;
}

/*!
    Adds a new QMailMessage object \a m into the message store, performing
    respective integrity checks. Returns \c true if the operation
    completed successfully, \c false otherwise.
*/

bool QMailStore::addMessage(QMailMessage* m)
{
    if(!m->parentFolderId().isValid())
    {
        qLog(Messaging) << "Unable to add folder. Invalid parent folder id";
        return false;
    }

    QString mailfile;
    if(!d->mailBodyStore.insert(*m,&mailfile))
    {
        qLog(Messaging) << "Could not store mail body";
        return false;
    }

    // Ensure that any phone numbers are added in minimal form
    QMailAddress from(m->from());
    QString fromText(from.isPhoneNumber() ? from.minimalPhoneNumber() : from.toString());

    QStringList recipients;
    foreach (const QMailAddress& address, m->to())
        recipients.append(address.isPhoneNumber() ? address.minimalPhoneNumber() : address.toString());

    QString sql = "INSERT INTO mailmessages (type, parentfolderid, sender, \
                   recipients, subject, stamp, status, fromaccount, \
                   frommailbox, mailfile, serveruid, size) \
                   VALUES (?,?,?,?,?,?,?,?,?,?,?,?)";

    d->database.transaction();

    QSqlQuery query = d->simpleQuery(sql,
                                     "MailMessage insert query",
                                     QVariantList() << static_cast<int>(m->messageType())
                                                    << m->parentFolderId().toULongLong()
                                                    << fromText
                                                    << recipients.join(",")
                                                    << m->subject()
                                                    << QMailTimeStamp(m->date()).toLocalTime()
                                                    << static_cast<int>(m->status())
                                                    << m->fromAccount()
                                                    << m->fromMailbox()
                                                    << mailfile
                                                    << m->serverUid()
                                                    << m->size());

    if(query.lastError().type() != QSqlError::NoError || !d->database.commit())
    {
        d->database.rollback();
        if(!d->mailBodyStore.remove(mailfile))
            qLog(Messaging) << "Could not remove temp mail body" << mailfile;
        return false;
    }

    //set the insert id

    quint64 newId = query.lastInsertId().toULongLong();
    QMailId insertId(newId);
    m->setId(insertId);

    //synchronize

    QMailIdList ids;
    ids.append(m->id());
    d->notifyMessagesChange(QMailStorePrivate::Added,ids);
    emit messagesAdded(ids);

    return true;
}

/*!
    Removes a QMailFolder with QMailId \a id from the message store. This action also
    removes the sub-folder and messages of the folder. Returns \c true if the operation
    completed successfully, \c false otherwise.
*/

bool QMailStore::removeFolder(const QMailId& id)
{
    QMailIdList deletedFolders;
    QMailIdList deletedMessages;

    d->database.transaction();
    if(!d->deleteFolder(id,deletedFolders,deletedMessages) || !d->database.commit())
    {
        d->database.rollback();
        qLog(Messaging) << "Folder deletion failed";
        return false;
    }

    //append target folder id for sync

    deletedFolders.append(id);

    //delete mails from cache

    foreach(QMailId id, deletedMessages)
    {
        if(d->headerCache.contains(id))
            d->headerCache.remove(id);
    }

    //synchronize

    d->notifyMessagesChange(QMailStorePrivate::Removed,deletedMessages);
    d->notifyFoldersChange(QMailStorePrivate::Removed,deletedFolders);
    emit messagesRemoved(deletedMessages);
    emit foldersRemoved(deletedFolders);

    return true;
}

/*!
    Removes a QMailMessage with QMailId \a id from the message store. Returns \c true if the operation
    completed successfully, \c false otherwise.
*/

bool QMailStore::removeMessage(const QMailId& id)
{
    //get the mailfile

    d->database.transaction();

    QSqlQuery query = d->simpleQuery("SELECT mailfile FROM mailmessages WHERE id=?",
                                     "Mailfile for MailMessage removal query",
                                     QVariantList() << id.toULongLong());

    if(query.lastError().type() != QSqlError::NoError)
    {
        d->database.rollback();
        return false;
    }

    query.first();

    QString mailfile = query.value(0).toString();

    query = d->simpleQuery("DELETE FROM mailmessages WHERE id = ?",
                           "MailMessage removal query",
                           QVariantList() << id.toULongLong());

    if(query.lastError().type() != QSqlError::NoError || !d->database.commit())
    {
        d->database.rollback();
        return false;
    }

    //delete the mail body

    if(!d->mailBodyStore.remove(mailfile))
        qLog(Messaging) << "Could not remove the mail body " << mailfile;

    if(d->headerCache.contains(id))
        d->headerCache.remove(id);

    //synchronize

    QMailIdList ids;
    ids.append(id);
    d->notifyMessagesChange(QMailStorePrivate::Removed, ids);
    emit messagesRemoved(ids);

    return true;

}
/*!
    Updates the existing QMailFolder \a f on the message store.
    Returns \c true if the operation completed successfully,
    \c false otherwise.
*/

bool QMailStore::updateFolder(QMailFolder* f)
{
    //check for a valid folder

    if(!f->id().isValid())
    {
        qLog(Messaging) << "Invalid folder to update";
        return false;
    }

    //check for self reference

    if(f->parentId().isValid() && f->parentId() == f->id())
    {
        qLog(Messaging) << "A folder cannot be a child to itself";
        return false;
    }

    d->database.transaction();

    //check that the parentfolder exists

    if(f->parentId().isValid() && !d->folderExists(f->parentId().toULongLong()))
    {
        d->database.rollback();
        qLog(Messaging) << "Parent folder does not exist";
        return false;
    }

    QSqlQuery query = d->simpleQuery("UPDATE mailfolders SET name=?, parentid=? WHERE id =?",
                                     "MailFolder update query",
                                     QVariantList() << f->name()
                                                    << f->parentId().toULongLong()
                                                    << f->id().toULongLong());

    if(query.lastError().type() != QSqlError::NoError || !d->database.commit())
    {
        d->database.rollback();
        return false;
    }

    //synchronize

    QMailIdList ids;
    ids.append(f->id());
    d->notifyFoldersChange(QMailStorePrivate::Updated,ids);
    emit foldersUpdated(ids);

    return true;
}

/*!
    Updates the existing QMailMessage \a m on the message store.
    Returns \c true if the operation completed successfully, or \c false otherwise.
*/

bool QMailStore::updateMessage(QMailMessage* m)
{
    if(!m->id().isValid())
        return false;

    if(!m->uncommittedChanges() && !m->uncommittedMetadataChanges())
        return true;

    QMailMessageKey idKey(QMailMessageKey::Id,m->id());

    QString sql = "UPDATE mailmessages SET " + d->expandProperties(updatableProperties,true) + " WHERE id=?";

    QSqlQuery query = d->simpleQuery(sql, "MailMessage update query",
                                     QVariantList() << d->updateValues(updatableProperties,*m)
                                                    << m->id().toULongLong());

    if(query.lastError().type() != QSqlError::NoError)
    {
        d->database.rollback();
        return false;
    }

    //update the mail body
    //get the mailfile reference

    if(m->uncommittedChanges())
    {
        query = d->simpleQuery("SELECT mailfile FROM mailmessages WHERE id=?",
                               "MailMessage update mailfile selection query",
                               QVariantList() << m->id().toULongLong());

        if(query.lastError().type() != QSqlError::NoError)
            return false;

        query.first();

        QString mailfile = query.value(0).toString();

        if(!d->mailBodyStore.update(mailfile,*m))
        {
            qLog(Messaging) << "Could not update mail body " << mailfile;
            return false;
        }
    }

    // The message is now up-to-date with data store
    m->changesCommitted();

    //update the header cache

    if(d->headerCache.contains(m->id()))
    {
        QMailMessage cachedHeader = d->headerCache.lookup(m->id());
        d->bindUpdateData(updatableProperties,*m,cachedHeader);
        cachedHeader.changesCommitted();
        d->headerCache.insert(cachedHeader);
    }

    //synchronize

    QMailIdList ids;
    ids.append(m->id());
    d->notifyMessagesChange(QMailStorePrivate::Updated,ids);
    emit messagesUpdated(ids);

    return true;
}

/*!
    Updates the message properties defined in \a properties with data
    contained in the message \a data for all messages which pass the criteria defined
    by the QMailMessageKey \a key.
    Returns \c true if the operation completed successfully, or \c false otherwise.
*/

bool QMailStore::updateMessages(const QMailMessageKey& key,
                                const QMailMessageKey::Properties& properties,
                                const QMailMessage& data)
{
    //do some checks first

    if(properties == QMailMessageKey::Id)
    {
        qLog(Messaging) << "Updating of messages id's is not supported";
        return false;
    }

    d->checkComparitors(key);

    d->database.transaction();

    if(properties & QMailMessageKey::ParentFolderId)
        if(!d->folderExists(data.parentFolderId().toULongLong()))
        {
            d->database.rollback();
            qLog(Messaging) << "Update of messages failed. Parent folder does not exist";
            return false;
        }

    //get the valid ids

    QMailIdList validIds = queryMessages(key);

    if(validIds.isEmpty())
    {
        d->database.rollback();
        return true;
    }

    QMailMessageKey validIdsKey(validIds);

    QString sql = "UPDATE mailmessages SET " + d->expandProperties(properties,true);
    sql += d->buildWhereClause(validIdsKey);

    QSqlQuery query = d->simpleQuery(sql,"MailMessage batch update query",
                                     QVariantList() << d->updateValues(properties,data)
                                                    << d->whereClauseValues(validIdsKey));

    if(query.lastError().type() != QSqlError::NoError || !d->database.commit())
    {
        d->database.rollback();
        return false;
    }

    //update the cache

    foreach(QMailId id,validIds)
    {
        if(d->headerCache.contains(id))
        {
            QMailMessage cachedHeader = d->headerCache.lookup(id);
            d->bindUpdateData(properties,data,cachedHeader);
            cachedHeader.changesCommitted();
            d->headerCache.insert(cachedHeader);
        }
    }

    //synchronize

    d->notifyMessagesChange(QMailStorePrivate::Updated,validIds);
    emit messagesUpdated(validIds);

    return true;
}

/*!
    Updates message status flags with \a status according to \a set
    for messages which pass the criteria defined in the QMailMessageKey \a key.
    Returns \c true if the operation completed successfully, or \c false otherwise.
*/

bool QMailStore::updateMessages(const QMailMessageKey& key,
                                const QMailMessage::Status status,
                                bool set)
{
    d->checkComparitors(key);

    d->database.transaction();

    //get the valid ids

    QMailIdList validIds = queryMessages(key);

    if(validIds.isEmpty())
        return true;

    QMailMessageKey validIdsKey(validIds);

    QString sql = "UPDATE mailmessages SET status=status";
    sql += set ? "|?" : "&?";
    sql += d->buildWhereClause(validIdsKey);

    int statusInt = static_cast<int>(status);
    if(!set) statusInt = ~statusInt;

    QSqlQuery query = d->simpleQuery(sql,"MailMessage batch update status query",
                                     QVariantList() << statusInt << d->whereClauseValues(validIdsKey));

    if(query.lastError().type() != QSqlError::NoError || !d->database.commit())
    {
        d->database.rollback();
        return false;
    }

    //update the cache
    foreach(QMailId id,validIds)
    {
        if(d->headerCache.contains(id))
        {
            QMailMessage cachedHeader = d->headerCache.lookup(id);
            QMailMessage::Status newStatus = cachedHeader.status();
            newStatus = set ? (newStatus | status) : (newStatus & ~status);
            cachedHeader.setStatus(newStatus);
            cachedHeader.changesCommitted();
            d->headerCache.insert(cachedHeader);
        }
    }

    //synchronize

    d->notifyMessagesChange(QMailStorePrivate::Updated,validIds);
    emit messagesUpdated(validIds);

    return true;
}

/*!
    Returns the count of the number of folders which pass the
    filtering criteria defined in QMailFolderKey \a key. If
    key is empty a count of all folders is returned.
*/

int QMailStore::countFolders(const QMailFolderKey& key) const
{
    QString sql = "SELECT COUNT(*) FROM mailfolders";
    if(!key.isEmpty())
        sql += d->buildWhereClause(key);

    QSqlQuery query = d->simpleQuery(sql, "MailFolder count query",
                                     QVariantList() << d->whereClauseValues(key));

    if(query.lastError().type() == QSqlError::NoError && query.first())
        return query.value(0).toInt();

    return 0;
}

/*!
    Returns the count of the number of messages which pass the
    filtering criteria defined in QMailMessageKey \a key. If
    key is empty a count of all messages is returned.
*/

int QMailStore::countMessages(const QMailMessageKey& key) const
{
    QString sql = "SELECT COUNT(*) FROM mailmessages";
    if(!key.isEmpty())
        sql += d->buildWhereClause(key);

    QSqlQuery query = d->simpleQuery(sql, "MailMessage count query",
                                     QVariantList() << d->whereClauseValues(key));

    if(query.lastError().type() == QSqlError::NoError && query.first())
        return query.value(0).toInt();

    return 0;
}

/*!
    Returns the total size of the messages which pass the
    filtering criteria defined in QMailMessageKey \a key. If
    key is empty the total size of all messages is returned.
*/

int QMailStore::sizeOfMessages(const QMailMessageKey& key) const
{
    QString sql = "SELECT SUM(size) FROM mailmessages";
    if(!key.isEmpty())
        sql += d->buildWhereClause(key);

    QSqlQuery query = d->simpleQuery(sql, "MailMessages size query",
                                     QVariantList() << d->whereClauseValues(key));

    if(query.lastError().type() == QSqlError::NoError && query.first())
        return query.value(0).toInt();

    return 0;
}

/*!
    Returns the \l{QMailId}s of folders in the message store. If \a key is not empty
    only folders matching the parameters set by \a key will be returned, otherwise
    all folder identifiers will be returned.
    If \a sortKey is not empty, the identifiers will be sorted by the parameters set
    by \a sortKey.
*/

QMailIdList QMailStore::queryFolders(const QMailFolderKey& key,
                                     const QMailFolderSortKey& sortKey) const
{
    d->checkComparitors(key);

    QString sql = "SELECT id FROM mailfolders";
    if(!key.isEmpty())
        sql += d->buildWhereClause(key);
    if(!sortKey.isEmpty())
        sql += " " + d->buildOrderClause(sortKey);

    QSqlQuery query = d->simpleQuery(sql, "MailFolders id query",
                                     QVariantList() << d->whereClauseValues(key));

    QMailIdList results;
    while(query.next())
        results.append(QMailId(query.value(0).toULongLong()));

    return results;
}

/*!
    Returns the \l{QMailId}s of messages in the message store. If \a key is not empty
    only messages matching the parameters set by \a key will be returned, otherwise
    all message identifiers will be returned.
    If \a sortKey is not empty, the identifiers will be sorted by the parameters set
    by \a sortKey.
*/

QMailIdList QMailStore::queryMessages(const QMailMessageKey& key,
                                      const QMailMessageSortKey& sortKey) const
{
    d->checkComparitors(key);

    QString sql = "SELECT id FROM mailmessages";
    if(!key.isEmpty())
        sql += d->buildWhereClause(key);
    if(!sortKey.isEmpty())
        sql += " " + d->buildOrderClause(sortKey);

    QSqlQuery query = d->simpleQuery(sql,"MailMessages id query",
                                     QVariantList() << d->whereClauseValues(key));

    QMailIdList results;
    while(query.next())
        results.append(QMailId(query.value(0).toULongLong()));

    return results;
}

/*!
   Returns the QMailFolder defined by a QMailId \a id from
   the message store.
*/

QMailFolder QMailStore::folder(const QMailId& id) const
{
    QSqlQuery query = d->simpleQuery("SELECT * FROM mailfolders WHERE id=?",
                                     "MailFolder query",
                                     QVariantList() << id.toULongLong());
    QMailFolder result;
    if(query.first())
        result = d->buildQMailFolder(query.record());

    return result;
}

/*!
   Returns the QMailMessage defined by a QMailId \a id from
   the message store.
*/

QMailMessage QMailStore::message(const QMailId& id) const
{
    //mailfile is not an exposed property, so add separately
    QString sql = "SELECT " + d->expandProperties(allProperties) + ", mailfile";
    sql += " FROM mailmessages WHERE id=?";

    QSqlQuery query = d->simpleQuery(sql, "MailMessage query",
                                     QVariantList() << id.toULongLong());

    QMailMessage result;
    if(query.first())
        result = d->buildQMailMessage(query.record());

    return result;
}

/*!
   Returns the QMailMessage defined by the unique identifier \a uid from the account \a account.
*/

QMailMessage QMailStore::message(const QString& uid, const QString& account) const
{
    //mailfile is not an exposed property, so add separately
    QString sql = "SELECT " + d->expandProperties(allProperties) + ",mailfile";
    sql += " FROM mailmessages WHERE serveruid = ? AND fromaccount = ? ";

    QSqlQuery query = d->simpleQuery(sql,"MailMessage by account query",
                                     QVariantList() << uid << account);

    QMailMessage result;
    if(query.first())
        result = d->buildQMailMessage(query.record());

    return result;
}

/*!
   Returns the QMailMessage header defined by a QMailId \a id from
   the message store.
*/

QMailMessage QMailStore::messageHeader(const QMailId& id) const
{
    QMailMessage cachedHeader = d->headerCache.lookup(id);

    if(cachedHeader.id().isValid())
        return cachedHeader;

    //query id batch

    QMailIdList idBatch;
    for(int i = -QMailStorePrivate::lookAhead; i < QMailStorePrivate::lookAhead; ++i)
        idBatch.append(QMailId(id.toULongLong()+i));

    QMailMessageList results = messageHeaders(QMailMessageKey(idBatch), allProperties );

    foreach(QMailMessage header,results)
        if(header.id().isValid())
            d->headerCache.insert(header);

    return d->headerCache.lookup(id);
}

/*!
   Returns the QMailMessage header defined by the unique identifier \a uid from the account \a account.
*/

QMailMessage QMailStore::messageHeader(const QString& uid, const QString& account) const
{
    QMailMessageKey uidKey(QMailMessageKey::ServerUid,uid);
    QMailMessageKey accountKey(QMailMessageKey::FromAccount,account);

    QMailMessageList results = messageHeaders(uidKey & accountKey,allProperties);

    if(!results.isEmpty())
    {
        if(results.count() > 1)
            qLog(Messaging) << "Warning, messageHeader by uid returned more than 1 result";

        d->headerCache.insert(results.first());
        return results.first();
    }
    return QMailMessage();
}

/*!
    \enum QMailStore::ReturnOption
    This enum defines the message header return option for QMailStore::messageHeaders()

    \value ReturnAll Return all message headers that match the selection criteria, including duplicates.
    \value ReturnDistinct Return distinct message headers that match the selection criteria, excluding duplicates.
*/

/*!
    Retrieves a list of message headers containing data defined by \a properties
    for messages which pass the criteria defined by \a key. If \a option is
    \c ReturnAll then duplicate headers are included in the list; otherwise
    duplicate headers are excluded from the returned list.

    Returns a list of headers if successfully completed, or an empty list for
    an error or no data.
*/

QMailMessageList QMailStore::messageHeaders(const QMailMessageKey& key,
                                            const QMailMessageKey::Properties& properties,
                                            const ReturnOption& option) const
{
    d->checkComparitors(key);
    QString sql = "SELECT ";
    if(option == ReturnDistinct)
        sql += "DISTINCT ";

    sql += d->expandProperties(properties) + " FROM mailmessages" + d->buildWhereClause(key);

    QSqlQuery query = d->simpleQuery(sql,"MailMessage headers query",
                                     QVariantList() << d->whereClauseValues(key));

    QMailMessageList headers;
    while(query.next())
    {
        QMailMessage header = d->buildQMailMessage(query.record(),properties);
        headers.append(header);
    }

    return headers;
}

Q_GLOBAL_STATIC(QMailStore,QMailStoreInstance);

/*!
    Returns an instance of the QMailStore object.
*/

QMailStore* QMailStore::instance()
{
    static bool init = false;
    if(!init) init = QMailStorePrivate::initStore();
    return QMailStoreInstance();
}

/*!
    \fn void QMailStore::messagesAdded(const QMailIdList& ids)

    Signal that is emitted when the messages in the list \a ids are
    added to the mail store.

    \sa messagesRemoved(), messagesUpdated()
*/

/*!
    \fn void QMailStore::messagesRemoved(const QMailIdList& ids)

    Signal that is emitted when the messages in the list \a ids are
    removed from the mail store.

    \sa messagesAdded(), messagesUpdated()
*/

/*!
    \fn void QMailStore::messagesUpdated(const QMailIdList& ids)

    Signal that is emitted when the messages in the list \a ids are
    updated within the mail store.

    \sa messagesAdded(), messagesRemoved()
*/

/*!
    \fn void QMailStore::foldersAdded(const QMailIdList& ids)

    Signal that is emitted when the folders in the list \a ids are
    added to the mail store.

    \sa foldersRemoved(), foldersUpdated()
*/

/*!
    \fn void QMailStore::foldersRemoved(const QMailIdList& ids)

    Signal that is emitted when the folders in the list \a ids are
    removed from the mail store.

    \sa foldersAdded(), foldersUpdated()
*/

/*!
    \fn void QMailStore::foldersUpdated(const QMailIdList& ids)

    Signal that is emitted when the folders in the list \a ids are
    updated within the mail store.

    \sa foldersAdded(), foldersRemoved()
*/
