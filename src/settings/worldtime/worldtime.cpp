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

// Qt4 Headers
#include <QGridLayout>
#include <QTimerEvent>
#include <QTimer>
#include <QMenu>
#include <QSettings>
#include <QDesktopWidget>
#include <QDebug>
#include <QPainter>

// Local includes
#include "worldtime.h"

// Qtopia includes
#include <qtopiaapplication.h>
#include <QWorldmap>

#include <qtimestring.h>
#include <qtimezone.h>
#include <qtimezoneselector.h>
#include <qtopialog.h>
#ifndef QTOPIA_HOST
#include <qtopiaipcenvelope.h>
#include <qtopiaipcadaptor.h>
#endif
#ifdef QTOPIA_PHONE
#include <qsoftmenubar.h>
#include <QAction>
#endif

WorldTime::WorldTime( QWidget *parent,
                      Qt::WFlags fl )
   : QWidget( parent, fl )
 {
#ifdef Q_WS_QWS
   setWindowTitle(tr("World Time"));
#endif
    while (listCities.count())
    { delete listCities.takeLast(); }
    while (listTimes.count())
    { delete listTimes.takeLast(); }

     float dpi = QApplication::desktop()->screen()->logicalDpiY();
     QFont font = QApplication::font();
//     qWarning()<<"DPI:"<<dpi;

     if(dpi < 100) {
         mMode = Minimal;
         setFont(font);
         maxVisibleZones = 3;
     } else if(dpi < 200) {
         mMode = Medium;
         font.setPointSize(font.pointSize() - 1);
         setFont(font);
         maxVisibleZones = 5;
     } else {
         mMode = Tall;
         maxVisibleZones = 4;
     }

   isEditMode = false;

   int columns/*,rows*/;
   columns = 3;/*rows = 3;*/


   // Don't need a select softkey for touchscreen phones as it does nothing
   if ( Qtopia::mousePreferred() )
      QSoftMenuBar::setLabel( this, Qt::Key_Select, QSoftMenuBar::NoLabel );

   // first, work out current number of columns?
   gl = new QGridLayout(this);
   frmMap = new QWorldmap(this);
   QSizePolicy sp = frmMap->sizePolicy();

   if ( qApp->desktop()->width() < qApp->desktop()->height() ) {
       sp.setHeightForWidth(true);
   }

   frmMap->setSizePolicy(sp);
   gl->addWidget(frmMap, 0, 0, 0, columns, Qt::Alignment(Qt::AlignTop));

   for(int i=0; i < maxVisibleZones + 1; i++) {
       gl->setRowStretch(i, 1);
   }

   gl->setColumnStretch(1,4);
   int goodWidth = qApp->desktop()->width() / 2 + 5;

   for (int i = 0; i < maxVisibleZones; i++) {

       listCities.append(new QPushButton(this));
       listCities.at(i)->setFixedWidth(goodWidth );
       if(mMode == Medium)  listCities.at(i)->setMaximumHeight(20);

       listCities.at(i)->setCheckable(true);

       connect(listCities.at(i), SIGNAL(clicked()),
               this, SLOT(slotSetZone()));

       listTimes.append(new CityInfo(this));

       gl->addWidget( listCities.at(i), i + 3, 0 );
       gl->addWidget( listTimes.at(i), i + 3, 1, Qt::Alignment(Qt::AlignVCenter) | Qt::AlignHCenter);
   }


   QMenu *contextMenu = QSoftMenuBar::menuFor(this);

   QAction *a = new QAction(QIcon(":icon/edit"),
                            tr("Select City"), this);
   connect(a, SIGNAL(triggered()), this, SLOT(beginNewTz()));
   contextMenu->addAction(a);
   contextMenu->addSeparator();

   gl->setSpacing(4);
   gl->setMargin(4);

   readInTimes();
   changed = false;
   QObject::connect( qApp, SIGNAL(clockChanged(bool)),
                     this, SLOT(showTime()));

   // now start the timer so we can update the time quickly every second
   timerEvent( 0 );

   frmMap->setContinuousSelect(true);

   connect( frmMap, SIGNAL(newZone(QTimeZone) ),
            this, SLOT( slotNewTz(QTimeZone)));

   if( Qtopia::mousePreferred()) {
       connect( frmMap, SIGNAL(buttonSelected()),
             this, SLOT(selected()));
   }
   connect( frmMap, SIGNAL(selectZoneCanceled()),
            this, SLOT(slotNewTzCancelled()));

   frmMap->setFocus();
   QTimer::singleShot(0,this,SLOT(slotNewTzCancelled()));
}

WorldTime::~WorldTime()
{
}

void WorldTime::saveChanges()
{
    if (changed) {
        writeTimezoneChanges();
        for (int i = 0;  i < maxVisibleZones; i++) {
            if(listCities.at(i)->isChecked()) {
                listCities.at(i)->setChecked(false);
            }
            listCities.at(i)->setEnabled( true);
        }

        readInTimes();
    }
}

void WorldTime::cancelChanges()
{
    viewMode();
    if(changed)
          for (int i = 0;  i < maxVisibleZones; i++) {
            if(listCities.at(i)->isChecked()) {
                listCities.at(i)->setChecked(false);
            }
        }

    changed = false;
}

void WorldTime::writeTimezoneChanges()
{
    changed = true;
    QSettings cfg("Trolltech", "WorldTime");
    cfg.beginGroup("TimeZones");

    int i;
    for ( i = 0;  i < maxVisibleZones; i++) {
        if ( !strCityTz[i].isNull() ) {
            cfg.setValue("Zone"+QString::number(i), strCityTz[i]);
            // qWarning()<<QString::number(i) << strCityTz[i];
        }
    }

    cfg.sync();

    emit timeZoneListChange();

    viewMode();
    changed = false;
 }

void WorldTime::timerEvent( QTimerEvent *)
{
   if ( timerId ){
      killTimer( timerId );
      timerId = 0;
   }
   // change the time again!!
   showTime();
   int ms = 1000 - QTime::currentTime().msec();
   ms += (60-QTime::currentTime().second())*1000;
   timerId = startTimer( ms );
}

void WorldTime::showTime( void )
{
   QDateTime curUtcTime = QTimeZone::utcDateTime();

   for (int i=0; i< maxVisibleZones; i++)
      listTimes.at(i)->setUtcTime(curUtcTime);
}

void WorldTime::slotSetZone()
{
    QPushButton *sendButton = qobject_cast<QPushButton *>(sender());
    if(! sendButton->isChecked()) {
        sendButton->setChecked(true);
        return;
    }
    int selButton = findCurrentButton();

    QTimeZone zone( strCityTz[selButton].toLocal8Bit());
    frmMap->setZone(zone);
    editMode();
    changed = true;
    frmMap->selectNewZone();

    setButtonAvailable(selButton);

    listTimes.at( selButton )->setZone( zone.id());
}

void WorldTime::editMode()
{
    setWindowTitle(tr("Select City"));
    frmMap->setFocus();
    isEditMode = true;
    changed = true;
     for ( int i=0; i < maxVisibleZones; i++ ) {
         listCities.at(i)->setFocusPolicy( Qt::NoFocus );
     }
    if( !frmMap->isZoom())
         frmMap->toggleZoom();
}


void WorldTime::viewMode()
{
    setWindowTitle(tr("World Time"));
    isEditMode = false;
    for ( int i=0; i < maxVisibleZones; i++ ) {
        listCities.at(i)->setFocusPolicy( Qt::StrongFocus );
    }
    if( frmMap->isZoom())
        frmMap->toggleZoom();

    setButtonAvailable( -1);
}

int WorldTime::findCurrentButton()
{
    for (int i = 0; i < maxVisibleZones; i++) {
        if(listCities.at(i)->isChecked()) {
             return i;
        }
    }
    return -2;
}

void WorldTime::setButtonAvailable(int selButton)
{
    if(selButton == -2) return;

    for ( int i=0; i < maxVisibleZones; i++ ) {
        if (selButton == -1) {
            listCities.at(i)->setEnabled( true);
        } else if(i == selButton) {
            listCities.at(i)->setEnabled( true);
        } else {
            listCities.at(i)->setEnabled( false);
        }
    }
}

void WorldTime::beginNewTz()
{
    changed = false;

    QString selectedTz;
    int selButton = findCurrentButton();
    if(selButton > 0) {
        if(! listCities.at(selButton)->isChecked())
            listCities.at(selButton)->setChecked(true);
        return;
    }

    if(selButton == -2) {
        selButton = 0;
        listCities.at(0)->setChecked(true);
        setButtonAvailable(0);
    }

    else if(selButton != -1 ) {
        selectedTz = strCityTz[selButton];
    }


    if(selectedTz.isEmpty()) {
        selectedTz = strCityTz[0];
        listCities.at(0)->setChecked(true);
    }

    frmMap->setZone( QTimeZone( selectedTz.toLocal8Bit() ) );
     frmMap->setFocus();
     editMode();
     frmMap->selectNewZone();

}

void WorldTime::slotNewTz( const QTimeZone& zone )
{
    if( Qtopia::mousePreferred()) {
        changed = true;
        return;
    }
    QTimeZone curZone;
    int selButton = findCurrentButton();

    if(selButton > -1 ) {
        strCityTz[selButton] = zone.id();
//        qWarning()<< "slotNewTz"<<strCityTz[selButton];
        listCities.at(selButton)->setText( zone.city());
        listTimes.at(selButton)->setZone( zone.id());

        if( !isEditMode) {
            changed = true;
        }
    }
}

void WorldTime::slotNewTzCancelled()
{
   QString currTz;
   if(isEditMode) {
       int selButton = findCurrentButton();
       currTz = strCityTz[selButton];
       frmMap->setZone( QTimeZone( currTz.toLocal8Bit() ) );
       slotNewTz( QTimeZone( currTz.toLocal8Bit() ) );
           if( listCities.at(selButton)->isChecked())
            listCities.at(selButton)->setChecked( false);

   }

   cancelChanges();
   readInTimes();
}

void WorldTime::readInTimes( )
{
   QSettings cfg("Trolltech", "WorldTime");
   cfg.beginGroup("TimeZones");

   int i;
   QString zn;

//create zoneslist
   for (i = 0; i < maxVisibleZones; i++ ) {
       zn = cfg.value("Zone" + QString::number(i), QString(i)).toString();
       strCityTz[i] = zn;

       if ( zn.isEmpty() )
           break;
       QString nm =  zn.section("/",-1) ;
       nm = nm.replace("_"," ");
       strCityTz[i] = zn;

       listCities.at(i)->setText(nm);

//       int index = cfg.value("Clock"+QString::number(i), QString::number(i)).toInt();
       zn = cfg.value("Zone" + QString::number(i), QString(i)).toString();
       listTimes.at(i)->setZone(zn);
   }

}

void WorldTime::keyReleaseEvent( QKeyEvent *ke )
{
    switch(ke->key())  {
    case  Qt::Key_Select:
        if( isEditMode) {
            selected();
        }
        break;
    };
}

void WorldTime::selected()
{

    if(!changed){
        changed = true;
    } else {
        QTimeZone zone = frmMap->zone();

        int selButton = findCurrentButton();
        if(selButton != -1 ) {
            strCityTz[selButton] = zone.id();
            //   qWarning() << "selected" << strCityTz[selButton] << zone.id();
            listCities.at(selButton)->setText( zone.city());
            listTimes.at(selButton)->setZone( zone.id());

            saveChanges();
             changed = false;
        }
        viewMode();
    }
}

