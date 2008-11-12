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

#ifndef __QBLUETOOTHABSTRACTSERVICE_H__
#define __QBLUETOOTHABSTRACTSERVICE_H__

#include <qbluetoothnamespace.h>

class QBluetoothAddress;
class QBluetoothSdpRecord;
class QBluetoothAbstractServicePrivate;

class QBLUETOOTH_EXPORT QBluetoothAbstractService : public QObject
{
    Q_OBJECT

public:
    explicit QBluetoothAbstractService(const QString &name, const QString &displayName, QObject *parent = 0);
    virtual ~QBluetoothAbstractService();

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void setSecurityOptions(QBluetooth::SecurityOptions options) = 0;

    QString name() const;
    QString displayName() const;

protected:
    quint32 registerRecord(const QBluetoothSdpRecord &record);
    quint32 registerRecord(const QString &filename);
    bool unregisterRecord(quint32 handle);

private:
    QBluetoothAbstractServicePrivate *m_data;

signals:
    void started(bool error, const QString &description);
    void stopped();
};

#endif
