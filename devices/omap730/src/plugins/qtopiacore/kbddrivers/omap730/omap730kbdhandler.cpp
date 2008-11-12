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

#include "omap730kbdhandler.h"

#ifdef QT_QWS_OMAP730
#include <QFile>
#include <QTextStream>
#include <QScreen>
#include <QSocketNotifier>
#include <QDebug>

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

struct Omap730Input {
    unsigned int   dummy1;
    unsigned int   dummy2;
    unsigned short type;
    unsigned short code;
    unsigned int   value;
};

Omap730KbdHandler::Omap730KbdHandler()
{
    qWarning( "***Loaded Omap730 keypad plugin!");
    setObjectName( "Omap730 Keypad Handler" );
    kbdFD = ::open("/dev/input/event0", O_RDONLY, 0);
    if (kbdFD >= 0) {
      qWarning("Opened event0 as keypad input");
      m_notify = new QSocketNotifier( kbdFD, QSocketNotifier::Read, this );
      connect( m_notify, SIGNAL(activated(int)), this, SLOT(readKbdData()));
    } else {
      qWarning("Cannot open event0 for keypad (%s)", strerror(errno));
      return;
    }
    shift=false;
}

Omap730KbdHandler::~Omap730KbdHandler()
{
}

void Omap730KbdHandler::readKbdData()
{
    Omap730Input event;

    int n = read(kbdFD, &event, sizeof(Omap730Input));
    if(n !=16) {
      qWarning("keypressed: n=%03d",n);
      return;
    }  

    qWarning("keypressed: type=%03d, code=%03d, value=%03d (%s)",event.type, event.code,event.value,((event.value)!=0) ? "Down":"Up");

    int unicode=0;
    int key_code=0;

    switch(event.value)
    {
      case 0x2000000:
        key_code = Qt::Key_1;
        unicode  = 0x31;
        break;
      case 0x80000:
        key_code = Qt::Key_2;
        unicode  = 0x32;
        break;
      case 0x2000:
        key_code = Qt::Key_3;
        unicode  = 0x33;
        break;
      case 0x4000000:
        key_code = Qt::Key_4;
        unicode  = 0x34;
        break;
      case 0x100000:
        key_code = Qt::Key_5;
        unicode  = 0x35;
        break;
      case 0x4000:
        key_code = Qt::Key_6;
        unicode  = 0x36;
        break;
      case 0x8000000:
        key_code = Qt::Key_7;
        unicode  = 0x37;
        break;
      case 0x200000:
        key_code = Qt::Key_8;
        unicode  = 0x38;
        break;
      case 0x8000:
        key_code = Qt::Key_9;
        unicode  = 0x39;
        break;
      case 0x10000000:
        key_code = Qt::Key_Asterisk;
        unicode = 0x2A;
        break;
      case 0x400000:
        key_code = Qt::Key_0;
        unicode = 0x30;
        break;
      case 0x10000:
        key_code = Qt::Key_NumberSign;
        unicode  = 0x23;
        break;
      case 0x80:
        key_code = Qt::Key_Call; 
        unicode  = 0xffff; 
        break;
      case 0x100:
        key_code = Qt::Key_Hangup;
        unicode  = 0xffff; 
        break;
      case 0x1000:
        key_code = Qt::Key_Context1; 
        unicode  = 0xffff; 
        break;
      case 0x40:
        key_code = Qt::Key_Back;
        unicode  = 0xffff; 
        break;
      case 0x40000:
        key_code = Qt::Key_Back;
        unicode  = 0xffff;
        break;
      case 0x1:
        key_code = Qt::Key_Up;
        unicode  = 0xffff;
        break;
      case 0x8:
        key_code = Qt::Key_Down;
        unicode  = 0xffff;
        break;
      case 0x4:
        key_code = Qt::Key_Left;
        unicode  = 0xffff;
        break;
      case 0x2:
        key_code = Qt::Key_Right;
        unicode  = 0xffff;
        break;
      case 0x10:
        key_code = Qt::Key_Select;
        unicode  = 0xffff;
        break;
      case 0x1000000:
        key_code = Qt::Key_F10;
        unicode  = 0xffff;
        break;
    }
    if(event.value!=0) {
      processKeyEvent(unicode, key_code, 0, event.value!=0, false);
      beginAutoRepeat(unicode, key_code, 0);
      prev_key = key_code;
      prev_unicode = unicode;
    } else {
      processKeyEvent(prev_unicode, prev_key, 0, event.value!=0, false);
      endAutoRepeat();
    }
}

#endif // QT_QWS_OMAP730
