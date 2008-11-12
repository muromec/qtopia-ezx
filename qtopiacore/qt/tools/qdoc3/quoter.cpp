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

#include <qregexp.h>

#include "quoter.h"

Quoter::Quoter()
    : silent( false ), splitPoint( "\n(?!\n|$)" ), manyEndls( "\n\n+" )
{
}

void Quoter::reset()
{
    silent = false;
    plainLines.clear();
    markedLines.clear();
    codeLocation = Location::null;
}

void Quoter::quoteFromFile( const QString& userFriendlyFilePath,
			    const QString& plainCode,
			    const QString& markedCode )
{
    silent = false;

    /*
      Split the source code into logical lines. Empty lines are
      treated specially. Before:

	  p->alpha();
	  p->beta();

	  p->gamma();


	  p->delta();

      After:

	  p->alpha();
	  p->beta();\n
	  p->gamma();\n\n
	  p->delta();

      Newlines are preserved because they affect codeLocation.
    */
    codeLocation = Location( userFriendlyFilePath );

    plainLines = plainCode.split(splitPoint);
    markedLines = markedCode.split(splitPoint);
    if (markedLines.count() != plainLines.count()) {
        codeLocation.warning(tr("Something is wrong with qdoc's handling of marked code"));
        markedLines = plainLines;
    }

    /*
      Squeeze blanks (cat -s).
    */
    QStringList::Iterator m = markedLines.begin();
    while ( m != markedLines.end() ) {
	(*m).replace( manyEndls, "\n" );
	(*m).append( "\n" );
	++m;
    }
    codeLocation.start();
}

QString Quoter::quoteLine( const Location& docLocation, const QString& command,
			   const QString& pattern )
{
    if ( plainLines.isEmpty() ) {
	failedAtEnd( docLocation, command );
	return "";
    }

    if ( pattern.isEmpty() ) {
	docLocation.warning( tr("Missing pattern after '\\%1'").arg(command) );
	return "";
    }

    if ( match(docLocation, pattern, plainLines.first()) ) {
	return getLine();
    } else {
	if ( !silent ) {
	    docLocation.warning( tr("Command '\\%1' failed").arg(command) );
	    codeLocation.warning( tr("Pattern '%1' didn't match here")
				  .arg(pattern) );
	    silent = true;
	}
	return "";
    }
}

QString Quoter::quoteTo( const Location& docLocation, const QString& command,
			 const QString& pattern )
{
    QString t;

    if ( pattern.isEmpty() ) {
	while ( !plainLines.isEmpty() )
	    t += getLine();
    } else {
	while ( !plainLines.isEmpty() ) {
	    if ( match(docLocation, pattern, plainLines.first()) )
		return t;
	    t += getLine();
	}
	failedAtEnd( docLocation, command );
    }
    return t;
}

QString Quoter::quoteUntil( const Location& docLocation, const QString& command,
			    const QString& pattern )
{
    QString t = quoteTo( docLocation, command, pattern );
    t += getLine();
    return t;
}

QString Quoter::getLine()
{
    if ( plainLines.isEmpty() )
	return "";

    QString t = markedLines.first();
    int n = t.indexOf( '\n' );
    for ( int i = 0; i < n; i++ )
	codeLocation.advance( '\n' );
    plainLines.removeFirst();
    markedLines.removeFirst();
    return t;
}

bool Quoter::match( const Location& docLocation, const QString& pattern,
		    const QString& line )
{
    QString str = line;
    while ( str.endsWith("\n") )
	str.truncate( str.length() - 1 );

    if ( pattern.startsWith("/") && pattern.endsWith("/") &&
	 pattern.length() > 2 ) {
	QRegExp rx( pattern.mid(1, pattern.length() - 2) );
	if ( !silent && !rx.isValid() ) {
	    docLocation.warning( tr("Invalid regular expression '%1'")
				 .arg(rx.pattern()) );
	    silent = true;
	}
	return str.indexOf( rx ) != -1;
    } else {
	return trimWhiteSpace( str ).indexOf( trimWhiteSpace(pattern) ) != -1;
    }
}

void Quoter::failedAtEnd( const Location& docLocation, const QString& command )
{
    if (!silent && !command.isEmpty()) {
	if ( codeLocation.filePath().isEmpty() ) {
	    docLocation.warning( tr("Unexpected '\\%1'").arg(command) );
	} else {
	    docLocation.warning( tr("Command '\\%1' failed at end of file '%2'")
				 .arg(command).arg(codeLocation.filePath()) );
	}
	silent = true;
    }
}

QString Quoter::fix( const QString& str )
{
    QString t = str;
    while ( t.endsWith("\n\n") )
	t.truncate( t.length() - 1 );
    return t;
}

/*
  Transforms 'int x = 3 + 4' into 'int x=3+4'. A white space is kept
  between 'int' and 'x' because it is meaningful in C++.
*/
QString Quoter::trimWhiteSpace( const QString& str )
{
    enum { Normal, MetAlnum, MetSpace } state = Normal;
    QString t;

    for ( int i = 0; i < (int) str.length(); i++ ) {
	if ( str[i].isSpace() ) {
	    if ( state == MetAlnum )
		state = MetSpace;
	} else {
	    if ( str[i].isLetterOrNumber() ) {
		if ( state == Normal ) {
		    state = MetAlnum;
		} else {
		    if ( state == MetSpace )
			t += ' ';
		    state = Normal;
		}
	    } else {
		state = Normal;
	    }
	    t += str[i];
	}
    }
    return t;
}
