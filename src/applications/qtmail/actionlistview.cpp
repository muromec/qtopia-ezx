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

#include "actionlistview.h"
#include <QtopiaItemDelegate>
#include <QPainter>
#include <QPair>

class ActionListItem : public QListWidgetItem
{
public:
    ActionListItem(QListWidget *parent, const QIcon &icon, const QString& name, const QString& mailbox, const QString& in);

    QString name() const;
    QString mailbox() const;

    void setStatus( const QString &str, IconType type );
    QPair<const QString*, IconType> status() const;
    
private:
    QString _mailbox;
    QString _internalName;
    QString _statusText;
    IconType _type;
};

ActionListItem::ActionListItem(QListWidget *parent, const QIcon &icon, const QString& name, const QString& mailbox, const QString& in)
  : QListWidgetItem(icon, name, parent), _mailbox(mailbox), _internalName( in ), _type( AllMessages )
{
}

QString ActionListItem::name() const
{
    return _internalName;
}

QString ActionListItem::mailbox() const
{
    return _mailbox;
}

void ActionListItem::setStatus( const QString &str, IconType type )
{
    _statusText = str;
    _type = type;

    listWidget()->update();
}

QPair<const QString*, IconType> ActionListItem::status() const
{
    return qMakePair(&_statusText, _type);
}


class ActionListItemDelegate : public QtopiaItemDelegate
{
public:
    ActionListItemDelegate(ActionListView *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
                    const QModelIndex &index) const;
    void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option,
                     const QRect &rect, const QString &text) const;

private:
    ActionListView *mParent;
    mutable QPair<const QString*, IconType> status;
};

static QPixmap* pm_normal = 0;
static QPixmap* pm_unread = 0;
static QPixmap* pm_unsent = 0;

static bool ensurePixmaps()
{
    if(!pm_normal)
    {
        // These should be replaced once new icons are available
        pm_normal = new QPixmap(":image/flag_normal");
        pm_unread = new QPixmap(":image/flag_unread");
        pm_unsent = new QPixmap(":image/flag_tosend");
    } 

    return true;
}

ActionListItemDelegate::ActionListItemDelegate(ActionListView *parent)
    : QtopiaItemDelegate(parent),
      mParent(parent)
{
    ensurePixmaps();
}

void ActionListItemDelegate::paint(QPainter *painter,
                                   const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    if (ActionListItem* item = mParent->actionItemFromIndex(index))
        status = item->status();

    QtopiaItemDelegate::paint(painter, option, index);
}

void ActionListItemDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem &option,
                     const QRect &rect, const QString &text) const
{
    QtopiaItemDelegate::drawDisplay(painter, option, rect, text);
    if (!status.first->isEmpty()) {
        QString str = *status.first;
        if (option.direction == Qt::RightToLeft) {
            QString trim = str.trimmed();
            // swap new/total counts in rtl mode
            int sepPos = trim.indexOf( "/" );
            if (sepPos != -1) {
                str = trim.mid( sepPos + 1 );
                str += "/";
                str += trim.left( sepPos );
            }
        }
        QPixmap *pm = 0;
        if (status.second == UnreadMessages) {
            pm = pm_unread;
        } else if (status.second == UnsentMessages) {
            pm = pm_unsent;
        } else if (status.second == AllMessages) {
            pm = pm_normal;
        }

        QFontMetrics fontMetrics(option.font);
        int tw = fontMetrics.width(str);
        int margin = 5;
        int pw = margin;
        int ph = 0;
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
        painter->drawText(statusRect, Qt::AlignCenter, str);
        if (pm)
            painter->drawPixmap(pixRect, *pm);
    }
}


ActionListView::ActionListView(QWidget *parent)
    : QListWidget( parent )
{
    setObjectName( "actionView" );

    ActionListItemDelegate *delegate = new ActionListItemDelegate( this );
    setItemDelegate( delegate );

    mComposeItem = newItem( QT_TRANSLATE_NOOP("ActionListView","New message"), "ActionListView", QString() );
    mComposeItem->setIcon(QIcon(":icon/new"));

    mInboxItem = newItem( MailboxList::InboxString );
    mSentItem = newItem( MailboxList::SentString );
    mDraftsItem = newItem( MailboxList::DraftsString );
    mTrashItem = newItem( MailboxList::TrashString );
    mOutboxItem = newItem( MailboxList::OutboxString );
    mEmailItem = newItem( MailboxList::EmailString );

    setCurrentRow( 0 );

    connect( this, SIGNAL(itemActivated(QListWidgetItem*)),
             this, SLOT(itemSlot(QListWidgetItem*)) );
    connect( this, SIGNAL(currentRowChanged(int)),
             this, SLOT(currentFolderChanged(int)) );

    // Required to work around bug in Qt 4.3.2, where additional vertical
    // space is allocated but not used for the listview
    setVerticalScrollMode(ScrollPerPixel);
}

ActionListView::~ActionListView()
{
}

// Simple convenience function
ActionListItem *ActionListView::newItem( const char *name, const char* context )
{
    return newItem( name, context, QString(name) );
}

ActionListItem *ActionListView::newItem( const char *name, const char* context, const QString& mailbox )
{
    QString displayName( context ? qApp->translate( context, name ) : MailboxList::mailboxTrName( name ) );
    return new ActionListItem( this, MailboxList::mailboxIcon(name), displayName, mailbox, name );
}

void ActionListView::itemSlot(QListWidgetItem *item)
{
    if (item == mComposeItem)
        emit composeMessage();
    else if (item == mEmailItem)
        emit emailSelected();
    else
        emit displayFolder( static_cast<ActionListItem *>(item)->mailbox() );
}

void ActionListView::currentFolderChanged(int row)
{
    emit currentFolderChanged( actionItemFromRow(row)->mailbox() );
}

ActionListItem *ActionListView::folder(const QString &mailbox) const
{
    for (int i = 1; i < count(); ++i)
        if (ActionListItem* item = actionItemFromRow(i))
            if (item->mailbox() == mailbox)
                return item;

    return 0;
}

void ActionListView::updateFolderStatus(const QString &mailbox, const QString &txt, IconType type)
{
    if (ActionListItem *aItem = folder(mailbox))
        aItem->setStatus( txt, type );
}

QString ActionListView::currentFolder() const
{
    return actionItemFromIndex(currentIndex())->mailbox();
}

void ActionListView::setCurrentFolder(const QString &mailbox)
{
    if (ActionListItem *aItem = folder(mailbox))
        setCurrentItem(aItem);
}

ActionListItem *ActionListView::actionItemFromIndex( QModelIndex index ) const
{
    return static_cast<ActionListItem *>(itemFromIndex( index ));
}

ActionListItem *ActionListView::actionItemFromRow( int row ) const
{
    return static_cast<ActionListItem *>(item( row ));
}

