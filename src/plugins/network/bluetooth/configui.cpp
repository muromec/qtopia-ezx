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

#include "configui.h"

#ifdef QTOPIA_KEYPAD_NAVIGATION
#include <qsoftmenubar.h>
#endif
#include <qtopiaapplication.h>

AdvancedBTPage::AdvancedBTPage( const QtopiaNetworkProperties& cfg, QWidget* parent,
            Qt::WFlags flags )
    : QWidget( parent, flags )
{
    ui.setupUi( this );
    init();
    readConfig( cfg );
    setObjectName("bluetooth-advanced");
}

void AdvancedBTPage::init()
{
#ifdef QTOPIA_KEYPAD_NAVIGATION
    QSoftMenuBar::menuFor( this );
    QSoftMenuBar::setHelpEnabled( this , true );
#endif
    connect( ui.timeout_box, SIGNAL(stateChanged(int)),
            this, SLOT(timeoutSelected(int)) );
}

void AdvancedBTPage::readConfig( const QtopiaNetworkProperties& prop )
{
    QVariant v = prop.value( QLatin1String("Serial/Timeout") );
    if (!v.isValid()) {
        ui.timeout_box->setCheckState(Qt::Checked);
    } else if (v.toString() == "none") {
        ui.timeout_box->setCheckState(Qt::Unchecked);
        ui.timeout->setValue( 120 );
    } else if (v.canConvert(QVariant::Int)) {
        ui.timeout_box->setCheckState(Qt::Checked);
        ui.timeout->setValue( v.toInt() );
    }
    timeoutSelected(ui.timeout_box->checkState());

    QString speed = prop.value( QLatin1String("Serial/Speed") ).toString();
    ui.speed->setCurrentIndex( 0 ); //auto or none
    for (int i=ui.speed->count()-1; i>0; i-- ) {
        if ( ui.speed->itemText(i) == speed ) {
            ui.speed->setCurrentIndex( i );
            break;
        }
    }
}

QtopiaNetworkProperties AdvancedBTPage::properties()
{
    QtopiaNetworkProperties props;
    if (ui.timeout_box->checkState() == Qt::Checked) {
        props.insert("Serial/Timeout", ui.timeout->value());
    } else {
        props.insert("Serial/Timeout", QString("none"));
    }
    props.insert( "Serial/Speed", ui.speed->currentText() );

    return props;
}

void AdvancedBTPage::timeoutSelected( int state )
{
    ui.timeout->setEnabled( state == Qt::Checked );
}

#include "ui_dialingbtbase.h"

DialingBTPage::DialingBTPage( const QtopiaNetworkProperties& cfg, QWidget* parent,
            Qt::WFlags flags )
    : QWidget( parent, flags )
{
    ui.setupUi( this );
    init();
    readConfig( cfg );
    setObjectName("bluetooth-dial");
}

void DialingBTPage::init()
{
    QtopiaApplication::setInputMethodHint( ui.dialString, QtopiaApplication::Text );
    QtopiaApplication::setInputMethodHint( ui.extraDialString, QtopiaApplication::Text );

#ifdef QTOPIA_KEYPAD_NAVIGATION
    QSoftMenuBar::menuFor( this );
    QSoftMenuBar::setHelpEnabled( this , true );
#endif
}

void DialingBTPage::readConfig( const QtopiaNetworkProperties& prop )
{
    QString tmp = prop.value("Serial/DialString").toString();
    if ( !tmp.isEmpty() )
        ui.dialString->setText( tmp );
    tmp = prop.value("Serial/ExtraDialString").toString();
    if ( !tmp.isEmpty() )
        ui.extraDialString->setPlainText( tmp );
}

QtopiaNetworkProperties DialingBTPage::properties()
{
    QtopiaNetworkProperties props;
    props.insert( "Serial/DialString", ui.dialString->text() );
    props.insert( "Serial/ExtraDialString", ui.extraDialString->toPlainText() );

    return props;
}
