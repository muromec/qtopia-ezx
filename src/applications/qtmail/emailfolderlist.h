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



#ifndef EMAILFOLDERLIST_H
#define EMAILFOLDERLIST_H

#include "account.h"

#include <qobject.h>
#include <qstring.h>
#include <qlist.h>
#include <qfile.h>
#include <qtextstream.h>

#include <qtopiaglobal.h>
#include <QMailMessage>
#include <QMailFolder>
#include <QMailMessageKey>
#include <QMailMessageSortKey>

class EmailFolderList;
class QMailAccount;

enum IconType
{
    NoIcon = 0,
    AllMessages = 1,
    UnreadMessages = 2,
    UnsentMessages = 3
};

class QTOPIAMAIL_EXPORT EmailFolderList : public QObject
{
    Q_OBJECT

public:
    enum MailType { All, Unread, Unsent, Unfinished };
    enum SortOrder { Submission, AscendingDate, DescendingDate };

    EmailFolderList(QString mailbox, QObject *parent=0);
    ~EmailFolderList();

    void openMailbox();
    bool addMail(QMailMessage &mail);
    bool removeMail(QMailId id);
    bool moveMail(const QMailId& id, EmailFolderList& dest);
    bool copyMail(const QMailId& id, EmailFolderList& dest);
    bool empty(QMailMessage::MessageType type = QMailMessage::AnyType);

    bool moveMailList(const QMailIdList& list, EmailFolderList& dest);

    QString mailbox() const;
    QMailFolder mailFolder() const;

    bool contains(const QMailId& id) const;

    QMailIdList messages(QMailMessage::MessageType type = QMailMessage::AnyType, 
                         const SortOrder& order = Submission ) const;

    QMailIdList messages(QMailMessage::Status status, 
                         bool contains,
                         QMailMessage::MessageType type = QMailMessage::AnyType,
                         const SortOrder& order = Submission ) const;

    QMailIdList messagesFromMailbox(const Mailbox& mailbox, 
                                    QMailMessage::MessageType type = QMailMessage::AnyType,
                                    const SortOrder& order = Submission ) const;

    QMailIdList messagesFromAccount(const QMailAccount& account, 
                                    QMailMessage::MessageType type = QMailMessage::AnyType,
                                    const SortOrder& order = Submission ) const;

    uint messageCount( MailType status, QMailMessage::MessageType type = QMailMessage::AnyType ) const;
    uint messageCount( MailType status, QMailMessage::MessageType type, const QMailAccount& account ) const;
    uint messageCount( MailType status, QMailMessage::MessageType type, const Mailbox& mailbox, bool subfolders ) const;

signals:
    void externalEdit(const QString &);
    void mailAdded(const QMailId& id, const QString &);
    void mailUpdated(const QMailId& id, const QString &);
    void mailRemoved(const QMailId& id, const QString &);
    void mailMoved(const QMailId& id, const QString&, const QString&);
    void mailMoved(const QMailIdList& list, const QString&, const QString&);
    void stringStatus(QString &);

protected slots:
    void externalChange();

private:
    QMailIdList messages(QMailMessageKey queryKey, const SortOrder& order) const;
    uint messageCount(QMailMessageKey queryKey) const;

    QString mMailbox;
    QMailFolder mFolder;
    QMailMessageKey mParentFolderKey;
};

class QTOPIAMAIL_EXPORT MailboxList : public QObject
{
    Q_OBJECT

public:
    MailboxList(QObject *parent = 0);
    ~MailboxList();

    void openMailboxes();

    QStringList mailboxes() const;
    EmailFolderList* mailbox(const QString &name) const;
    EmailFolderList* mailbox(const QMailId& mailFolderId) const;

    EmailFolderList* owner(const QMailId& id) const;

    // Identifier strings not visible to user
    static const char* InboxString;
    static const char* OutboxString;
    static const char* DraftsString;
    static const char* SentString;
    static const char* TrashString;
    static const char* LastSearchString;
    static const char* EmailString;

    // Get the translated name for a mailbox
    static QString mailboxTrName( const QString &s );

    // Get the translated header text for a mailbox
    static QString mailboxTrHeader( const QString &s );

    // Get an icon for mailbox.
    static QIcon mailboxIcon( const QString &s );

    static QMailIdList messages(QMailMessage::MessageType type = QMailMessage::AnyType, 
                                const EmailFolderList::SortOrder& order = EmailFolderList::Submission);

    static QMailIdList messages(QMailMessage::Status status, 
                                bool contains,
                                QMailMessage::MessageType type = QMailMessage::AnyType,
                                const EmailFolderList::SortOrder& order = EmailFolderList::Submission);

    static QMailIdList messagesFromMailbox(const Mailbox& mailbox, 
                                           QMailMessage::MessageType type = QMailMessage::AnyType,
                                           const EmailFolderList::SortOrder& order = EmailFolderList::Submission);

    static QMailIdList messagesFromAccount(const QMailAccount& account, 
                                           QMailMessage::MessageType type = QMailMessage::AnyType,
                                           const EmailFolderList::SortOrder& order = EmailFolderList::Submission);

    static QMailIdList messages(QMailMessageKey queryKey, const EmailFolderList::SortOrder& order);

    static uint messageCount( EmailFolderList::MailType status, QMailMessage::MessageType type = QMailMessage::AnyType );
    static uint messageCount( EmailFolderList::MailType status, QMailMessage::MessageType type, const QMailAccount& account );
    static uint messageCount( EmailFolderList::MailType status, QMailMessage::MessageType type, const Mailbox& mailbox, bool subfolders );

    static uint messageCount( QMailMessageKey queryKey );

    static QMailMessageKey statusFilterKey(EmailFolderList::MailType status);
    static QMailMessageKey statusFilterKey(QMailMessage::Status status, bool contains);
    static QMailMessageKey messageFilterKey(QMailMessage::MessageType type, const QString& account = QString(), const QString& mailbox = QString(), bool subfolders = false);

signals:
    void externalEdit(const QString &);
    void mailAdded(const QMailId& id, const QString &);
    void mailUpdated(const QMailId& id, const QString &);
    void mailRemoved(const QMailId &, const QString &);
    void mailMoved(const QMailId& id, const QString&, const QString&);
    void mailMoved(const QMailIdList& list, const QString&, const QString&);
    void stringStatus(QString &);

private:
    QList<EmailFolderList*> _mailboxes;
};

#endif
