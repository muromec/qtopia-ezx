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

#ifndef ACTIVITYMONITOR_H
#define ACTIVITYMONITOR_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qtopia API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/observer_p.h>

#include <QtCore>
#include <qtopiaglobal.h>

class QTOPIAMEDIA_EXPORT ActivityMonitor : public QObject
{
    Q_OBJECT
public:
    explicit ActivityMonitor( int interval, QObject* parent = 0 )
        : QObject( parent), m_interval( interval ), m_active( false ), m_updateCalled( false )
    { }

    bool isActive() { return m_active; }

signals:
    void active();
    void inactive();

public slots:
    void update();

protected:
    void timerEvent( QTimerEvent* );

private:
    int m_interval;
    int m_timer;

    bool m_active;
    bool m_updateCalled;
};

#endif // ACTIVITYMONITOR_H
