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

#ifndef __RECEIVEWINDOW_H__
#define __RECEIVEWINDOW_H__

#include <QMainWindow>
#include <QList>
#include <qcontent.h>

class FileTransferListModel;
class QModelIndex;
class FileTransfer;
class QListView;
class QCloseEvent;
class TaskManagerEntry;;
class QAction;
class QTabWidget;

struct VObjectTransfer
{
    int id;
    QString fileName;
    QString mimeType;
};


class ReceiveWindow : public QMainWindow
{
    Q_OBJECT

public:
    ReceiveWindow(QWidget *parent = 0);

public slots:
    void receiveInitiated(int id, const QString &filename,
            const QString &mime, const QString &description);
    void sendInitiated(int id, const QString &filename, const QString &mime,
            const QString &description);
    void progress(int id, qint64 bytes, qint64 total);
    void completed(int id, bool error);

signals:
    void abortTransfer(int id);

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void activated(const QModelIndex &index);
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    void showWindow();
    void stopCurrentTransfer();

private:
    void setUpView(QListView *view);
    QContentId saveFile(const FileTransfer &file);
    void handleIncomingVObject(int id, const QString &fileName,
            const QString &mime, const QString &description);
    bool handleVObjectReceived(int id, bool error);

    FileTransferListModel *m_model;
    QList<VObjectTransfer> m_vObjects;
    TaskManagerEntry *m_taskManagerEntry;
    QAction *m_cancelAction;
    QTabWidget *m_tabs;
    QListView *m_incomingView;
    QListView *m_outgoingView;
};

#endif
