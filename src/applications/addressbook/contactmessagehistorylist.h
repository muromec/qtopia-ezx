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
#ifndef CONTACTMESSAGEHISTORYLIST_H
#define CONTACTMESSAGEHISTORYLIST_H

#include <QWidget>
#include <qtopia/pim/qcontact.h>

class ContactMessageHistoryModel;
class QListView;
class QContactModel;
class QModelIndex;

class ContactMessageHistoryList : public QWidget
{
    Q_OBJECT

public:
    ContactMessageHistoryList( QWidget *parent );
    virtual ~ContactMessageHistoryList();

    QContact entry() const {return ent;}

    void setModel(QContactModel *model);

public slots:
    void init( const QContact &entry );

signals:
    void externalLinkActivated();
    void backClicked();

protected:
    void keyPressEvent( QKeyEvent *e );
    bool eventFilter(QObject*, QEvent*);

protected slots:
    void updateItemUI(const QModelIndex& idx);
    void showMessage(const QModelIndex &idx);

private:
    QContact ent;
    bool mInitedGui;
    ContactMessageHistoryModel *mModel;
    QListView *mListView;
    QContactModel *mContactModel;
};

#endif // CONTACTMESSAGEHISTORYLIST_H

