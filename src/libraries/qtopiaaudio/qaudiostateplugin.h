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

#ifndef __QAUDIOSTATEPLUGIN_H__
#define __QAUDIOSTATEPLUGIN_H__

#include <qglobal.h>
#include <qobject.h>
#include <qtopiaglobal.h>
#include <qaudionamespace.h>
#include <QList>

class QAudioState;

class QTOPIAMEDIA_EXPORT QAudioStatePlugin : public QObject
{
    Q_OBJECT

public:
    explicit QAudioStatePlugin(QObject *parent = 0);
    virtual ~QAudioStatePlugin();

    virtual QList<QAudioState *> statesProvided() const = 0;
};

#endif
