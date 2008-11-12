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

#include <qmessagebox.h>
#include <qstringlist.h>
#include <qtopiaapplication.h>
#include <sys/vfs.h>

#include "emailfolderlist.h"
#include "accountlist.h"
#include <QMailFolderKey>
#include <QMailStore>

const char* MailboxList::InboxString = "inbox_ident";     // No tr
const char* MailboxList::OutboxString = "outbox_ident"; // No tr
const char* MailboxList::DraftsString = "drafts_ident"; // No tr
const char* MailboxList::SentString = "sent_ident"; // No tr
const char* MailboxList::TrashString = "trash_ident"; // No tr
const char* MailboxList::LastSearchString = "last_search_ident"; // No tr
const char* MailboxList::EmailString = "email_ident"; // No tr

// Stores the translated names of mailbox properties
struct MailboxProperties
{
    MailboxProperties( const char* nameSource, const char* headerSource, const char *iconSource ) :
        name( qApp->translate( "QtMail", nameSource ) ),
        header( QObject::tr( headerSource ) ),
        icon( iconSource )
    {
    }

    QString name;
    QString header;
    const char *icon;
};

typedef QHash<QString, MailboxProperties> MailboxGroup;

// Returns a map containing translated properties for well-known mailboxes
static MailboxGroup initMailboxTr()
{
    MailboxGroup map;

    // Translated properties are stored in a map (perhaps a hash would be 
    // preferable?)
    // Heap storage is required, but lookup will be quick, and translation 
    // occurs only once
    map.insert( MailboxList::InboxString, 
                MailboxProperties( QT_TRANSLATE_NOOP( "QtMail", "Inbox"), 
                                   QT_TRANSLATE_NOOP( "QtMail", "From"),
                                   ":icon/inbox") );
    map.insert( MailboxList::SentString, 
                MailboxProperties( QT_TRANSLATE_NOOP( "QtMail", "Sent"), 
                                   QT_TRANSLATE_NOOP( "QtMail", "To"),
                                   ":icon/sent") );
    map.insert( MailboxList::DraftsString, 
                MailboxProperties( QT_TRANSLATE_NOOP( "QtMail", "Drafts"), 
                                   QT_TRANSLATE_NOOP( "QtMail", "To"),
                                   ":icon/drafts") );
    map.insert( MailboxList::TrashString, 
                MailboxProperties( QT_TRANSLATE_NOOP( "QtMail", "Trash"), 
                                   QT_TRANSLATE_NOOP( "QtMail", "From/To"),
                                   ":icon/trash") );
    map.insert( MailboxList::OutboxString, 
                MailboxProperties( QT_TRANSLATE_NOOP( "QtMail", "Outbox"), 
                                   QT_TRANSLATE_NOOP( "QtMail", "To"),
                                   ":icon/outbox") );
    map.insert( MailboxList::LastSearchString, 
                MailboxProperties( QT_TRANSLATE_NOOP( "QtMail", "Last search"),
                                   "",
                                   ":icon/find") );
    map.insert( MailboxList::EmailString, 
                MailboxProperties( QT_TRANSLATE_NOOP( "QtMail", "Email"),
                                   "",
                                   ":icon/email") );

    return map;
}

static const MailboxProperties* mailboxProperties( const QString &s )
{
    static MailboxGroup map( initMailboxTr() );

    // Search for the properties of the named mailbox
    MailboxGroup::const_iterator i = map.find(s);
    if ( i != map.end() )
        return &(i.value());

    return 0;
}


/*  Mailbox List    */
MailboxList::MailboxList(QObject *parent)
    : QObject(parent )
{
    EmailFolderList *mailbox = new EmailFolderList(InboxString, this);
    connect(mailbox, SIGNAL(stringStatus(QString&)), this,
            SIGNAL(stringStatus(QString&)) );
    _mailboxes.append( mailbox );

    mailbox = new EmailFolderList(SentString, this);
    connect(mailbox, SIGNAL(stringStatus(QString&)), this,
            SIGNAL(stringStatus(QString&)) );
    _mailboxes.append( mailbox );

    mailbox = new EmailFolderList(DraftsString, this);
    connect(mailbox, SIGNAL(stringStatus(QString&)), this,
            SIGNAL(stringStatus(QString&)) );
    _mailboxes.append( mailbox );

    mailbox = new EmailFolderList(TrashString, this);
    connect(mailbox, SIGNAL(stringStatus(QString&)), this,
            SIGNAL(stringStatus(QString&)) );
    _mailboxes.append( mailbox );

    mailbox = new EmailFolderList(OutboxString, this);
    connect(mailbox, SIGNAL(stringStatus(QString&)), this,
            SIGNAL(stringStatus(QString&)) );
    _mailboxes.append( mailbox );
}

MailboxList::~MailboxList()
{
}

void MailboxList::openMailboxes()
{
    QListIterator<EmailFolderList*> it(_mailboxes);
    while (it.hasNext()) {
        EmailFolderList* current = it.next();
        current->openMailbox();

        //connect after mail has been read to speed up reading */
        connect(current, SIGNAL(mailAdded(QMailId,QString)), this,
                SIGNAL(mailAdded(QMailId,QString)) );
        connect(current, SIGNAL(mailUpdated(QMailId,QString)), this,
                SIGNAL(mailUpdated(QMailId,QString)) );
        connect(current, SIGNAL(mailRemoved(QMailId,QString)), this,
                SIGNAL(mailRemoved(QMailId,QString)) );
        connect(current, SIGNAL(externalEdit(QString)), this,
                SIGNAL(externalEdit(QString)) );
        connect(current, SIGNAL(mailMoved(QMailId,QString,QString)),this,
                SIGNAL(mailMoved(QMailId,QString,QString)));
        connect(current, SIGNAL(mailMoved(QMailIdList,QString,QString)),this,
                SIGNAL(mailMoved(QMailIdList,QString,QString)));
    }
}

QStringList MailboxList::mailboxes() const
{
    QStringList list;

    QListIterator<EmailFolderList*> it(_mailboxes);
    while (it.hasNext()) {
        list.append( it.next()->mailbox() );
    }

    return list;
}

EmailFolderList* MailboxList::mailbox(const QString &name) const
{
    QListIterator<EmailFolderList*> it(_mailboxes);
    while (it.hasNext()) {
        EmailFolderList* current = it.next();
        if ( current->mailbox() == name )
            return current;
    }

    return NULL;
}

EmailFolderList* MailboxList::mailbox(const QMailId& mailFolderId) const
{
    QListIterator<EmailFolderList*> it(_mailboxes);

    foreach(EmailFolderList* f, _mailboxes)
    {
        if(f->mailFolder().id() == mailFolderId)
            return f;
    }
    
    return NULL;

}

EmailFolderList* MailboxList::owner(const QMailId& id) const
{
    QListIterator<EmailFolderList*> it(_mailboxes);
    while (it.hasNext()) {
        EmailFolderList* folder = it.next();
        if (folder->contains(id))
            return folder;
    }

    return 0;
}

QString MailboxList::mailboxTrName( const QString &s )
{
    // Find the properties of the named mailbox
    const MailboxProperties* properties( mailboxProperties(s) );
    if (properties)
        return properties->name;

    // Return the untranslated name
    return s;
}

QString MailboxList::mailboxTrHeader( const QString &s )
{
    // Find the properties of the named mailbox
    const MailboxProperties* properties( mailboxProperties(s) );
    if (properties)
        return properties->header;

    // No header is defined, indicate with an empty string
    return QString("");
}

QIcon MailboxList::mailboxIcon( const QString &s )
{
    QMap<QString,QIcon> cache;
    QMap<QString,QIcon>::const_iterator it = cache.find(s);
    if (it != cache.end())
        return (*it);

    // Find the properties of the named mailbox
    const MailboxProperties* properties( mailboxProperties(s) );
    if (properties) {
        QIcon icon(properties->icon);
        cache[s] = icon;
        return icon;
    }

    return QIcon(":icon/folder");
}

QMailIdList MailboxList::messages(QMailMessage::MessageType messageType, const EmailFolderList::SortOrder& order )
{
    return messages(messageFilterKey(messageType), order);
}

QMailIdList MailboxList::messages(QMailMessage::Status status, 
                                  bool contains,
                                  QMailMessage::MessageType messageType, 
                                  const EmailFolderList::SortOrder& order)
{
    return messages(statusFilterKey(status, contains) & messageFilterKey(messageType), order);
}

QMailIdList MailboxList::messagesFromMailbox(const Mailbox& mailbox, 
                                             QMailMessage::MessageType messageType, 
                                             const EmailFolderList::SortOrder& order)
{
    return messages(messageFilterKey(messageType, mailbox.account()->id(), mailbox.pathName()), order);
}

QMailIdList MailboxList::messagesFromAccount(const QMailAccount& account,
                                             QMailMessage::MessageType messageType,
                                             const EmailFolderList::SortOrder& order)
{
    return messages(messageFilterKey(messageType, account.id()), order);
}

QMailIdList MailboxList::messages(QMailMessageKey queryKey, const EmailFolderList::SortOrder& order)
{
    if (order != EmailFolderList::Submission) {
        Qt::SortOrder srtOrder(order == EmailFolderList::DescendingDate ? Qt::DescendingOrder : Qt::AscendingOrder);
        QMailMessageSortKey sortKey(QMailMessageSortKey::TimeStamp, srtOrder);
        return QMailStore::instance()->queryMessages(queryKey, sortKey);
    } else {
        return QMailStore::instance()->queryMessages(queryKey);
    }
}

uint MailboxList::messageCount( EmailFolderList::MailType status, QMailMessage::MessageType type )
{
    return messageCount(statusFilterKey(status) & messageFilterKey(type));
}

uint MailboxList::messageCount( EmailFolderList::MailType status, QMailMessage::MessageType type, const QMailAccount& account )
{
    return messageCount(statusFilterKey(status) & messageFilterKey(type, account.id()));
}

uint MailboxList::messageCount( EmailFolderList::MailType status, QMailMessage::MessageType type, const Mailbox& mailbox, bool subfolders )
{
    return messageCount(statusFilterKey(status) & messageFilterKey(type, mailbox.account()->id(), mailbox.pathName(), subfolders));
}

uint MailboxList::messageCount(QMailMessageKey queryKey)
{
    return QMailStore::instance()->countMessages(queryKey);
}

QMailMessageKey MailboxList::statusFilterKey( EmailFolderList::MailType status )
{
    QMailMessageKey statusKey;

    if (status == EmailFolderList::Unfinished) {
        // Currently unhandled!
    } else if (status == EmailFolderList::Unread) {
        // Ensure 'read' and 'read elsewhere' flags are not set
        statusKey = (~QMailMessageKey(QMailMessageKey::Status, QMailMessage::Read, QMailMessageKey::Contains) &
                     ~QMailMessageKey(QMailMessageKey::Status, QMailMessage::ReadElsewhere, QMailMessageKey::Contains));
    } else if (status == EmailFolderList::Unsent) {
        // Ensure 'sent' flag is not set
        statusKey = ~QMailMessageKey(QMailMessageKey::Status, QMailMessage::Sent, QMailMessageKey::Contains);
    }

    return statusKey;
}

QMailMessageKey MailboxList::statusFilterKey( QMailMessage::Status status, bool contains )
{
    QMailMessageKey statusKey(QMailMessageKey::Status, static_cast<int>(status), QMailMessageKey::Contains);
    return (contains ? statusKey : ~statusKey);
}

QMailMessageKey MailboxList::messageFilterKey( QMailMessage::MessageType type, const QString& account, const QString& mailbox, bool subfolders )
{
    QMailMessageKey filter;
    
    if (type != QMailMessage::AnyType)
        filter = QMailMessageKey(QMailMessageKey::Type, type, QMailMessageKey::Contains);

    if (!account.isNull()) {
        filter &= QMailMessageKey(QMailMessageKey::FromAccount, account);

        if (!mailbox.isNull()) {
            filter &= QMailMessageKey(QMailMessageKey::FromMailbox, mailbox, (subfolders ? QMailMessageKey::Contains : QMailMessageKey::Equal));
        }
    }

    return filter;
}

//===========================================================================

/*  Email Folder List */
EmailFolderList::EmailFolderList(QString mailbox, QObject *parent)
    : QObject(parent),
    mMailbox(mailbox),
    mFolder(mailbox),
    mParentFolderKey(QMailMessageKey::ParentFolderId,mFolder.id())
{
}

EmailFolderList::~EmailFolderList()
{
}

void EmailFolderList::openMailbox()
{
    QString mbox = MailboxList::mailboxTrName(mMailbox);

    QMailFolderKey key(QMailFolderKey::Name,mMailbox);
    key &= QMailFolderKey(QMailFolderKey::ParentId,QMailId());
    
    QMailIdList folderIdList = QMailStore::instance()->queryFolders(key);
    
    if(folderIdList.isEmpty()) //create folder
    {
        QMailFolder newFolder(mMailbox);
        if(!QMailStore::instance()->addFolder(&newFolder))
			qWarning() << "Failed to add folder " << mMailbox;
        mFolder = newFolder;
    }
    else //load folder
    {
        QMailId folderId = folderIdList.first();
        mFolder = QMailFolder(folderId);
    }

    //set the folder key   
    mParentFolderKey = QMailMessageKey(QMailMessageKey::ParentFolderId,mFolder.id());
}

bool EmailFolderList::addMail(QMailMessage &m)
{
    if(m.id().isValid()){
        m.setParentFolderId(mFolder.id());
        if(!QMailStore::instance()->updateMessage(&m))
            return false;
        emit mailUpdated( m.id(), mailbox() );
    }
    else 
    {
        m.setParentFolderId(mFolder.id());
        if(!QMailStore::instance()->addMessage(&m))
            return false;
        emit mailAdded(m.id(),mailbox());
    }

    return true;
}

bool EmailFolderList::removeMail(QMailId id)
{
    if(!QMailStore::instance()->removeMessage(id))
        return false;

    emit mailRemoved(id, mailbox() );

    return true;
}

bool EmailFolderList::empty(QMailMessage::MessageType type)
{
    foreach(QMailId id, messages(type))
        QMailStore::instance()->removeMessage(id);

    return true;
}

bool EmailFolderList::contains(const QMailId& id) const
{
    return ( messageCount(QMailMessageKey(QMailMessageKey::Id, id)) > 0 );
}

QMailIdList EmailFolderList::messages(QMailMessage::MessageType messageType, const SortOrder& order ) const
{
    return messages(MailboxList::messageFilterKey(messageType), order);
}

QMailIdList EmailFolderList::messages(QMailMessage::Status status, 
                                      bool contains,
                                      QMailMessage::MessageType messageType, 
                                      const SortOrder& order) const
{
    return messages(MailboxList::statusFilterKey(status, contains) & MailboxList::messageFilterKey(messageType), order);
}

QMailIdList EmailFolderList::messagesFromMailbox(const Mailbox& mailbox, 
                                                 QMailMessage::MessageType messageType, 
                                                 const SortOrder& order ) const
{
    return messages(MailboxList::messageFilterKey(messageType, mailbox.account()->id(), mailbox.pathName()), order);
}

QMailIdList EmailFolderList::messagesFromAccount(const QMailAccount& account,
                                                 QMailMessage::MessageType messageType,
                                                 const SortOrder& order) const
{
    return messages(MailboxList::messageFilterKey(messageType, account.id()), order);
}

QMailIdList EmailFolderList::messages(QMailMessageKey queryKey, const SortOrder& order) const
{
    return MailboxList::messages(queryKey & mParentFolderKey, order);
}

uint EmailFolderList::messageCount( MailType status, QMailMessage::MessageType type ) const
{
    return messageCount(MailboxList::statusFilterKey(status) & MailboxList::messageFilterKey(type));
}

uint EmailFolderList::messageCount( MailType status, QMailMessage::MessageType type, const QMailAccount& account ) const
{
    return messageCount(MailboxList::statusFilterKey(status) & MailboxList::messageFilterKey(type, account.id()));
}

uint EmailFolderList::messageCount( MailType status, QMailMessage::MessageType type, const Mailbox& mailbox, bool subfolders ) const
{
    return messageCount(MailboxList::statusFilterKey(status) & MailboxList::messageFilterKey(type, mailbox.account()->id(), mailbox.pathName(), subfolders));
}

uint EmailFolderList::messageCount( QMailMessageKey queryKey ) const
{
    return MailboxList::messageCount(queryKey & mParentFolderKey);
}

void EmailFolderList::externalChange()
{
    emit externalEdit( mailbox() );
}

bool EmailFolderList::moveMail(const QMailId& id, EmailFolderList& dest)
{
    QMailFolder otherFolder = dest.mFolder;
    QMailMessage m(id,QMailMessage::Header);

    if(m.parentFolderId() != mFolder.id())
    {
        qWarning() << "Cannot move mail that does not exist in folder " << mMailbox;
        return false;
    }

    m.setParentFolderId(otherFolder.id());
        
    if(!QMailStore::instance()->updateMessage(&m))
        return false;

    emit mailMoved(id,mailbox(),dest.mailbox());

    return true;
}

bool EmailFolderList::copyMail(const QMailId& id, EmailFolderList& dest)
{
    QMailFolder otherFolder = dest.mFolder;
    QMailMessage m(id,QMailMessage::HeaderAndBody);

    if(m.parentFolderId() != mFolder.id())
    {
        qWarning() << "Cannot move mail that does not exist in folder " << mMailbox;
        return false;
    }

    m.setId(QMailId()); //reset id
    m.setParentFolderId(otherFolder.id());

    if(!QMailStore::instance()->addMessage(&m))
        return false;

    emit mailAdded(m.id(),dest.mailbox());

    return true;
}

QString EmailFolderList::mailbox() const
{
    return mMailbox;
}

QMailFolder EmailFolderList::mailFolder() const
{
    return mFolder;
}

bool EmailFolderList::moveMailList(const QMailIdList& list, EmailFolderList& dest)
{

    // Check that these messages belong to our folder
    /*QMailIdList folderIds(QMailStore::instance()->parentFolderIds(list));

    if ((folderIds.count() != 1) || (folderIds.first() != mFolder.id())) {
        qWarning() << "Cannot move mail that does not exist in folder " << mMailbox;
        return false;
    }*/

    QMailMessageList headers = QMailStore::instance()->messageHeaders(QMailMessageKey(list),
                                                                      QMailMessageKey::ParentFolderId,
                                                                      QMailStore::ReturnDistinct);


    if((headers.count() != 1) || (headers.first().parentFolderId() != mFolder.id()))
    {
        qWarning() << "Cannot move mail that does not exist in folder " << mMailbox;
        return false;
    }

/*    if (!QMailStore::instance()->updateParentFolderIds(list, dest.mFolder.id()))
        return false; */
    QMailMessage updateHeader;
    updateHeader.setParentFolderId(dest.mFolder.id());

    if(!QMailStore::instance()->updateMessages(QMailMessageKey(list),QMailMessageKey::ParentFolderId,updateHeader))
        return false;

    emit mailMoved(list, mailbox(), dest.mailbox());

    return true;
}

