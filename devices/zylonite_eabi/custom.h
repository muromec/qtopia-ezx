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

#ifndef QT_QWS_ZYLONITEA1
#define QT_QWS_ZYLONITEA1
#endif

#if defined(__GNUC__) && (__GNUC__ > 2)
#define QPE_USE_MALLOC_FOR_NEW
#endif

#define QPE_DEFAULT_TODAY_MODE "Daily"

#define QPE_NEED_CALIBRATION

//#define NO_WIRELESS_LAN

#define QTOPIA_ENABLE_EXPORTED_BACKGROUNDS
#define QTOPIA_ENABLE_GLOBAL_BACKGROUNDS

#define QGLOBAL_PIXMAP_CACHE_LIMIT 5242880   //use 5M instead of default 1M

#define NO_VISUALIZATION

#define V4L_VIDEO_DEVICE "/dev/video"
// Define the devices whose packages are compatible with this device.
// by convention the first device listed is this device.
#define QTOPIA_COMPATIBLE_DEVICES "Zylonite_eabi"

