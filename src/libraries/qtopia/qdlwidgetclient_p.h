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

#ifndef QDL_WIDGET_CLIENT_PRIVATE_H
#define QDL_WIDGET_CLIENT_PRIVATE_H

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

// Qt includes
#include <QTextEdit>
#include <QTextBrowser>

// Forward class declarations
class QDLClient;
class QDLLink;
class QDLWidgetClientPrivate;

// ============================================================================
//
// QDLClientWidgets
//
// ============================================================================

union QDLClientWidgets
{
    QTextEdit *te;
    QTextBrowser *tb;
};

// ============================================================================
//
// QDLWidgetClientPrivate
//
// ============================================================================

class QDLWidgetClientPrivate
{
public:
    explicit QDLWidgetClientPrivate( QWidget *widget );
    ~QDLWidgetClientPrivate();

    // Modification
    void insertText( const QString& text );
    void setText( const QString& text );
    void verifyLinks( QDLClient* client );

    // Access
    bool isValid() const;
    QString text() const;
    QWidget* widget() const;
    QString determineHint() const;

private:

    // Modification
    void setWidget( QWidget* widget );
    void setWidgetType( const QString& type );
    void breakLink( QDLClient* client, const int linkId );

    // Access
    QString widgetType() const;

    // Data members
    bool mValid;
    QDLClientWidgets mWidget;
    QString mWidgetType;
};

#endif //QDL_WIDGET_CLIENT_PRIVATE_H
