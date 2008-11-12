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

#ifndef HOMESCREENWIDGETS_H
#define HOMESCREENWIDGETS_H

// Qt includes
#include <QWidget>
#include <QMap>
#include <QString>
#include <QList>

// Qtopia includes
#include <QContent>
#include <qtopiapim/qappointmentmodel.h>
#include <QValueSpaceItem>
#include <QValueSpaceObject>
#include <QWorldmap>
#include <QAnalogClock>

// ============================================================================
//
// HomeScreenWidgets
//
// ============================================================================

class HomeScreenWidgets
{
public:
    static QWidget* create( const QString& name, QWidget* parent = 0 );

    typedef QWidget* (*CreateHSWidgetFunc)(void *);
    static void add( const char* name, CreateHSWidgetFunc function );


private:
    static QMap<QString, CreateHSWidgetFunc> mWidgets;
};

#define HSWIDGET(name, function) \
    static QWidget *_hswidget_install_create_ ## name(void *) { \
        return new function; \
    } \
    struct _hswidget_install_ ## name { \
        _hswidget_install_ ## name() { \
            HomeScreenWidgets::add( \
                # name, \
                _hswidget_install_create_ ## name ); \
        } \
    }; \
    static _hswidget_install_ ## name _hswidget_install_instance_ ## name;

// ============================================================================
//
// LauncherIcon
//
// ============================================================================

class LauncherIcon : public QWidget
{
    Q_OBJECT

public:
    LauncherIcon( const QContent& app, QWidget* parent = 0 );

protected:
    void paintEvent( QPaintEvent* /*event*/ );
    void keyPressEvent( QKeyEvent* event );

public slots:
    void launch();

private:
    const QContent mApp;
};

// ============================================================================
//
// LauncherHSWidget
//
// ============================================================================

class LauncherHSWidget : public QWidget
{
    Q_OBJECT

public:
    LauncherHSWidget( QWidget* parent = 0 );

public slots:
    void launch();

private:
    QList<QContent> mApps;
    QList<LauncherIcon*> mIcons;
};

// ============================================================================
//
// AppointmentsHSWidget
//
// ============================================================================

class AppointmentsHSWidget : public QWidget
{
    Q_OBJECT

public:
    AppointmentsHSWidget( QWidget* parent = 0 );

public slots:
    void showNextAppointment();

private slots:
    void update();

private:
    bool updateModel();
    QString appProgress( const int minutesPast );
    QString appScheduled( const QOccurrence& occurence );

    QValueSpaceObject* mVsObject;
    QValueSpaceItem* mVsItem;
    QUniqueId mUid;
    QDate     mDate;
    QOccurrenceModel* mModel;
};

// ============================================================================
//
// WorldmapHSWidget
//
// ============================================================================

class WorldmapHSWidget : public QWorldmap
{
    Q_OBJECT

public:
    WorldmapHSWidget( QWidget* parent = 0 );

public slots:
    void showCity();

protected:
    void paintEvent( QPaintEvent *event );

private slots:
    void showTZ();

private:
    int mCheck;
};

// ============================================================================
//
// AnalogClockHSWidget
//
// ============================================================================

class AnalogClockHSWidget : public QAnalogClock
{
    Q_OBJECT

public:
    AnalogClockHSWidget( QWidget* parent = 0 );

private slots:
    void update();
};

#endif //HOMESCREENWIDGETS_H
