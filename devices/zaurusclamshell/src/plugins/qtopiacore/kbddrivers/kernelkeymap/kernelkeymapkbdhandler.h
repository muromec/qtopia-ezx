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

#ifndef KERNELKEYMAPKBDHANDLER_H
#define KERNELKEYMAPKBDHANDLER_H

//#ifdef QT_QWS_KERNELKEYMAP

#include <QObject>
#include <QWSKeyboardHandler>

#include <termios.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

class QSocketNotifier;

typedef struct QWSKeyMap {
    uint key_code;
    ushort unicode;
    ushort shift_unicode;
    ushort ctrl_unicode;
};

class KernelkeymapKbdHandler : public QObject, public QWSKeyboardHandler
{
    Q_OBJECT
public:
    KernelkeymapKbdHandler();
    ~KernelkeymapKbdHandler();
    void readKeyboardMap();
    void readUnicodeMap();
    void handleKey(unsigned char code);

    virtual const QWSKeyMap *keyMap() const;
    
private:
    QSocketNotifier *m_notify;
    int  kbdFD;
    struct termios origTermData;

    unsigned short acm[E_TABSZ];
    unsigned char kernel_map[(1<<KG_CAPSSHIFT)][NR_KEYS];

    int current_map;	

private Q_SLOTS:
    void readKeyboardData();

    void handleTtySwitch(int sig);
protected:
      Qt::KeyboardModifiers modifiers;
    int prevuni;
    int prevkey;
  
};

//#endif // QT_QWS_KERNELKEYMAP

#endif // KERNELKEYMAPKBDHANDLER_H
