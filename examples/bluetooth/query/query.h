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

#ifndef __QUERY_H__
#define __QUERY_H__

#include <QMainWindow>

class QBluetoothLocalDevice;
class QBluetoothSdpQuery;
class QBluetoothSdpQueryResult;

class QWaitWidget;
class QListWidget;
class QAction;

class Query : public QMainWindow
{
    Q_OBJECT

public:
    Query(QWidget *parent = 0, Qt::WFlags f = 0);
    ~Query();

public slots:
    void cancelQuery();

private slots:
    void startQuery();
    void searchCancelled();
    void searchComplete(const QBluetoothSdpQueryResult &result);

private:
    QBluetoothLocalDevice *btDevice;
    QBluetoothSdpQuery *sdap;
    QWaitWidget *waiter;
    bool canceled;
    QListWidget *serviceList;
    QAction *startQueryAction;
};

#endif
