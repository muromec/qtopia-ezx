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

#include "qmediavideocontrolserver.h"


/*!
    \class QMediaVideoControlServer
    \preliminary
    \brief The QMediaVideoControlServer class is used by Media Engines to
            inform clients that video is available for the associated media session.

    A Media Engine using the QtopiaMedia API constructs a
    QMediaVideoControlServer when it would like to inform and enable a client
    to access a QWidget based video widget in the server.

    \code
    {
        VideoWidget* videoWidget = new VideoWidget(videoSource, parent);

        QMediaVideoControlServer* videoControlServer = new QMediaVideoControlServer(id, parent);

        videoControlServer->setRenderTarget(videoWidget);
    }
    \endcode

    \sa QMediaVideoControl, QMediaAbstractControlServer

    \ingroup multimedia
*/

/*!
    Constructs a QMediaVideoControlServer with the session \a handle, and
    optionally the video \a target widget for sharing with the client and the
    \a parent QObject.
*/

QMediaVideoControlServer::QMediaVideoControlServer
(
 QMediaHandle const& handle,
 QWidget*       target,
 QObject*       parent
):
    QMediaAbstractControlServer(handle, "Video", parent)
{
    proxyAll();

    if (target != 0)
        setRenderTarget(target);
    else
    {
        setValue("hasVideo", false);
        setValue("windowId", -1);
    }
}

/*!
    Destroy the QMediaVideoControlServer object.
*/

QMediaVideoControlServer::~QMediaVideoControlServer()
{
}

/*!
    Set the video target widget to \a renderTarget.

    The Media Engine calls this function when it has a widget that it would
    like to make available to a client application.

    The QWidget will exist in the Media Server process, but will be made
    available to the session client.  A session client will be able to control
    different widget states, for example whether the widget is hidden or visible.
*/

void QMediaVideoControlServer::setRenderTarget(QWidget* renderTarget)
{
    setRenderTarget(renderTarget == 0 ? -1 : int(renderTarget->winId()));
}

/*!
    Set the video target to the window with \a wid.

    The Media Engine calls this function when it has a widget that it would
    like to make available to a client application.

    The QWidget will exist in the Media Server process, but will be made
    available to the session client.  A session client will be able to control
    different widget states, for example whether the widget is hidden or visible.
*/

void QMediaVideoControlServer::setRenderTarget(int wid)
{
    if (wid == -1)
        unsetRenderTarget();
    else
    {
        setValue("hasVideo", true);
        setValue("windowId", wid);

        emit videoTargetAvailable();
    }
}

/*!
    Remove the video target widget.

    The Media Engine should call this function when it will no longer be rendering video
    into the QWidget set in setRenderTarget(). The Session client will be notified that the
    widget is no longer valid.
*/

void QMediaVideoControlServer::unsetRenderTarget()
{
    setValue("hasVideo", false);
    setValue("windowId", -1);

    emit videoTargetRemoved();
}

/*!
    \fn void QMediaVideoControlServer::videoTargetAvailable();

    This signal is emitted when a render target has been set. It will indicate
    to the client QMediaVideoControl class, that a video widget is available to
    be displayed.
*/

/*!
    \fn void QMediaVideoControlServer::videoTargetRemoved();

    This signal is emitted when a render target has been removed. It will indicate
    to the client QMediaVideoControl class, that the Media Engine is no longer rendering
    video for this session.
*/



