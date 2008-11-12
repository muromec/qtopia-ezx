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

#ifndef PHOTOTIMER_H
#define PHOTOTIMER_H

#include <QWidget>
#include <QDialog>

class QSpinBox;
class QLCDNumber;
class QAnalogClock;

class PhotoTimer : public QWidget
{
    Q_OBJECT

public:
    PhotoTimer( int timeout,
                int number,
                int interval,
                QWidget* parent = 0,
                Qt::WFlags f = 0 );
signals:
    void takePhoto();

private slots:
    void timeout();

private:
    int mTimeout;
    int mNumber;
    int mInterval;

    QAnalogClock* mClock;
};

class PhotoTimerDialog : public QDialog
{
    Q_OBJECT

public:
    PhotoTimerDialog( QWidget* parent = 0, Qt::WFlags f = 0 );

    int timeout() const;
    int number() const;
    int interval() const;

private slots:
    void timeoutChanged( int timeout );
    void numberChanged( int number );
    void intervalChanged( int interval );

private:
    int mTimeout;
    int mNumber;
    int mInterval;

    QSpinBox* mIntervalSpin;
};

#endif //PHOTOTIMER_H
