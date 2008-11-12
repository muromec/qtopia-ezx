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

#ifndef __RFCOMMSERVER_H__
#define __RFCOMMSERVER_H__

#include <QMainWindow>

class QTextEdit;
class QBluetoothRfcommServer;
class QBluetoothRfcommSocket;

class RfcommServer : public QMainWindow
{
    Q_OBJECT

public:
    RfcommServer(QWidget *parent = 0, Qt::WFlags f = 0);
    ~RfcommServer();

private slots:
    void newConnection();
    void readyRead();
    void disconnected();

private:
    void handleClient(QBluetoothRfcommSocket *socket);

private:
    QTextEdit *textArea;
    QBluetoothRfcommServer *server;
};

#endif
