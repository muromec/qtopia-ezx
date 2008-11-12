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

#ifndef __QIRREMOTEDEVICE_H__
#define __QIRREMOTEDEVICE_H__

#include <qirnamespace.h>
#include <qirglobal.h>

#include <QString>

class QIR_EXPORT QIrRemoteDevice
{
public:
    QIrRemoteDevice(const QString &name,
                    QIr::DeviceClasses &devClasses,
                    uint addr);
    QIrRemoteDevice(const QIrRemoteDevice &dev);
    ~QIrRemoteDevice();

    QIrRemoteDevice &operator=(const QIrRemoteDevice &other);
    bool operator==(const QIrRemoteDevice &other) const;

    uint address() const;
    QIr::DeviceClasses deviceClasses() const;
    QString name() const;

private:
    QString m_name;
    QIr::DeviceClasses m_dev_class;
    uint m_addr;
};

#endif
