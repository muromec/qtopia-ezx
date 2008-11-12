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
#include "popupimpl.h"
#include "popupim.h"
#include <qpixmap.h>
#include <qapplication.h>

/*
   Constructs the PopupIMImpl.
*/
PopupIMImpl::PopupIMImpl()
    : input(0), icn(0), ref(0)
{
}

/*
   Destroys the PopupIMImpl
*/
PopupIMImpl::~PopupIMImpl()
{
    if (input)
	delete input;
    if (icn)
	delete icn;
}

/*
   If uuid is a valid id for this plugin, sets \a iface to point to the
   Input Method provided and returns QS_OK.  Otherwise sets iface to 0
   and returns QS_FALSE
*/
QRESULT PopupIMImpl::queryInterface( const QUuid &uuid, QUnknownInterface **iface )
{
    *iface = 0;
    if ( uuid == IID_QUnknown )
        *iface = this;
    else if ( uuid == IID_InputMethod )
        *iface = this;
    else
	return QS_FALSE;

    (*iface)->addRef();
    return QS_OK;
}

/*
   Returns the widget used in the Popup Input Method.
   If the widget is not yet created, constructs the input method widget as a
   child of \a parent with flags \a f.  Otherwise returns a pointer
    to the previously created input method widget.
*/
QWidget *PopupIMImpl::inputMethod( QWidget *parent, Qt::WFlags f )
{
    if ( !input )
	input = new PopupIM( parent, "SimpleInput", f );
    return input;
}

/*
   Returns the name of the input method plugin
*/
QString PopupIMImpl::name()
{
    return qApp->translate( "InputMethods", "SimpleInput" );
}

/*
   Returns the icon for the input method plugin
*/
QPixmap *PopupIMImpl::icon()
{
    /* XPM */
    static const char * pix_xpm[] = {
	"16 13 3 1",
	" 	c #FFFFFFFFFFFF",
	"#	c #000000000000",
	".	c #FFFFFFFFFFFF",
	"                ",
	"      ####      ",
	"    ##....##    ",
	"   #........#   ",
	"   #..####..#   ",
	"  #...#...#..#  ",
	"  #...#...#..#  ",
	"  #...####...#  ",
	"  #...#......#  ",
	"   #..#.....#   ",
	"   #........#   ",
	"    ##....##    ",
	"      ####      "};

    if ( !icn )
	icn = new QPixmap( (const char **)pix_xpm );
    return icn;
}

/*
   Connects the signal for key press events from the popup input method
   to the slot provided by Qtopia server.
*/
void PopupIMImpl::onKeyPress( QObject *receiver, const char *slot )
{
    if ( input )
        QObject::connect( input, SIGNAL(keyPress(ushort,ushort,ushort,bool,bool)), receiver, slot );
}

/*
   Exports the interface so can be loaded by Qtopia
*/
Q_EXPORT_INTERFACE()
{
    Q_CREATE_INSTANCE( PopupIMImpl )
}
