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

#include <QUrl>
#include <QSettings>
#include <QMediaSessionRequest>

#include "gstreamerplaybinsession.h"

#include "gstreamerurisessionbuilder.h"


namespace gstreamer
{

// {{{ UriSessionBuilderPrivate
class UriSessionBuilderPrivate
{
public:
    Engine*     engine;
    QMediaSessionBuilder::Attributes    attributes;
};
// }}}


// {{{ UriSessionBuilder

/*!
    \class gstreamer::UriSessionBuilder
    \internal
*/

UriSessionBuilder::UriSessionBuilder(Engine* engine):
    d(new UriSessionBuilderPrivate)
{
    d->engine = engine;

    // GStreamer does not support dynamically discovering the mime types and
    // uri schemes.  Grab them from QSettings
    QSettings   settings("Trolltech", "gstreamer");
    settings.beginGroup("Simple");

    // Supported URI Schemes
    d->attributes.insert("uriSchemes", settings.value("UriSchemes").toStringList());

    // Supported mime types
    d->attributes.insert("mimeTypes", settings.value("MimeTypes").toStringList());
}

UriSessionBuilder::~UriSessionBuilder()
{
    delete d;
}


QString UriSessionBuilder::type() const
{
    return "com.trolltech.qtopia.uri";
}

QMediaSessionBuilder::Attributes const& UriSessionBuilder::attributes() const
{
    return d->attributes;
}

QMediaServerSession* UriSessionBuilder::createSession(QMediaSessionRequest sessionRequest)
{
    QUrl            url;
    PlaybinSession* mediaSession = 0;

    sessionRequest >> url;

    if (url.isValid())
    {
        mediaSession = new PlaybinSession(d->engine,
                                          sessionRequest.id(),
                                          url);

        if (!mediaSession->isValid())
        {
            delete mediaSession;
            mediaSession = 0;
        }
    }

    return mediaSession;
}

void UriSessionBuilder::destroySession(QMediaServerSession* mediaSession)
{
    delete mediaSession;
}
// }}}

}   // ns gstreamer

