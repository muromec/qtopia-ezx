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

#ifndef __QTOPIA_MEDIASERVER_QMEDIAHELIXSETTINGSSERVER_H
#define __QTOPIA_MEDIASERVER_QMEDIAHELIXSETTINGSSERVER_H

#include <QAbstractIpcInterface>

class IHXClientEngine;

namespace qtopia_helix
{

class QMediaHelixSettingsServerPrivate;

class QMediaHelixSettingsServer : public QAbstractIpcInterface
{
    Q_OBJECT

public:
    QMediaHelixSettingsServer(IHXClientEngine* engine);
    ~QMediaHelixSettingsServer();

public slots:
    void setOption(QString const& value, QVariant const& value);

signals:
    void optionChanged(QString name, QVariant value);

private:
    QMediaHelixSettingsServerPrivate*   d;
};

}   // ns qtopia_helix

#endif  // __QTOPIA_MEDIASERVER_QMEDIAHELIXSETTINGSSERVER_H
