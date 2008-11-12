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
#include "qhardwareinterface.h"
#include "qhardwareinterface_p.h"

#include <QValueSpaceItem>

// Qt includes
#include <QSettings>

// ============================================================================
//
// QHardwareInterface
//
// ============================================================================

/*!
    \class QHardwareInterface
    \mainclass
    \brief The QHardwareInterface class is the base class of all accessory classes.

    QHardwareInterface and the accessory classes are part of the
    \l {Qtopia Accessory System}, which provides information and on the
    availability of physical accessories and an API for controlling those
    accessories.  The accessory API is split into two sets of classes:
    accessory provider classes and accessory client classes.  Both sets are
    subclasses of QHardwareInterface.
    
    The accessory provider classes implement device-specific code for
    controlling the hardware feature and manage the state that is reported to
    the rest of Qtopia through the accessory client API.  Provider classes are
    created by passing QAbstractIpcInterface::Server as the mode parameter when
    constructing QHardwareInterface derived classes.

    The accessory client classes provide an API for querying the state of and
    controlling accessories.  Client classes communicate with provider classes
    through an Inter Process Communication (IPC) mechanism.  Multiple client
    instances can connect to a single accessory provider.  Client classes are
    created by passing QAbstractIpcInterface::Client as the mode parameter when
    constructing QHardwareInterface derived classes.

    Qtopia automatically recognizes any subclass of QHardwareInterface as a Qtopia accessory.
    Each accessory follows the principal of accessory and accessory provider whereby
    the provider is a subclass of the accessory class. See the documentation for 
    QAbstractIpcInterface for more information on wrting and using interface classes.

    The accessory system supports multiple accessory providers of the same
    interface type.  For example, a device could have a primary and secondary
    battery.  Each battery would be exposed as a separate instance of a
    QPowerSourceProvider derived class.  Default accessories can be defined in
    the \c {Trolltech/HardwareAccessories.conf} configuration file.  The file
    contains a list of interface names and the identity of the default
    accessory, for example:

    \code
    [Defaults]
    QPowerSource = DefaultBattery
    QSignalSource = DefaultSignal
    QVibrateAccessory = Modem
    \endcode

    \sa QHardwareManager, QAbstractIpcInterface

    \ingroup hardware
*/

/*!
    Constructs a new QHardwareInterface object with interface type \a name,
    identity \a id and operates in \a mode.  The object is attached to
    \a parent.

    If \a id is empty the default accessory for \a name will be
    automatically selected.
*/
QHardwareInterface::QHardwareInterface( const QString& name,
                                        const QString& id,
                                        QObject* parent,
                                        QAbstractIpcInterface::Mode mode )
: QAbstractIpcInterface( HARDWAREINTERFACE_VALUEPATH,
                         name,
                         id,
                         parent,
                         mode )
{
    if ( mode == QAbstractIpcInterface::Server ) {
        QSettings defaults( "Trolltech", "HardwareAccessories" );
        defaults.beginGroup( "Defaults" );
        if ( defaults.value( name ) == id )
            setPriority( 1 );
        defaults.endGroup();
    }
}

/*!
    Destroys the hardware interface object.
*/
QHardwareInterface::~QHardwareInterface()
{
}
