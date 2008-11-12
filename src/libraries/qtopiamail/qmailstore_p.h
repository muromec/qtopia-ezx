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

#ifndef __QMAILSTOREPRIVATE_H
#define __QMAILSTOREPRIVATE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qtopia API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QSqlDatabase>
#include <QString>
#include <QCache>
#include <QtopiaIpcEnvelope>
#include "mailbodystore_p.h"
#include "qmailfolder.h"
#include "qmailmessage.h"
#include "qmailmessagekey.h"
#include "qmailmessagesortkey.h"
#include "qmailfoldersortkey.h"

class QMailId;
class QMailFolderKey;
class QMailFolderSortKey;
class QSqlRecord;
class QSqlQuery;

typedef QMap<QMailMessageKey::Property,QString> MessagePropertyMap;
typedef QPair<int,int> Segment; //start,end - end non inclusive
typedef QList<Segment> SegmentList;

static  QMailMessageKey::Properties updatableProperties = QMailMessageKey::ParentFolderId |
                                                          QMailMessageKey::Type |
                                                          QMailMessageKey::Sender |
                                                          QMailMessageKey::Recipients |
                                                          QMailMessageKey::Subject |
                                                          QMailMessageKey::TimeStamp |
                                                          QMailMessageKey::Status |
                                                          QMailMessageKey::FromAccount |
                                                          QMailMessageKey::FromMailbox |
                                                          QMailMessageKey::ServerUid |
                                                          QMailMessageKey::Size;
//all exposed message properties
static QMailMessageKey::Properties allProperties = QMailMessageKey::Id | updatableProperties;

class MailMessageCache
{
public:
    MailMessageCache(unsigned int headerCacheSize = 100);
    ~MailMessageCache();

    QMailMessage lookup(const QMailId& id) const;
    void insert(const QMailMessage& message);
    bool contains(const QMailId& id) const;
    void remove(const QMailId& id);

private:
    QCache<quint64,QMailMessage> mCache;
};


class QMailStorePrivate : public QObject
{
    Q_OBJECT

public:
    static const int lookAhead = 5;
    static const int maxComparitorsCutoff = 50;
    static const int maxNotifySegmentSize = 50;

    enum ChangeType
    {
        Added,
        Removed,
        Updated
    };

public:
    QMailStorePrivate(QObject* parent = 0);
    ~QMailStorePrivate();

    bool deleteFolder(const QMailId& f,
                      QMailIdList& deletedSubfolders,
                      QMailIdList& deletedMessages);
    bool deleteMailsFromFolder(const QMailId& f,
                               QMailIdList& deletedMessages);

    QSqlQuery prepare(const QString& sql) const;
    bool execute(QSqlQuery& q) const;

    bool folderExists(const quint64& id);

    QMailMessage buildQMailMessage(const QSqlRecord& r,
                                   const QMailMessageKey::Properties& properties = allProperties) const;
    QMailFolder buildQMailFolder(const QSqlRecord& r) const;

    QString buildOrderClause(const QMailFolderSortKey& key) const;
    QString buildOrderClause(const QMailMessageSortKey& key) const;

    QString buildWhereClause(const QMailMessageKey& k, bool subQuery = false) const;
    QVariantList whereClauseValues(const QMailMessageKey& key) const;
    void bindWhereData(const QMailMessageKey& key, QSqlQuery& query) const;

    QString buildWhereClause(const QMailFolderKey& k, bool subQuery = false) const;
    QVariantList whereClauseValues(const QMailFolderKey& key) const;
    void bindWhereData(const QMailFolderKey& key, QSqlQuery& query) const;

    QVariantList updateValues(const QMailMessageKey::Properties& properties,
                              const QMailMessage& data) const;
    void bindUpdateData(const QMailMessageKey::Properties& properties,
                        const QMailMessage& data,
                        QSqlQuery& query) const;
    void bindUpdateData(const QMailMessageKey::Properties& properties,
                        const QMailMessage& fromMessage,
                        QMailMessage& toMessage) const;

    static bool initStore();
    static bool setupTables(QStringList tableList, QSqlDatabase& db);
    static QString parseSql(QTextStream& ts);

    QString expandValueList(const QVariantList& valueList) const;
    const MessagePropertyMap& messagePropertyMap() const;
    QString expandProperties(const QMailMessageKey::Properties& p, bool update = false) const;

    int numComparitors(const QMailMessageKey& key) const;
    int numComparitors(const QMailFolderKey& key) const;

    void checkComparitors(const QMailMessageKey& key) const;
    void checkComparitors(const QMailFolderKey& key) const;

    void notifyMessagesChange(const ChangeType& changeType,
                              const QMailIdList& ids);
    void notifyFoldersChange(const ChangeType& changeType,
                             const QMailIdList& ids);

    SegmentList createSegments(int numItems, int segmentSize);

    QSqlQuery simpleQuery(const QString& statement,
                          const QString& descriptor,
                          const QVariantList& values = QVariantList());

public slots:
    void ipcMessage(const QString& message, const QByteArray& data);

signals:
    void messagesAdded(const QMailIdList& ids);
    void messagesUpdated(const QMailIdList& ids);
    void messagesRemoved(const QMailIdList& ids);
    void foldersAdded(const QMailIdList& ids);
    void foldersUpdated(const QMailIdList& ids);
    void foldersRemoved(const QMailIdList& ids);

public:
    mutable MailMessageCache headerCache;
    mutable QSqlDatabase database;
    MailBodyStore mailBodyStore;
};

#endif
