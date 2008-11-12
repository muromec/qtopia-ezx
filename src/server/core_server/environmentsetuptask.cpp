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

#include "environmentsetuptask.h"

#include <QTextCodec>
#include <QSettings>
#include <QString>
#include <QPixmapCache>
#ifdef Q_WS_QWS
#include <qwindowsystem_qws.h>
#endif
#include <QRegExp>
#include <stdlib.h>
#include "qtopiaserverapplication.h"
#include <qtopialog.h>

extern int qws_display_id;

/*!
  \class EnvironmentSetupTask
  \ingroup QtopiaServer::Task
  \brief The EnvironmentSetupTask class initializes the basic system environment required by Qtopia.

  The EnvironmentSetupTask configures the basic environment variables required
  by Qtopia.  It should execute before QtopiaApplication or any other task in
  the system.

  The EnvironmentSetupTask class provides the \c {EnvironmentSetup} task.
  It is part of the Qtopia server and cannot be used by other Qtopia applications.
 */

/*! \internal */
void EnvironmentSetupTask::initEnvironment()
{
    int argc = QtopiaServerApplication::argc();
    char **argv = QtopiaServerApplication::argv();

    QSettings config("Trolltech","locale");
    config.beginGroup( "Location" );
    QString tz = config.value( "Timezone", getenv("TZ") ).toString().trimmed();

    setenv( "TZ", tz.toLatin1(), 1 );

    config.endGroup();

    config.beginGroup( "Language" );
    QString lang = config.value( "Language", getenv("LANG") ).toString().trimmed();
    if( lang.isNull() || lang.isEmpty())
        lang = "en_US";
    lang += ".UTF-8";

    setenv( "LANG", lang.toLatin1().constData(), 1 );

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    /*
       Figure out what QWS_DISPLAY should be set to.
       The algorithm goes like this:

       1) -display <arg> overrides QWS_DISPLAY, which overrides defaultbuttons.conf::Environment::QWS_DISPLAY

       It's expected that a device would have a defaultbuttons.conf::Environment::QWS_DISPLAY like one of
       these:
       LinuxFb:mmWidth34:mmHeight44:0 -- single screen device
       Multi: LinuxFb:mmHeight57:0 LinxFb:offset=0,320:1 :0  -- multi-screen device
    */

#ifdef Q_WS_QWS
    // Start with QWS_DISPLAY
    QString qws_display = getenv("QWS_DISPLAY");
    // -display overrides QWS_DISPLAY
    for (int i = 1; i<argc; i++) {
        QString arg = argv[i];
        if (arg == "-display") {
            if (++i < argc) {
                qws_display = argv[i];
                break;
            }
        }
    }
    if ( qws_display.isEmpty() ) {
        // fall back to defaultbuttons.conf (doesn't work with QVFb skins but runqtopia figures it out)
        QSettings env(Qtopia::defaultButtonsFile(), QSettings::IniFormat);
        env.beginGroup("Environment");
        qws_display = env.value("QWS_DISPLAY").toString();
    }

    // final fall back, :0
    if ( qws_display.isEmpty() )
        qws_display = ":0";

    // Let the search for defaultbuttons.conf (below) work for QVFb's defaultbuttons.conf
    QRegExp display(":(\\d+)$");
    if ( display.indexIn( qws_display ) != -1 )
        qws_display_id = QVariant(display.cap(1)).toInt();

    qLog(QtopiaServer) << "QWS_DISPLAY" << qws_display;
    setenv( "QWS_DISPLAY", qws_display.toLocal8Bit().constData(), 1 );
#endif

    // We know we'll have lots of cached pixmaps due to App/DocLnks
    QPixmapCache::setCacheLimit(512);

#ifdef Q_WS_QWS
    //Turn off green screen frame buffer init
    QWSServer::setBackground(Qt::NoBrush);
#endif

    // Set other, miscellaneous environment
    QSettings env(Qtopia::defaultButtonsFile(), QSettings::IniFormat);
    env.beginGroup("Environment");
    QStringList envKeys = env.childKeys();
    for(int ii = 0; ii < envKeys.count(); ++ii) {
        const QString & key = envKeys.at(ii);
        // QWS_DISPLAY is handled above
        if ( key == "QWS_DISPLAY" ) continue;
        QString value = env.value(key).toString();
        if ( qgetenv(key.toAscii().constData()).count() == 0 )
            setenv(key.toAscii().constData(), value.toAscii().constData(), 1);
    }

    // The default timeout (5 seconds) is too short for Qtopia because the server
    // needs to launch processes while it is starting up (and blocking the event loop).
    // Use a 30 second timeout instead so that the processes do not die prematurely.
    QString qws_connection_timeout = getenv("QWS_CONNECTION_TIMEOUT");
    if ( qws_connection_timeout.isEmpty() )
        setenv("QWS_CONNECTION_TIMEOUT", "30", 1);

    // Ensure the selected theme is present, pick an available one if it isn't.
    validateTheme();
}

static bool themeFileExists(const QString &file)
{
    bool themeExists = false;
    QStringList instPaths = Qtopia::installPaths();
    foreach (QString path, instPaths) {
        QString themeDataPath(path + QLatin1String("etc/themes/") + file);
        if (QFile::exists(themeDataPath)) {
            themeExists = true;
            break;
        }
    }

    return themeExists;
}

/*! \internal */
void EnvironmentSetupTask::validateTheme()
{
    QSettings config(QLatin1String("Trolltech"),QLatin1String("qpe"));
    config.beginGroup( QLatin1String("Appearance") );
    bool setTheme = false;

    // Start by asking QSettings normally
    QString newTheme = config.value("Theme", QString()).toString();
    newTheme = newTheme.replace(QRegExp("\\.desktop"), ".conf");  // backwards compat

    if ( !newTheme.isEmpty() && !themeFileExists(newTheme)) {
        qWarning() << "Selected theme" << newTheme << "does not exist.";
        // Get the default theme (could be the same as the theme we got before)
        QSettings defSettings( Qtopia::qtopiaDir() + "etc/default/Trolltech/qpe.conf", QSettings::IniFormat );
        defSettings.beginGroup("Appearance");
        newTheme = defSettings.value("Theme", QString()).toString();
        setTheme = true;
    }
    if ( newTheme.isEmpty() ) {
        qWarning("No default theme specified in qpe.conf");
    }

    if ( !newTheme.isEmpty() && !themeFileExists(newTheme) ) {
        qWarning() << "Default theme" << newTheme << "does not exist.";
        newTheme = QString();
    }

    if ( newTheme.isEmpty() ) {
        // Catastrophic failure. Don't bail just yet though because tere might be a
        // .conf file we can use on the system.
        QStringList confFiles = QDir(Qtopia::qtopiaDir() + "etc/themes/").entryList(QStringList() << "*.conf");
        if ( confFiles.count() == 0 ) {
            // Qtopia doesn't work without a theme!
            qFatal("No theme files found!");
        }
        // Arbitrary choice here, pick the first entry (if there's more than one).
        newTheme = confFiles[0];
        setTheme = true;
        qWarning() << "Found theme" << newTheme;
    }

    if ( setTheme ) {
        // Since QSettings can't be used to pull out a valid theme, we need to set the theme now.
        // This lets the rest of the code that uses the theme avoid doing all the checks above.
        config.setValue("Theme", newTheme);
    }
}

QTOPIA_STATIC_TASK(EnvironmentSetup, EnvironmentSetupTask::initEnvironment());

