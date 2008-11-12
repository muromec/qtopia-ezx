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

#ifndef ACTIONLISTVIEW_H
#define ACTIONLISTVIEW_H

#include <qlistwidget.h>
#include <quuid.h>
#include "emailfolderlist.h"

class MailboxList;
class ActionListItem;
class ActionListItemDelegate;

class ActionListView : public QListWidget
{
    Q_OBJECT

public:
    ActionListView(QWidget *parent);
    virtual ~ActionListView();

    void updateFolderStatus( const QString &, const QString &, IconType );

    QString currentFolder() const;
    void setCurrentFolder(const QString &);

signals:
    void composeMessage();
    void displayFolder(const QString &);
    void emailSelected();
    void currentFolderChanged(const QString &);

protected slots:
    void itemSlot(QListWidgetItem *item);
    void currentFolderChanged(int); 

private:
    friend class ActionListItemDelegate;

    ActionListItem *newItem( const char *name, const char* context = 0 );
    ActionListItem *newItem( const char *name, const char* context, const QString& mailbox );
    ActionListItem *actionItemFromIndex( QModelIndex index ) const;
    ActionListItem *actionItemFromRow( int row ) const;
    ActionListItem *folder(const QString &) const;

    QListWidgetItem *mComposeItem;
    QListWidgetItem *mInboxItem;
    QListWidgetItem *mSentItem;
    QListWidgetItem *mDraftsItem;
    QListWidgetItem *mTrashItem;
    QListWidgetItem *mOutboxItem;
    QListWidgetItem *mEmailItem;
    QListWidgetItem *mSearchItem;
};

#endif
