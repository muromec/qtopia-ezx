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

#include <qmap.h>
#include <qstringlist.h>

#include <qpluginmanager.h>
#include <qmediacodecplugin.h>
#include <qmediaengineinformation.h>

#include "cruxusurisessionbuilder.h"
#include "cruxussimplesession.h"

#include "cruxusengine.h"


namespace cruxus
{

// {{{ EngineInformation
class EngineInformation : public QMediaEngineInformation
{
public:
    EngineInformation(QMediaSessionBuilderList const& sessionBuilders):
        m_sessionBuilders(sessionBuilders)
    {
    }

    ~EngineInformation()
    {
    }

    QString name() const
    {
        return "Cruxus";
    }

    QString version() const
    {
        return "1.0";
    }

    int idleTime() const
    {
        return 0;
    }

    bool hasExclusiveDeviceAccess() const
    {
#ifdef HAVE_OSS
        return true;
#else
        return false;
#endif
    }

    QMediaSessionBuilderList sessionBuilders() const
    {
        return m_sessionBuilders;
    }

private:
    QMediaSessionBuilderList const& m_sessionBuilders;
};
// }}}

// {{{ CruxusEnginePrivate
class CruxusEnginePrivate
{
public:
    QPluginManager*             pluginManager;
    MediaCodecPluginMap         pluginCodecs;
    MediaCodecPluginMap         pluginExtensionMapping;
    EngineInformation*          engineInfo;
    QMediaSessionBuilderList    sessionBuilders;
};
// }}}

// {{{ Engine

/*!
    \class cruxus::Engine
    \internal
*/

Engine::Engine():
    d(new CruxusEnginePrivate)
{
}

Engine::~Engine()
{
    delete d->pluginManager;
    delete d->engineInfo;
    delete d;
}

void Engine::initialize()
{
    QStringList         mimeTypes;

    d->pluginManager = new QPluginManager("codecs", this);

    // Find plugins
    foreach (QString const& pluginName, d->pluginManager->list())
    {
        QMediaCodecPlugin*  plugin;
        QObject*            instance = d->pluginManager->instance(pluginName);

        if ((plugin = qobject_cast<QMediaCodecPlugin*>(instance)) != 0)
        {
            // Mime types
            mimeTypes += plugin->mimeTypes();

            // Extensions
            foreach (QString const& extension, plugin->fileExtensions())
            {
                d->pluginExtensionMapping.insert(extension, plugin);
            }
        }
        else
            delete instance;    // unload unwanted
    }

    // Register Builders
    d->sessionBuilders.push_back(new UriSessionBuilder(this,
                                                       mimeTypes,
                                                       d->pluginExtensionMapping));

    // Create our info object
    d->engineInfo = new EngineInformation(d->sessionBuilders);
}


void Engine::start()
{
}

void Engine::stop()
{
}

void Engine::suspend()
{
    for(int i = 0; i < s.size(); ++i) {
        s.at(i)->suspend();
    }
}

void Engine::resume()
{
    for(int i = 0; i < s.size(); ++i) {
        s.at(i)->resume();
    }
}

QMediaEngineInformation const* Engine::engineInformation()
{
    return d->engineInfo;
}

void Engine::registerSession(QMediaServerSession* session)
{
    s.append(static_cast<SimpleSession*>(session));
}

void Engine::unregisterSession(QMediaServerSession* session)
{
    for(int i = 0; i < s.size(); ++i) {
        if(s.at(i) == (static_cast<SimpleSession*>(session))) {
            s.removeAt(i);
        }
    }
}
// }}}

}   // ns cruxus
