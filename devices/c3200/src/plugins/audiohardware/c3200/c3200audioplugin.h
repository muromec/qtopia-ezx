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

#ifndef __C3200_AUDIO_PLUGIN_H__
#define __C3200_AUDIO_PLUGIN_H__

#include <QAudioStatePlugin>

class C3200AudioPluginPrivate;

class C3200AudioPlugin : public QAudioStatePlugin
{
    Q_OBJECT
    friend class C3200AudioPluginPrivate;

public:
    C3200AudioPlugin(QObject *parent = 0);
    ~C3200AudioPlugin();

    QList<QAudioState *> statesProvided() const;

private:
    C3200AudioPluginPrivate *m_data;
};

#endif
