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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qtopia API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#define EQ 257
#define COLON 258
#define DOT 259
#define SEMICOLON 260
#define SPACE 261
#define HTAB 262
#define LINESEP 263
#define NEWLINE 264
#define BEGIN_VCARD 265
#define END_VCARD 266
#define BEGIN_VCAL 267
#define END_VCAL 268
#define BEGIN_VEVENT 269
#define END_VEVENT 270
#define BEGIN_VTODO 271
#define END_VTODO 272
#define ID 273
#define STRING 274
typedef union {
    char *str;
    VObject *vobj;
    } YYSTYPE;
extern YYSTYPE vcclval;
