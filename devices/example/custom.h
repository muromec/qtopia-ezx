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

#ifndef QT_QWS_EXAMPLE
#define QT_QWS_EXAMPLE
#endif

#if defined(__GNUC__) && (__GNUC__ > 2)
#define QPE_USE_MALLOC_FOR_NEW
#endif

#define QPE_NEED_CALIBRATION

// Define this if wireless LAN support should be removed from the lan plugin
// Removing Wireless LAN support reduces the size of the lan plugin by about 1 MB.
// Extended Wireless LAN support (scanning and active reconnection) requires Wireless extension v14+ and will only be enabled if the
// device supports WE v14+
#define NO_WIRELESS_LAN

// The serial device for AT command access to the phone
// hardware:
//
//#define QTOPIA_PHONE_DEVICE "/dev/ttyS1"

// Define this to use the "advanced" GSM 07.10 CMUX mode instead of "basic".
//#define QTOPIA_ADVANCED_CMUX

// Define these to use direct serial ports for multiplexing channels.
// The primary AT command channel is defined by "QTOPIA_PHONE_DEVICE".
// Channels may be omitted if they aren't appropriate.
//
// This enables the use of CSMI on the TI device, instead of GMS 07.10.
//#define QTOPIA_MUX_SECONDARY_DEVICE "/dev/csmi/6"
//#define QTOPIA_MUX_DATA_DEVICE "/dev/csmi/8"
//#define QTOPIA_MUX_SPARE_DEVICE "/dev/ttyXX"
//#define QTOPIA_MUX_RAW_DEVICE "/dev/ttyXX"

// Define this if setup commands for GPRS data sessions should be sent
// on the primary command channel instead of the data channel.
//#define QTOPIA_MUX_DATA_SETUP_ON_COMMAND_CHANNEL

#define QTOPIA_ENABLE_EXPORTED_BACKGROUNDS
#define QTOPIA_ENABLE_GLOBAL_BACKGROUNDS


// The location of where the zoneinfo files are under are stored.
// Normally QTOPIA_ZONEINFO_PATH does not need to be defined
// since "/usr/share/zoneinfo/" is used by default.
// However in rare cases using a non-standard location maybe required
//#define QTOPIA_ZONEINFO_PATH "/usr/share/zoneinfo/"

// Define the devices whose packages are compatible with this device,
// by convention the first device listed is this device.
#define QTOPIA_COMPATIBLE_DEVICES "Example"

// Define the name of the Video4Linux device to use for the camera.
#ifndef V4L_VIDEO_DEVICE
#define V4L_VIDEO_DEVICE            "/dev/video"
#endif
