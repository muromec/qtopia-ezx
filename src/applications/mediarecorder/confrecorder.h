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

#ifndef CONFRECORDER_H
#define CONFRECORDER_H

#include <qdialog.h>

#include "ui_confrecorderbase.h"

struct QualitySetting;
class MediaRecorderPluginList;


class ConfigureRecorder : public QDialog
{
    Q_OBJECT

public:
    ConfigureRecorder( QualitySetting *qualities, MediaRecorderPluginList *plugins, QWidget *parent = 0, Qt::WFlags f = 0 );
    ~ConfigureRecorder();

public:
    int currentQuality() const { return quality; }

    void processPopup();

public slots:
    void setQuality( int index );
    void saveConfig();

protected slots:
    void setChannels( int index );
    void setSampleRate( int index );
    void setFormat( int index );

private:
    void updateConfig( int channels, int frequency, const QString& mimeType, const QString& formatTag );
    void loadConfig();

private:
    Ui::ConfigureRecorderBase *conf;
    QualitySetting *qualities;
    MediaRecorderPluginList *plugins;
    int quality;
};

#endif
