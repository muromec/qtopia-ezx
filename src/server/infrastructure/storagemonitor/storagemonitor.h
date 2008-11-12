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

#ifndef _STORAGEMONITOR_H_
#define _STORAGEMONITOR_H_

#include <QObject>
class QFileSystem;
class QStorageMetaInfo;
class QtopiaTimer;
class QMessageBox;

class StorageMonitor : public QObject
{
    Q_OBJECT
public:
    StorageMonitor(QObject *o = 0);
    ~StorageMonitor();

public slots:
    void checkAvailStorage();
    void systemMsg(const QString &msg, const QByteArray &data);

private slots:
    void showCleanupWizard();

    void availableDisksChanged();

private:
    void outOfSpace(QString& text);
    long fileSystemMetrics(const QFileSystem* fs);

    QStorageMetaInfo *sinfo;
    QMessageBox *box;
    QtopiaTimer *storageTimer;

    int minimalStorageLimit;
    int pollFrequency;
};

#endif // _STORAGEMONITOR_H_
