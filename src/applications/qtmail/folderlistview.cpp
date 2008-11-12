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


#include "folderlistview.h"

#include <qtopiaapplication.h>
#include <qmailtimestamp.h>

#include "searchview.h"
#include "accountlist.h"

#include <qmessagebox.h>
#include <qcursor.h>
#include <qpainter.h>
#include <qdesktopwidget.h>
#include <qevent.h>
#include <qscrollbar.h>
#include <QHeaderView>

static QPixmap* pm_normal = 0;
static QPixmap* pm_unread = 0;
static QPixmap* pm_unsent = 0;

static void ensurePixmaps()
{
    if ( !pm_normal ) {
	// These should be replaced once new icons are available
        pm_normal = new QPixmap(":image/flag_normal");
        pm_unread = new QPixmap(":image/flag_unread");
        pm_unsent = new QPixmap(":image/flag_tosend");
    }
}

FolderListItem::FolderListItem(QTreeWidget *parent, Folder *in)
    : QTreeWidgetItem(parent),
      _folder(in),
      _highlight(false)
{
    init();
}

FolderListItem::FolderListItem(QTreeWidget *parent, QTreeWidgetItem* predecessor, Folder *in)
    : QTreeWidgetItem(parent, predecessor),
      _folder(in),
      _highlight(false)
{
    init();
}

FolderListItem::FolderListItem(QTreeWidgetItem *parent, Folder *in)
    : QTreeWidgetItem(parent),
      _folder(in),
      _highlight(false)
{
    init();
}

FolderListItem::FolderListItem(QTreeWidgetItem *parent, QTreeWidgetItem* predecessor, Folder *in)
    : QTreeWidgetItem(parent, predecessor),
      _folder(in),
      _highlight(false)
{
    init();
}

void FolderListItem::init()
{
    QIcon icon;
    if (_folder->folderType() == FolderTypeSystem) {
        icon = MailboxList::mailboxIcon(_folder->mailbox());
    } else if (_folder->folderType() == FolderTypeAccount) {
        icon = QIcon(":icon/account");
    } else if (_folder->folderType() == FolderTypeMailbox) {
        icon = QIcon(":icon/folder");
    }

    setName( _folder->displayName() );
    int extent = qApp->style()->pixelMetric(QStyle::PM_SmallIconSize);
    setIcon( 0, icon.pixmap(extent));
}

Folder* FolderListItem::folder() const
{
    return _folder;
}

void FolderListItem::setName(const QString& name)
{
    setText(0, name);
}

QString FolderListItem::name() const 
{
    return text(0);
}

void FolderListItem::setStatusText( const QString &str, bool highlight, IconType type )
{
    _statusText = str;
    _highlight = highlight;
    _type = type;
}

void FolderListItem::statusText( QString *str, bool *highlight, IconType *type )
{
    *str = _statusText;
    *highlight = _highlight;
    *type = _type;

    // Don't show the excess indicators if this item is expanded
    if (isExpanded())
        str->remove(FolderListView::excessIndicator());
}

int FolderListItem::depth() const
{
    int count = 0;
    const QTreeWidgetItem *item = this;
    while (item->parent()) {
        ++count;
        item = item->parent();
    }
    return count;
}

/*  We want a particular layout, use sort to enforce it  */
QString FolderListItem::key(int c, bool) const
{
  if ( c != 0 )
    return QString();

  int type = _folder->folderType();
  switch( type ) {
    case FolderTypeSystem:
    {
        // system search folder(last search) has a mailbox equal to one of the other real mailboxes,
        // but we still want it to appear last though.
      if ( static_cast<SystemFolder *>(_folder)->isSearch() )
        return ( "77" );

      QChar i = '7';
      QString s = _folder->mailbox();
      if ( s == MailboxList::InboxString)
        i = '1';
      else if ( s == MailboxList::OutboxString )
        i = '2';
      else if ( s == MailboxList::DraftsString )
        i = '3';
      else if ( s == MailboxList::SentString )
        i = '4';
      else if ( s == MailboxList::TrashString )
        i = '5';

      return QString::number( type ) + i;
    }
    case FolderTypeAccount:
    {
      return "2" + name();
    }
    case FolderTypeMailbox:
    {
      return "3" + name();
    }
    default:    //folderTypeSearch
    {
      return "4" + name();
    }
  }
}

/* Folder list view  */

FolderListView::FolderListView(MailboxList *list, QWidget *parent, const char *name)
    : QTreeWidget( parent )
{
    setObjectName( name );
    setFrameStyle( NoFrame );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    sortItems(0, Qt::AscendingOrder );

    _mailboxList = list;

    FolderListItemDelegate *delegate = new FolderListItemDelegate( this );
    setItemDelegate( delegate );

    connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(folderChanged(QTreeWidgetItem*)) );
    header()->hide();
    connect( this, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
             this, SLOT(itemClicked(QTreeWidgetItem*)) );
}

FolderListView::~FolderListView()
{
}


Folder* FolderListView::currentFolder() const
{
    FolderListItem *item = currentFolderItem();
    if ( item != NULL )
        return item->folder();

    return NULL;
}

bool FolderListView::setCurrentFolder(const Folder* folder)
{
    QModelIndex index = model()->index( 0, 0 );
    for ( ; index.isValid(); index = next( index ) ) {
        FolderListItem *item = folderItemFromIndex( index );
        if ( item->folder() == folder ) {
            setCurrentItem( item );
            return true;
        }
    }

    return false;
}

Folder* FolderListView::systemFolder(const QString& identifier) const
{
    QModelIndex index(systemFolderIndex(identifier));
    if (index.isValid())
        return folderItemFromIndex( index )->folder();
    else if (systemFolders.contains(identifier))
        return systemFolders[identifier];

    return 0;
}

QModelIndex FolderListView::systemFolderIndex(const QString& identifier) const
{
    bool search = (identifier == MailboxList::LastSearchString);

    QModelIndex index = model()->index( 0, 0 );
    for ( ; index.isValid(); index = next( index ) ) {
        Folder* folder = folderItemFromIndex( index )->folder();
        if ( folder->folderType() == FolderTypeSystem ) {
            if ( (search && static_cast<SystemFolder*>(folder)->isSearch()) ||
                 (folder->mailbox() == identifier) ) {
                break;
            }
        }
    }

    return index;
}

Folder* FolderListView::accountFolder(const Folder* account, const QString& mailbox) const
{
    QModelIndex index(accountFolderIndex(account, mailbox));
    if (index.isValid())
        return folderItemFromIndex( index )->folder();

    return 0;
}

const QTreeWidgetItem* FolderListView::accountFolderItem(const Folder* account, const QString& mailbox) const
{
    QModelIndex index(accountFolderIndex(account, mailbox));
    if (index.isValid())
        return itemFromIndex( index );

    return 0;
}

static QMailAccount* itemAccount(const FolderListItem* item)
{
    QMailAccount *account = 0;

    while (item &&
           item->folder() &&
           item->folder()->folderType() != FolderTypeAccount)
        item = static_cast<FolderListItem*>(item->parent());

    if (item &&
        item->folder() &&
        item->folder()->folderType() == FolderTypeAccount)
        account = static_cast<QMailAccount*>(item->folder());

    return account;
}

QModelIndex FolderListView::accountFolderIndex(const Folder* account, const QString& mailbox) const
{
    QModelIndex index = model()->index( 0, 0 );
    for ( ; index.isValid(); index = next( index ) ) {
        FolderListItem* item = folderItemFromIndex( index );
        Folder* folder = item->folder();
        if ( ( mailbox.isNull() &&
               (folder == account) ) ||
             ( (folder->folderType() == FolderTypeMailbox) &&
               (static_cast<Mailbox*>(folder)->pathName() == mailbox) &&
               (itemAccount(item) == account) ) ) {
            break;
        }
    }

    return index;
}

QMailAccount* FolderListView::currentAccount() const
{
    return itemAccount(currentFolderItem());
}

void FolderListView::keyPressEvent( QKeyEvent *e )
{
    switch( e->key() ) {
        case Qt::Key_Select:
        case Qt::Key_Space:
        case Qt::Key_Return:
        case Qt::Key_Enter:
        {
            e->accept();
            emit viewMessageList();
        }
        break;

        case Qt::Key_Back:
        {
            e->accept();
            emit finished();
        }
        break;

        default:  QTreeWidget::keyPressEvent( e );
    }
}

QSize FolderListView::sizeHint() const
{
    return QSize(-1,-1);
}

QSize FolderListView::minimumSizeHint() const
{
    return QSize(-1,-1);
}

void FolderListView::setupFolders(AccountList *list)
{
    clear();

    if (systemFolders.isEmpty()) {
        // Create the persistent system folder entries
        foreach (const QString& mailbox, _mailboxList->mailboxes())
            systemFolders.insert(mailbox, new SystemFolder(SystemTypeMailbox, mailbox));
    }

    QList<QMailAccount*> accounts;
    QListIterator<QMailAccount*> itAccount = list->accountIterator();
    while (itAccount.hasNext()) {
        QMailAccount* accountInfo = itAccount.next();
        if (accountInfo->accountType() == QMailAccount::POP ||
            accountInfo->accountType() == QMailAccount::IMAP) {
            accounts.append(accountInfo);
        }
    }

    bool withInbox = (accounts.count() != 1);
    if (!withInbox) {
        // Add our single account folder first
        FolderListItem* accountItem = new FolderListItem(this, accounts.first());
        synchroniseFolderStructure(accountItem, accounts.first());
        setCurrentItem(accountItem);

        setItemExpanded(accountItem, true);
    }

    //build system folders
    QStringList mboxList = _mailboxList->mailboxes();
    for (QStringList::Iterator it = mboxList.begin(); it != mboxList.end(); ++it) {
        if (withInbox || (*it != MailboxList::InboxString))
            new FolderListItem(this, systemFolders[*it]);
    }

    if (withInbox) {
        //build inbox
        FolderListItem* inbox = static_cast<FolderListItem*>(topLevelItem(0));

        //build all other accounts
        QListIterator<QMailAccount*> itAccount = list->accountIterator();
        while (itAccount.hasNext()) {
            QMailAccount* accountInfo = itAccount.next();
            if (accountInfo->accountType() == QMailAccount::POP ||
                accountInfo->accountType() == QMailAccount::IMAP) {
                FolderListItem* accountItem = new FolderListItem(inbox, accountInfo);
                synchroniseFolderStructure(accountItem, accountInfo);
            }
        }

        setItemExpanded(inbox, true);
        setCurrentItem(inbox);
    }
}

QModelIndex FolderListView::next(QModelIndex mi, bool nextParent) const
{
    if (mi.child(0,0).isValid() && !nextParent)
      return mi.child(0,0);
    if (mi.sibling( mi.row() + 1, mi.column() ).isValid())
        return mi.sibling( mi.row() + 1, mi.column() );
    if (mi.parent().isValid())
         return next( mi.parent(),true);
    return QModelIndex(); // invalid e.g. correct
}

QTreeWidgetItem* FolderListView::next(QTreeWidgetItem *item) const
{
  QModelIndex m = indexFromItem(item);
  QModelIndex nextIndex = next(m);
  if(!nextIndex.isValid())
    return 0;
  else
    return itemFromIndex(nextIndex);
}

// receives an account, finds it in the listview and makes
// any necessary updates
void FolderListView::updateAccountFolder(QMailAccount *account)
{
    QModelIndex index(accountFolderIndex(account, QString()));
    if (index.isValid()) {
        FolderListItem *item = folderItemFromIndex(index);
        synchroniseFolderStructure(item, account);
        folderChanged( currentItem() );

        QModelIndex rootIndex = model()->index(0, 0);
        if ((rootIndex == index) && (item->childCount() != 0)) {
            // This account is the only one - expand if necessary
            setItemExpanded(item, true);
        }
    } else {
        // New account folder
        FolderListItem *item = 0;
        FolderListItem *inbox = folderItemFromIndex(model()->index(0, 0));
        if (inbox->folder()->folderType() == FolderTypeAccount) {
            // There is only a single account currently, in place of the inbox
            QMailAccount* firstAccount = static_cast<QMailAccount*>(inbox->folder());

            // Delete the existing folder
            delete inbox;

            // Add the inbox to precede the existing folders
            new FolderListItem(this, 0, systemFolders[MailboxList::InboxString]);
            inbox = folderItemFromIndex(model()->index(0, 0));

            // Add the previous folder back to the inbox
            item = new FolderListItem(inbox, firstAccount);
            synchroniseFolderStructure(item, firstAccount);

            setItemExpanded(inbox, true);
            item = 0;
        } else {
            // If this is the first account, remove the empty inbox
            if (inbox->childCount() == 0) {
                delete inbox;

                // Add the new account to precede the existing folders
                item = new FolderListItem(this, 0, account);
            }
        }

        if (!item)
            item = new FolderListItem(inbox, account);

        synchroniseFolderStructure(item, account);

        emit folderModified(inbox->folder());
        folderChanged(inbox);
    }
}

void FolderListView::deleteAccountFolder(QMailAccount *account)
{
    Folder *folder;

    QModelIndex rootindex = model()->index( 0, 0 );
    QModelIndex index;

    folder = folderItemFromIndex( rootindex )->folder();
    if ( (folder->folderType() == FolderTypeAccount) && (folder == account) ) {
        index = rootindex;
    } else {
        index = next(rootindex);
        if(index == rootindex)
            return;

        for ( ; index.isValid(); index = next( index ) ) {
            folder = folderItemFromIndex( index )->folder();
            if ( (folder->folderType() == FolderTypeAccount) && (folder == account) )
                break;
        }
    }

    if (folder == account) {
        delete itemFromIndex( index );

        if (index == rootindex) {
            // We deleted the top-level single account - restore the inbox in its place
            new FolderListItem(this, 0, systemFolders[MailboxList::InboxString]);
        } else {
            FolderListItem *inbox = folderItemFromIndex(model()->index(0, 0));
            if (inbox->childCount() == 1) {
                // We only have a single child left - replace the inbox
                QTreeWidgetItem *remainingAccount = inbox->takeChild(0);
                
                insertTopLevelItem(0, remainingAccount);
                setItemExpanded(remainingAccount, true);
                delete inbox;
            }
        }

        emit folderModified( folderItemFromIndex( model()->index(0, 0) )->folder() );
        folderChanged( currentItem() );
    }
}

void FolderListView::changeToSystemFolder(const QString &identifier)
{
    QModelIndex index(systemFolderIndex(identifier));
    if (index.isValid()) {
        setCurrentItem(itemFromIndex(index));
    } else if (identifier == MailboxList::InboxString) {
        // There may be no inbox present, if there is a single account
        setCurrentItem(itemFromIndex(model()->index(0,0)));
    }
}

void FolderListView::updateFolderStatus(const QString &mailbox, const QString &txt, bool highlight, IconType type)
{
    QModelIndex index(systemFolderIndex(mailbox));
    if (index.isValid()) {
        FolderListItem *item = folderItemFromIndex( index );
        item->setStatusText( txt, highlight, type );

        dataChanged( index, index );
    }
}

void FolderListView::updateAccountStatus(const Folder *account, const QString &txt, bool highlight, IconType type, const QString& mailbox)
{
    QModelIndex index(accountFolderIndex(account, mailbox));
    if (index.isValid()) {
        FolderListItem *item = folderItemFromIndex( index );
        item->setStatusText( txt, highlight, type );

        dataChanged( index, index );
    }
}

void FolderListView::folderChanged(QTreeWidgetItem *item)
{
    QString str;

    if ( item == NULL ) {
        emit folderSelected( reinterpret_cast<Folder *>( NULL ) );
        return;
    }

    emit folderSelected( static_cast<FolderListItem *>(item)->folder() );
}

static QString folderPath(const QTreeWidgetItem* item, const FolderListItem* root, const QString& delimiter)
{
    QString path = item->text(0);

    item = item->parent();
    while (item && (item != root)) {
        path.prepend(delimiter).prepend(item->text(0));
        item = item->parent();
    }

    return path;
}

static bool mailboxExists(QTreeWidgetItem* item, const FolderListItem* root, const QString& delimiter,
                          QList<QList<Mailbox*>::const_iterator>& existing)
{
    QString mailboxName = folderPath(item, root, delimiter);

    QList<QList<Mailbox*>::const_iterator>::iterator it = existing.begin(), end = existing.end();
    for ( ; it != end; ++it) {
        Mailbox* box = *(*it);
        if (box->pathName() == mailboxName) {
            // This mail box is in the list of extant mailboxes
            existing.erase(it);
            return true;
        }
    }

    return false;
}

static bool verifyChildren(QTreeWidgetItem* item, const FolderListItem* root, const QString& delimiter,
                           QList<QList<Mailbox*>::const_iterator>& existing)
{
    QList<QTreeWidgetItem*> nonexistent;

    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem* child = item->child(i);

        if (verifyChildren(child, root, delimiter, existing) == false)
            nonexistent.prepend(child);
    }

    foreach (QTreeWidgetItem* child, nonexistent) {
        item->removeChild(child);
        delete child;
    }

    if (item == root)
        return true;

    return mailboxExists(item, root, delimiter, existing);
}

void FolderListView::synchroniseFolderStructure(FolderListItem *item, QMailAccount* account)
{
    Q_ASSERT(item);
    Q_ASSERT(account);

    QString delimiter;
    QList<QList<Mailbox*>::const_iterator> mailboxes;

    const QList<Mailbox*>& boxes(account->mailboxes());
    if (!boxes.isEmpty()) {
        // Create a list of the mailboxes in this account
        QList<Mailbox*>::const_iterator it = boxes.begin(), end = boxes.end();
        delimiter = (*it)->getDelimiter();

        for ( ; it != end; ++it)
            mailboxes.append(it);
    }

    // If we have only a single mailbox, do not present any folder hierarchy
    if (mailboxes.count() == 1)
        mailboxes.clear();

    // Remove any mailboxes no longer present in the tree
    verifyChildren(item, item, delimiter, mailboxes);

    // Add any new mailboxes not in the tree
    foreach (QList<Mailbox*>::const_iterator it, mailboxes) {
        Mailbox* box(*it);
        if ( !box->isDeleted() && box->localCopy()) {
            FolderListItem* parent = getParent(item, box->pathName(), box->getDelimiter());
            QTreeWidgetItem* predecessor = getPredecessor(parent, box->displayName());
            new FolderListItem(parent, predecessor, box);
        }
    }

    // Check whether the account name has changed
    if (account->displayName() != item->name())
        item->setName(account->displayName());
}

static QString parentFolderName(const QString& path, const QString& delimiter)
{
    QStringList list = path.split(delimiter);
    if (list.isEmpty())
        return QString();

    list.removeLast();
    return list.join(delimiter);
}

/*  Returns the parent mailbox/account in the treewidget of the given account.
        All mailboxes are sorted by name before added, so the conversion from
        flat to tree should not produce inccorect results.
*/
FolderListItem* FolderListView::getParent(FolderListItem *parent, QString name, QString delimiter)
{
    QString target = parentFolderName(name, delimiter);
    if (target.isEmpty())
        return parent;

    int level = parent->depth();
    Folder *folder;
    QModelIndex index = indexFromItem(parent); //parent index
    index = next(index); //get first child

    for ( ; index.isValid(); index = next( index ) ) {
        FolderListItem *item = folderItemFromIndex( index );
        if ( item->depth() <= level )
            return parent;              //failed

        folder = item->folder();
        if ( folder->folderType() != FolderTypeMailbox )
            return parent;              //failed

        if ( static_cast<Mailbox *>(folder)->pathName() == target )
            return item;
    }

    return parent;
}

QTreeWidgetItem* FolderListView::getPredecessor(FolderListItem *parent, QString name)
{
    QTreeWidgetItem* predecessor = 0;

    // Find the last child whose name precedes this name in lexicographical order
    for (int i = 0; i < parent->childCount(); ++i) {
        QTreeWidgetItem* child = parent->child(i);
        if (child->text(0) < name) {
            predecessor = child;
        } else {
            break;
        }
    }

    return predecessor;
}

void FolderListView::itemClicked(QTreeWidgetItem *i)
{
    if (i)
        emit viewMessageList();
}

void FolderListView::popFolderSelected(int value)
{
    QModelIndex index = model()->index( 0, 0 );
    while (value) {
        index = next( index );
        --value;
    }
    setCurrentItem( itemFromIndex( index ) );
    scrollToItem( itemFromIndex( index ) );
}

FolderListItem *FolderListView::folderItemFromIndex( QModelIndex index ) const
{
    return static_cast<FolderListItem *>(itemFromIndex( index ));
}

FolderListItem *FolderListView::currentFolderItem() const
{
    return static_cast<FolderListItem *>(currentItem());
}

void FolderListView::restoreCurrentFolder()
{
    if (currentIndex.isValid()) {
	setCurrentItem( itemFromIndex( currentIndex ) );
	scrollToItem( itemFromIndex( currentIndex ) );
    }
}

void FolderListView::rememberCurrentFolder()
{
    currentIndex = indexFromItem( currentItem() );
}

QString FolderListView::excessIndicator()
{
    return QString("*");
}


/* Folder List Item Delegate */

FolderListItemDelegate::FolderListItemDelegate(FolderListView *parent)
    : QtopiaItemDelegate(parent),
      mParent(parent),
      mScrollBar(mParent->verticalScrollBar())
{
    ensurePixmaps();
}

void FolderListItemDelegate::paint(QPainter *painter,
                                   const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    FolderListItem *item = mParent->folderItemFromIndex( index );
    bool statusHighlight;
    statusText = QString();
    if (item)
        item->statusText( &statusText, &statusHighlight, &type );

    QtopiaItemDelegate::paint(painter, option, index);
}

void FolderListItemDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem &option,
                     const QRect &originalRect, const QString &text) const
{
    // Workaround for QtopiaItemDelegate bug?
    // Reduce the available width by the scrollbar size, if necessary
    QRect rect(originalRect);
    if (mScrollBar && mScrollBar->isVisible())
        rect.setWidth(rect.width() - mParent->style()->pixelMetric(QStyle::PM_ScrollBarExtent));

    int tw = 0;
    QString str;
    if (!statusText.isEmpty()) {
        str = statusText;
        if (option.direction == Qt::RightToLeft) {
            QString trim = statusText.trimmed();
            // swap new/total counts in rtl mode
            int sepPos = trim.indexOf( "/" );
            if (sepPos != -1) {
                str = trim.mid( sepPos + 1 );
                str += "/";
                str += trim.left( sepPos );
            }
        }
        QFontMetrics fontMetrics(option.font);
        tw = fontMetrics.width(str);
    }

    QRect textRect(rect);
    textRect.setWidth(rect.width() - tw);
    QtopiaItemDelegate::drawDisplay(painter, option, textRect, text);

    if (tw || (type != NoIcon)) {
        int margin = 5;
        int pw = margin;
        int ph = 0;

        QPixmap *pm = 0;
        if (type == UnreadMessages) {
            pm = pm_unread;
        } else if (type == UnsentMessages) {
            pm = pm_unsent;
        } else if (type == AllMessages) {
            pm = pm_normal;
        }
        if (pm) {
            pw = pm->width() + 2*margin;
            ph = pm->height();
        }
        QRect statusRect = option.direction == Qt::RightToLeft
            ? QRect(0, rect.top(), tw + pw, rect.height())
            : QRect(rect.left()+rect.width()-tw-pw, rect.top(), tw, rect.height());
        
        int v = (rect.height() - ph) / 2;
        QRect pixRect = option.direction == Qt::RightToLeft
            ? QRect(margin, rect.top(), pw-margin, v)
            : QRect(rect.left()+rect.width()-pw+margin, rect.top() + v, pw-2*margin, ph);
        if (!str.isEmpty())
            painter->drawText(statusRect, Qt::AlignCenter, str);
        if (pm)
            painter->drawPixmap(pixRect, *pm);
    }
}
