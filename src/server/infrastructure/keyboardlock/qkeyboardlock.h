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

#ifndef _QKEYBOARDLOCK_H_
#define _QKEYBOARDLOCK_H_

#include <QObject>

class QKeyboardLockPrivate;
class QKeyboardLock : public QObject
{
Q_OBJECT
public:
    QKeyboardLock(QObject *parent = 0);
    virtual ~QKeyboardLock();

    bool isLocked() const;

    void lock( bool lock = true );
    void unlock();

    QList<Qt::Key> exemptions() const;
    void setExemptions(const QList<Qt::Key> &);

signals:
    void lockedKeyEvent(uint, ushort, bool);

private:
    friend class QKeyboardLockPrivate;
    QKeyboardLockPrivate *d;
};

#endif // _QKEYBOARDLOCK_H_
