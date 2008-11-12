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
#ifndef ALARM_H
#define ALARM_H

#include "ui_alarmbase.h"
#include <qdatetime.h>
#include <QHash>

class QTimer;
class QLabel;
class QDialog;
class QEvent;

class Alarm : public QWidget, Ui::AlarmBase
{
    Q_OBJECT
public:
    Alarm( QWidget *parent=0, Qt::WFlags fl=0 );
    ~Alarm();

    void triggerAlarm(const QDateTime &when, int type);
    bool eventFilter(QObject *o, QEvent *e);
    void setRingPriority(bool);
public slots:
    void setDailyEnabled(bool);

private slots:
    void changeClock( bool );
    void alarmTimeout();
    void applyDailyAlarm();
    void changeAlarmDays();

protected:
    QDateTime nextAlarm( int h, int m );
    QString getAlarmDaysText() const;
    void resetAlarmDaysText();

private:
    QTimer *alarmt;
    bool ampm;
    bool weekStartsMonday;
    int alarmCount;
    bool initEnabled;
    QDialog* alarmDlg;
    QLabel* alarmDlgLabel;
    bool init;
    QHash<int, bool> daysSettings;
};

#endif

