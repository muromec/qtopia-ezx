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
#include <inputmethodinterface.h>
class PopupIM;
class PopupIMImpl : public InputMethodInterface
{
public:
    PopupIMImpl();
    ~PopupIMImpl();

#ifndef QT_NO_COMPONENT
    QRESULT queryInterface( const QUuid&, QUnknownInterface** );
    Q_REFCOUNT
#endif

    QWidget *inputMethod( QWidget *parent, Qt::WFlags f );
    QPixmap *icon();
    QString name();
    void onKeyPress( QObject *receiver, const char *slot );

    // empty because no state kept in keyboard.
    void resetState() {}

private:
    PopupIM *input;
    QPixmap *icn;
    ulong ref;
};
