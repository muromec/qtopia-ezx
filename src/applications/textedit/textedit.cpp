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

#include "textedit.h"

#include <qcontent.h>
#include <qtopiaapplication.h>
#include <qdocumentproperties.h>
#include <qdocumentselector.h>
#include <qsoftmenubar.h>
#include <qtopiaservices.h>

#include <QListWidget>
#include <QClipboard>
#include <QCloseEvent>
#include <QFontDatabase>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QLabel>
#include <QStackedWidget>
#include <QTextCodec>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QTimer>
#include <QSettings>
#include <QWhatsThis>
#include <unistd.h>
#include <QToolTip>

class QpeEditor : public QTextEdit
{
    Q_OBJECT
public:
    QpeEditor( QWidget *parent )
        : QTextEdit( parent ), lastTxtFound(false)
    {
    }

    void findText( const QString &txt, bool caseSensitive )
    {
        QTextDocument::FindFlags flags = 0;
        if ( caseSensitive )
            flags |= QTextDocument::FindCaseSensitively;
        Qt::CaseSensitivity s = Qt::CaseInsensitive;
        if ( caseSensitive )
            s = Qt::CaseSensitive ;

        const bool findNext = txt == lastTxt;
        if ( !findNext &&
                (txt.startsWith( lastTxt, s ) || lastTxt.startsWith( txt, s ) ) ){
            setTextCursor( QTextCursor( document() ) );
        }

        QList<QTextEdit::ExtraSelection> extrasels;
        if (!txt.isEmpty()) {
            bool found = find( txt, flags );

            if ( !found ) {
                if ( findNext && lastTxtFound) {
                    emit findWrapped(); // We don't actually wrap, though
                    setTextCursor( QTextCursor( document() ) ); // make sure we start at the start
                } else
                    emit findNotFound();
                lastTxtFound = false;
            } else {
                lastTxtFound = true;

                emit findFound();

                QTextEdit::ExtraSelection es;
                es.cursor = textCursor();
                QPalette p = palette();
                es.format.setBackground(p.brush(QPalette::Active, QPalette::Highlight));
                es.format.setForeground(p.brush(QPalette::Active, QPalette::HighlightedText));
                extrasels.append(es);
            }
        }
        setExtraSelections(extrasels);
        lastTxt = txt;
    }

    void clearSearchResults()
    {
        setExtraSelections(QList<QTextEdit::ExtraSelection>());
    }

signals:
    void findNotFound();
    void findWrapped();
    void findFound();

private:
    QString lastTxt;
    bool lastTxtFound;
};

class SimpleToolTip : public QObject
{
    Q_OBJECT;

public:
    SimpleToolTip(QWidget *parent);
    ~SimpleToolTip();

public slots:
    void show(const QString& lbl);
    void hide();

protected:
    QWidget *mParent;
    QLabel* mLabel;
    QTimer* mTimer;
};

SimpleToolTip::SimpleToolTip(QWidget *parent)
    : QObject(parent), mParent(parent), mLabel(0), mTimer(0)
{

}

SimpleToolTip::~SimpleToolTip()
{
}

void SimpleToolTip::hide()
{
    if(mLabel)
        mLabel->hide();
    if(mTimer && mTimer->isActive())
        mTimer->stop();
}

void SimpleToolTip::show(const QString& lbl)
{
    if (!mLabel) {
        mLabel = new QLabel(mParent);
        mLabel->setFocusPolicy(Qt::NoFocus);
        mLabel->setBackgroundRole(QPalette::Button);
        mLabel->setForegroundRole(QPalette::ButtonText);
        mLabel->setFrameStyle(QFrame::Panel);
        mLabel->setAutoFillBackground(true);
        mLabel->setMargin(2);
        QPalette p = mLabel->palette();
        QColor bg = p.brush(QPalette::Button).color();
        bg.setAlpha(255);
        p.setBrush(QPalette::Button, bg);
        mLabel->setPalette(p);
    }
    if (!mTimer) {
        mTimer = new QTimer(this);
        mTimer->setSingleShot(true);
        connect(mTimer, SIGNAL(timeout()), mLabel, SLOT(hide()));
    }
    mLabel->setText(lbl);

    // Hide the tooltip in 2.5 seconds
    mTimer->start(2500);

    // Now set the geometry.
    QSize lblsh = mLabel->sizeHint();
    mLabel->setGeometry(0, mParent->height() - lblsh.height() - 2, mParent->width(), lblsh.height());
    mLabel->show();
}

TextEdit::TextEdit( QWidget *parent, Qt::WFlags f )
    : QMainWindow( parent, f )
{
    connect(qApp, SIGNAL(appMessage(QString,QByteArray)),
            this, SLOT(message(QString,QByteArray)));

    qCopActivated = canceled = false;

    doc = 0;

    editorStack = new QStackedWidget( this );
    setCentralWidget( editorStack );

    editor = new QpeEditor( editorStack );
    editor->setFrameStyle( QFrame::NoFrame );
    editor->installEventFilter(this);

    QWidget *editWidget = new QWidget();
    mEditorLayout = new QVBoxLayout();
    mEditorLayout->setMargin(0);
    mEditorLayout->setSpacing(0);
    mEditorLayout->addWidget(editor);
    editWidget->setLayout(mEditorLayout);
    editorStack->addWidget( editWidget );

    fileSelector = new QDocumentSelector( editorStack );
    fileSelector->setFilter( QContentFilter( QContent::Document )
            & QContentFilter( QContentFilter::MimeType, "text/*" )
            & QContentFilter( QContentFilter::DRM, QLatin1String( "Unprotected" ) ) );
    fileSelector->enableOptions( QDocumentSelector::NewDocument );
    fileSelector->setFocus();
    editorStack->addWidget(fileSelector);

    mToolTip = new SimpleToolTip(editor);
    mFindTextWidget = 0;
    mFindTextEntry = 0;
    mFindIcon = 0;

    QAction *newAction = new QAction(QIcon( ":icon/new" ), tr( "New" ), this);
    connect( newAction, SIGNAL(triggered()), this, SLOT(fileNew()) );
    newAction->setWhatsThis( tr( "Create a document." ) );

    QAction *openAction = new QAction(QIcon( ":icon/txt" ), tr( "Open" ), this );
    connect( openAction, SIGNAL(triggered()), this, SLOT(fileOpen()) );
    openAction->setWhatsThis( tr( "Open a document." ) );

    QAction *propAction = new QAction(QIcon( ":icon/info" ), tr( "Properties" ), this );
    connect( propAction, SIGNAL(triggered()), this, SLOT(fileName()) );
    propAction->setWhatsThis( tr( "Edit the document properties." ) );

#ifndef QT_NO_CLIPBOARD
    QAction *cutAction = new QAction(QIcon( ":icon/cut" ), tr( "Cut" ), this );
    connect( cutAction, SIGNAL(triggered()), this, SLOT(editCut()) );
    cutAction->setWhatsThis( tr("Cut the currently selected text and move it to the clipboard.") );
    cutAction->setVisible( false );

    QAction *copyAction = new QAction(QIcon( ":icon/copy" ), tr( "Copy" ), this );
    connect( copyAction, SIGNAL(triggered()), this, SLOT(editCopy()) );
    copyAction->setWhatsThis( tr("Copy the currently selected text to the clipboard.") );
    copyAction->setVisible( false );

    pasteAction = new QAction(QIcon( ":icon/paste" ), tr( "Paste" ), this );
    connect( pasteAction, SIGNAL(triggered()), this, SLOT(editPaste()) );
    pasteAction->setWhatsThis( tr("Paste the text in the clipboard at the cursor position.") );
    QTimer::singleShot(0, this, SLOT(clipboardChanged()));
#endif

    findAction = new QAction(QIcon( ":icon/find" ), tr( "Find" ), this );
    findAction->setCheckable( true );
    connect( findAction, SIGNAL(toggled(bool)), this, SLOT(editFind(bool)) );
    findAction->setWhatsThis( tr("Click to find text in the document.\nClick again to hide the search bar.") );

    zin = new QAction(tr( "Zoom In" ), this);
    connect( zin, SIGNAL(triggered()), this, SLOT(zoomIn()) );
    zin->setWhatsThis( tr( "Increase the font size." ) );

    zout = new QAction(tr( "Zoom Out" ), this);
    connect( zout, SIGNAL(triggered()), this, SLOT(zoomOut()) );
    zout->setWhatsThis( tr( "Decrease the font size." ) );

    QAction *wa = new QAction(tr( "Wrap Lines" ), this);
    connect( wa, SIGNAL(toggled(bool)), this, SLOT(setWordWrap(bool)) );
    wa->setWhatsThis( tr("Break long lines into two or more lines.") );
    wa->setCheckable(true);

    QMenu *contextMenu = new QMenu(editor);
    QSoftMenuBar::addMenuTo(editor, contextMenu, QSoftMenuBar::AnyFocus);
    QSoftMenuBar::setHelpEnabled(editor, true);

    QMenu *settingsMenu = new QMenu( tr("Settings"), contextMenu);
    settingsMenu->addAction( zin );
    settingsMenu->addAction( zout );
    settingsMenu->addAction( wa );

    contextMenu->addAction( propAction );
#ifndef QT_NO_CLIPBOARD
    contextMenu->addAction( cutAction );
    contextMenu->addAction( copyAction );
    contextMenu->addAction( pasteAction );
#endif
    contextMenu->addAction( findAction );
    contextMenu->addMenu( settingsMenu );
    contextMenu->addSeparator();
    contextMenu->addAction( QIcon( ":icon/print" ), tr( "Print" ), this, SLOT(print()) );
    contextMenu->addAction( QIcon( ":icon/cancel" ), tr( "Cancel" ), this, SLOT(fileRevert()) );

    bool wrap;

    QSettings cfg("Trolltech","TextEdit");
    cfg.beginGroup("View");
    defaultFontSize = cfg.value("FontSize", 0.0).toDouble();
    originalFontSize = cfg.value("OriginalFontSize", 0.0).toDouble();

    if (defaultFontSize == 0.0) {
        QSettings gcfg("Trolltech","qpe");
        gcfg.beginGroup("Font");
        defaultFontSize = gcfg.value("FontSize[]", 7.0).toDouble();
    }
    if (originalFontSize == 0.0) {
        originalFontSize = defaultFontSize;
        cfg.setValue("OriginalFontSize",originalFontSize);
    }
    setupFontSizes();

    wrap = cfg.value("Wrap",true).toBool();

    connect( editor, SIGNAL(findWrapped()), this, SLOT(findWrapped()) );
    connect( editor, SIGNAL(findNotFound()), this, SLOT(findNotFound()) );
    connect( editor, SIGNAL(findFound()), mToolTip, SLOT(hide()));
#ifndef QT_NO_CLIPBOARD
    connect( editor, SIGNAL(copyAvailable(bool)), cutAction, SLOT(setVisible(bool)) );
    connect( editor, SIGNAL(copyAvailable(bool)), copyAction, SLOT(setVisible(bool)) );
#endif

    // create search bar on demand
    searchVisible = false;

    connect( fileSelector, SIGNAL(newSelected()), this, SLOT(newFile()) );
    connect( fileSelector, SIGNAL(documentSelected(QContent)), this, SLOT(openFile(QContent)) );
    fileOpen();

#ifndef QT_NO_CLIPBOARD
    connect( qApp->clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardChanged()) );
#endif
    connect( qApp, SIGNAL(contentChanged(QContentIdList,QContent::ChangeType)),
        this, SLOT(contentChanged(QContentIdList,QContent::ChangeType)));

    setFontSize(defaultFontSize);
    wa->setChecked(wrap);

    if ( qApp->argc() == 3 && qApp->argv()[1] == QLatin1String("-f") )
        setDocument(qApp->argv()[2]);
}

TextEdit::~TextEdit()
{
    QSettings cfg("Trolltech","TextEdit");
    cfg.beginGroup("View");
    cfg.setValue("FontSize", defaultFontSize);
    cfg.setValue("Wrap",editor->lineWrapMode() == QTextEdit::WidgetWidth);
}

//
// Figure out how many "zoom" levels there for the given font.
//
void
TextEdit::setupFontSizes(void)
{
    QFontDatabase fd;
    // Pick a spread of sizes (at least 25% bigger from the previous)
    QFont f = editor->font();
    QFontInfo qfi(f);

    QString s = fd.styleString(qfi);
    QList<int> smSizes = fd.smoothSizes(qfi.family(), s);

    qreal last = -1;
    foreach (int e, smSizes) {
        QFont g = fd.font(qfi.family(), s, e);

        QFontInfo gfi(g);

        if ((gfi.pixelSize() * 4) > (last * 5) || (e == smSizes.last() && gfi.pixelSize() != last)) {
            fontSizes << (qreal)gfi.pointSize();
            last = gfi.pixelSize();
        }
    }
    if (!fontSizes.contains(defaultFontSize))
        fontSizes << defaultFontSize;
    if (!fontSizes.contains(originalFontSize))
        fontSizes << originalFontSize;
    qSort(fontSizes.begin(), fontSizes.end());
}

void TextEdit::zoomIn()
{
    qreal fsize;
    if (editor->document())
        fsize = editor->document()->defaultFont().pointSizeF();
    else
        fsize = defaultFontSize;

    int index = fontSizes.indexOf(fsize);
    setFontSize(fontSizes.at(index + 1));
    defaultFontSize = fontSizes.at(index + 1);
}

void TextEdit::zoomOut()
{
    qreal fsize;
    if (editor->document())
        fsize = editor->document()->defaultFont().pointSizeF();
    else
        fsize = defaultFontSize;

    int index = fontSizes.indexOf(fsize);
    setFontSize(fontSizes.at(index - 1));
    defaultFontSize = fontSizes.at(index - 1);
}

void TextEdit::setFontSize(qreal size)
{
    QFont f = editor->font();
    f.setPointSizeF(size);
    editor->setFont(f);
    // Zooming only makes sense if we have more than one font size.
    if (fontSizes.count() > 1) {
        zin->setVisible(size != fontSizes.last());
        zout->setVisible(size != fontSizes.first());
        zinE = zin->isVisible();
        zoutE = zout->isVisible();
    }
}

void TextEdit::setWordWrap(bool y)
{
    bool state = editor->document()->isModified();
    editor->setLineWrapMode(y ? QTextEdit::WidgetWidth : QTextEdit::NoWrap );
    editor->document()->setModified( state );
}

void TextEdit::clipboardChanged()
{
#ifndef QT_NO_CLIPBOARD
    pasteAction->setVisible( !qApp->clipboard()->text().isEmpty() );
#endif
}

void TextEdit::contentChanged( const QContentIdList& idList, const QContent::ChangeType type )
{
    if ( !doc )
        return;

    foreach( QContentId id, idList ) {
        if ( doc->id() != id )
            continue;
         if ( QContent::Removed == type ) {
            fileRevert();
        } else {
            doc = new QContent( id );
            updateCaption(doc->name());
        }
         break;
    }
}


void TextEdit::fileNew()
{
    save();
    newFile();
}

void TextEdit::fileOpen()
{

    if ( !save() ) {
        QMessageBox box( tr( "Out of space"),
                        tr( "<qt>Notes was unable to "
                            "save your changes. "
                            "Free some space and try again."
                            "<br>Continue anyway?</qt>" ),
                        QMessageBox::Critical,
                        QMessageBox::Yes|QMessageBox::Escape,
                        QMessageBox::No|QMessageBox::Default,
                        QMessageBox::NoButton, this);
        switch(box.exec()){
            case QMessageBox::Yes:
                delete doc;
                doc = 0;
                break;
            case QMessageBox::No:
                return;
                break;
        }
    }

    if (mFindTextWidget)
        mFindTextWidget->hide();
    editorStack->setCurrentIndex(1);
    updateCaption();
}

void TextEdit::fileRevert()
{
    if (wasCreated)
        doc->removeFiles();
    else if (saved)
        doc->save(backup.toUtf8());
    clear();
    if( qCopActivated ) {
        close();
        canceled = true;
    } else
        fileOpen();
}

void TextEdit::editCut()
{
#ifndef QT_NO_CLIPBOARD
    if( qApp->focusWidget() == editor )
        editor->cut();
    else if( qApp->focusWidget() == mFindTextEntry )
        mFindTextEntry->copy();
#endif
}

void TextEdit::editCopy()
{
#ifndef QT_NO_CLIPBOARD
    if( qApp->focusWidget() == editor )
        editor->copy();
    else if( qApp->focusWidget() == mFindTextEntry )
        mFindTextEntry->copy();
#endif
}

void TextEdit::editPaste()
{
#ifndef QT_NO_CLIPBOARD
    if( qApp->focusWidget() == editor )
        editor->paste();
    else if( qApp->focusWidget() == mFindTextEntry )
        mFindTextEntry->copy();
#endif
}

void TextEdit::editFind(bool s)
{
    if (!mFindTextWidget) {
        mFindTextWidget = new QWidget();
        mFindTextEntry = new QLineEdit();
        int findHeight = mFindTextEntry->sizeHint().height();
        mFindIcon = new QLabel;
        mFindIcon->setPixmap(QIcon(":icon/find").pixmap(findHeight-2, findHeight-2));
        mFindIcon->setMargin(2);
        mFindIcon->installEventFilter(this);

        QHBoxLayout *findLayout = new QHBoxLayout;
        findLayout->addWidget(mFindIcon);
        findLayout->addWidget(mFindTextEntry);
        mFindTextWidget->setLayout(findLayout);

        mEditorLayout->addWidget(mFindTextWidget);

        connect( mFindTextEntry, SIGNAL(textChanged(QString)), this, SLOT(search(QString)) );

        QMenu * menu = QSoftMenuBar::menuFor(mFindTextEntry);
        menu->addAction(findAction);

        QSoftMenuBar::setLabel(mFindTextEntry, Qt::Key_Select, QSoftMenuBar::Next);
        mFindTextEntry->installEventFilter(this);
    }
    if ( s ) {
        mFindTextWidget->show();
        editor->setFocusPolicy(Qt::NoFocus);
        searchVisible = true;
        mFindTextEntry->setFocus();
        if( !Qtopia::mousePreferred() ) {
            if (!mFindTextEntry->hasEditFocus())
                mFindTextEntry->setEditFocus(true);
        }
    } else {
        searchVisible = false;
        if( !Qtopia::mousePreferred() ) {
            if (mFindTextEntry->hasEditFocus())
                mFindTextEntry->setEditFocus(false);
        }
        editor->setFocusPolicy(Qt::StrongFocus);
        mFindTextWidget->hide();
        editor->setFocus();
        editor->clearSearchResults();
        if( !Qtopia::mousePreferred() ) {
            if (!editor->hasEditFocus())
                editor->setEditFocus(true);
        }
    }
}

void TextEdit::search(const QString& text)
{
    editor->findText(text, false);
    if (text.isEmpty())
        mToolTip->hide();
}

void TextEdit::search()
{
    if (mFindTextEntry)
        editor->findText( mFindTextEntry->text(), false );
}

void TextEdit::searchNext()
{
    search();
}

void TextEdit::findWrapped()
{
    mToolTip->show(tr("Find: reached end"));
}

void TextEdit::findNotFound()
{
    mToolTip->show(tr("Find: not found"));
}

void TextEdit::newFile()
{
    clear();
    doc = new QContent;
    doc->setType("text/plain");
    editorStack->setCurrentIndex(0);
    editor->setFocus();
    editor->document()->setModified(false);
    setReadOnly(false);
    updateCaption();
}

void TextEdit::setDocument(const QString& f)
{
    qCopActivated = true;
    save();
    QContent nf(f);
    if (nf.type().isEmpty())
        nf.setType("text/plain");
    openFile(nf);
    if (doc) {
        showEditTools();
        // Show filename in caption
        QString name;
        if ( nf.linkFileKnown() && !nf.name().isEmpty() ) {
            name = nf.name();
        } else {
            name = f;
            int sep = name.lastIndexOf( '/' );
            if ( sep > 0 )
                name = name.mid( sep+1 );
        }
        updateCaption( name );
    } else {
        // Ensure we just go away if there is a problem (eg. too large file).
        if (isHidden())
            deleteLater();
    }
}

class TextCodecSelector : public QDialog {
    Q_OBJECT
public:
    TextCodecSelector(QWidget* parent=0) :
        QDialog(parent)
    {
        list = new QListWidget(this);
        connect(list,SIGNAL(itemActivated(QListWidgetItem*)),this,SLOT(accept()));
        connect(list,SIGNAL(itemPressed(QListWidgetItem*)),this,SLOT(accept()));
        codecs = QTextCodec::availableCodecs();
        qSort(codecs);
        list->addItem(tr("Automatic"));
        foreach (QByteArray n, codecs) {
            list->addItem(n);
        }
        QLabel *label = new QLabel(tr("<qt>Choose the encoding for this file:</qt>"));
        label->setWordWrap(true);
        QVBoxLayout *vb = new QVBoxLayout(this);
        vb->addWidget(label);
        vb->addWidget(list);
        setLayout(vb);
    }

    QTextCodec *selection(QByteArray ba) const
    {
        int n = list->currentRow();
        if ( n<0 ) {
            return 0;
        } else if ( n==0 ) {
            // Automatic... just try them all and pick the one that can convert
            // in and out without losing anything with the smallest intermediate length.
            QTextCodec *best=0;
            int shortest=INT_MAX;
            foreach (QByteArray name, codecs) {
                QTextCodec* c = QTextCodec::codecForName(name);
                QString enc = c->toUnicode(ba);
                if ( c->fromUnicode(enc)==ba ) {
                    if ( enc.length() < shortest ) {
                        best=c;
                        shortest=enc.length();
                    }
                }
            }
            return best;
        } else {
            return QTextCodec::codecForName(codecs.at(n-1));
        }
    }

    static QTextCodec* codecForContent(QByteArray ba, QWidget *parent=0)
    {
        TextCodecSelector tcs(parent);
        tcs.setModal(true);
        QtopiaApplication::setMenuLike(&tcs,true);
        if (QtopiaApplication::execDialog(&tcs)) {
            return tcs.selection(ba);
        }
        return 0;
    }

private:
    QListWidget *list;
    QList<QByteArray> codecs;
};

void TextEdit::openFile( const QContent &f )
{
    // XXX 8 times this space will be used.
    QSettings cfg("Trolltech","TextEdit");
    const int max_supported_text_file_size = cfg.value("Limits/FileSize",256*1024).toInt();

    clear();
    QString txt;
    bool needsave = false;
    {
        QByteArray ba;
        if (f.load(ba)) {
            if ( ba.size() > max_supported_text_file_size ) {
                QMessageBox::critical(this, tr( "File Too Large"),
                                tr("<qt>This file is too large for Notes to open."));
                return;
            }

            txt = QString::fromUtf8(ba, ba.size());
            if ( txt.toUtf8().size() != ba.size() ) {
                // not UTF8
                QTextCodec* codec = TextCodecSelector::codecForContent(ba,this);
                if ( codec ) {
                    txt = codec->toUnicode(ba);
                    needsave = true;
                } else {
                    txt = "";
                }
            }
        }
    }
    fileNew();
    if ( doc )
        delete doc;
    doc = new QContent(f);

    QTextDocument *d = new QTextDocument(txt, editor);

    backup = txt;

    editor->setDocument(d);
    d->setModified(needsave);
    QFont font = editor->font();
    font.setPointSizeF(defaultFontSize);
    d->setDefaultFont(font);
    updateCaption();
}

void TextEdit::showEditTools()
{
    if ( !doc )
        close();
    fileSelector->hide();
    editorStack->setCurrentIndex(0);
    if ( mFindTextWidget && searchVisible )
        mFindTextWidget->show();
    updateCaption();
}

bool TextEdit::save()
{
    // case of nothing to save...
    if ( !doc )
        return true;
    if ( !editor->document()->isModified() ) {
        if( wasCreated ) doc->removeFiles();
        delete doc;
        doc = 0;
        return true;
    }

    QString rt = editor->toPlainText();

    if ( doc->name().isEmpty() )
        doc->setName(calculateName(rt));

    if (!doc->save(rt.toUtf8()))
        return false;
    delete doc;
    doc = 0;
    editor->document()->setModified( false );
    return true;
}

QString TextEdit::calculateName(QString rt)
{
    QString pt = rt.left(20).simplified();
    int i = pt.lastIndexOf(' ');
    QString docname = pt;
    if ( i > 0 )
        docname = pt.left( i );
    // remove "." at the beginning
    while( docname.startsWith( "." ) )
        docname = docname.mid( 1 );
    docname.replace(QChar('/'), QChar('_'));
    if ( docname.isEmpty() )
        docname = tr("Empty Text");
    return docname;
}

void TextEdit::fileName()
{
    if (doc->name().isEmpty())
        doc->setName(calculateName(editor->document()->toPlainText()));

    //
    // Document properties operations depend on the file being
    // up-to-date.  Force a write before changing properties.
    //
    wasCreated = wasCreated || !doc->fileKnown();

    if (!doc->save(editor->document()->toPlainText().toUtf8()))
        return;

    saved = true;

    QDocumentPropertiesDialog *lp = new QDocumentPropertiesDialog(*doc, this);
    if (QtopiaApplication::execDialog(lp)) {
        updateCaption(doc->name());
    }
    delete lp;
}

void TextEdit::clear()
{
    delete doc;
    doc = 0;
    editor->clear();
    saved = false;
    wasCreated = false;
}

void TextEdit::updateCaption( const QString &name )
{
    if ( !doc )
        setWindowTitle( tr("Notes") );
    Q_UNUSED( name );
}

void TextEdit::accept()
{
    fileOpen();
}

void TextEdit::message(const QString& msg, const QByteArray& data)
{
    if ( msg == "viewFile(QString)" || msg == "openFile(QString)" ) {
        save();
        qCopActivated = true;
        QDataStream d(data);
        QString filename;
        d >> filename;

        //
        // .desktop files should _not_ be able to be edited easily,
        // as they are generated by the server.  Force opening the
        // file they refer to, rather than the .desktop file.
        //
        if (!filename.contains(".desktop")) {
            if (filename.trimmed().isEmpty()){
                newFile();
            }else{
                QContent dc;
                dc.setFile(filename);
                if (dc.type().isEmpty())
                    dc.setType("text/plain");
                openFile(dc);
            }
        } else {
            openFile(QContent(filename));
        }
        showEditTools();
        updateCaption( filename );
        if ( msg == "viewFile(QString)" )
            setReadOnly(true);
    }
}

void TextEdit::setReadOnly(bool y)
{
    editor->setReadOnly(y);
    if ( y )
        editor->document()->setModified(false);
}

void TextEdit::closeEvent( QCloseEvent* e )
{
    if( searchVisible ) {
        findAction->setChecked( false );
    } else {
        if (editorStack->currentIndex() == 0) {
            if (qCopActivated) {
                if (!canceled)
                    save();
                e->accept();
            } else {
                fileOpen();
                e->ignore();
            }
        } else {
            e->accept();
        }
    }
}

bool TextEdit::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(e);
        if (ke->key() == Qt::Key_Select) {
            if (editor->hasEditFocus()) {
                if ( doc->name().isEmpty() )
                    fileName();
                close();
                return true;
            } else if (mFindTextEntry->hasEditFocus()) {
                searchNext();
                return true;
            }
        } else if (ke->key() == Qt::Key_Back) {
            if (mFindTextEntry && mFindTextEntry->hasEditFocus() && mFindTextEntry->text().isEmpty()) {
                editFind(false);
                findAction->setChecked(false);
                return true;
            }
        }
    } else if (e->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent*>(e);
        if (me->button() == Qt::LeftButton) {
            if (o == mFindIcon) {
                searchNext();
                return true;
            }
        }
    }
    return false;
}

void TextEdit::print()
{
    QtopiaServiceRequest srv( "Print", "print(QString)" );
    srv << doc->fileName();
    srv.send();
}

#include "textedit.moc"
