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
#include "speeddialedit.h"

#include <qtopiaapplication.h>
#include <qsoftmenubar.h>

#include <qtranslatablesettings.h>

#include <QVBoxLayout>


QSpeedDialEdit::QSpeedDialEdit( QWidget* parent, Qt::WFlags fl )
:   QDialog( parent, fl ),
    origEmpty(),
    origSet()
{
    setWindowTitle(tr("Speed Dial"));

    QVBoxLayout* vbLayout = new QVBoxLayout(this);
    vbLayout->setMargin(0);
    vbLayout->setSpacing(0);

    list = new QSpeedDialList(this);
    list->setFrameStyle(QFrame::NoFrame);
    vbLayout->addWidget(list);

    // Store original settings
    for ( int i=0; i<list->count(); ++i ) {
        QString input = list->rowInput( i );
        QtopiaServiceDescription* desc = QSpeedDial::find( input );
        if ( desc != 0 )
            origSet[input] = *desc;
        else
            origEmpty.append( input );
    }
}

QSpeedDialEdit::~QSpeedDialEdit()
{
}

void QSpeedDialEdit::reject()
{
    QMap<QString, QtopiaServiceDescription>::const_iterator setCit;
    for ( setCit = origSet.begin(); setCit != origSet.end(); ++setCit ) {
        QSpeedDial::set( setCit.key(), setCit.value() );
    }

    QList<QString>::const_iterator emptyCit;
    for ( emptyCit = origEmpty.begin(); emptyCit != origEmpty.end(); ++emptyCit ) {
        if ( QSpeedDial::find( *emptyCit ) != 0 )
            QSpeedDial::remove( *emptyCit );
    }

    QDialog::reject();
}

