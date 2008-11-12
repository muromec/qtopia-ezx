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

#ifndef CHANGELISTENER_H
#define CHANGELISTENER_H

#include <QLabel>
#include <QContent>
#include <QFileInfo>

class ChangeListener : public QLabel
{
    Q_OBJECT

public:
    ChangeListener( QWidget *parent = 0, Qt::WindowFlags flags = 0 );
    ~ChangeListener();

private slots:
    void timeout();
    void changed( const QContentIdList &contentIds, QContent::ChangeType type );

private:
    int nextIndex;
    QContentId lastContentId;
    QString categoryId;
    QFileInfoList imageFiles;
};

#endif // CHANGELISTENER_H
