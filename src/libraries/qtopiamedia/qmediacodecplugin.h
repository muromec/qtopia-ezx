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

#ifndef __QTOPIA_MEDIASERVER_MEDIACODECPLUGIN_H
#define __QTOPIA_MEDIASERVER_MEDIACODECPLUGIN_H

#include <QStringList>

#include <qtopiaglobal.h>

class QMediaEncoder;
class QMediaDecoder;

class QTOPIAMEDIA_EXPORT QMediaCodecPlugin
{
public:
    virtual ~QMediaCodecPlugin();

    virtual QString name() const = 0;
    virtual QString comment() const = 0;
    virtual QStringList mimeTypes() const = 0;
    virtual QStringList fileExtensions() const = 0;

    virtual double version() const = 0;

    virtual bool canEncode() const = 0;
    virtual bool canDecode() const = 0;

    virtual QMediaEncoder* encoder(QString const& mimeType) = 0;
    virtual QMediaDecoder* decoder(QString const& mimeType) = 0;
};


Q_DECLARE_INTERFACE(QMediaCodecPlugin, "com.trolltech.qtopia.MediaCodecPlugin/1.0");

#endif  // __QTOPIA_MEDIASERVER_MEDIACODECPLUGIN_H
