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
#ifndef VOLUMEIMPL_H
#define VOLUMEIMPL_H

#include "ui_volume.h"
#include <QDialog>

#include "qtopiaserverapplication.h"

class VolumeWidget;
class VolumeDialogImplPrivate;

class VolumeDialogImpl : public QDialog
{
    Q_OBJECT
public:
    VolumeDialogImpl( QWidget* parent = 0, Qt::WFlags fl = 0 );

    void setVolume( bool up );

    static const int TIMEOUT = 1500;

signals:
    void volumeChanged( bool up);
    void setText(QString volume);

protected:
    void timerEvent( QTimerEvent *e );

private slots:
    void resetTimer();
    void valueSpaceVolumeChanged();

private:
    int m_tid;
    int m_oldValue;
    VolumeWidget *volumeWidget;
    VolumeDialogImplPrivate  *m_d;
};

#endif

