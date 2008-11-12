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

#ifndef __QALARMSERVER_P_H__
#define __QALARMSERVER_P_H__

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

#include <QObject>
#include <QtopiaIpcAdaptor>
#include <QDateTime>

class AlarmServerService : public QtopiaIpcAdaptor
{
    Q_OBJECT
public:
    AlarmServerService( QObject *parent = 0 )
        : QtopiaIpcAdaptor( "QPE/AlarmServer", parent ) { publishAll( Slots ); }
    ~AlarmServerService() {}

public slots:
    void addAlarm( QDateTime when, const QString& channel,
                   const QString& msg, int data );
    void deleteAlarm( QDateTime when, const QString& channel,
                      const QString& msg, int data );
    void dailyAlarmEnabled( bool flag );
};

#endif
