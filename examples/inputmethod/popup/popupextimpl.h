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
#include <qobject.h>
class PopupIM;

/*
   When using multiple inheritence, QObject should always be listed first.

   The implementation does not need to inherit Q_OBJECT if the QWSInputMethod
   calls QWSServer::sendKey itself rather than emitting a signal
*/
class PopupIMExtImpl : public QObject, public ExtInputMethodInterface
{
    Q_OBJECT
public:
    PopupIMExtImpl();
    virtual ~PopupIMExtImpl();

#ifndef QT_NO_COMPONENT
    QRESULT queryInterface( const QUuid&, QUnknownInterface** );
    Q_REFCOUNT
#endif
    QString name();
    QPixmap *icon();

    void resetState() {}

    QStringList compatible() { return QStringList(); }

    QWSInputMethod *inputMethod( ) { return 0; }

    QWidget *statusWidget( QWidget *, Qt::WFlags ) { return 0; }
    QWidget *keyboardWidget( QWidget *, Qt::WFlags );

    void qcopReceive( const QString &, const QByteArray & ) { }

private slots:
    void sendKeyEvent(ushort, ushort, ushort, bool, bool);

private:
    PopupIM *input;
    QPixmap *icn;
    ulong ref;
};

