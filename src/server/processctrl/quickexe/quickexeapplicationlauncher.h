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

#ifndef _QUICKEXEAPPLICATIONLAUNCHER_H_
#define _QUICKEXEAPPLICATIONLAUNCHER_H_

#include "applicationlauncher.h"

class QuickExeApplicationLauncherPrivate;
class QuickExeApplicationLauncher : public ExeApplicationLauncher
{
Q_OBJECT
public:
    QuickExeApplicationLauncher();
    virtual ~QuickExeApplicationLauncher();

    // ApplicationTypeLauncher
    virtual bool canLaunch(const QString &app);
    virtual void launch(const QString &app);

private slots:
    void quickLauncherChannel(const QString &,const QByteArray &);
    void startNewQuicklauncher();
    void qlProcessExited(int);
    void qlProcessError(QProcess::ProcessError);

private:
    void respawnQuicklauncher(bool);

    static QStringList quicklaunchPlugin(const QString &);

    QuickExeApplicationLauncherPrivate *d;
};



#endif // _QUICKEXEAPPLICATIONLAUNCHER_H_

