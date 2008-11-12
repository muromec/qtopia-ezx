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

#ifndef RADIOSERVICE_H
#define RADIOSERVICE_H

#include <QtopiaAbstractService>

class RadioPlayer;

class RadioService : public QtopiaAbstractService
{
    Q_OBJECT
public:
    RadioService( RadioPlayer *parent );
    ~RadioService();

public slots:
    void mute();
    void unmute();
    void setStation( const QString& band, qlonglong frequency );

private:
    RadioPlayer *parent;
};

#endif /* RADIOSERVICE_H */
