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

#include "launcher.h"

#include <QCoreApplication>

#include <stdio.h>

void print_help(const char *app)
{
    fprintf(stdout, "Useage: %s <application>\n", app);
    fprintf(stdout, "The %s will send a message to the server and wait until\n", app);
    fprintf(stdout, "the application starts...\n");
}

int main( int argc, char *argv[] )
{
    if (argc != 2) {
        print_help(argv[0]);
        return -1;
    }

    QCoreApplication app(argc, argv);

    Launcher launcher;
    launcher.startup(argv[1]);

    app.exec();
}
