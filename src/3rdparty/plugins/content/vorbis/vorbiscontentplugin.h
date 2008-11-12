/****************************************************************************
**
** Copyright (C) 2007-2008 TROLLTECH ASA. All rights reserved.
**
** This file is part of the Opensource Edition of the Qtopia Toolkit.
**
** This software is licensed under the terms of the GNU General Public
** License (GPL) version 2.
**
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <qcontentplugin.h>
#include <qtopiaglobal.h>
#include <qtopiamedia/media.h>
#include <QMediaDecoder>

class QMediaDevice;

extern "C"
{
#include "ivorbiscodec.h"
#include "ivorbisfile.h"
}

/**
     callbacks from vorbisfile
     */

extern "C" {
    extern size_t  fread_func  (void *ptr,size_t size,size_t nmemb, void *stream);
    extern int     fseek_func  (void *stream, ogg_int64_t offset, int whence);
    extern int     fclose_func (void *stream);
    extern long    ftell_func  (void *stream);
}


class QTOPIA_PLUGIN_EXPORT VorbisContentPlugin  : public QObject, public QContentPlugin 
{
    Q_OBJECT
    Q_INTERFACES(QContentPlugin)
public:
    VorbisContentPlugin();
    ~VorbisContentPlugin();

    virtual QStringList keys() const;
    virtual bool installContent( const QString &filePath, QContent *content ); 
    virtual bool updateContent( QContent *content);


};
    

