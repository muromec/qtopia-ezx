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



#ifndef EMAILLISTITEM_H
#define EMAILLISTITEM_H

#include <qtablewidget.h>
#include <qitemdelegate.h>
#include <qtopia/pim/qcontactmodel.h>
#include <QMailId>
#include <QMailMessage>

class MailListView;
class QFont;
class QPixmap;
class QObject;
class QPainter;
class QMailAddress;

class EmailListItem: public QTableWidgetItem
{
public:
    EmailListItem(MailListView *parent, const QMailId& id, int col);
    virtual ~EmailListItem();

    QMailId id() const;
    void setId(const QMailId& id);

    QPixmap *pixmap() { return typePm; }
    virtual bool operator<(const QTableWidgetItem &other) const;

    bool stateUpdated() { return columnsSet; }
    void updateState();
    static void deletePixmaps();
    static QString dateToString( QDateTime dateTime );
    QString cachedName() const;
    void setCachedName(const QString&);

    QMailMessage::MessageType type() const;

protected:
    void setColumns();

private:
    QMailId mId;
    bool columnsSet;
    QPixmap *typePm;
    bool alt;
    int mCol;
    QString mCachedName;
    QMailMessage::MessageType messageType;
};

class EmailListItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    EmailListItemDelegate(MailListView *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
private:
    MailListView *mParent;
};

#endif
