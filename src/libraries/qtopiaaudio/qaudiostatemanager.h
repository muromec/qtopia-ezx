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

#ifndef __QAUDIOSTATEMANAGER_H__
#define __QAUDIOSTATEMANAGER_H__

#include <QObject>
#include <qtopiaglobal.h>
#include <qaudionamespace.h>

class QAudioStateInfo;
class QAudioStateManagerPrivate;

class QTOPIAAUDIO_EXPORT QAudioStateManager : public QObject
{
    Q_OBJECT

public:
    QAudioStateManager(QObject *parent = 0);
    ~QAudioStateManager();

public:
    bool setState(const QAudioStateInfo &state, QAudio::AudioCapability capability);

    bool setProfile(const QByteArray &profile);
    bool setDomain(const QByteArray &domain, QAudio::AudioCapability capability);

private:
    QAudioStateManagerPrivate *m_data;
    Q_DISABLE_COPY(QAudioStateManager)
};

#endif
