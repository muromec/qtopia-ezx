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

#ifndef SCREENSAVERSERVICE_H
#define SCREENSAVERSERVICE_H

#include <qtopiaabstractservice.h>
#include "qtopiapowermanager.h"

class QtopiaPowerManager;

class QtopiaPowerManagerService : public QtopiaAbstractService
{
    Q_OBJECT
public:
    QtopiaPowerManagerService( QtopiaPowerManager *manager, QObject *parent );
    ~QtopiaPowerManagerService();

public slots:
    void setIntervals( int dim, int lightOff, int suspend );
    void setDefaultIntervals();
    void setBacklight( int brightness );
    void setConstraint( int hint, QString);
    void setActive( bool on);

private:
    QtopiaPowerManager *m_powerManager;
};

#endif
