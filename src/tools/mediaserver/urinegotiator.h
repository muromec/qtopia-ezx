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


#ifndef __MEDIASERVER_URINEGOTIATOR_H
#define __MEDIASERVER_URINEGOTIATOR_H

#include "buildernegotiator.h"


namespace mediaserver
{

class UriNegotiatorPrivate;

class UriNegotiator : public BuilderNegotiator
{
public:
    UriNegotiator();
    ~UriNegotiator();

    QString type() const;
    Attributes const& attributes() const;

    void addBuilder(QString const& tag, int priority, QMediaSessionBuilder* sessionBuilder);
    void removeBuilder(QString const& tag, QMediaSessionBuilder* sessionBuilder);

    QMediaServerSession* createSession(QMediaSessionRequest sessionRequest);
    void destroySession(QMediaServerSession* mediaSession);

private:
    UriNegotiatorPrivate*   d;
};

}   // ns mediaserver

#endif  // __MEDIASERVER_URINEGOTIATOR_H
