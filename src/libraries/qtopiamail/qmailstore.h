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

#ifndef __QMAILSTORE_H
#define __QMAILSTORE_H

#include <QMailId>
#include <qtopiaglobal.h>
#include "qmailfolderkey.h"
#include "qmailfoldersortkey.h"
#include "qmailmessagekey.h"
#include "qmailmessagesortkey.h"

class QMailFolder;
class QMailMessage;
class QMailStorePrivate;
class QMailStore;



typedef QList<QMailFolder> QMailFolderList;
typedef QList<QMailMessage> QMailMessageList;
typedef QList<QMailId> QMailIdList;

#ifdef QMAILSTOREINSTANCE_DEFINED_HERE
static QMailStore* QMailStoreInstance();
#endif

class QTOPIAMAIL_EXPORT QMailStore : public QObject
{
    Q_OBJECT

public:
    enum ReturnOption
    {
        ReturnAll,
        ReturnDistinct
    };

public:
    virtual ~QMailStore();

    bool addFolder(QMailFolder* f);
    bool addMessage(QMailMessage* m);

    bool removeFolder(const QMailId& id);
    bool removeMessage(const QMailId& id);

    bool updateFolder(QMailFolder* f);
    bool updateMessage(QMailMessage* m);
    bool updateMessages(const QMailMessageKey& key,
                        const QMailMessageKey::Properties& properties,
                        const QMailMessage& data);
    bool updateMessages(const QMailMessageKey& key,
                        const QMailMessage::Status status,
                        bool set);

    int countFolders(const QMailFolderKey& key = QMailFolderKey()) const;
    int countMessages(const QMailMessageKey& key = QMailMessageKey()) const;

    int sizeOfMessages(const QMailMessageKey& key = QMailMessageKey()) const;

    QMailIdList queryFolders(const QMailFolderKey& key = QMailFolderKey(),
                             const QMailFolderSortKey& sortKey = QMailFolderSortKey()) const;
    QMailIdList queryMessages(const QMailMessageKey& key = QMailMessageKey(),
                              const QMailMessageSortKey& sortKey = QMailMessageSortKey()) const;

    QMailFolder folder(const QMailId& id) const;

    QMailMessage message(const QMailId& id) const;
    QMailMessage message(const QString& uid, const QString& account) const;

    QMailMessage messageHeader(const QMailId& id) const;
    QMailMessage messageHeader(const QString& uid, const QString& account) const;
    QMailMessageList messageHeaders(const QMailMessageKey& key,
                                    const QMailMessageKey::Properties& properties,
                                    const ReturnOption& option = ReturnAll) const;

    static QMailStore* instance();
#ifdef QMAILSTOREINSTANCE_DEFINED_HERE
    friend QMailStore* QMailStoreInstance();
#endif

signals:
    void messagesAdded(const QMailIdList& ids);
    void messagesRemoved(const QMailIdList& ids);
    void messagesUpdated(const QMailIdList& ids);
    void foldersAdded(const QMailIdList& ids);
    void foldersRemoved(const QMailIdList& ids);
    void foldersUpdated(const QMailIdList& ids);

private:
    QMailStore();

    friend class EmailFolderList;
    friend class EmailClient;

private:
    QMailStorePrivate* d;

};



#endif //QMAILSTORE_H

