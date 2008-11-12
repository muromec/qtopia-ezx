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

#ifndef _QABSTRACTDIALERSCREEN_H_
#define _QABSTRACTDIALERSCREEN_H_

#include <QWidget>
class QString;
class QUniqueId;

class QAbstractDialerScreen : public QWidget
{
Q_OBJECT
public:
    QAbstractDialerScreen(QWidget *parent = 0, Qt::WFlags f = 0)
    : QWidget(parent, f) { setObjectName( "dialer" ); }

    virtual QString digits() const = 0;

public slots:
    virtual void reset() = 0;
    virtual void appendDigits(const QString &digits) = 0;
    virtual void setDigits(const QString &digits) = 0;

signals:
    void requestDial(const QString &, const QUniqueId &);
    void speedDial(const QString&);
    void filterKeys(const QString&, bool&);
    void filterSelect(const QString&, bool&);
};


#endif // _QABSTRACTDIALERSCREEN_H_

