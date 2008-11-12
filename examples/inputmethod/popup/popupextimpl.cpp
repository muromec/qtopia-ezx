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
#include "popupextimpl.h"
#include "popupim.h"
#include <qpixmap.h>
#include <qapplication.h>
#include <qwindowsystem_qws.h>

/*
   Constructs the PopupIMExtImpl
*/
PopupIMExtImpl::PopupIMExtImpl()
    : input(0), icn(0), ref(0)
{
}

/*
   Destroys the PopupIMExtImpl
*/
PopupIMExtImpl::~PopupIMExtImpl()
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
QRESULT PopupIMExtImpl::queryInterface( const QUuid &uuid, QUnknownInterface **iface )
{
    *iface = 0;
    if ( uuid == IID_QUnknown )
        *iface = this;
    else if ( uuid == IID_ExtInputMethod )
        *iface = this;
    else
	return QS_FALSE;

    (*iface)->addRef();
    return QS_OK;
}

/*
   Returns the name of the input method plugin
*/
QString PopupIMExtImpl::name()
{
    return qApp->translate( "InputMethods", "SimpleInput" );
}

/*
   Returns the icon for the input method plugin
*/
QPixmap *PopupIMExtImpl::icon()
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
   Returns the widget used in the Popup Input Method.
   If the widget is not yet created, constructs the input method widget as a
   child of \a parent with flags \a f.  Otherwise returns a pointer
    to the previously created input method widget.
*/
QWidget *PopupIMExtImpl::keyboardWidget( QWidget *parent, Qt::WFlags f)
{
    if (!input) {
	input = new PopupIM( parent, "SimpleInput", f );
	connect(input, SIGNAL(keyPress(ushort,ushort,ushort,bool,bool)),
		this, SLOT(sendKeyEvent(ushort,ushort,ushort,bool,bool)));
    }
    return input;
}

/*
   Sends a key event to the server.  It is possible to have the Input Method
   widget call the QWSServer::sendKeyEvent rather than emit a singal.
*/
void PopupIMExtImpl::sendKeyEvent(ushort u, ushort k, ushort m, bool p, bool r)
{
    qwsServer->sendKeyEvent(u, k, m, p, r);
}

/*
   Exports the interface so can be loaded by Qtopia
*/
Q_EXPORT_INTERFACE()
{
    Q_CREATE_INSTANCE( PopupIMExtImpl )
}
