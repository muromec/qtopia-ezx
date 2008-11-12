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
#include "helppreprocessor.h"

#include <QFile>
#include <QSettings>
#include <QTextCodec>
#include <QTextStream>
#include <QStyle>

#include <qdesktopwidget.h>
#include <qtopiaapplication.h>
#include <qtopialog.h>

#include <QDebug>

/*
    Supports very basic SSI
     - line based - don't put set/include/conditions on the same line
     - supports limited recursion (parameterize) to avoid DoS
    When changing this function, please ensure that doc/src/tools/help-preprocessor.qdoc
    is updated to accurately reflect the changes.
*/

HelpPreProcessor::HelpPreProcessor( const QString &file, int maxrecurs )
    : mFile( file ), levels(maxrecurs)
{
    // setup the compile-time variables
#if defined(QTOPIA_VOIP)
    replace["VOIP"]="1";
#endif
#if defined(QTOPIA_CELL)
    replace["CELL"]="1";
#endif
#if defined(QTOPIA_TELEPHONY)
    replace["TELEPHONY"]="1";
#endif
#if defined(QTOPIA_INFRARED)
    replace["INFRARED"]="1";
#endif
#if defined(QTOPIA_BLUETOOTH)
    replace["BLUETOOTH"]="1";
#endif
#if !defined(QT_NO_CLIPBOARD)
    replace["CLIPBOARD"]="1";
#endif
#if defined(QTOPIA_VPN)
    replace["VPN"]="1";
#endif

    if (QApplication::style()->inherits("QThumbStyle"))
        replace["FINGER"]="1";
    else if ( Qtopia::mousePreferred() )
        replace["TOUCH"]="1";
    else
        replace["KEYPAD"]="1";
    if ( Qtopia::hasKey( Qt::Key_Flip ) )
        replace["FLIP"]="1";
    if (QApplication::desktop()->numScreens() > 1)
        replace["MULTISCREEN"]="1";
}

QString HelpPreProcessor::text()
{
    return parse(mFile);
}

QString HelpPreProcessor::parse(const QString& filename)
{
    QFile f( filename );
    if ( !f.exists() ) {
        QStringList helpPaths = Qtopia::helpPaths();
        QStringList::Iterator it;
        for ( it = helpPaths.begin(); it != helpPaths.end(); it++ ) {
            QString file = (*it) + "/" + filename;
            f.setFileName( file );
            if ( f.exists() )
                break;
        }
        if ( it == helpPaths.end() )
            return tr("Could not locate %1", "%1 - file").arg( filename );
    }
    qLog(Help) << "Loading help file: " << filename << "(" << f.fileName() << ")" ;
    f.open( QIODevice::ReadOnly );
    QByteArray data = f.readAll();
    QTextStream ts( data, QIODevice::ReadOnly );
    ts.setCodec( QTextCodec::codecForName("UTF-8") );
    QString text;

    QString line;

    QRegExp tagAny( "<!--#" );
    QRegExp tagIf( "<!--#if\\s+expr=\"\\$([^\"]*)\"\\s*-->" );
    QRegExp tagElif( "<!--#elif\\s+expr=\"\\$([^\"]*)\"\\s*-->" ); // not supported
    QRegExp tagElse( "<!--#else\\s*-->" );
    QRegExp tagEndif( "<!--#endif\\s*-->" );
    QRegExp tagSet( "<!--#set\\s+var=\"([^\"]*)\"\\s*value=\"([^\"]*)\"\\s*-->" );
    QRegExp tagEcho( "<!--#echo\\s+var=\"([^\"]*)\"\\s*-->" );
    QRegExp tagInclude( "<!--#include\\s+file=\"([^\"]*)\"\\s*-->" );

    bool skip = false;
    int lnum=0;

    do {
        line = ts.readLine();
        lnum++;
        if ( tagAny.indexIn(line) != -1 ) {
            int offset;
            int matchLen;

            offset = 0;
            matchLen = 0;

            while ( (offset = tagIf.indexIn( line, offset + matchLen )) != -1 ) {
                matchLen = tagIf.matchedLength();
                tests.push(tagIf.capturedTexts().at(1).split(QRegExp("\\s*\\|\\|\\s*\\$")));
                inverts.push(false);
                QStringList t = tagIf.capturedTexts().at(1).split(QRegExp("\\s*\\|\\|\\s*\\$"));
                //text+="TEST("+t.join(" or ")+")";
            }

            offset = 0;
            matchLen = 0;
            while ( (offset = tagElse.indexIn( line, offset + matchLen )) != -1 ) {
                matchLen = tagEndif.matchedLength();
                inverts.push(!inverts.pop());
            }

            offset = 0;
            matchLen = 0;
            bool err=false;
            while ( (offset = tagEndif.indexIn( line, offset + matchLen )) != -1 ) {
                matchLen = tagEndif.matchedLength();
                if (!tests.isEmpty()) tests.pop(); else err=true;
                if (!inverts.isEmpty()) inverts.pop(); else err=true;
            }
            if (err)
                qWarning("%s:%d:Unexpected #endif",filename.toLatin1().data(),lnum);

            QStack<QStringList>::ConstIterator it;
            QStack<bool>::ConstIterator bit;

            // recalculate skip
            skip = false;
            for ( it = tests.begin(),bit=inverts.begin(); it != tests.end() && !skip; ++it,++bit ) {
                skip = true;
                foreach (QString t, *it)
                    if ( !replace[t].isEmpty() )
                        skip = false;
                if (*bit)
                    skip = !skip;
            }

            if ( !skip ) {
                offset = 0;
                matchLen = 0;

                while ( (offset = tagSet.indexIn( line, offset + matchLen )) != -1 ) {
                    matchLen = tagSet.matchedLength();
                    QString key = tagSet.capturedTexts().at(1);
                    QString value = tagSet.capturedTexts().at(2);
                    replace[key] = value;
                }

                while ( (offset = tagEcho.indexIn( line )) != -1 ) {
                    QString key = tagEcho.capturedTexts().at(1);
                    line.replace( offset, tagEcho.matchedLength(), replace[key] );
                }

                if ( levels ) {
                    while ( (offset = tagInclude.indexIn( line )) != -1 ) {
                        QString file = tagInclude.capturedTexts().at(1);
                        // Recurse.
                        levels--;
                        line.replace( offset, tagInclude.matchedLength(), parse(file) );
                        levels++;
                    }
                }
            }
        }
        if ( !skip )
            text += line + "\n";
    } while (!ts.atEnd());
    return text;
}

