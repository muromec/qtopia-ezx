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

#ifndef __QTOPIAMEDIA_MEDIASERVERPROXY_H
#define __QTOPIAMEDIA_MEDIASERVERPROXY_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qtopia API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QString>
#include <QMap>
#include <QUuid>
#include <QUrl>

#include "qmediahandle_p.h"

class QValueSpaceItem;
class QtopiaIpcAdaptor;

namespace mlp
{

class MediaServerCallback
{
public:

    virtual ~MediaServerCallback();
    virtual void mediaReady() = 0;
    virtual void mediaError(QString const& error) = 0;
};


class MediaServerProxy : public QObject
{
    Q_OBJECT

    typedef QMap<QUuid, MediaServerCallback*>   CallbackMap;

public:
    ~MediaServerProxy();

    // {{{ Info
    QStringList simpleProviderTags() const;
    QStringList simpleMimeTypesForProvider(QString const& providerTag);
    QStringList simpleUriSchemesForProvider(QString const& providerTag);
    // }}}

    // {{{ QMediaContent
    QMediaHandle prepareContent(MediaServerCallback* callback,
                                QString const& domain,
                                QUrl const& url);
    // }}}

    void destroySession(QMediaHandle const& handle);

    static MediaServerProxy* instance();

private slots:
    void sessionCreated(QUuid const& id);
    void sessionError(QUuid const& id, QString const& error);

    void simpleInfoChanged();

private:
    MediaServerProxy();

    void buildCodecList();
    void buildDeviceList();

    QString                 m_channel;
    QValueSpaceItem*        m_simpleInfo;
    QValueSpaceItem*        m_mediaServerCodecs;
    QValueSpaceItem*        m_mediaServerDevices;
    CallbackMap             m_callbackMap;
    QtopiaIpcAdaptor*       m_adaptor;
};

}   // ns mlp

#endif  //  __QTOPIAMEDIA_MEDIASERVERPROXY_H
