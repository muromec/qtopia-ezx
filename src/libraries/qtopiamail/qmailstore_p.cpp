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

#include "qmailstore_p.h"
#include "qmailfoldersortkey.h"
#include "qmailfoldersortkey_p.h"
#include "qmailmessagesortkey.h"
#include "qmailmessagesortkey_p.h"
#include "qmailmessagekey.h"
#include "qmailmessagekey_p.h"
#include "qmailfolderkey.h"
#include "qmailfolderkey_p.h"
#include "qmailtimestamp.h"

#include <qtopialog.h>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QMutex>
#include <QDSAction>
#include <QDSServiceInfo>
#include <QTextCodec>
#include <QtopiaSql>
#include <QSystemMutex>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h>

static QString comparitorWarning = "Warning!! Queries with many comparitors can cause problems. " \
                    "Prefer multiple calls with smaller keys when possible."; //no tr
static QString messageAddedSig = "messageAdded(int,quint64)";
static QString messageRemovedSig = "messageRemoved(int,quint64)";
static QString messageUpdatedSig = "messageUpdated(int,quint64)";
static QString folderAddedSig = "folderAdded(int,quint64)";
static QString folderRemovedSig = "folderRemoved(int,quint64)";
static QString folderUpdatedSig = "folderUpdated(int,quint64)";

MailMessageCache::MailMessageCache(unsigned int headerCacheSize)
:
    mCache(headerCacheSize)
{
}

MailMessageCache::~MailMessageCache()
{
}

QMailMessage MailMessageCache::lookup(const QMailId& id) const
{
   if(!id.isValid())
       return QMailMessage();
   else
   {
       QMailMessage* cachedMessage = mCache.object(id.toULongLong());
       if(!cachedMessage)
           return QMailMessage();
       else return *cachedMessage;
   }
}

void MailMessageCache::insert(const QMailMessage& message)
{
    if(!message.id().isValid())
        return;
    else
        mCache.insert(message.id().toULongLong(),new QMailMessage(message));
}

bool MailMessageCache::contains(const QMailId& id) const
{
    return mCache.contains(id.toULongLong());
}

void MailMessageCache::remove(const QMailId& id)
{
    mCache.remove(id.toULongLong());
}

QMailStorePrivate::QMailStorePrivate(QObject* parent)
:
    QObject(parent)
{
    QtopiaChannel* ipcChannel = new QtopiaChannel("QPE/Qtopiamail",this);
    connect(ipcChannel,
            SIGNAL(received(const QString&, const QByteArray&)),
            this,
            SLOT(ipcMessage(const QString&,const QByteArray&)));
}

QMailStorePrivate::~QMailStorePrivate()
{
}

/*!
    Recursive helper function used to delete a QMailFolder \a p
    and its child folders.  Returns \c true if the operation
    completed successfully, \c false otherwise.
*/

bool QMailStorePrivate::deleteFolder(const QMailId& p,
                                     QMailIdList& deletedSubFolders,
                                     QMailIdList& deletedMessages)
{
    //delete the child folders
    QSqlQuery query = prepare("SELECT id FROM mailfolders WHERE parentid = ?");

    if(query.lastError().type() != QSqlError::NoError)
        return false;

    query.addBindValue(p.toULongLong());

    if(!execute(query))
        return false;

    //get the list of id's

    QMailIdList idList;

    while(query.next())
        idList.append(QMailId(query.value(0).toULongLong()));

    foreach(QMailId id, idList)
        if(!deleteFolder(id,deletedSubFolders,deletedMessages))
            return false;

    //record deleted sub folders

    deletedSubFolders += idList;

    //delete the mails for this folder

    if(!deleteMailsFromFolder(p,deletedMessages))
        return false;

    //delete this folder

    query = prepare("DELETE FROM mailfolders WHERE id = ?");

    if(query.lastError().type() != QSqlError::NoError)
        return false;

    query.addBindValue(p.toULongLong());

    if(!execute(query))
        return false;

    return true;

}

/*!
    Helper function used to delete the mails contained in
    QMailfolder \a f. Returns \c true if the operation
    completed successfully, \c false otherwise.
*/

bool QMailStorePrivate::deleteMailsFromFolder(const QMailId& f, QMailIdList& deletedMessages)
{
    //get the list of mail files to delete

    QSqlQuery query = prepare("SELECT id, mailfile FROM mailmessages WHERE parentfolderid = ?");

    if(query.lastError().type() != QSqlError::NoError)
        return false;

    query.addBindValue(f.toULongLong());

    if(!execute(query))
        return false;

    QStringList mailfiles;
    QMailIdList mailIds;

    while(query.next())
    {
        mailIds.append(QMailId(query.value(0).toULongLong()));
        mailfiles.append(query.value(1).toString());
    }

    query = prepare("DELETE FROM mailmessages WHERE parentfolderid = ?");

    if(query.lastError().type() != QSqlError::NoError)
        return false;

    query.addBindValue(f.toULongLong());

    if(!execute(query))
        return false;

    //delete the mail files

    foreach(QString mailfile, mailfiles)
    {
        if(!mailBodyStore.remove(mailfile))
            qLog(Messaging) << "Could not remove the mail body " << mailfile;
    }

    //record deleted ids
    deletedMessages += mailIds;

    return true;
}

QSqlQuery QMailStorePrivate::prepare(const QString& sql) const
{
    QSqlQuery query(database);

    if(!query.prepare(sql))
    {
        qLog(Messaging) << "Failed to prepare query " <<
                            query.lastQuery().toLocal8Bit().constData() << " : " <<
                            query.lastError().text().toLocal8Bit().constData();
        database.rollback();
    }
    return query;
}

bool QMailStorePrivate::execute(QSqlQuery& query) const
{
    if(!query.exec())
    {
        qLog(Messaging) << "Failed to execute query " <<
                            query.lastQuery().toLocal8Bit().constData() << " : " <<
                            query.lastError().text().toLocal8Bit().constData();
        database.rollback();
        return false;
    }
    qLog(Messaging) << query.executedQuery().simplified();
    return true;
}

bool QMailStorePrivate::folderExists(const quint64& id)
{
    QSqlQuery query(database);
    if(!query.prepare("SELECT id FROM mailfolders WHERE id = ?"))
    {
        qLog(Messaging) << "Failed to prepare query " <<
                            query.lastQuery().toLocal8Bit().constData() << " : " <<
                            query.lastError().text().toLocal8Bit().constData();
        database.rollback();
        return false;
    }

    query.addBindValue(id);

    if(!query.exec())
    {
        qLog(Messaging) << "Failed to select from table mailfolders :" <<
                            query.lastError().text().toLocal8Bit().constData();
        database.rollback();
        return false;
    }

    return (query.first());
}

QMailMessage QMailStorePrivate::buildQMailMessage(const QSqlRecord& r,
                                                  const QMailMessageKey::Properties& properties) const
{
    const MessagePropertyMap& map = messagePropertyMap();
    const QList<QMailMessageKey::Property> keys = map.keys();

    QMailMessage newMessage;

    QString mailfile = r.value("mailfile").toString();
    bool hasMailfile = !mailfile.isEmpty();

    if(hasMailfile)
    {
        if(!mailBodyStore.load(mailfile,&newMessage))
            qLog(Messaging) << "Could not load message body " << mailfile;
    }

    foreach(QMailMessageKey::Property p,keys)
    {
        switch(properties & p)
        {
            case QMailMessageKey::Id:
                {
                    QMailId id(r.value("id").toULongLong());
                    newMessage.setId(id);
                }
                break;
            case QMailMessageKey::Type:
                {
                    QMailMessage::MessageType t = QMailMessage::MessageType(r.value("type").toInt());
                    newMessage.setMessageType(t);
                }
                break;
            case QMailMessageKey::ParentFolderId:
                {
                    QMailId parentfolderid(r.value("parentfolderid").toULongLong());
                    newMessage.setParentFolderId(parentfolderid);
                }
                break;
            case QMailMessageKey::Sender:
                {
                    if(hasMailfile) continue; //sender from message has precedence

                    QString sender = r.value("sender").toString();

                    static const QString smsTag("@sms");

                    // Remove sms-origin tag, if present - the SMS client previously appended
                    // "@sms" to the from address, which is no longer necesary
                    if (sender.endsWith(smsTag))
                        sender.chop(smsTag.length());

                    newMessage.setFrom(QMailAddress(sender));
                }
                break;
            case QMailMessageKey::Recipients:
                {
                    if(hasMailfile) continue;//recipients from message has precedence

                    QStringList recipients(r.value("recipients").toString());
                    newMessage.setTo(QMailAddress::fromStringList(recipients));
                }
                break;
            case QMailMessageKey::Subject:
                {
                    if(hasMailfile) continue;//subject from message has precedence

                    QString subject = r.value("subject").toString();
                    newMessage.setSubject(subject);
                }
                break;
            case QMailMessageKey::TimeStamp:
                {
                    if(hasMailfile) continue;//timestamp from message has precedence

                    QDateTime timestamp = r.value("stamp").toDateTime();
                    newMessage.setDate(QMailTimeStamp(timestamp));
                }
                break;
            case QMailMessageKey::Status:
                {
                    QMailMessage::Status flags(r.value("status").toInt());
                    newMessage.setStatus(flags);
                }
                break;
            case QMailMessageKey::FromAccount:
                {
                    QString fromAccount = r.value("fromaccount").toString();
                    newMessage.setFromAccount(fromAccount);
                }
                break;
            case QMailMessageKey::FromMailbox:
                {
                    QString fromMailbox = r.value("frommailbox").toString();
                    newMessage.setFromMailbox(fromMailbox);
                }
                break;
            case QMailMessageKey::ServerUid:
                {
                    QString serveruid = r.value("serveruid").toString();
                    newMessage.setServerUid(serveruid);
                }
                break;
            case QMailMessageKey::Size:
                {
                    int size = r.value("size").toInt();
                    newMessage.setSize(size);
                }
                break;
        }
    }

    newMessage.changesCommitted();

    return newMessage;
}

QMailFolder QMailStorePrivate::buildQMailFolder(const QSqlRecord& r) const
{
    QMailId id = QMailId(r.value("id").toULongLong());
    QString name = r.value("name").toString();
    QMailId parentId = QMailId(r.value("parentid").toULongLong());

    QMailFolder result(name,parentId);
    result.setId(id);
    return result;
}

QString QMailStorePrivate::buildOrderClause(const QMailFolderSortKey& key) const
{
    //convert the key to an sql selection string

    QString sortClause = " ORDER BY ";

    for(int i =0; i < key.d->arguments.count(); ++i)
    {
        QMailFolderSortKeyPrivate::Argument a = key.d->arguments.at(i);
        if(i > 0)
            sortClause += ",";
        switch(a.first)
        {
            case QMailFolderSortKey::Id:
                sortClause += "id";
                break;
            case QMailFolderSortKey::Name:
                sortClause += "name";
                break;
            case QMailFolderSortKey::ParentId:
                sortClause += "parentid";
                break;
        }
        if(a.second == Qt::AscendingOrder)
            sortClause += " ASC";
        else
            sortClause += " DESC";
    }
    return sortClause;
}

QString QMailStorePrivate::buildOrderClause(const QMailMessageSortKey& key) const
{
    //convert the key to an sql selection string

    QString sortClause = " ORDER BY ";

    for(int i =0; i < key.d->arguments.count(); ++i)
    {
        QMailMessageSortKeyPrivate::Argument a = key.d->arguments.at(i);
        if(i > 0)
            sortClause += ",";

        switch(a.first)
        {
            case QMailMessageSortKey::Id:
                sortClause += "id";
                break;
            case QMailMessageSortKey::Type:
                sortClause += "type";
                break;
            case QMailMessageSortKey::ParentFolderId:
                sortClause += "parentfolderid";
                break;
            case QMailMessageSortKey::Sender:
                sortClause += "sender";
                break;
            case QMailMessageSortKey::Recipients:
                sortClause += "recipients";
                break;
            case QMailMessageSortKey::Subject:
                sortClause += "subject";
                break;
            case QMailMessageSortKey::TimeStamp:
                sortClause += "stamp";
                break;
            case QMailMessageSortKey::Status:
                sortClause += "status";
                break;
            case QMailMessageSortKey::FromAccount:
                sortClause += "fromaccount";
                break;
            case QMailMessageSortKey::FromMailbox:
                sortClause += "frommailbox";
                break;
            case QMailMessageSortKey::ServerUid:
                sortClause += "serveruid";
                break;
            case QMailMessageSortKey::Size:
                sortClause += "size";
                break;

        }
        if(a.second == Qt::AscendingOrder)
            sortClause += " ASC";
        else
            sortClause += " DESC";
    }
    return sortClause;
}

QString QMailStorePrivate::buildWhereClause(const QMailMessageKey& key, bool subQuery) const
{
    //convert the key to an sql selection string
    QString logicalOpString = key.d->logicalOp == QMailMessageKeyPrivate::Or ? " OR " : " AND ";
    QString queryString;
    if(!subQuery)
        queryString = " WHERE ";
    QTextStream q(&queryString);

    QString compareOpString;
    QString op = " ";

    foreach(QMailMessageKeyPrivate::Argument a,key.d->arguments)
    {
        bool addressRelated(a.property == QMailMessageKey::Sender || a.property == QMailMessageKey::Recipients);

        switch(a.op)
        {
        case QMailMessageKey::LessThan:
            compareOpString = " < ";
            break;
        case QMailMessageKey::GreaterThan:
            compareOpString = " > ";
            break;
        case QMailMessageKey::Equal:
            if(a.valueList.count() > 1)
                compareOpString = " IN ";
            else
                // When matching addreses, we will force equality to actually use a content match
                compareOpString = (addressRelated ? " LIKE " : " = ");
            break;
        case QMailMessageKey::LessThanEqual:
            compareOpString = " <= ";
            break;
        case QMailMessageKey::GreaterThanEqual:
            compareOpString = " >= ";
            break;
        case QMailMessageKey::NotEqual:
            compareOpString = " <> ";
            break;
        case QMailMessageKey::Contains:
            compareOpString = " LIKE ";
            break;
        }

        switch(a.property)
        {
        case QMailMessageKey::Id:
            q << op << "id " << compareOpString << expandValueList(a.valueList);
            break;
        case QMailMessageKey::Type:
            if(a.op == QMailMessageKey::Contains)
                q << op << "type & " << '?';
            else
                q << op << "type " << compareOpString << expandValueList(a.valueList);
            break;
        case QMailMessageKey::ParentFolderId:
            q << op << "parentfolderid " << compareOpString << expandValueList(a.valueList);
            break;
        case QMailMessageKey::Sender:
            q << op << "sender " << compareOpString << expandValueList(a.valueList);
            break;
        case QMailMessageKey::Recipients:
            q << op << "recipients " << compareOpString << expandValueList(a.valueList);
            break;
        case QMailMessageKey::Subject:
            q << op << "subject " << compareOpString << expandValueList(a.valueList);
            break;
        case QMailMessageKey::TimeStamp:
            q << op << "timestamp " << compareOpString << expandValueList(a.valueList);
            break;
        case QMailMessageKey::Status:
            if(a.op == QMailMessageKey::Contains)
                q << op << "status & " << '?';
            else
                q << op << "status " << compareOpString << expandValueList(a.valueList);
            break;
        case QMailMessageKey::FromAccount:
            q << op << "fromaccount " << compareOpString << expandValueList(a.valueList);
            break;
        case QMailMessageKey::FromMailbox:
            q << op << "frommailbox " << compareOpString << expandValueList(a.valueList);
            break;
        case QMailMessageKey::ServerUid:
            q << op << "serveruid " << compareOpString << expandValueList(a.valueList);
            break;
        case QMailMessageKey::Size:
            q << op << "size " << compareOpString << expandValueList(a.valueList);
            break;
        }
        op = logicalOpString;
    }

    //subkeys

    if(queryString.isEmpty())
        op = " ";

    foreach(QMailMessageKey subkey,key.d->subKeys)
    {
        QString subquery = buildWhereClause(subkey,true);
        q << op << " ( " << subquery << " ) ";
        op = logicalOpString;
    }

    if(key.d->negated)
        return "NOT " + queryString;
    else
        return queryString;
}

QVariantList QMailStorePrivate::whereClauseValues(const QMailMessageKey& key) const
{
    QVariantList values;

    foreach(QMailMessageKeyPrivate::Argument a,key.d->arguments)
    {
        foreach(QVariant var,a.valueList)
        {
            QString stringData = var.toString();

            if ((a.property == QMailMessageKey::Sender) || (a.property == QMailMessageKey::Recipients)) {
                // If the query argument is a phone number, ensure it is in minimal form
                QMailAddress address(stringData);
                if (address.isPhoneNumber()) {
                    stringData = address.minimalPhoneNumber();

                    // Rather than compare exact numbers, we will only use the trailing
                    // digits to compare phone numbers - otherwise, slightly different
                    // forms of the same number will not be matched
                    static const int significantDigits = 8;

                    int extraneous = stringData.length() - significantDigits;
                    if (extraneous > 0)
                        stringData.remove(0, extraneous);
                }
            }

            //delimit data for sql "LIKE" operator
            //if valuelist > 1; dont delimit for "IN" operator
            bool addressRelated(a.property == QMailMessageKey::Sender || a.property == QMailMessageKey::Recipients);
            if((a.op == QMailMessageKey::Contains) ||
               ((a.op == QMailMessageKey::Equal) && addressRelated && a.valueList.count() == 1)) {
                if(!stringData.isEmpty())
                    stringData = "\%" + stringData + "\%";
            }

            switch(a.property)
            {
                case QMailMessageKey::Id:
                    {
                        QMailId id = var.value<QMailId>();
                        values.append(id.toULongLong());
                    }
                    break;
                case QMailMessageKey::Type:
                    values.append(var.toInt());
                    break;
                case QMailMessageKey::ParentFolderId:
                    {
                        QMailId id = var.value<QMailId>();
                        values.append(id.toULongLong());
                    }
                    break;
                case QMailMessageKey::Sender:
                    values.append(stringData);
                    break;
                case QMailMessageKey::Recipients:
                    values.append(stringData);
                    break;
                case QMailMessageKey::Subject:
                    values.append(stringData);
                    break;
                case QMailMessageKey::TimeStamp:
                    values.append(stringData);
                    break;
                case QMailMessageKey::Status:
                    values.append(var.toUInt());
                    break;
                case QMailMessageKey::FromAccount:
                    values.append(stringData);
                    break;
                case QMailMessageKey::FromMailbox:
                    values.append(stringData);
                    break;
                case QMailMessageKey::ServerUid:
                    values.append(stringData);
                    break;
                case QMailMessageKey::Size:
                    values.append(var.toInt());
                    break;
            }
        }
    }

    //subkeys

    foreach(QMailMessageKey subkey,key.d->subKeys)
        values << whereClauseValues(subkey);

    return values;
}

void QMailStorePrivate::bindWhereData(const QMailMessageKey& key, QSqlQuery& query) const
{
    QVariantList values = whereClauseValues(key);

    foreach(const QVariant& v, values)
        query.addBindValue(v);
}

QString QMailStorePrivate::buildWhereClause(const QMailFolderKey& key, bool subQuery) const
{
    //convert the key to an sql selection string
    QString logicalOpString = key.d->logicalOp == QMailFolderKeyPrivate::Or ? " OR " : " AND ";
    QString queryString;
    if(!subQuery)
        queryString = " WHERE ";
    QTextStream q(&queryString);

    QString compareOpString;
    QString op = " ";

    foreach(QMailFolderKeyPrivate::Argument a,key.d->arguments)
    {
        switch(a.op)
        {
        case QMailFolderKey::LessThan:
            compareOpString = " < ";
            break;
        case QMailFolderKey::GreaterThan:
            compareOpString = " > ";
            break;
        case QMailFolderKey::Equal:
            if(a.valueList.count() > 1)
                compareOpString = " IN ";
            else
                compareOpString = " = ";
            break;
        case QMailFolderKey::LessThanEqual:
            compareOpString = " <= ";
            break;
        case QMailFolderKey::GreaterThanEqual:
            compareOpString = " >= ";
            break;
        case QMailFolderKey::NotEqual:
            compareOpString = " <> ";
            break;
        case QMailFolderKey::Contains:
            compareOpString = " LIKE ";
            break;
        }
        switch(a.property)
        {
        case QMailFolderKey::Id:
            q << op << "id " << compareOpString << expandValueList(a.valueList);
            break;
        case QMailFolderKey::Name:
            q << op << "name " << compareOpString << expandValueList(a.valueList);
            break;
        case QMailFolderKey::ParentId:
            q << op << "parentid " << compareOpString << expandValueList(a.valueList);
            break;
        }
        op = logicalOpString;
    }

    //subkeys
    if(queryString.isEmpty())
        op = " ";

    foreach(QMailFolderKey subkey,key.d->subKeys)
    {
        QString subquery = buildWhereClause(subkey,true);
        q << op << " ( " << subquery << " ) ";
        op = logicalOpString;

    }

    if(key.d->negated)
        return "NOT " + queryString;
    else
        return queryString;

}

QVariantList QMailStorePrivate::whereClauseValues(const QMailFolderKey& key) const
{
    QVariantList values;

    foreach(QMailFolderKeyPrivate::Argument a,key.d->arguments)
    {
        foreach(QVariant var, a.valueList)
        {
            switch(a.property)
            {
            case QMailFolderKey::Id:
                {
                    QMailId id = var.value<QMailId>();
                    values.append(id.toULongLong());
                }
                break;
            case QMailFolderKey::Name:
                {
                    //delimit data for sql "LIKE" operator

                    QString stringData = var.toString();
                    if(a.op == QMailFolderKey::Contains)
                        if(!stringData.isEmpty())
                            stringData = "\%" + stringData + "\%";
                    values.append(stringData);
                }
                break;
            case QMailFolderKey::ParentId:
                {
                    QMailId id = var.value<QMailId>();
                    values.append(id.toULongLong());
                }
                break;
            }
        }
    }

    foreach(QMailFolderKey subkey,key.d->subKeys)
        values << whereClauseValues(subkey);

    return values;
}

void QMailStorePrivate::bindWhereData(const QMailFolderKey& key, QSqlQuery& query) const
{
    QVariantList values = whereClauseValues(key);

    foreach(const QVariant& v, values)
        query.addBindValue(v);
}

QVariantList QMailStorePrivate::updateValues(const QMailMessageKey::Properties& properties,
                                             const QMailMessage& data) const
{
    QVariantList values;

    const MessagePropertyMap& map = messagePropertyMap();
    const QList<QMailMessageKey::Property> keys = map.keys();

    foreach(QMailMessageKey::Property p,keys)
    {
        switch(properties & p)
        {
            case QMailMessageKey::Id:
                values.append(data.id().toULongLong());
                break;
            case QMailMessageKey::Type:
                values.append(static_cast<int>(data.messageType()));
                break;
            case QMailMessageKey::ParentFolderId:
                values.append(data.parentFolderId().toULongLong());
                break;
            case QMailMessageKey::Sender:
                values.append(data.from().toString());
                break;
            case QMailMessageKey::Recipients:
                values.append(QMailAddress::toStringList(data.to()).join(","));
                break;
            case QMailMessageKey::Subject:
                values.append(data.subject());
                break;
            case QMailMessageKey::TimeStamp:
                values.append(data.date().toLocalTime());
                break;
            case QMailMessageKey::Status:
                values.append(static_cast<int>(data.status()));
                break;
            case QMailMessageKey::FromAccount:
                values.append(data.fromAccount());
                break;
            case QMailMessageKey::FromMailbox:
                values.append(data.fromMailbox());
                break;
            case QMailMessageKey::ServerUid:
                values.append(data.serverUid());
                break;
            case QMailMessageKey::Size:
                values.append(data.size());
                break;
        }
    }
    return values;
}

void QMailStorePrivate::bindUpdateData(const QMailMessageKey::Properties& properties,
                                       const QMailMessage& data,
                                       QSqlQuery& query) const
{
    QVariantList values = updateValues(properties,data);
    foreach(const QVariant& v, values)
        query.addBindValue(v);
}

void QMailStorePrivate::bindUpdateData(const QMailMessageKey::Properties& properties,
                                       const QMailMessage& fromMessage,
                                       QMailMessage& toMessage) const
{
    const MessagePropertyMap& map = messagePropertyMap();
    const QList<QMailMessageKey::Property> keys = map.keys();

    foreach(QMailMessageKey::Property p,keys)
    {
        switch(properties & p)
        {
            case QMailMessageKey::Id:
                toMessage.setId(fromMessage.id());
                break;
            case QMailMessageKey::Type:
                toMessage.setMessageType(fromMessage.messageType());
                break;
            case QMailMessageKey::ParentFolderId:
                toMessage.setParentFolderId(fromMessage.parentFolderId());
                break;
            case QMailMessageKey::Sender:
                toMessage.setFrom(fromMessage.from());
                break;
            case QMailMessageKey::Recipients:
                toMessage.setTo(fromMessage.to());
                break;
            case QMailMessageKey::Subject:
                toMessage.setSubject(fromMessage.subject());
                break;
            case QMailMessageKey::TimeStamp:
                toMessage.setDate(fromMessage.date());
                break;
            case QMailMessageKey::Status:
                toMessage.setStatus(fromMessage.status());
                break;
            case QMailMessageKey::FromAccount:
                toMessage.setFromAccount(fromMessage.fromAccount());
                break;
            case QMailMessageKey::FromMailbox:
                toMessage.setFromMailbox(fromMessage.fromMailbox());
                break;
            case QMailMessageKey::ServerUid:
                toMessage.setServerUid(fromMessage.serverUid());
                break;
            case QMailMessageKey::Size:
                toMessage.setSize(fromMessage.size());
                break;
        }
    }
}

bool QMailStorePrivate::initStore()
{
    bool result = false;
    QSqlDatabase db = QtopiaSql::instance()->applicationSpecificDatabase("qtmail");

    int id = (int)::ftok("qmailstoredb", 0);
    QSystemMutex sMutex(id,false);
    sMutex.lock(100);

    if(!db.isOpenError())
        result = setupTables(QStringList() <<
                             "mailfolders" <<
                             "mailmessages",db);

    sMutex.unlock();

    return result;
}

bool QMailStorePrivate::setupTables(QStringList tableList, QSqlDatabase& db)
{
    QStringList tables = db.tables();

    bool result = true;

    foreach(QString table, tableList) {

        if (tables.contains(table, Qt::CaseInsensitive))
            continue;

        // load schema.
        QFile data(QLatin1String(":/QtopiaSql/") + db.driverName() + QLatin1String("/") + table);
        if(!data.open(QIODevice::ReadOnly))
        {
            qLog(Messaging) << "Failed to load table resource " << table;
            result &= false;
        }
        QTextStream ts(&data);
        // read assuming utf8 encoding.
        ts.setCodec(QTextCodec::codecForName("utf8"));
        ts.setAutoDetectUnicode(true);

        QString sql = parseSql(ts);
        while(!sql.isEmpty())
        {
            QSqlQuery query(db);
            if(!query.exec(sql))
            {
                qLog(Messaging) << "Failed to exec table creation SQL query " << sql << " " \
                    << query.lastError().text();
                result &= false;
            }
            sql = parseSql(ts);
        }
    }
    return result;
}

QString QMailStorePrivate::parseSql(QTextStream& ts)
{
    QString qry = "";
    while(!ts.atEnd())
    {
        QString line = ts.readLine();
        // comment, remove.
        if (line.contains (QLatin1String("--")))
            line.truncate (line.indexOf (QLatin1String("--")));
        if (line.trimmed ().length () == 0)
            continue;
        qry += line;

        if ( line.contains( ';' ) == false)
            qry += QLatin1String(" ");
        else
            return qry;
    }
    return qry;
}

QString QMailStorePrivate::expandValueList(const QVariantList& valueList) const
{
    Q_ASSERT(!valueList.isEmpty());

    if(valueList.count() == 1)
        return "(?)";
    else
    {
        QString inList = " (?";
        for(int i = 1; i < valueList.count(); ++i)
            inList += ",?";
        inList += ")";
        return inList;
    }
}

const MessagePropertyMap& QMailStorePrivate::messagePropertyMap() const
{
    static MessagePropertyMap map;
    if(map.isEmpty())
    {
        map.insert(QMailMessageKey::Id,"id");
        map.insert(QMailMessageKey::Type,"type");
        map.insert(QMailMessageKey::ParentFolderId,"parentfolderid");
        map.insert(QMailMessageKey::Sender,"sender");
        map.insert(QMailMessageKey::Recipients,"recipients");
        map.insert(QMailMessageKey::Subject,"subject");
        map.insert(QMailMessageKey::TimeStamp,"stamp");
        map.insert(QMailMessageKey::Status,"status");
        map.insert(QMailMessageKey::FromAccount,"fromaccount");
        map.insert(QMailMessageKey::FromMailbox,"frommailbox");
        map.insert(QMailMessageKey::ServerUid,"serveruid");
        map.insert(QMailMessageKey::Size,"size");
    }
    return map;
}

QString QMailStorePrivate::expandProperties(const QMailMessageKey::Properties& p, bool update) const
{
    const MessagePropertyMap& map = messagePropertyMap();
    const QList<QMailMessageKey::Property> keys = map.keys();
    QString out;

    for(int i = 0 ; i < keys.count(); ++i)
    {
        QMailMessageKey::Property prop = keys[i];
        if(p & prop)
        {
            if(!out.isEmpty())
                out += ",";
            out += map.value(prop);
            if(update)
                out += "=?";
        }
    }

    return out;
}

int QMailStorePrivate::numComparitors(const QMailMessageKey& key) const
{
    int total = 0;
    foreach(QMailMessageKeyPrivate::Argument a, key.d->arguments)
        total += a.valueList.count();
    foreach(QMailMessageKey k,key.d->subKeys)
        total += numComparitors(k);
    return total;
}

int QMailStorePrivate::numComparitors(const QMailFolderKey& key) const
{
    int total = 0;
    foreach(QMailFolderKeyPrivate::Argument a, key.d->arguments)
        total += a.valueList.count();
    foreach(QMailFolderKey k,key.d->subKeys)
        total += numComparitors(k);
    return total;

}


void QMailStorePrivate::checkComparitors(const QMailMessageKey& key) const
{
    if(numComparitors(key) > maxComparitorsCutoff)
        qLog(Messaging) << comparitorWarning;
}

void QMailStorePrivate::checkComparitors(const QMailFolderKey& key) const
{
    if(numComparitors(key) > maxComparitorsCutoff)
        qLog(Messaging) << comparitorWarning;

}

void QMailStorePrivate::notifyMessagesChange(const ChangeType& changeType,
                                             const QMailIdList& ids)
{
    SegmentList segments = createSegments(ids.count(),maxNotifySegmentSize);

    QString funcSig;
    switch(changeType)
    {
        case Added:
            funcSig = messageAddedSig;
            break;
        case Removed:
            funcSig = messageRemovedSig;
            break;
        case Updated:
            funcSig = messageUpdatedSig;
            break;
    }

    foreach(Segment segment, segments)
    {
        QMailIdList idSegment;
        for(int i = segment.first; i < segment.second; ++i)
            idSegment.append(ids[i]);

        QtopiaIpcEnvelope e("QPE/Qtopiamail",funcSig);
        e << getpid();
        e << idSegment;
    }

}

void QMailStorePrivate::notifyFoldersChange(const ChangeType& changeType,
                                            const QMailIdList& ids)
{
    SegmentList segments = createSegments(ids.count(),maxNotifySegmentSize);

    QString funcSig;

    switch(changeType)
    {
        case Added:
            funcSig = folderAddedSig;
            break;
        case Removed:
            funcSig = folderRemovedSig;
            break;
        case Updated:
            funcSig = folderUpdatedSig;
            break;
    }

    foreach(Segment segment,segments)
    {
        QMailIdList idSegment;
        for(int i = segment.first; i < segment.second; ++i)
            idSegment.append(ids[i]);

        QtopiaIpcEnvelope e("QPE/Qtopiamail",funcSig);
        e << getpid();
        e << ids;
    }
}


void QMailStorePrivate::ipcMessage(const QString& message, const QByteArray& data)
{
    QDataStream ds(data);

    int pid;
    ds >> pid;

    if(getpid() == pid) //dont notify ourselves
        return;

    QMailIdList ids;
    ds >> ids;

    //for update and remove, clear header cache
    if(message == messageAddedSig)
        emit messagesAdded(ids);
    else if(message == messageRemovedSig)
    {
        foreach(QMailId id,ids)
            if(headerCache.contains(id))
                headerCache.remove(id);
        emit messagesRemoved(ids);
    }
    else if(message == messageUpdatedSig)
    {
        foreach(QMailId id,ids)
            if(headerCache.contains(id))
                headerCache.remove(id);
        emit messagesUpdated(ids);
    }
    else if(message == folderAddedSig)
        emit foldersAdded(ids);
    else if(message == folderUpdatedSig)
        emit foldersUpdated(ids);
    else if(message == folderRemovedSig)
        emit foldersRemoved(ids);
}

SegmentList QMailStorePrivate::createSegments(int numItems, int segmentSize)
{
    Q_ASSERT(segmentSize > 0);

    if(numItems <= 0)
        return SegmentList();

    int segmentCount = numItems % segmentSize ? 1 : 0;
    segmentCount += numItems / segmentSize;

    SegmentList segments;
    for(int i = 0; i < segmentCount ; ++i)
    {
        int start = segmentSize * i;
        int end = (i+1) == segmentCount ? numItems : (i+1) * segmentSize;
        segments.append(Segment(start,end));
    }
    return segments;
}

QSqlQuery QMailStorePrivate::simpleQuery(const QString& statement, const QString& descriptor,
                                         const QVariantList& values)
{
    QSqlQuery query = prepare(statement);

    foreach(const QVariant& v, values)
        query.addBindValue(v);

    execute(query);

    if(query.lastError().type() != QSqlError::NoError)
        qLog(Messaging) << "Query failed for " << descriptor;

    return query;
}

