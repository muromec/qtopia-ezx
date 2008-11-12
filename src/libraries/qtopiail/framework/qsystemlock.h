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

#ifndef _QSYSTEMLOCK_H_
#define _QSYSTEMLOCK_H_

#include "qtopiailglobal.h"

class QSystemReadWriteLockPrivate;
class QTOPIAIL_EXPORT QSystemReadWriteLock
{
public:
    QSystemReadWriteLock(unsigned int id, bool own);
    ~QSystemReadWriteLock();

    bool isNull() const;
    unsigned int id() const;

    bool lockForRead(int milliSec);
    bool lockForWrite(int milliSec);
    void unlock();

private:
    QSystemReadWriteLockPrivate * d;
};

class QSystemMutex_Private;
class QTOPIAIL_EXPORT QSystemMutex
{
public:
    QSystemMutex(unsigned int id, bool own);
    ~QSystemMutex();

    bool isNull() const;
    unsigned int id() const;

    bool lock(int milliSec);
    void unlock();

private:
    QSystemMutex_Private *m_data;
};

#endif // _QSYSTEMLOCK_H_

