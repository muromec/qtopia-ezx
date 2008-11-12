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

#ifndef _TIMEUPDATESERVICE_H_
#define _TIMEUPDATESERVICE_H_

#include <QtopiaAbstractService>
#include <QTimeZone>
#include <QBasicTimer>

class QMessageBox;

class QValueSpaceItem;

class TimeUpdater : public QObject {
    Q_OBJECT
public:
    TimeUpdater(int timeoffset, const QTimeZone& tz, bool chtime, bool chzone);
    void ask();

    void commitUpdate();

protected:
    void timerEvent(QTimerEvent*);

signals:
    void changeSystemTime(uint time, QString newtz);

private slots:
    void userCommit(int);

private:
    QMessageBox *prompt;
    QBasicTimer timer;
    int offset;
    QTimeZone zone;
    bool updatetime;
    bool updatezone;
};

class TimeUpdateService : public QtopiaAbstractService
{
    Q_OBJECT
public:
    TimeUpdateService();

public slots:
    void updateFromExternalSources();
    void updateFromExternalSources(bool autotz, bool autotm, bool ask_tz, bool ask_time);
    void storeExternalSource(QString, uint time, int tzoffset, int dstoffset);
    void changeSystemTime(uint time, QString newtz);

private:
    int lasttz;
    QString externalTimeZone;
    uint externalTimeTimeStamp;
    uint externalTime;
    int externalDstOffset;
    TimeUpdater *updater;
};

#endif // _TIMEUPDATESERVICE_H_

