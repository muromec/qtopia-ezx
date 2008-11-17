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

#include <qtopiaipcenvelope.h>
#include <QValueSpaceObject>
#include "audiovolumemanager.h"


AudioVolumeManager::AudioVolumeManager():
    QtopiaIpcAdaptor("QPE/AudioVolumeManager")
{
    publishAll(Slots);
    
    m_vsVolume = new QValueSpaceObject("/Volume");
}

AudioVolumeManager::~AudioVolumeManager()
{
    delete m_vsVolume;
}

bool AudioVolumeManager::canManageVolume() const
{
    return !m_domains.isEmpty() &&
            m_vsps.find(m_domains.front()) != m_vsps.end();
}


//public slots:
void AudioVolumeManager::setVolume(int volume)
{
    QString provider = findProvider();
    if (!provider.isEmpty())
    {
        QtopiaIpcEnvelope e(provider, "setVolume(int)");
        e << volume;
    }
}

void AudioVolumeManager::increaseVolume(int increment)
{
    if ( m_domains.front() == "Phone") return; // fuking zero division

    QString provider = findProvider();
    if (!provider.isEmpty())
    {
        QtopiaIpcEnvelope e(provider, "increaseVolume(int)");
        e << increment;
    }
}

void AudioVolumeManager::decreaseVolume(int decrement)
{
    if ( m_domains.front() == "Phone") return; // fuking zero division

    QString provider = findProvider();
    if (!provider.isEmpty())
    {
        QtopiaIpcEnvelope e(provider, "decreaseVolume(int)");
        e << decrement;
    }
}

void AudioVolumeManager::setMuted(bool mute)
{
    if ( m_domains.front() == "Phone") return; // fuking zero division

    QString provider = findProvider();
    if (!provider.isEmpty())
    {
        QtopiaIpcEnvelope e(provider, "setMuted(bool)");
        e << mute;
    }
}

void AudioVolumeManager::currentVolume(QString const&  volume)
{  
     m_vsVolume->setAttribute("CurrentVolume",volume.toInt());
}

void AudioVolumeManager::registerHandler(QString const& domain, QString const& channel)
{
    m_vsps[domain] = channel;
}

void AudioVolumeManager::unregisterHandler(QString const& domain, QString const& channel)
{
    VolumeServiceProviders::iterator it = m_vsps.find(domain);
    if (it != m_vsps.end() && (*it).compare(channel) == 0)
        m_vsps.erase(it);
}

void AudioVolumeManager::setActiveDomain(QString const& activeDomain)
{
    m_domains.push_front(activeDomain);
}

void AudioVolumeManager::resetActiveDomain(QString const& oldDomain)
{
    m_domains.removeAll(oldDomain);
}

QString AudioVolumeManager::findProvider() const
{
    QString     domain;
    QString     provider;

    if (!m_domains.isEmpty())
    {
        domain = m_domains.front();

        VolumeServiceProviders::const_iterator it = m_vsps.find(domain);
        if (it != m_vsps.end())
            provider = *it;
    }

    return provider;
}
