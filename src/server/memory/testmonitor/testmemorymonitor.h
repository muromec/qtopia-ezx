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

#ifndef _TESTMEMORYMONITOR_H_
#define _TESTMEMORYMONITOR_H_

#include "memorymonitor.h"
#include <QDateTime>
class QValueSpaceObject;

class TestMemoryMonitor : public MemoryMonitor
{
Q_OBJECT
public:
    TestMemoryMonitor();

    virtual MemState memoryState() const;
    virtual unsigned int timeInState() const;

private slots:
    void setValue(const QByteArray &, const QVariant &);

private:
    void refresh();

    QValueSpaceObject *m_vso;
    MemState m_memstate;
    QDateTime m_lastDateTime;
};

#endif // _TESTMEMORYMONITOR_H_
