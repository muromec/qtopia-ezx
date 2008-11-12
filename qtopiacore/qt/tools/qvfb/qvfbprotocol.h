/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the tools applications of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#ifndef QVFBPROTOCOL_H
#define QVFBPROTOCOL_H

#include <QImage>
#include <QVector>
#include <QColor>
class QVFbKeyProtocol;
class QVFbMouseProtocol;
class QVFbViewProtocol : public QObject
{
    Q_OBJECT
public:
    QVFbViewProtocol(int display_id, QObject *parent = 0);

    virtual ~QVFbViewProtocol();

    int id() const { return mDisplayId; }

    void sendKeyboardData(QString unicode, int keycode,
            int modifiers, bool press, bool repeat);
    void sendMouseData(const QPoint &pos, int buttons, int wheel);

    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual int depth() const = 0;
    virtual int linestep() const = 0;
    virtual int  numcols() const = 0;
    virtual QVector<QRgb> clut() const = 0;
    virtual unsigned char *data() const = 0;

    virtual void setRate(int) {}
public slots:
    virtual void flushChanges();

signals:
    void displayDataChanged(const QRect &);

protected:
    virtual QVFbKeyProtocol *keyHandler() const = 0;
    virtual QVFbMouseProtocol *mouseHandler() const = 0;

private:
    int mDisplayId;
};

class QVFbKeyProtocol
{
public:
    QVFbKeyProtocol(int display_id) : mDisplayId(display_id) {}
    virtual ~QVFbKeyProtocol() {}

    int id() const { return mDisplayId; }

    virtual void sendKeyboardData(QString unicode, int keycode,
            int modifiers, bool press, bool repeat) = 0;

private:
    int mDisplayId;
};

class QVFbMouseProtocol
{
public:
    QVFbMouseProtocol(int display_id) : mDisplayId(display_id) {}
    virtual ~QVFbMouseProtocol() {}

    int id() const { return mDisplayId; }

    virtual void sendMouseData(const QPoint &pos, int buttons, int wheel) = 0;

private:
    int mDisplayId;
};

/* since there is very little variation in input protocols defaults are
   provided */

class QVFbKeyPipeProtocol : public QVFbKeyProtocol
{
public:
    QVFbKeyPipeProtocol(int display_id);
    ~QVFbKeyPipeProtocol();

    void sendKeyboardData(QString unicode, int keycode,
            int modifiers, bool press, bool repeat);

    QString pipeName() const { return fileName; }
private:
    int fd;
    QString fileName;
};

class QVFbMousePipe: public QVFbMouseProtocol
{
public:
    QVFbMousePipe(int display_id);
    ~QVFbMousePipe();

    void sendMouseData(const QPoint &pos, int buttons, int wheel);

    QString pipeName() const { return fileName; }
protected:
    int fd;
    QString fileName;
};

class QTimer;
class QVFbMouseLinuxTP : public QObject, public QVFbMousePipe
{
    Q_OBJECT
public:
    QVFbMouseLinuxTP(int display_id);
    ~QVFbMouseLinuxTP();

    void sendMouseData(const QPoint &pos, int buttons, int wheel);

protected slots:
    void repeatLastPress();

protected:
    void writeToPipe(const QPoint &pos, ushort pressure);
    QPoint lastPos;
    QTimer *repeater;
};
#endif
