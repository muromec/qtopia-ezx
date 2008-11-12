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

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <qmainwindow.h>
#include <QContent>
#include <QList>

class QpeEditor;
class QLineEdit;
class QToolBar;
class QToolButton;
class QAction;
class QStackedWidget;
class QDocumentSelector;
class SimpleToolTip;
class QVBoxLayout;
class QLabel;

class TextEdit : public QMainWindow
{
    Q_OBJECT

public:
    TextEdit( QWidget *parent = 0, Qt::WFlags f = 0 );
    ~TextEdit();

    bool eventFilter(QObject *obj, QEvent *event);

protected:
    void closeEvent( QCloseEvent* );

public slots:
    void setDocument(const QString&);

private slots:
    void message(const QString& msg, const QByteArray& data);

    void fileNew();
    void fileRevert();
    void fileOpen();
    void fileName();

    void editCut();
    void editCopy();
    void editPaste();
    void editFind(bool);

    void search(const QString&);
    void search();
    void searchNext();
    void findNotFound();
    void findWrapped();

    void accept();

    void newFile();
    void openFile( const QContent & );
    void showEditTools();

    void zoomIn();
    void zoomOut();
    void setWordWrap(bool y);

    void clipboardChanged();

    void contentChanged(const QContentIdList& id,const QContent::ChangeType type);
    void print();

private:
    void colorChanged( const QColor &c );
    bool save();
    void clear();
    void updateCaption( const QString &name=QString() );
    void setFontSize(qreal size);
    void setupFontSizes(void);
    void setReadOnly(bool);

private:
    QStackedWidget *editorStack;
    QDocumentSelector *fileSelector;
    QpeEditor* editor;
    QAction *pasteAction;
    QAction *findAction;
    QContent *doc;
    SimpleToolTip *mToolTip;
    QWidget *mFindTextWidget;
    QLineEdit *mFindTextEntry;
    QLabel *mFindIcon;
    QVBoxLayout *mEditorLayout;

    QString backup;
    bool qCopActivated, canceled, saved;

    bool wasCreated;
    bool searchVisible;
    QAction *zin, *zout;
    bool zinE, zoutE;
    qreal defaultFontSize;
    qreal originalFontSize;
    QList<qreal> fontSizes;
    QString calculateName(QString);
};

#endif
