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
#include <qpeapplication.h>
#include <qmultilineedit.h>
#include <qvbox.h>

#include <popupim.h>

/*
   A Stub class to allow testing of a Popup Input Method in isolation
   of plugin code and the Qtopia server
*/
class TestMultiLineEdit : public QMultiLineEdit
{
    Q_OBJECT
public:
    TestMultiLineEdit(QWidget *parent, const char *name = 0)
	: QMultiLineEdit(parent, name) {}

public slots:
    void interpretKeyPress( ushort unicode, ushort keycode,
	    ushort modifiers, bool press, bool repeat )
    {
	QKeyEvent ke(press ? QEvent::KeyPress : QEvent::KeyRelease,
		keycode, 0, modifiers, QChar(unicode), repeat);

	if (press)
	    keyPressEvent(&ke);
	else
	    keyReleaseEvent(&ke);
    }
};

int main(int argc, char **argv) 
{
    QPEApplication a(argc, argv);

    QVBox *vb = new QVBox();
    TestMultiLineEdit *mle = new TestMultiLineEdit(vb);
    PopupIM *pi = new PopupIM(vb);
    QObject::connect(pi, SIGNAL(keyPress(ushort,ushort,ushort,bool,bool)),
	    mle, SLOT(interpretKeyPress(ushort,ushort,ushort,bool,bool)));

    a.showMainWidget(vb);
    return a.exec();
}

#include "main.moc"
