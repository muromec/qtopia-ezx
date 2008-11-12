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



#ifndef FOLDERLISTVIEW_H
#define FOLDERLISTVIEW_H

#include <qsettings.h>

#include "account.h"
#include "search.h"
#include "folder.h"
#include "emailfolderlist.h"

#include <QtopiaItemDelegate>

#include <qtreewidget.h>
#include <qtimer.h>
#include <qmenubar.h>
#include <qmenu.h>
#include <qwidget.h>
#include <qlineedit.h>

class AccountList;
class MailboxList;

class FolderListItem : public QTreeWidgetItem
{
public:
    FolderListItem(QTreeWidget *parent, Folder *in);
    FolderListItem(QTreeWidget *parent, QTreeWidgetItem *predecessor, Folder *in);
    FolderListItem(QTreeWidgetItem *parent, Folder *in);
    FolderListItem(QTreeWidgetItem *parent, QTreeWidgetItem *predecessor, Folder *in);

    Folder* folder() const;

    void setName(const QString& name);
    QString name() const;

    void setStatusText( const QString &str, bool highlight, IconType type );
    void statusText( QString *str, bool *highlight, IconType *type );

    int depth() const;

protected:
    void init();

#ifdef QTOPIA4_TODO
    void paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int alignment );
    int width( const QFontMetrics& fm,  const QTreeWidget* lv, int c ) const;
#endif
    QString key(int c, bool) const;

private:
    Folder *_folder;
    QString _statusText;
    bool _highlight;
    IconType _type;
};

class FolderListView : public QTreeWidget
{
    Q_OBJECT
public:
    FolderListView(MailboxList *list, QWidget *parent, const char *name);
    virtual ~FolderListView();
    void setupFolders(AccountList *list);
    QModelIndex next(QModelIndex mi, bool nextParent = false) const;
    QTreeWidgetItem* next(QTreeWidgetItem *item) const;
    void updateAccountFolder(QMailAccount *account);
    void deleteAccountFolder(QMailAccount *account);

    Folder* currentFolder() const;
    bool setCurrentFolder(const Folder* folder);

    Folder* systemFolder(const QString& identifier) const;

    Folder* accountFolder(const Folder* account, const QString& mailbox = QString()) const;
    const QTreeWidgetItem* accountFolderItem(const Folder* account, const QString& mailbox = QString()) const;

    QMailAccount* currentAccount() const;

    void changeToSystemFolder(const QString &str);
    void updateFolderStatus(const QString &mailbox, const QString &txt, bool highlight, IconType type);
    void updateAccountStatus(const Folder *account, const QString &txt, bool highlight, IconType type, const QString& mailbox = QString());

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    FolderListItem *folderItemFromIndex( QModelIndex index ) const;
    FolderListItem *currentFolderItem() const;
    void restoreCurrentFolder();
    void rememberCurrentFolder();

    static QString excessIndicator();

signals:
    void emptyFolder();
    void folderSelected(Folder *);
    void folderModified(Folder *);
    void viewMessageList();
    void finished();

public slots:
    void popFolderSelected(int);

protected slots:
    virtual void keyPressEvent( QKeyEvent *e );

    void folderChanged(QTreeWidgetItem *);
    void itemClicked(QTreeWidgetItem *);

private:
    void synchroniseFolderStructure(FolderListItem* item, QMailAccount* account);
    FolderListItem* getParent(FolderListItem *parent, QString name, QString delimiter);
    QTreeWidgetItem* getPredecessor(FolderListItem *parent, QString name);
    QModelIndex systemFolderIndex(const QString& identifier) const;
    QModelIndex accountFolderIndex(const Folder* account, const QString& mailbox = QString()) const;

private:
    MailboxList *_mailboxList;
    QModelIndex currentIndex;
    QMap<QString, SystemFolder*> systemFolders;
};

class FolderListItemDelegate : public QtopiaItemDelegate
{
    Q_OBJECT

public:
    FolderListItemDelegate(FolderListView *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
                    const QModelIndex &index) const;
    void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option,
                     const QRect &rect, const QString &text) const;

private:
    FolderListView *mParent;
    QScrollBar* mScrollBar;
    mutable QString statusText;
    mutable IconType type;
};

#endif
