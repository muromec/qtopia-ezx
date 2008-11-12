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

#include "qcontentplugin.h"
#include "contentpluginmanager_p.h"
#include "drmcontent_p.h"
#include <QtDebug>
#include <qcategorymanager.h>


/*!
    \class QContentPlugin
    \mainclass
    \brief The QContentPlugin class provide an interface for the Qtopia Document System to discover detailed information about a file.

    Content plug-ins are used by the document system to read meta-data from files that may be used to index the content or better
    describe it to the user.  When a new file is first discovered by the document system its extension is used to identify
    possible plug-ins that may be able to identify the file. If any such plug-in is found the plug-in's installContent() method
    is called with the file name and the destination QContent as arguments.  The document system will keep trying potential
    content plug-ins until one is found that identifies itself as having successfully processed the file by returning true.

    The content plug-ins are also invoked when a content record is found to be out of date, but in this case the updateContent()
    method of the plug-in is called.  A content record is determined to be out of date when the backing file's last modified date
    is more recent than the content records last update date.

  \ingroup content
  \ingroup plugins
*/

/*!
    Destroys a QContentPlugin.
*/
QContentPlugin::~QContentPlugin()
{
}

/*!
    \fn QContentPlugin::keys() const

    Returns a list of the file extensions the QContentPlugin instance can report on.
*/

/*!
    \fn QContentPlugin::installContent( const QString &fileName, QContent *content )

    Populates \a content with data from the file with the file name \a fileName.  Returns true if the content
    plug-in successfully populated the QContent.

    Installation is only performed when the content is first identified by the content system, if the file changes
    after installation updateContent() will be called to ensure the content data is up to date; otherwise returns
    false
*/

/*!
    Refreshes the content data of \a content following a change to the file it references.

    Returns true if the content data has been ensured to be up to date; otherwise returns false.
*/
bool QContentPlugin::updateContent( QContent *content )
{
    Q_UNUSED( content );

    return false;
}
