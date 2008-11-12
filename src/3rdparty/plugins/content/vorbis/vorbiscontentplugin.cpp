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

#include "vorbiscontentplugin.h"
#include <qmimetype.h>
#include <QTextStream>
#include <QFileInfo>
#include <stdlib.h>

// copy from oggdecoder.cpp

size_t fread_func(void *ptr, size_t size, size_t nmemb, void *stream) {
    QFile *p = (QFile*)stream;
    return p->read((char*)ptr, size*nmemb);
}

int fseek_func(void *stream, ogg_int64_t offset, int whence) {
    QFile *p = (QFile*)stream;

    bool ret;

    if (whence==SEEK_SET) {
        //qWarning("fseek() SET %ld pos=%ld",(long)offset,(long)p->pos());
        ret = p->seek(offset);
        if(!ret) return -1;
        return 0;
    }
    if (whence==SEEK_CUR) {
        //qWarning("fseek() CUR %ld pos=%ld",(long)(p->pos()+offset),(long)p->pos());
        ret = p->seek(p->pos()+offset);
        if(!ret) return -1;
        return 0;
    }
    if (whence==SEEK_END) {
        //qWarning("fseek() END %ld pos=%ld",(long)p->dataType().dataSize,(long)p->pos());
        if(p->size() <= 0) return -1;
        ret = p->seek(p->size());
        if(!ret) {
            //qWarning("FAILED seek EOF");
            return -1;
        }
        return 0;
    }
    ret = p->seek(offset);
    if(!ret) {
        //qWarning("FAILED fseek() to %ld",(long)offset);
        return -1;
    } else {
        //qWarning("fseek() to %ld",(long)offset);
        return 0;
    }
    return -1;
}

int fclose_func (void * /*stream*/) {
    return 0;
}

long ftell_func  (void *stream) {
    QFile *p = (QFile*)stream;
    //qWarning("ftell_func() %ld",(long)p->pos());
    return (long)p->pos();
}


VorbisContentPlugin::VorbisContentPlugin()
{
}

VorbisContentPlugin::~VorbisContentPlugin()
{
}

QStringList VorbisContentPlugin::keys() const
{
    printf("keys\n");
    QStringList fileExtensions;
    fileExtensions << "ogg";

    return  fileExtensions;
}

bool VorbisContentPlugin::installContent( const QString &filePath, QContent *content)
{
    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly))
    {
        qint32 ftypSize = 0;
        QString sig;

        content->setName( QFileInfo(filePath).baseName());
        content->setFile( filePath);
        content->setRole( QContent::Document);

        content->setType( QString("audio/ogg+vorbis"));

        vorbis_info *vi=NULL;
        OggVorbis_File      vf;
        
        ov_callbacks callbacks;

        callbacks.read_func = fread_func;
        callbacks.seek_func = fseek_func;
        callbacks.close_func = fclose_func;
        callbacks.tell_func = ftell_func;

        if(ov_open_callbacks(&file,&vf,NULL,0,callbacks) < 0) {
            return -1;
        } 

        char **ptr1=ov_comment(&vf,-1)->user_comments;
        while(*ptr1) {
          QString tag = QString(*ptr1);
          QStringList nameValue = tag.split("=");
          tag = nameValue[0];
          tag = tag.toLower();
          QString value = nameValue[1];

          if       (tag == "title") 
             content->setName( value );
          else if  (tag == "artist")
             content->setProperty(QContent::Artist,  value);
          else if  (tag == "album")
              content->setProperty(QContent::Album,  value);
          else if  (tag == "tracknumber")
              content->setProperty( QContent::Track, value);
          ++ptr1;
        }


    }
    
    file.close();

    return 0;
}

bool VorbisContentPlugin::updateContent( QContent *content)
{
    return installContent(content->fileName(), content);
}


QTOPIA_EXPORT_PLUGIN(VorbisContentPlugin);

