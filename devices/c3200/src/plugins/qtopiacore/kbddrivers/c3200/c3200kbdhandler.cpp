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

#include "c3200kbdhandler.h"

#ifdef QT_QWS_C3200
#include <QFile>
#include <QTextStream>
#include <QScreen>
#include <QSocketNotifier>
#include <QDebug>
#include <QObject>
#include <qtopialog.h>

#include "qscreen_qws.h"
#include "qwindowsystem_qws.h"
#include "qapplication.h"
#include "qnamespace.h"
//#include "qperformancelog.h"

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

C3200KbdHandler::C3200KbdHandler()
{
    qWarning( "***Loaded C3200 keypad plugin!");
    setObjectName( "C3200 Keypad Handler" );
    kbdFD = ::open("/dev/tty0", O_RDONLY | O_NDELAY, 0);
    if (kbdFD >= 0) {
      qWarning("Opened tty0 as keypad input");
      m_notify = new QSocketNotifier( kbdFD, QSocketNotifier::Read, this );
      connect( m_notify, SIGNAL(activated(int)), this, SLOT(readKbdData()));
    } else {
      qWarning("Cannot open tty0 for keypad (%s)", strerror(errno));
      return;
    }
    tcgetattr(kbdFD, &origTermData);
    struct termios termdata;
    tcgetattr(kbdFD, &termdata);

    ioctl(kbdFD, KDSKBMODE, K_RAW);

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

    shift=false;
    fn=false;
    ctrl=false;
    state=false;
    prevkey=0;
}

C3200KbdHandler::~C3200KbdHandler()
{
    if (kbdFD >= 0)
    {
        ioctl(kbdFD, KDSKBMODE, K_XLATE);
        tcsetattr(kbdFD, TCSANOW, &origTermData);
        ::close(kbdFD);
    }
}

void C3200KbdHandler::handleTtySwitch(int sig)
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

void C3200KbdHandler::readKbdData()
{
    unsigned char           buf[80];
    unsigned short          driverKeyCode;
    unsigned short          unicode;
    unsigned int            qtKeyCode;
    bool                    isPressed, control=false;
    Qt::KeyboardModifiers   modifiers = Qt::NoModifier;

    int n = ::read(kbdFD, buf, 80);

    for (int loop = 0; loop < n; loop++)
    {
        driverKeyCode   = (unsigned short)buf[loop];
        qtKeyCode       = 0;
        unicode         = 0xffff;
        isPressed       = (driverKeyCode & 0x80) == 0;

        if(prevkey != driverKeyCode) {
        qLog(Input) << "keypressed: code=" << driverKeyCode << " (" << 
                    (isPressed ? "Down" : "Up") << ")";
        //QPerformanceLog("keypressed") << "code=" << driverKeyCode << " (" << 
                    //(isPressed ? "Down" : "Up") << ")";

        switch (driverKeyCode & 0x7f)
        {
            case 0x3D: 
                if(isPressed) {
                    if(fn) fn=false; 
                    else   fn=true;
                    control=true;
                }
                break;
            case 0x1D: 
                if(isPressed) {
                    if(ctrl) ctrl=false; 
                    else   ctrl=true;
                    control=true;
                }
                break;
            case 0x2A: 
                if(isPressed) {
                    if(shift) shift=false; 
                    else   shift=true;
                    control=true;
                }
                break;
            case 0x01: 
                qtKeyCode = Qt::Key_F12; 
                unicode  = 0xffff; 
                break;
            case 0x0B: 
                qtKeyCode = ((!shift) ? Qt::Key_0       : Qt::Key_AsciiTilde  );
                unicode  = ((!shift) ? 0x30 : 0x7E);
                break;
            case 0x02:
                qtKeyCode = ((!shift) ? Qt::Key_1       : Qt::Key_Exclam      );
                unicode  = ((!shift) ? 0x31 : 0x21);
                break;
            case 0x03:
                qtKeyCode = ((!shift) ? Qt::Key_2       : Qt::Key_QuoteDbl    );
                unicode  = ((!shift) ? 0x32 : 0x22);
                break;
            case 0x04:
                qtKeyCode = ((!shift) ? Qt::Key_3       : Qt::Key_NumberSign  );
                unicode  = ((!shift) ? 0x33 : 0x23);
                break;
            case 0x05:
                qtKeyCode = ((!shift) ? Qt::Key_4       : Qt::Key_Dollar      );
                unicode  = ((!shift) ? 0x34 : 0x24);
                break;
            case 0x06:
                qtKeyCode = ((!shift) ? Qt::Key_5       : Qt::Key_Percent     );
                unicode  = ((!shift) ? 0x35 : 0x25);
                break;
            case 0x07:
                qtKeyCode = ((!shift) ? Qt::Key_6       : Qt::Key_Ampersand   );
                unicode  = ((!shift) ? 0x36 : 0x26);
                break;
            case 0x08:
                qtKeyCode = ((!shift) ? Qt::Key_7       : Qt::Key_QuoteLeft   );
                unicode  = ((!shift) ? 0x37 : 0x27);
                break;
            case 0x09:
                qtKeyCode = ((!shift) ? Qt::Key_8       : Qt::Key_BracketLeft );
                unicode  = ((!shift) ? 0x38 : 0x28);
                break;
            case 0x0A:
                qtKeyCode = ((!shift) ? Qt::Key_9       : Qt::Key_BracketRight);
                unicode  = ((!shift) ? 0x39 : 0x29);
                break;
            case 0x1E:
                qtKeyCode = Qt::Key_A;
                unicode  = ((!shift) ? 0x61 : 0x41);
                break;
            case 0x30:
                qtKeyCode = ((!fn) ? Qt::Key_B : Qt::Key_Underscore);
                if(fn) unicode  = 0x5F;
                else unicode = ((!shift) ? 0x62 : 0x42);
                break;
            case 0x2E:
                qtKeyCode = Qt::Key_C;
                unicode  = ((!shift) ? 0x63 : 0x43);
                break;
            case 0x20:
                qtKeyCode = Qt::Key_D;
                unicode  = ((!shift) ? 0x64 : 0x44);
                break;
            case 0x12:
                qtKeyCode = ((!fn) ? Qt::Key_E : Qt::Key_Equal);
                if(fn) unicode  = 0x3D;
                else unicode = ((!shift) ? 0x65 : 0x45);
                break;
            case 0x21:
                qtKeyCode = ((!fn) ? Qt::Key_F : Qt::Key_Backslash);
                if(fn) unicode  = 0x5C;
                else unicode = ((!shift) ? 0x66 : 0x46);
                break;
            case 0x22:
                qtKeyCode = ((!fn) ? Qt::Key_G : Qt::Key_Semicolon);
                if(fn) unicode  = 0x3B;
                else unicode = ((!shift) ? 0x67 : 0x47);
                break;
            case 0x23:
                qtKeyCode = ((!fn) ? Qt::Key_H : Qt::Key_Colon);
                if(fn) unicode  = 0x3A;
                else unicode = ((!shift) ? 0x68 : 0x48);
                break;
            case 0x17:
                qtKeyCode = Qt::Key_I;
                unicode  = ((!shift) ? 0x69 : 0x49);
                break;
            case 0x24:
                qtKeyCode = ((!fn) ? Qt::Key_J : Qt::Key_Asterisk);
                if(fn) unicode  = 0x2A;
                else unicode = ((!shift) ? 0x6A : 0x4A);
                break;
            case 0x25:
                qtKeyCode = Qt::Key_K;
                unicode  = ((!shift) ? 0x6B : 0x4B);
                break;
            case 0x26:
                qtKeyCode = ((!fn) ? Qt::Key_L : Qt::Key_Bar);
                if(fn) unicode  = 0x7C;
                else unicode = ((!shift) ? 0x6C : 0x4C);
                break;
            case 0x32:
                qtKeyCode = Qt::Key_M;
                unicode  = ((!shift) ? 0x6D : 0x4D);
                break;
            case 0x31:
                qtKeyCode = Qt::Key_N;
                unicode  = ((!shift) ? 0x6E : 0x4E);
                break;
            case 0x18:
                qtKeyCode = Qt::Key_O;
                unicode  = ((!shift) ? 0x6F : 0x4F);
                break;
            case 0x19:
                qtKeyCode = Qt::Key_P;
                unicode  = ((!shift) ? 0x70 : 0x50);
                break;
            case 0x10:
                qtKeyCode = Qt::Key_Q;
                unicode  = ((!shift) ? 0x71 : 0x51);
                break;
            case 0x39:
                qtKeyCode = Qt::Key_Space;
                unicode  = ((!shift) ? 0x20 : 0x20);
                break;
            case 0x13:
                qtKeyCode = ((!fn) ? Qt::Key_R : Qt::Key_Plus);
                if(fn) unicode  = 0x2B;
                else unicode = ((!shift) ? 0x72 : 0x52);
                break;
            case 0x1F:
                qtKeyCode = Qt::Key_S;
                unicode  = ((!shift) ? 0x73 : 0x53);
                break;
            case 0x14:
                qtKeyCode = ((!fn) ? Qt::Key_T : Qt::Key_BracketLeft);
                if(fn) unicode  = 0x5B;
                else unicode = ((!shift) ? 0x74 : 0x54);
                break;
            case 0x16:
                qtKeyCode = Qt::Key_U;
                unicode  = ((!shift) ? 0x75 : 0x55);
                break;
            case 0x2F:
                qtKeyCode = Qt::Key_V;
                unicode  = ((!shift) ? 0x76 : 0x56);
                break;
            case 0x11:
                qtKeyCode = ((!fn) ? Qt::Key_W : Qt::Key_Period);
                if(fn) unicode  = 0x5E;
                else unicode = ((!shift) ? 0x77 : 0x57);
                break;
            case 0x2D:
                qtKeyCode = Qt::Key_X;
                unicode  = ((!shift) ? 0x78 : 0x58);
                break;
            case 0x15:
                qtKeyCode = ((!fn) ? Qt::Key_Y : Qt::Key_BracketRight);
                if(fn) unicode  = 0x5D;
                else unicode = ((!shift) ? 0x79 : 0x59);
                break;
            case 0x2C:
                qtKeyCode = Qt::Key_Z;
                unicode  = ((!shift) ? 0x7A : 0x5A);
                break;
            case 0x34:
                qtKeyCode = ((!fn) ? Qt::Key_Question : Qt::Key_Greater);
                if(fn) unicode  = 0x3E;
                else unicode = ((!shift) ? 0x3F : 0x2E);
                break;
            case 0x33:
                qtKeyCode = ((!fn) ? Qt::Key_Slash : Qt::Key_Less);
                if(fn) unicode  = 0x3C;
                else unicode = ((!shift) ? 0x2F : 0x2C);
                break;
            case 0x0C:
                qtKeyCode = ((!fn) ? Qt::Key_Minus : Qt::Key_At);
                if(fn) unicode  = 0x40;
                else unicode = ((!shift) ? 0x2D : 0x2D);
                break;
            case 0x0E:
                qtKeyCode = Qt::Key_Back;
                unicode  = 0xffff;
                break;
            case 0x0F:
                qtKeyCode = Qt::Key_Tab;
                unicode  = 0xffff;
                break;
            case 0x42:
                qtKeyCode = Qt::Key_VolumeUp;
                unicode  = 0xffff; 
                break;
            case 0x67:
                qtKeyCode = Qt::Key_Up;
                unicode  = 0xffff; 
                break;
            case 0x41:
                qtKeyCode = Qt::Key_VolumeDown;
                unicode  = 0xffff; 
                break;
            case 0x6C:
                qtKeyCode = Qt::Key_Down;
                unicode  = 0xffff; 
                break;
            case 0x69:
                qtKeyCode = Qt::Key_Left;
                unicode  = 0xffff;
                break;
            case 0x6A:
                qtKeyCode = Qt::Key_Right;
                unicode  = 0xffff;
                break;
            case 0x57:
                qtKeyCode = Qt::Key_Select;
                unicode  = 0xffff;
                break;
            case 0x1C:
                qtKeyCode = Qt::Key_Select;
                unicode  = 0xffff;
                break;
            case 0x3F:
                qtKeyCode = Qt::Key_Select;
                unicode  = 0xffff;
                break;
            case 0x40:
                qtKeyCode = Qt::Key_Back;
                unicode  = 0xffff; 
                break;
            case 0x3E:
                qtKeyCode = Qt::Key_Back;
                unicode  = 0xffff; 
                break;
            case 0x38:
                qtKeyCode = Qt::Key_Call;
                unicode  = 0xffff;
                break;
            case 0x61:
                qtKeyCode = Qt::Key_Hangup;
                unicode  = 0xffff;
                break;
            case 0x58:
                qtKeyCode = Qt::Key_Context1;
                unicode  = 0xffff; 
                break;
            case 0x3B:
                qtKeyCode = Qt::Key_F1;
                unicode  = 0xffff;
                break;
            case 0x3C:
                qtKeyCode = Qt::Key_F2;
                unicode  = 0xffff; 
                break;
            case 0x44:
                qtKeyCode = Qt::Key_F3;
                unicode  = 0xffff; 
                break;
            case 0x63:
                qtKeyCode = Qt::Key_F4;
                unicode  = 0xffff; 
                break;
        }

        if(!control) {
            processKeyEvent(unicode, qtKeyCode, modifiers, isPressed, false);
            prevkey = driverKeyCode;

            if (isPressed)
               beginAutoRepeat(unicode, qtKeyCode, modifiers);
            else
               endAutoRepeat();
        }
     }
     }
}

#endif // QT_QWS_C3200
