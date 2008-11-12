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

// Local includes
#include "cameramonitor.h"
#include "qtopiaserverapplication.h"

// Qtopia includes
#include <QtopiaFeatures>
#include <QValueSpaceItem>
#include <QTimer>
#include <custom.h>

// System includes
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// Constants
static const char* const CAMERA_MONITOR_VIDEODEV        =   V4L_VIDEO_DEVICE;
static const char* const CAMERA_MONITOR_CAMERA_FEATURE  =   "Camera";

// ============================================================================
//
//  CameraMonitor
//
// ============================================================================

/*!
  \class CameraMonitor
  \ingroup QtopiaServer::Task
  \brief The CameraMonitor class updates the camera feature

  The default implementation checks the camera availablity at startup and
  exports the "Camera" QtopiaFeature, if the camera can added and removed
  during device operation then update() should be called when this occurs.
  
  This class is part of the Qtopia server and cannot be used by other Qtopia applications.
*/

/*!
    Constructs the camera monitor task and attaches it to \a parent.
*/
CameraMonitor::CameraMonitor( QObject* parent )
:   QObject( parent )
{
    //delay the update to minimize startup time
    serverWidgetVsi = new QValueSpaceItem("/System/ServerWidgets/Initialized", this);
    connect( serverWidgetVsi, SIGNAL(contentsChanged()), this, SLOT(delayedUpdate()) );
    delayedUpdate(); //in case its visible already
}

/*!
    \internal
*/
CameraMonitor::~CameraMonitor()
{
}

/*!
  \internal
  */
void CameraMonitor::delayedUpdate()
{
    if ( serverWidgetVsi && serverWidgetVsi->value( QByteArray(), false ).toBool() ) {
        serverWidgetVsi->disconnect();
        serverWidgetVsi->deleteLater();
        serverWidgetVsi = 0;
        QTimer::singleShot( 5000, this, SLOT(update()) );
    }
}
/*!
    Updates the availability of the camera feature.
*/
void CameraMonitor::update()
{
    // TODO This code is duplicated in src/applications/camera/videocapturedevicefactory
    // so move into one convenient class
    int fd = open( CAMERA_MONITOR_VIDEODEV, O_RDWR );
    if ( fd != -1 ) {
        QtopiaFeatures::setFeature( CAMERA_MONITOR_CAMERA_FEATURE );
    } else {
        QtopiaFeatures::removeFeature( CAMERA_MONITOR_CAMERA_FEATURE );
    }
    close( fd );
}

