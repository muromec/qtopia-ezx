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

#ifndef CONTENTPLUGINMANAGER_H
#define CONTENTPLUGINMANAGER_H

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

#include <qcontentplugin.h>
#include <QMultiHash>
#include <QList>
#include <QSettings>
#include <qpluginmanager.h>



class DotDesktopContentPlugin : public QContentPlugin
{
public:
    DotDesktopContentPlugin();
    virtual ~DotDesktopContentPlugin();

    virtual QStringList keys() const;

    virtual bool installContent( const QString &filePath, QContent *content );
    virtual bool updateContent( QContent *content );
};


class ContentPluginManager
{
public:
    ContentPluginManager();
    ~ContentPluginManager();

    QList< QContentPlugin * > findPlugins( const QString &type );

private:
    QPluginManager manager;

    QMultiHash< QString, QContentPlugin * > typePluginMap;

    QList< QContentPlugin * > plugins;
};

class QContentFactory
{
public:
    static bool installContent( const QString &fileName, QContent *content );
    static bool updateContent( QContent *content );
};


#endif
