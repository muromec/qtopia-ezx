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
#ifndef QCOPENVELOPE_H
#define QCOPENVELOPE_H

#include <qtopiaglobal.h>
#include <qtopiachannel.h>
#include <qdatastream.h>

class QTOPIABASE_EXPORT QtopiaIpcEnvelope : public QDataStream {
    QString ch, msg;
public:
    QtopiaIpcEnvelope( const QString& channel, const QString& message );
    ~QtopiaIpcEnvelope();
};

#endif
