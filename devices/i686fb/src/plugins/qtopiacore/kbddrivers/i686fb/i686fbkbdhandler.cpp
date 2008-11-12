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

#include "i686fbkbdhandler.h"

#ifdef QT_QWS_I686FB
#include <QFile>
#include <QTextStream>
#include <QScreen>
#include <QSocketNotifier>
#include <QDebug>

#include "qscreen_qws.h"
#include "qwindowsystem_qws.h"
#include "qapplication.h"
#include "qnamespace.h"

#include <qtopialog.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/vt.h>
#include <sys/kd.h>

#define VTACQSIG SIGUSR1
#define VTRELSIG SIGUSR2

static int vtQws = 0;

I686fbKbdHandler::I686fbKbdHandler()
{
    qWarning( "***Loaded I686fb keypad plugin!");
    setObjectName( "I686fb Keyboard Handler" );
    kbdFD = ::open("/dev/tty0", O_RDONLY|O_NDELAY, 0);
    if (kbdFD >= 0) {
      qWarning("Opened tty0 as keyboard input");
      m_notify = new QSocketNotifier( kbdFD, QSocketNotifier::Read, this );
      connect( m_notify, SIGNAL(activated(int)), this, SLOT(readKbdData()));
    } else {
      qWarning("Cannot open tty0 for keypad (%s)", strerror(errno));
      return;
    }

    tcgetattr(kbdFD, &origTermData);
    struct termios termdata;
    tcgetattr(kbdFD, &termdata);

    ioctl(kbdFD, KDSKBMODE, K_MEDIUMRAW);

    termdata.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
    termdata.c_oflag = 0;
    termdata.c_cflag = CREAD | CS8;
    termdata.c_lflag = 0;
    termdata.c_cc[VTIME]=0;
    termdata.c_cc[VMIN]=1;
    cfsetispeed(&termdata, 9600);
    cfsetospeed(&termdata, 9600);
    tcsetattr(kbdFD, TCSANOW, &termdata);

    connect(QApplication::instance(), SIGNAL(unixSignal(int)), this, SLOT(handleTtySwitch(int)));
    QApplication::instance()->watchUnixSignal(VTACQSIG, true);
    QApplication::instance()->watchUnixSignal(VTRELSIG, true);

    struct vt_mode vtMode;
    ioctl(kbdFD, VT_GETMODE, &vtMode);

    // let us control VT switching
    vtMode.mode = VT_PROCESS;
    vtMode.relsig = VTRELSIG;
    vtMode.acqsig = VTACQSIG;
    ioctl(kbdFD, VT_SETMODE, &vtMode);

    struct vt_stat vtStat;
    ioctl(kbdFD, VT_GETSTATE, &vtStat);
    vtQws = vtStat.v_active;
    pressed = false;
}

I686fbKbdHandler::~I686fbKbdHandler()
{
    if (kbdFD >= 0) {
      ioctl(kbdFD, KDSKBMODE, K_XLATE);
      tcsetattr(kbdFD, TCSANOW, &origTermData);
      ::close(kbdFD);
      kbdFD = -1;
    }
}

void I686fbKbdHandler::handleTtySwitch(int sig)
{
    if (sig == VTACQSIG) {
      if (ioctl(kbdFD, VT_RELDISP, VT_ACKACQ) == 0) {
        qwsServer->enablePainting(true);
        qt_screen->restore();
        qwsServer->resumeMouse();
        qwsServer->refresh();
      }
    } else if (sig == VTRELSIG) {
      qwsServer->enablePainting(false);
      qt_screen->save();
      if(ioctl(kbdFD, VT_RELDISP, 1) == 0) {
        qwsServer->suspendMouse();
      } else {
        qwsServer->enablePainting(true);
      }
    }
}

void I686fbKbdHandler::readKbdData()
{
    unsigned char  buf[10];
    unsigned int   key_code=0;
    unsigned int   key_code2=0;
    unsigned int   unicode;
    int            modifiers;
    bool           press;

    int n = read(kbdFD, buf, 2);
    if(n<1)
      return;

    if(n==1) 
      key_code =  (unsigned int)(buf[0] & 0xFF);
    else
      key_code =  (unsigned int)(buf[1] & 0xFF);

    if((key_code & 0x80) == 0)
      press = true;
    else {
      key_code = key_code & 0x7F;
      press = false;
    }  
    unicode = 0xffff;
    modifiers = 0;
    qLog(Input) << "keypressed: code=" << key_code << " (" << (press ? "Down":"Up") << ")";

    switch(key_code)
    {
      case 0x02: key_code2 = Qt::Key_1; unicode  = 0x31; break;
      case 0x03: key_code2 = Qt::Key_2; unicode  = 0x32; break;
      case 0x04: key_code2 = Qt::Key_3; unicode  = 0x33; break;
      case 0x05: key_code2 = Qt::Key_4; unicode  = 0x34; break;
      case 0x06: key_code2 = Qt::Key_5; unicode  = 0x35; break;
      case 0x07: key_code2 = Qt::Key_6; unicode  = 0x36; break;
      case 0x08: key_code2 = Qt::Key_7; unicode  = 0x37; break;
      case 0x09: key_code2 = Qt::Key_8; unicode  = 0x38; break;
      case 0x0a: key_code2 = Qt::Key_9; unicode  = 0x39; break;
      case 0x0b: key_code2 = Qt::Key_0; unicode  = 0x30; break;

      case 0x1E: key_code2 = Qt::Key_A; unicode  = 0x61; break;
      case 0x30: key_code2 = Qt::Key_B; unicode  = 0x62; break;
      case 0x2E: key_code2 = Qt::Key_C; unicode  = 0x63; break;
      case 0x20: key_code2 = Qt::Key_D; unicode  = 0x64; break;
      case 0x12: key_code2 = Qt::Key_E; unicode  = 0x65; break;
      case 0x21: key_code2 = Qt::Key_F; unicode  = 0x66; break;
      case 0x22: key_code2 = Qt::Key_G; unicode  = 0x67; break;
      case 0x23: key_code2 = Qt::Key_H; unicode  = 0x68; break;
      case 0x17: key_code2 = Qt::Key_I; unicode  = 0x69; break;
      case 0x24: key_code2 = Qt::Key_J; unicode  = 0x6A; break;
      case 0x25: key_code2 = Qt::Key_K; unicode  = 0x6B; break;
      case 0x26: key_code2 = Qt::Key_L; unicode  = 0x6C; break;
      case 0x32: key_code2 = Qt::Key_M; unicode  = 0x6D; break;
      case 0x31: key_code2 = Qt::Key_N; unicode  = 0x6E; break;
      case 0x18: key_code2 = Qt::Key_O; unicode  = 0x6F; break;
      case 0x19: key_code2 = Qt::Key_P; unicode  = 0x70; break;
      case 0x10: key_code2 = Qt::Key_Q; unicode  = 0x71; break;
      case 0x13: key_code2 = Qt::Key_R; unicode  = 0x72; break;
      case 0x1F: key_code2 = Qt::Key_S; unicode  = 0x73; break;
      case 0x14: key_code2 = Qt::Key_T; unicode  = 0x74; break;
      case 0x16: key_code2 = Qt::Key_U; unicode  = 0x75; break;
      case 0x2F: key_code2 = Qt::Key_V; unicode  = 0x76; break;
      case 0x11: key_code2 = Qt::Key_W; unicode  = 0x77; break;
      case 0x2D: key_code2 = Qt::Key_X; unicode  = 0x78; break;
      case 0x15: key_code2 = Qt::Key_Y; unicode  = 0x79; break;
      case 0x2C: key_code2 = Qt::Key_Z; unicode  = 0x7A; break;

      case 0x37:
        key_code2 = Qt::Key_Asterisk;
        unicode  = 0x2A;
        break;

      case 0x48:
        key_code2 = Qt::Key_Up;
        unicode = 0xffff;
        break;
      case 0x67:
        key_code2 = Qt::Key_Up;
        unicode = 0xffff;
        break;

      case 0x4D:
        key_code2 = Qt::Key_Right;
        unicode = 0xffff;
        break;
      case 0x6A:
        key_code2 = Qt::Key_Right;
        unicode = 0xffff;
        break;

      case 0x4B:
        key_code2 = Qt::Key_Left;
        unicode = 0xffff;
        break;
      case 0x69:
        key_code2 = Qt::Key_Left;
        unicode = 0xffff;
        break;

      case 0x50:
        key_code2 = Qt::Key_Down;
        unicode = 0xffff;
        break;
      case 0x6C:
        key_code2 = Qt::Key_Down;
        unicode = 0xffff;
        break;

      case 0x3C:
        key_code2 = Qt::Key_Hangup; // F2
        unicode = 0xffff;
        break;

      case 0x3B:
        key_code2 = Qt::Key_Call; // F1
        unicode = 0xffff;
        break;

      case 0x66:
        key_code2 = Qt::Key_Home;
        unicode = 0xffff;
        break;

      case 0x0E:
        key_code2 = Qt::Key_Back;
        unicode = 0xffff;
        break;
      case 0x01:
        key_code2 = Qt::Key_Back;
        unicode = 0xffff;
        break;

      case 0x2B: // '\'
        key_code2 = Qt::Key_Context1;
        unicode = 0xffff;
        break;

      case 0x1C:
        key_code2 = Qt::Key_Select;
        unicode = 0xffff;
        break;
    } 
    if(press && !pressed) {
      // Down
      qLog(Input) << "processKeyEvent(): key=" << key_code2 << ", unicode=" << unicode;
      pressed=true;
      processKeyEvent(unicode, key_code2, (Qt::KeyboardModifiers)modifiers, pressed, false);

      beginAutoRepeat(unicode, key_code2, (Qt::KeyboardModifiers)modifiers);
    } else if(!press) {
      // Released
      qLog(Input) << "processKeyEvent(): key=" << key_code2 << ", unicode=" << unicode;
      pressed=false;
      processKeyEvent(unicode, key_code2, (Qt::KeyboardModifiers)modifiers, pressed, false);
      endAutoRepeat();
    }
}
#endif // QT_QWS_I686FB
