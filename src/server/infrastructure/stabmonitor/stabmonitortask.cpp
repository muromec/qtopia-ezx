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

#include <sys/stat.h>

#include "stabmonitortask.h"

#include "qtopiaserverapplication.h"

#include <QFileSystemWatcher>
#include <QtopiaIpcEnvelope>

/*!
  \class StabMonitor
  \ingroup QtopiaServer::Task
  \brief The StabMonitor class supports monitoring of stab changes.

  The StabMonitor task provides stab change notifications by monitoring
  the following files:

  \list
    \o \c{/var/run/stab}
    \o \c{/var/state/pcmcia/stab}
    \o \c{/var/lib/pcmcia/stab}
  \endlist

  These files are maintained by the cardmgr application. When a new PCMCIA card is plugged 
  into the system one of the above files is edited to reflect the changed state of the PCMCIA 
  sub system. Most systems only have one of the above files. The StabMonitor task sends 
  the \c{stabChanged()} message on the \c{QPE/Card} channel in order to notify Qtopia about 
  the change.

  If the target device doesn't support PCMCIA or has a different mechanism for stab change 
  notifications the StabMonitor task can be disabled and alternative arrangements have to be
  provided. 

  This task is a replacement for the SysFileMonitor class in earlier versions of Qtopia 
  (before Qtopia 4.2.2). The old SysFileMonitor used active polling to detect stab changes which
  was very inefficient.
  
  This class is part of the Qtopia server and cannot be used by other Qtopia applications.
*/

const char* stab0 = "/var/run/stab";
const char* stab1 = "/var/state/pcmcia/stab";
const char* stab2 = "/var/lib/pcmcia/stab";

/*!
  Construct a new StabMonitor instance. \a parent is the QObject parent parameter.
  */
StabMonitor::StabMonitor( QObject* parent )
    : QObject( parent ), watcher( 0 )
{
    static const char* tab[] = {
        stab0,
        stab1, 
        stab2
    };

    static const int nstab = sizeof(tab)/sizeof(const char*);
    bool found = false;
    struct stat s;
    for( int i = 0; i<nstab; i++ ) {
        if ( ::stat(tab[i], &s ) == 0 ) {
            found = true;
        }

        if ( found ) {
            //we can use file system watcher because the parent directory and the watched file
            //are never deleted
            watcher = new QFileSystemWatcher( this );
            watcher->addPath( QLatin1String( tab[i] ) );
            connect( watcher, SIGNAL(fileChanged(QString)),
                     this, SLOT(stabChanged(QString)) );
            break;
        }
    }
}

/*!
  \internal
  */
void StabMonitor::stabChanged( const QString& /*file*/ )
{
    QtopiaIpcEnvelope( "QPE/Card", "stabChanged()" );
}


QTOPIA_TASK( StabMonitor, StabMonitor );

