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

//**  Copyright 2003, Chris Larson <kergoth@handhelds.org>

#include "kernelkeymapkbdhandler.h"

//#ifdef QT_QWS_KERNELKEYMAP
#include <QScreen>
#include <QSocketNotifier>

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

static const QWSKeyMap sl5000KeyMap[] = {
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 00
    {        Qt::Key_A,                'a'     , 'A'     , 'A'-64  }, // 01
    {        Qt::Key_B,                'b'     , 'B'     , 'B'-64  }, // 02
    {        Qt::Key_C,                'c'     , 'C'     , 'C'-64  }, // 03
    {        Qt::Key_D,                'd'     , 'D'     , 'D'-64  }, // 04
    {        Qt::Key_E,                'e'     , 'E'     , 'E'-64  }, // 05
    {        Qt::Key_F,                'f'     , 'F'     , 'F'-64  }, // 06
    {        Qt::Key_G,                'g'     , 'G'     , 'G'-64  }, // 07
    {        Qt::Key_H,                'h'     , 'H'     , 'H'-64  }, // 08
    {        Qt::Key_I,                'i'     , 'I'     , 'I'-64  }, // 09
    {        Qt::Key_J,                'j'     , 'J'     , 'J'-64  }, // 0a 10
    {        Qt::Key_K,                'k'     , 'K'     , 'K'-64  }, // 0b
    {        Qt::Key_L,                'l'     , 'L'     , 'L'-64  }, // 0c
    {        Qt::Key_M,                'm'     , 'M'     , 'M'-64  }, // 0d
    {        Qt::Key_N,                'n'     , 'N'     , 'N'-64  }, // 0e
    {        Qt::Key_O,                'o'     , 'O'     , 'O'-64  }, // 0f
    {        Qt::Key_P,                'p'     , 'P'     , 'P'-64  }, // 10
    {        Qt::Key_Q,                'q'     , 'Q'     , 'Q'-64  }, // 11
    {        Qt::Key_R,                'r'     , 'R'     , 'R'-64  }, // 12
    {        Qt::Key_S,                's'     , 'S'     , 'S'-64  }, // 13
    {        Qt::Key_T,                't'     , 'T'     , 'T'-64  }, // 14 20
    {        Qt::Key_U,                'u'     , 'U'     , 'U'-64  }, // 15
    {        Qt::Key_V,                'v'     , 'V'     , 'V'-64  }, // 16
    {        Qt::Key_W,                'w'     , 'W'     , 'W'-64  }, // 17
    {        Qt::Key_X,                'x'     , 'X'     , 'X'-64  }, // 18
    {        Qt::Key_Y,                'y'     , 'Y'     , 'Y'-64  }, // 19
    {        Qt::Key_Z,                'z'     , 'Z'     , 'Z'-64  }, // 1a
    {        Qt::Key_Shift,                0xffff  , 0xffff  , 0xffff  }, // 1b
    {        Qt::Key_Return,                13      , 13      , 0xffff  }, // 1c
    {        Qt::Key_F11,                0xffff  , 0xffff  , 0xffff  }, // 1d todo
    {        Qt::Key_F22,                0xffff  , 0xffff  , 0xffff  }, // 1e 30
    {        Qt::Key_Backspace,        8       , 8       , 0xffff  }, // 1f
    {        Qt::Key_F31,                0xffff  , 0xffff  , 0xffff  }, // 20
    {        Qt::Key_F35,                0xffff  , 0xffff  , 0xffff  }, // 21 light
    {        Qt::Key_Escape,                0xffff  , 0xffff  , 0xffff  }, // 22

    // Direction key code are for *UNROTATED* display.
    {        Qt::Key_Up,                0xffff  , 0xffff  , 0xffff  }, // 23
    {        Qt::Key_Right,                0xffff  , 0xffff  , 0xffff  }, // 24
    {        Qt::Key_Left,                0xffff  , 0xffff  , 0xffff  }, // 25
    {        Qt::Key_Down,                0xffff  , 0xffff  , 0xffff  }, // 26

    {        Qt::Key_F33,                0xffff  , 0xffff  , 0xffff  }, // 27 OK
    {        Qt::Key_F12,                0xffff  , 0xffff  , 0xffff  }, // 28 40 home
    {        Qt::Key_1,                '1'     , 'q'     , 'Q'-64  }, // 29
    {        Qt::Key_2,                '2'     , 'w'     , 'W'-64  }, // 2a
    {        Qt::Key_3,                '3'     , 'e'     , 'E'-64  }, // 2b
    {        Qt::Key_4,                '4'     , 'r'     , 'R'-64  }, // 2c
    {        Qt::Key_5,                '5'     , 't'     , 'T'-64  }, // 2d
    {        Qt::Key_6,                '6'     , 'y'     , 'Y'-64  }, // 2e
    {        Qt::Key_7,                '7'     , 'u'     , 'U'-64  }, // 2f
    {        Qt::Key_8,                '8'     , 'i'     , 'I'-64  }, // 30
    {        Qt::Key_9,                '9'     , 'o'     , 'O'-64  }, // 31
    {        Qt::Key_0,                '0'     , 'p'     , 'P'-64  }, // 32 50
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 33
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 34
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 35
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 36
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 37
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 38
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 39
    {        Qt::Key_Minus,                '-'     , 'b'     , 'B'-64  }, // 3a
    {        Qt::Key_Plus,                '+'     , 'n'     , 'N'-64  }, // 3b
    {        Qt::Key_CapsLock,        0xffff  , 0xffff  , 0xffff  }, // 3c 60
    {        Qt::Key_At,                '@'     , 's'     , 'S'-64  }, // 3d
    {        Qt::Key_Question,        '?'     , '?'     , 0xffff  }, // 3e
    {        Qt::Key_Comma,                ','     , ','     , 0xffff  }, // 3f
    {        Qt::Key_Period,                '.'     , '.'     , 0xffff  }, // 40
    {        Qt::Key_Tab,                9       , '\\'    , 0xffff  }, // 41
    {        Qt::Key_X,                0xffff         , 'x'            , 'X'-64  }, // 42
    {        Qt::Key_C,                0xffff         , 'c'     , 'C'-64  }, // 43
    {        Qt::Key_V,                0xffff         , 'v'     , 'V'-64  }, // 44
    {        Qt::Key_Slash,                '/'     , '/'     , 0xffff  }, // 45
    {        Qt::Key_Apostrophe,        '\''    , '\''    , 0xffff  }, // 46 70
    {        Qt::Key_Semicolon,        ';'     , ';'     , 0xffff  }, // 47
    {        Qt::Key_QuoteDbl,        '\"'    , '\"'    , 0xffff  }, // 48
    {        Qt::Key_Colon,                ':'     , ':'     , 0xffff  }, // 49
    {        Qt::Key_NumberSign,        '#'     , 'd'     , 'D'-64  }, // 4a
    {        Qt::Key_Dollar,                '$'     , 'f'     , 'F'-64  }, // 4b
    {        Qt::Key_Percent,        '%'     , 'g'     , 'G'-64  }, // 4c
    {        Qt::Key_Underscore,        '_'     , 'h'     , 'H'-64  }, // 4d
    {        Qt::Key_Ampersand,        '&'     , 'j'     , 'J'-64  }, // 4e
    {        Qt::Key_Asterisk,        '*'     , 'k'     , 'K'-64  }, // 4f
    {        Qt::Key_ParenLeft,        '('     , 'l'     , 'L'-64  }, // 50 80
    {        Qt::Key_Delete,                '['     , '['     , '['     }, // 51
    {        Qt::Key_Z,                0xffff         , 'z'     , 'Z'-64  }, // 52
    {        Qt::Key_Equal,                '='     , 'm'     , 'M'-64  }, // 53
    {        Qt::Key_ParenRight,        ')'     , ']'     , ']'     }, // 54
    {        Qt::Key_AsciiTilde,        '~'     , '^'     , '^'     }, // 55
    {        Qt::Key_Less,                '<'     , '{'     , '{'     }, // 56
    {        Qt::Key_Greater,        '>'     , '}'     , '}'     }, // 57
    {        Qt::Key_F9,                0xffff  , 0xffff  , 0xffff  }, // 58 datebook
    {        Qt::Key_F10,                0xffff  , 0xffff  , 0xffff  }, // 59 address
    {        Qt::Key_F13,                0xffff  , 0xffff  , 0xffff  }, // 5a 90 email
    {        Qt::Key_F30,                ' '      , ' '    , 0xffff  }, // 5b select
    {        Qt::Key_Space,                ' '     , '|'     , '`'     }, // 5c
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 5d
    {        Qt::Key_Exclam,                '!'     , 'a'     , 'A'-64  }, // 5e
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 5f
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 60
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 61
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 62
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 63
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 64
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 65
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 66
    {        Qt::Key_Meta,                0xffff  , 0xffff  , 0xffff  }, // 67
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 68
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 69
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 6a
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 6b
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 6c
    {        Qt::Key_F34,                0xffff  , 0xffff  , 0xffff  }, // 6d power
    {        Qt::Key_F13,                0xffff  , 0xffff  , 0xffff  }, // 6e mail long
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 6f
    {        Qt::Key_NumLock,        0xffff  , 0xffff  , 0xffff  }, // 70
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 71
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 72
    {         0x20ac,        0xffff  , 0x20ac , 0x20ac }, // 73 Euro sign
    {        Qt::Key_unknown,        0xffff  , 0xffff  , 0xffff  }, // 74
    {        Qt::Key_F32,                0xffff  , 0xffff  , 0xffff  }, // 75 Sync
    {        0,                        0xffff  , 0xffff  , 0xffff  }
};

static const int keyMSize = sizeof(sl5000KeyMap)/sizeof(QWSKeyMap)-1;

const QWSKeyMap *KernelkeymapKbdHandler::keyMap() const
{
    return sl5000KeyMap;
}

KernelkeymapKbdHandler::KernelkeymapKbdHandler()
{
    qLog(Input) << "Loaded Kernelkeymap keypad plugin";
    setObjectName( "Kernelkeymap Keypad Handler" );
    kbdFD = ::open("/dev/tty0", O_RDONLY|O_NDELAY, 0);
    if (kbdFD >= 0) {
        qLog(Input) << "Opened tty0 as keypad input";
        m_notify = new QSocketNotifier( kbdFD, QSocketNotifier::Read, this );
        connect( m_notify, SIGNAL(activated(int)), this, SLOT(readKeyboardData()));
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

    readUnicodeMap();
    readKeyboardMap();
    
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
}

KernelkeymapKbdHandler::~KernelkeymapKbdHandler()
{
    if (kbdFD >= 0) {
        ioctl(kbdFD, KDSKBMODE, K_XLATE);
        tcsetattr(kbdFD, TCSANOW, &origTermData);
        ::close(kbdFD);
        kbdFD = -1;
    }
}

void KernelkeymapKbdHandler::readUnicodeMap()
{
    if (kbdFD < 0)
        return;

    if (ioctl(kbdFD,GIO_UNISCRNMAP,acm) != 0)
        return;
}

void KernelkeymapKbdHandler::readKeyboardMap()
{
    struct kbentry  kbe;
    if (kbdFD < 0)
        return;

    for (int map = 0; map < (1<<KG_CAPSSHIFT); ++map) {
        unsigned short kval;

        kbe.kb_table = map;

        for (int key = 0; key < NR_KEYS; ++key) {
            kbe.kb_index = key;

            if (ioctl(kbdFD, KDGKBENT, &kbe) != 0)
                continue;

            if ((kbe.kb_value == K_HOLE) ||
                (kbe.kb_value == K_NOSUCHMAP))
                continue;

            kval = KVAL(kbe.kb_value);
            switch (KTYP(kbe.kb_value)) {
            case KT_LETTER:
            case KT_LATIN:
            case KT_ASCII:
            case KT_PAD:
            case KT_SHIFT:
                kernel_map[map][key] = kbe.kb_value;
 //                qWarning("keycode %d, map %d, type %d, val %d, acm %c\n", key, map, KTYP(kbe.kb_value), kval, acm[kval]);
                break;
            }
        }
    }
}

void KernelkeymapKbdHandler::handleTtySwitch(int sig)
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

void KernelkeymapKbdHandler::readKeyboardData()
{
    unsigned char buf[81];
    int n = read(kbdFD, buf, 80);
    for (int loop = 0; loop < n; loop++)
        handleKey(buf[loop]);
}

static inline int map_to_modif(int current_map)
{
    int modifiers = 0;

    if (current_map & KG_ALT)
        modifiers |= Qt::Key_Alt;
    else if (current_map & KG_CTRL)
        modifiers |= Qt::Key_Control;
    else if (current_map & KG_SHIFT)
        modifiers |= Qt::Key_Shift;

    return modifiers;
}

void KernelkeymapKbdHandler::handleKey(unsigned char code)
{
    struct kbentry kbe;

    bool release = false;
    if (code & 0x80) {
        release = true;
        code &= 0x7f;
    }

    unsigned short mCode = KVAL(kernel_map[current_map][code]);
    unsigned short unicode = acm[mCode];

    const QWSKeyMap *currentKey = 0;
    int qtKeyCode = Qt::Key_unknown;

    currentKey = &keyMap()[code];

    if ( currentKey )
        qtKeyCode = currentKey->key_code;


    // Handle map changes based on press/release of modifiers
    int modif = -1;
    switch (qtKeyCode) {
    case 0x2f:
        qtKeyCode = Qt::Key_Context1;
        break;
    case 0x27:
        qtKeyCode = Qt::Key_Back;
        break;
    case Qt::Key_F33: //Ok
        qtKeyCode = Qt::Key_Select;
        break;
        
        case Qt::Key_Alt:
        case Qt::Key_F22:
            modif = (1<<KG_ALT);
		break;
        case Qt::Key_Control:
            modif = (1<<KG_CTRL);
		break;
        case Qt::Key_Shift:
            modif = (1<<KG_SHIFT);
		break;
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            if (qt_screen->isTransformed())
                qtKeyCode =  transformDirKey(qtKeyCode);
		break;
	}

    if (modif != -1) {
        if (release)
            current_map &= ~modif;
	else
            current_map |= modif;
    }
    unsigned int uni = unicode;

//    qWarning("code %d, mCode %d,qtKeyCode %x\n", code, mCode, qtKeyCode);
    qLog(Input) << "processKeyEvent(): key=" << qtKeyCode << ", unicode=" << unicode;

    processKeyEvent(uni & 0xff, qtKeyCode,  (Qt::KeyboardModifiers)map_to_modif(current_map), !release, 0);

    if (!release) {
        prevuni = unicode;
        prevkey = qtKeyCode;
    } else {
        prevkey = prevuni = 0;
    }


    if (!release)
        beginAutoRepeat(prevuni, prevkey, (Qt::KeyboardModifiers)map_to_modif(current_map));
    else
        endAutoRepeat();

}


//#endif // QT_QWS_KERNELKEYMAP
