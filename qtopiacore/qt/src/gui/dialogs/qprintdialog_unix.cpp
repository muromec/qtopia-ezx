/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qplatformdefs.h"

#ifndef QT_NO_PRINTDIALOG

#include "private/qabstractprintdialog_p.h"
#include <QtGui/qapplication.h>
#include <QtGui/qcheckbox.h>
#include <QtCore/qlibrary.h>
#include <QtCore/qlist.h>
#include <QtGui/qprinter.h>
#include <QtGui/qtreeview.h>
#include <QtGui/qlistview.h>
#include <QtGui/qevent.h>
#include <QtGui/qmessagebox.h>
#include "qprintdialog.h"
#include "qfiledialog.h"
#include <QtCore/qdebug.h>

#include <QtCore/qobject.h>
#include <QtGui/qabstractprintdialog.h>
#include <QtGui/qitemdelegate.h>
#include "qprintengine.h"

#include "ui_qprintdialog.h"
#include "ui_qprintpropertiesdialog.h"

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
#  include <private/qcups_p.h>
#  include <cups/cups.h>
#  include <private/qpdf_p.h>
#endif

#ifndef QT_NO_NIS
#  ifndef BOOL_DEFINED
#    define BOOL_DEFINED
#  endif

#  include <sys/types.h>
#  include <rpc/rpc.h>
#  include <rpcsvc/ypclnt.h>
#  include <rpcsvc/yp_prot.h>
#endif // QT_NO_NIS

#ifdef Success
#  undef Success
#endif

#include <ctype.h>

class OptionTreeItem;

struct QPrinterDescription {
    QPrinterDescription(const QString &n, const QString &h, const QString &c, const QStringList &a)
        : name(n), host(h), comment(c), aliases(a) {}
    QString name;
    QString host;
    QString comment;
    QStringList aliases;
    bool samePrinter(const QString& printer) const {
        return name == printer || aliases.contains(printer);
    }
};

class QPrintDialogPrivate : public QAbstractPrintDialogPrivate
{
    Q_DECLARE_PUBLIC(QPrintDialog)
public:
    QPrintDialogPrivate();
    ~QPrintDialogPrivate();

    void init();
    void applyPrinterProperties(QPrinter *p);

    void _q_printToFileChanged(int);
    void _q_rbPrintRangeToggled(bool);
    void _q_printerChanged(int index);
    void _q_chbPrintLastFirstToggled(bool);
#ifndef QT_NO_FILEDIALOG
    void _q_btnBrowseClicked();
#endif
    void _q_btnPropertiesClicked();
    void refreshPageSizes();

    bool setupPrinter();
    void updateWidgets();

    Ui::QPrintDialog ui;
    QList<QPrinterDescription> lprPrinters;

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    QCUPSSupport* cups;
    int cupsPrinterCount;
    const cups_dest_t* cupsPrinters;
    const ppd_file_t* cupsPPD;
#endif
};

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
class PPDOptionsModel : public QAbstractItemModel
{
    friend class PPDPropertiesDialog;
    friend class PPDOptionsEditor;
public:
    PPDOptionsModel(QCUPSSupport *cups, QObject *parent = 0);
    ~PPDOptionsModel();

    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index) const;
    Qt::ItemFlags flags(const QModelIndex& index) const;
    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

private:
    OptionTreeItem* rootItem;
    QCUPSSupport *cups;
    const ppd_file_t* ppd;
    void parseItems();
    void parseGroups(OptionTreeItem* parent);
    void parseOptions(OptionTreeItem* parent);
    void parseChoices(OptionTreeItem* parent);
};

class PPDOptionsEditor : public QItemDelegate
{
    Q_OBJECT
public:
    PPDOptionsEditor(QObject* parent = 0) : QItemDelegate(parent) {};
    ~PPDOptionsEditor() {};

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void setEditorData(QWidget* editor, const QModelIndex& index) const;
    void setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;

private slots:
    void cbChanged(int index);

};

class PPDPropertiesDialog : public QDialog , Ui::QPrintPropertiesDialog
{
    Q_OBJECT
public:
    PPDPropertiesDialog(PPDOptionsModel* model, QWidget* parent = 0, Qt::WindowFlags f = 0);
    ~PPDPropertiesDialog();

    void showEvent(QShowEvent * event);

private:
    void addItemToOptions(OptionTreeItem *parent, QList<const ppd_option_t*>& options, QList<const char*>& markedOptions);

private slots:
    void btnSaveClicked();
};
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum { Success = 's', Unavail = 'u', NotFound = 'n', TryAgain = 't' };
enum { Continue = 'c', Return = 'r' };

static void perhapsAddPrinter(QList<QPrinterDescription> *printers, const QString &name,
                               QString host, QString comment,
                               QStringList aliases = QStringList())
{
    for (int i = 0; i < printers->size(); ++i)
        if (printers->at(i).samePrinter(name))
            return;

    if (host.isEmpty())
        host = QPrintDialog::tr("locally connected");
    printers->append(QPrinterDescription(name.simplified(), host.simplified(), comment.simplified(), aliases));
}

static void parsePrinterDesc(QString printerDesc, QList<QPrinterDescription> *printers)
{
    if (printerDesc.length() < 1)
        return;

    printerDesc = printerDesc.simplified();
    int i = printerDesc.indexOf(QLatin1Char(':'));
    QString printerName, printerComment, printerHost;
    QStringList aliases;

    if (i >= 0) {
        // have ':' want '|'
        int j = printerDesc.indexOf(QLatin1Char('|'));
        if (j > 0 && j < i) {
            printerName = printerDesc.left(j);
            aliases = printerDesc.mid(j + 1, i - j - 1).split(QLatin1Char('|'));
            // try extracting a comment from the aliases
            printerComment = QPrintDialog::tr("Aliases: %1")
                             .arg(aliases.join(QLatin1String(", ")));
        } else {
            printerName = printerDesc.left(i);
        }
        // look for lprng pseudo all printers entry
        i = printerDesc.indexOf(QRegExp(QLatin1String(": *all *=")));
        if (i >= 0)
            printerName = QString();
        // look for signs of this being a remote printer
        i = printerDesc.indexOf(QRegExp(QLatin1String(": *rm *=")));
        if (i >= 0) {
            // point k at the end of remote host name
            while (printerDesc[i] != QLatin1Char('='))
                i++;
            while (printerDesc[i] == QLatin1Char('=') || printerDesc[i].isSpace())
                i++;
            j = i;
            while (j < (int)printerDesc.length() && printerDesc[j] != QLatin1Char(':'))
                j++;

            // and stuff that into the string
            printerHost = printerDesc.mid(i, j - i);
        }
    }
    if (printerName.length())
        perhapsAddPrinter(printers, printerName, printerHost, printerComment,
                           aliases);
}

static int parsePrintcap(QList<QPrinterDescription> *printers, const QString& fileName)
{
    QFile printcap(fileName);
    if (!printcap.open(QIODevice::ReadOnly))
        return NotFound;

    char *line_ascii = new char[1025];
    line_ascii[1024] = '\0';

    QString printerDesc;
    bool atEnd = false;

    while (!atEnd) {
        if (printcap.atEnd() || printcap.readLine(line_ascii, 1024) <= 0)
            atEnd = true;
        QString line = QString::fromLocal8Bit(line_ascii);
        line = line.trimmed();
        if (line.length() >= 1 && line[int(line.length()) - 1] == QLatin1Char('\\'))
            line.chop(1);
        if (line[0] == QLatin1Char('#')) {
            if (!atEnd)
                continue;
        } else if (line[0] == QLatin1Char('|') || line[0] == QLatin1Char(':')) {
            printerDesc += line;
            if (!atEnd)
                continue;
        }

        parsePrinterDesc(printerDesc, printers);

        // add the first line of the new printer definition
        printerDesc = line;
    }
    delete[] line_ascii;
    return Success;
}


// solaris, not 2.6
static void parseEtcLpPrinters(QList<QPrinterDescription> *printers)
{
    QDir lp(QLatin1String("/etc/lp/printers"));
    QFileInfoList dirs = lp.entryInfoList();
    if (dirs.isEmpty())
        return;

    QString tmp;
    for (int i = 0; i < dirs.size(); ++i) {
        QFileInfo printer = dirs.at(i);
        if (printer.isDir()) {
            tmp.sprintf("/etc/lp/printers/%s/configuration",
                         printer.fileName().toAscii().data());
            QFile configuration(tmp);
            char *line = new char[1025];
            QString remote(QLatin1String("Remote:"));
            QString contentType(QLatin1String("Content types:"));
            QString printerHost;
            bool canPrintPostscript = false;
            if (configuration.open(QIODevice::ReadOnly)) {
                while (!configuration.atEnd() &&
                        configuration.readLine(line, 1024) > 0) {
                    if (QString::fromLatin1(line).startsWith(remote)) {
                        const char *p = line;
                        while (*p != ':')
                            p++;
                        p++;
                        while (isspace((uchar) *p))
                            p++;
                        printerHost = QString::fromLocal8Bit(p);
                        printerHost = printerHost.simplified();
                    } else if (QString::fromLatin1(line).startsWith(contentType)) {
                        char *p = line;
                        while (*p != ':')
                            p++;
                        p++;
                        char *e;
                        while (*p) {
                            while (isspace((uchar) *p))
                                p++;
                            if (*p) {
                                char s;
                                e = p;
                                while (isalnum((uchar) *e))
                                    e++;
                                s = *e;
                                *e = '\0';
                                if (!qstrcmp(p, "postscript") ||
                                     !qstrcmp(p, "any"))
                                    canPrintPostscript = true;
                                *e = s;
                                if (s == ',')
                                    e++;
                                p = e;
                            }
                        }
                    }
                }
                if (canPrintPostscript)
                    perhapsAddPrinter(printers, printer.fileName(),
                                       printerHost, QLatin1String(""));
            }
            delete[] line;
        }
    }
}


// solaris 2.6
static char *parsePrintersConf(QList<QPrinterDescription> *printers, bool *found = 0)
{
    QFile pc(QLatin1String("/etc/printers.conf"));
    if (!pc.open(QIODevice::ReadOnly)) {
        if (found)
            *found = false;
        return 0;
    }
    if (found)
        *found = true;

    char *line = new char[1025];
    line[1024] = '\0';

    QString printerDesc;
    int lineLength = 0;

    char *defaultPrinter = 0;

    while (!pc.atEnd() &&
            (lineLength=pc.readLine(line, 1024)) > 0) {
        if (*line == '#') {
            *line = '\0';
            lineLength = 0;
        }
        if (lineLength >= 2 && line[lineLength-2] == '\\') {
            line[lineLength-2] = '\0';
            printerDesc += QString::fromLocal8Bit(line);
        } else {
            printerDesc += QString::fromLocal8Bit(line);
            printerDesc = printerDesc.simplified();
            int i = printerDesc.indexOf(QLatin1Char(':'));
            QString printerName, printerHost, printerComment;
            QStringList aliases;
            if (i >= 0) {
                // have : want |
                int j = printerDesc.indexOf(QLatin1Char('|'));
                if (j >= i)
                    j = -1;
                printerName = printerDesc.mid(0, j < 0 ? i : j);
                if (printerName == QLatin1String("_default")) {
                    i = printerDesc.indexOf(
                        QRegExp(QLatin1String(": *use *=")));
                    while (printerDesc[i] != QLatin1Char('='))
                        i++;
                    while (printerDesc[i] == QLatin1Char('=') || printerDesc[i].isSpace())
                        i++;
                    j = i;
                    while (j < (int)printerDesc.length() &&
                            printerDesc[j] != QLatin1Char(':') && printerDesc[j] != QLatin1Char(','))
                        j++;
                    // that's our default printer
                    defaultPrinter =
                        qstrdup(printerDesc.mid(i, j-i).toAscii().data());
                    printerName = QString();
                    printerDesc = QString();
                } else if (printerName == QLatin1String("_all")) {
                    // skip it.. any other cases we want to skip?
                    printerName = QString();
                    printerDesc = QString();
                }

                if (j > 0) {
                    // try extracting a comment from the aliases
                    aliases = printerDesc.mid(j + 1, i - j - 1).split(QLatin1Char('|'));
                    printerComment = QPrintDialog::tr("Aliases: %1")
                                     .arg(aliases.join(QLatin1String(", ")));
                }
                // look for signs of this being a remote printer
                i = printerDesc.indexOf(
                    QRegExp(QLatin1String(": *bsdaddr *=")));
                if (i >= 0) {
                    // point k at the end of remote host name
                    while (printerDesc[i] != QLatin1Char('='))
                        i++;
                    while (printerDesc[i] == QLatin1Char('=') || printerDesc[i].isSpace())
                        i++;
                    j = i;
                    while (j < (int)printerDesc.length() &&
                            printerDesc[j] != QLatin1Char(':') && printerDesc[j] != QLatin1Char(','))
                        j++;
                    // and stuff that into the string
                    printerHost = printerDesc.mid(i, j-i);
                    // maybe stick the remote printer name into the comment
                    if (printerDesc[j] == QLatin1Char(',')) {
                        i = ++j;
                        while (printerDesc[i].isSpace())
                            i++;
                        j = i;
                        while (j < (int)printerDesc.length() &&
                                printerDesc[j] != QLatin1Char(':') && printerDesc[j] != QLatin1Char(','))
                            j++;
                        if (printerName != printerDesc.mid(i, j-i)) {
                            printerComment =
                                QLatin1String("Remote name: ");
                            printerComment += printerDesc.mid(i, j-i);
                        }
                    }
                }
            }
            if (printerComment == QLatin1String(":"))
                printerComment = QString(); // for cups
            if (printerName.length())
                perhapsAddPrinter(printers, printerName, printerHost,
                                   printerComment, aliases);
            // chop away the line, for processing the next one
            printerDesc = QString();
        }
    }
    delete[] line;
    return defaultPrinter;
}

#ifndef QT_NO_NIS

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static int pd_foreach(int /*status */, char * /*key */, int /*keyLen */,
                    char *val, int valLen, char *data)
{
    parsePrinterDesc(QString::fromLatin1(val, valLen), (QList<QPrinterDescription> *)data);
    return 0;
}

#if defined(Q_C_CALLBACKS)
}
#endif

static int retrieveNisPrinters(QList<QPrinterDescription> *printers)
{
    typedef int (*WildCast)(int, char *, int, char *, int, char *);
    char printersConfByname[] = "printers.conf.byname";
    char *domain;
    int err;

    QLibrary lib(QLatin1String("nsl"));
    typedef int (*ypGetDefaultDomain)(char **);
    ypGetDefaultDomain _ypGetDefaultDomain = (ypGetDefaultDomain)lib.resolve("yp_get_default_domain");
    typedef int (*ypAll)(const char *, const char *, const struct ypall_callback *);
    ypAll _ypAll = (ypAll)lib.resolve("yp_all");

    if (_ypGetDefaultDomain && _ypAll) {
        err = _ypGetDefaultDomain(&domain);
        if (err == 0) {
            ypall_callback cb;
            // wild cast to support K&R-style system headers
            (WildCast &) cb.foreach = (WildCast) pd_foreach;
            cb.data = (char *) printers;
            err = _ypAll(domain, printersConfByname, &cb);
        }
        if (!err)
            return Success;
    }
    return Unavail;
}

#endif // QT_NO_NIS

static char *parseNsswitchPrintersEntry(QList<QPrinterDescription> *printers, char *line)
{
#define skipSpaces() \
    while (isspace((uchar) line[k])) \
        k++

    char *defaultPrinter = 0;
    bool stop = false;
    int lastStatus = NotFound;

    int k = 8;
    skipSpaces();
    if (line[k] != ':')
        return 0;
    k++;

    char *cp = strchr(line, '#');
    if (cp != 0)
        *cp = '\0';

    while (line[k] != '\0') {
        if (isspace((uchar) line[k])) {
            k++;
        } else if (line[k] == '[') {
            k++;
            skipSpaces();
            while (line[k] != '\0') {
                char status = tolower(line[k]);
                char action = '?';

                while (line[k] != '=' && line[k] != ']' && line[k] != '\0')
                    k++;
                if (line[k] == '=') {
                    k++;
                    skipSpaces();
                    action = tolower(line[k]);
                    while (line[k] != '\0' && !isspace((uchar) line[k]) && line[k] != ']')
                        k++;
                } else if (line[k] == ']') {
                    k++;
                    break;
                }
                skipSpaces();

                if (lastStatus == status)
                    stop = (action == (char) Return);
            }
        } else {
            if (stop)
                break;

            QByteArray source;
            while (!isspace((uchar) line[k]) && line[k] != '[') {
                source += line[k];
                k++;
            }

            if (source == "user") {
                lastStatus = parsePrintcap(printers,
                        QDir::homePath() + QLatin1String("/.printers"));
            } else if (source == "files") {
                bool found;
                defaultPrinter = parsePrintersConf(printers, &found);
                if (found)
                    lastStatus = Success;
#ifndef QT_NO_NIS
            } else if (source == "nis") {
                lastStatus = retrieveNisPrinters(printers);
#endif
            } else {
                // nisplus, dns, etc., are not implemented yet
                lastStatus = NotFound;
            }
            stop = (lastStatus == Success);
        }
    }
    return defaultPrinter;
}

static char *parseNsswitchConf(QList<QPrinterDescription> *printers)
{
    QFile nc(QLatin1String("/etc/nsswitch.conf"));
    if (!nc.open(QIODevice::ReadOnly))
        return 0;

    char *defaultPrinter = 0;

    char *line = new char[1025];
    line[1024] = '\0';

    while (!nc.atEnd() &&
            nc.readLine(line, 1024) > 0) {
        if (qstrncmp(line, "printers", 8) == 0) {
            defaultPrinter = parseNsswitchPrintersEntry(printers, line);
            delete[] line;
            return defaultPrinter;
        }
    }

    strcpy(line, "printers: user files nis nisplus xfn");
    defaultPrinter = parseNsswitchPrintersEntry(printers, line);
    delete[] line;
    return defaultPrinter;
}

// HP-UX
static void parseEtcLpMember(QList<QPrinterDescription> *printers)
{
    QDir lp(QLatin1String("/etc/lp/member"));
    if (!lp.exists())
        return;
    QFileInfoList dirs = lp.entryInfoList();
    if (dirs.isEmpty())
        return;

    QString tmp;
    for (int i = 0; i < dirs.size(); ++i) {
        QFileInfo printer = dirs.at(i);
        // I haven't found any real documentation, so I'm guessing that
        // since lpstat uses /etc/lp/member rather than one of the
        // other directories, it's the one to use.  I did not find a
        // decent way to locate aliases and remote printers.
        if (printer.isFile())
            perhapsAddPrinter(printers, printer.fileName(),
                               QPrintDialog::tr("unknown"),
                               QLatin1String(""));
    }
}

// IRIX 6.x
static void parseSpoolInterface(QList<QPrinterDescription> *printers)
{
    QDir lp(QLatin1String("/usr/spool/lp/interface"));
    if (!lp.exists())
        return;
    QFileInfoList files = lp.entryInfoList();
    if(files.isEmpty())
        return;

    for (int i = 0; i < files.size(); ++i) {
        QFileInfo printer = files.at(i);

        if (!printer.isFile())
            continue;

        // parse out some information
        QFile configFile(printer.filePath());
        if (!configFile.open(QIODevice::ReadOnly))
            continue;

        QByteArray line;
        line.resize(1025);
        QString namePrinter;
        QString hostName;
        QString hostPrinter;
        QString printerType;

        QString nameKey(QLatin1String("NAME="));
        QString typeKey(QLatin1String("TYPE="));
        QString hostKey(QLatin1String("HOSTNAME="));
        QString hostPrinterKey(QLatin1String("HOSTPRINTER="));

        while (!configFile.atEnd() &&
                (configFile.readLine(line.data(), 1024)) > 0) {
            QString uline = QString::fromLocal8Bit(line);
            if (uline.startsWith(typeKey) ) {
                printerType = uline.mid(nameKey.length());
                printerType = printerType.simplified();
            } else if (uline.startsWith(hostKey)) {
                hostName = uline.mid(hostKey.length());
                hostName = hostName.simplified();
            } else if (uline.startsWith(hostPrinterKey)) {
                hostPrinter = uline.mid(hostPrinterKey.length());
                hostPrinter = hostPrinter.simplified();
            } else if (uline.startsWith(nameKey)) {
                namePrinter = uline.mid(nameKey.length());
                namePrinter = namePrinter.simplified();
            }
        }
        configFile.close();

        printerType = printerType.trimmed();
        if (printerType.indexOf(QLatin1String("postscript"), 0, Qt::CaseInsensitive) < 0)
            continue;

        int ii = 0;
        while ((ii = namePrinter.indexOf(QLatin1Char('"'), ii)) >= 0)
            namePrinter.remove(ii, 1);

        if (hostName.isEmpty() || hostPrinter.isEmpty()) {
            perhapsAddPrinter(printers, printer.fileName(),
                               QLatin1String(""), namePrinter);
        } else {
            QString comment;
            comment = namePrinter;
            comment += QLatin1String(" (");
            comment += hostPrinter;
            comment += QLatin1Char(')');
            perhapsAddPrinter(printers, printer.fileName(),
                               hostName, comment);
        }
    }
}


// Every unix must have its own.  It's a standard.  Here is AIX.
static void parseQconfig(QList<QPrinterDescription> *printers)
{
    QFile qconfig(QLatin1String("/etc/qconfig"));
    if (!qconfig.open(QIODevice::ReadOnly))
        return;

    QTextStream ts(&qconfig);
    QString line;

    QString stanzaName; // either a queue or a device name
    bool up = true; // queue up?  default true, can be false
    QString remoteHost; // null if local
    QString deviceName; // null if remote

    QRegExp newStanza(QLatin1String("^[0-z\\-]*:$"));

    // our basic strategy here is to process each line, detecting new
    // stanzas.  each time we see a new stanza, we check if the
    // previous stanza was a valid queue for a) a remote printer or b)
    // a local printer.  if it wasn't, we assume that what we see is
    // the start of the first stanza, or that the previous stanza was
    // a device stanza, or that there is some syntax error (we don't
    // report those).

    do {
        line = ts.readLine();
        bool indented = line[0].isSpace();
        line = line.simplified();

        int i = line.indexOf(QLatin1Char('='));
        if (indented && i != -1) { // line in stanza
            QString variable = line.left(i).simplified();
            QString value=line.mid(i+1, line.length()).simplified();
            if (variable == QLatin1String("device"))
                deviceName = value;
            else if (variable == QLatin1String("host"))
                remoteHost = value;
            else if (variable == QLatin1String("up"))
                up = !(value.toLower() == QLatin1String("false"));
        } else if (line[0] == QLatin1Char('*')) { // comment
            // nothing to do
        } else if (ts.atEnd() || // end of file, or beginning of new stanza
                    (!indented && line.contains(newStanza))) {
            if (up && stanzaName.length() > 0 && stanzaName.length() < 21) {
                if (remoteHost.length()) // remote printer
                    perhapsAddPrinter(printers, stanzaName, remoteHost,
                                       QString());
                else if (deviceName.length()) // local printer
                    perhapsAddPrinter(printers, stanzaName, QString(),
                                       QString());
            }
            line.chop(1);
            if (line.length() >= 1 && line.length() <= 20)
                stanzaName = line;
            up = true;
            remoteHost.clear();
            deviceName.clear();
        } else {
            // syntax error?  ignore.
        }
    } while (!ts.atEnd());
}

static void populatePaperSizes(QComboBox* cb)
{
    cb->addItem(QPrintDialog::tr("A0 (841 x 1189 mm)"), QPrinter::A0);
    cb->addItem(QPrintDialog::tr("A1 (594 x 841 mm)"), QPrinter::A1);
    cb->addItem(QPrintDialog::tr("A2 (420 x 594 mm)"), QPrinter::A2);
    cb->addItem(QPrintDialog::tr("A3 (297 x 420 mm)"), QPrinter::A3);
    cb->addItem(QPrintDialog::tr("A4 (210 x 297 mm, 8.26 x 11.7 inches)"), QPrinter::A4);
    cb->addItem(QPrintDialog::tr("A5 (148 x 210 mm)"), QPrinter::A5);
    cb->addItem(QPrintDialog::tr("A6 (105 x 148 mm)"), QPrinter::A6);
    cb->addItem(QPrintDialog::tr("A7 (74 x 105 mm)"), QPrinter::A7);
    cb->addItem(QPrintDialog::tr("A8 (52 x 74 mm)"), QPrinter::A8);
    cb->addItem(QPrintDialog::tr("A9 (37 x 52 mm)"), QPrinter::A9);
    cb->addItem(QPrintDialog::tr("B0 (1000 x 1414 mm)"), QPrinter::B0);
    cb->addItem(QPrintDialog::tr("B1 (707 x 1000 mm)"), QPrinter::B1);
    cb->addItem(QPrintDialog::tr("B2 (500 x 707 mm)"), QPrinter::B2);
    cb->addItem(QPrintDialog::tr("B3 (353 x 500 mm)"), QPrinter::B3);
    cb->addItem(QPrintDialog::tr("B4 (250 x 353 mm)"), QPrinter::B4);
    cb->addItem(QPrintDialog::tr("B5 (176 x 250 mm, 6.93 x 9.84 inches)"), QPrinter::B5);
    cb->addItem(QPrintDialog::tr("B6 (125 x 176 mm)"), QPrinter::B6);
    cb->addItem(QPrintDialog::tr("B7 (88 x 125 mm)"), QPrinter::B7);
    cb->addItem(QPrintDialog::tr("B8 (62 x 88 mm)"), QPrinter::B8);
    cb->addItem(QPrintDialog::tr("B9 (44 x 62 mm)"), QPrinter::B9);
    cb->addItem(QPrintDialog::tr("B10 (31 x 44 mm)"), QPrinter::B10);
    cb->addItem(QPrintDialog::tr("C5E (163 x 229 mm)"), QPrinter::C5E);
    cb->addItem(QPrintDialog::tr("DLE (110 x 220 mm)"), QPrinter::DLE);
    cb->addItem(QPrintDialog::tr("Executive (7.5 x 10 inches, 191 x 254 mm)"), QPrinter::Executive);
    cb->addItem(QPrintDialog::tr("Folio (210 x 330 mm)"), QPrinter::Folio);
    cb->addItem(QPrintDialog::tr("Ledger (432 x 279 mm)"), QPrinter::Ledger);
    cb->addItem(QPrintDialog::tr("Legal (8.5 x 14 inches, 216 x 356 mm)"), QPrinter::Legal);
    cb->addItem(QPrintDialog::tr("Letter (8.5 x 11 inches, 216 x 279 mm)"), QPrinter::Letter);
    cb->addItem(QPrintDialog::tr("Tabloid (279 x 432 mm)"), QPrinter::Tabloid);
    cb->addItem(QPrintDialog::tr("US Common #10 Envelope (105 x 241 mm)"), QPrinter::Comm10E);
}

static int getLprPrinters(QList<QPrinterDescription>& printers)
{
    QByteArray etcLpDefault;
    parsePrintcap(&printers, QLatin1String("/etc/printcap"));
    parseEtcLpMember(&printers);
    parseSpoolInterface(&printers);
    parseQconfig(&printers);

    QFileInfo f;
    f.setFile(QLatin1String("/etc/lp/printers"));
    if (f.isDir()) {
        parseEtcLpPrinters(&printers);
        QFile def(QLatin1String("/etc/lp/default"));
        if (def.open(QIODevice::ReadOnly)) {
            etcLpDefault.resize(1025);
            if (def.readLine(etcLpDefault.data(), 1024) > 0) {
                QRegExp rx(QLatin1String("^(\\S+)"));
                if (rx.indexIn(QString::fromLatin1(etcLpDefault)) != -1)
                    etcLpDefault = rx.cap(1).toAscii();
            }
        }
    }

    char *def = 0;
    f.setFile(QLatin1String("/etc/nsswitch.conf"));
    if (f.isFile()) {
        def = parseNsswitchConf(&printers);
    } else {
        f.setFile(QLatin1String("/etc/printers.conf"));
        if (f.isFile())
            def = parsePrintersConf(&printers);
    }

    if (def) {
        etcLpDefault = def;
        delete [] def;
    }

    // all printers hopefully known.  try to find a good default
    QString dollarPrinter;
    {
        dollarPrinter = QString::fromLocal8Bit(qgetenv("PRINTER"));
        if (dollarPrinter.isEmpty())
            dollarPrinter = QString::fromLocal8Bit(qgetenv("LPDEST"));
        if (dollarPrinter.isEmpty())
            dollarPrinter = QString::fromLocal8Bit(qgetenv("NPRINTER"));
        if (dollarPrinter.isEmpty())
            dollarPrinter = QString::fromLocal8Bit(qgetenv("NGPRINTER"));
        if (!dollarPrinter.isEmpty())
            perhapsAddPrinter(&printers, dollarPrinter,
                              QPrintDialog::tr("unknown"),
                              QLatin1String(""));
    }

    int quality = 0;
    int best = 0;
    for (int i = 0; i < printers.size(); ++i) {
        QRegExp ps(QLatin1String("[^a-z]ps(?:[^a-z]|$)"));
        QRegExp lp(QLatin1String("[^a-z]lp(?:[^a-z]|$)"));

        QString name = printers.at(i).name;
        QString comment = printers.at(i).comment;
        if (quality < 4 && name == dollarPrinter) {
            best = i;
            quality = 4;
        } else if (quality < 3 && !etcLpDefault.isEmpty() &&
                    name == QLatin1String(etcLpDefault)) {
            best = i;
            quality = 3;
        } else if (quality < 2 &&
                    (name == QLatin1String("ps") ||
                     ps.indexIn(comment) != -1)) {
            best = i;
            quality = 2;
        } else if (quality < 1 &&
                    (name == QLatin1String("lp") ||
                     lp.indexIn(comment) > -1)) {
            best = i;
            quality = 1;
        }
    }

    return best;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QPrintDialogPrivate::QPrintDialogPrivate()
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    : cups(0), cupsPrinterCount(0), cupsPrinters(0), cupsPPD(0)
#endif
{}

QPrintDialogPrivate::~QPrintDialogPrivate()
{
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    delete cups;
#endif
}

void QPrintDialogPrivate::init()
{
    Q_Q(QPrintDialog);
    QPrinter* p = q->printer();

    ui.setupUi(q);
    ui.stackedWidget->setCurrentIndex(0);

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    cups = new QCUPSSupport;
    if (QCUPSSupport::isAvailable()) {
        cupsPPD = cups->currentPPD();
        cupsPrinterCount = cups->availablePrintersCount();
        cupsPrinters = cups->availablePrinters();

        for (int i = 0; i < cupsPrinterCount; ++i) {
            ui.cbPrinters->addItem(QString::fromLocal8Bit(cupsPrinters[i].name));
            if (cupsPrinters[i].is_default)
                ui.cbPrinters->setCurrentIndex(i);
        }
        //the model depends on valid ppd. so before enabling
        //the properties button we make sure the ppd is in fact
        //valid.
        if (cupsPrinterCount && cups->currentPPD()) {
            ui.btnProperties->setEnabled(true);
        }
        _q_printerChanged(cups->currentPrinterIndex());
    } else {
#endif
        int defprn = getLprPrinters(lprPrinters);
        // populating printer combo
        QList<QPrinterDescription>::const_iterator i = lprPrinters.constBegin();
        for(; i != lprPrinters.constEnd(); ++i) {
            ui.cbPrinters->addItem((*i).name);
        }
        ui.cbPrinters->setCurrentIndex(defprn);
        _q_printerChanged(defprn);
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    }
#endif
    if (!ui.cbPrinters->count())
        ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    ui.cbPaperLayout->addItem(QPrintDialog::tr("Portrait"), QPrinter::Portrait);
    ui.cbPaperLayout->addItem(QPrintDialog::tr("Landscape"), QPrinter::Landscape);

    QPushButton *printButton = ui.buttonBox->button(QDialogButtonBox::Ok);
    printButton->setText(QPrintDialog::tr("Print"));
    printButton->setDefault(true);

    applyPrinterProperties(p);

    QObject::connect(ui.buttonBox, SIGNAL(accepted()), q, SLOT(accept()));
    QObject::connect(ui.buttonBox, SIGNAL(rejected()), q, SLOT(reject()));

    QObject::connect(ui.chbPrintToFile, SIGNAL(stateChanged(int)),
                     q, SLOT(_q_printToFileChanged(int)));

    QObject::connect(ui.rbPrintRange, SIGNAL(toggled(bool)),
                     q, SLOT(_q_rbPrintRangeToggled(bool)));
    QObject::connect(ui.cbPrinters, SIGNAL(currentIndexChanged(int)),
                     q, SLOT(_q_printerChanged(int)));
    QObject::connect(ui.chbPrintLastFirst, SIGNAL(toggled(bool)),
                     q, SLOT(_q_chbPrintLastFirstToggled(bool)));

#ifndef QT_NO_FILEDIALOG
    QObject::connect(ui.btnBrowse, SIGNAL(clicked()), q, SLOT(_q_btnBrowseClicked()));
#endif
    QObject::connect(ui.btnProperties, SIGNAL(clicked()), q, SLOT(_q_btnPropertiesClicked()));
}


void QPrintDialogPrivate::applyPrinterProperties(QPrinter *p)
{
    if (p->orientation() == QPrinter::Portrait)
        ui.cbPaperLayout->setCurrentIndex(0);
    else
        ui.cbPaperLayout->setCurrentIndex(1);

    ui.sbNumCopies->setValue(p->numCopies());

    if (p->collateCopies())
        ui.chbCollate->setChecked(true);
    else
        ui.chbCollate->setChecked(false);

    if (p->pageOrder() == QPrinter::LastPageFirst)
        ui.chbPrintLastFirst->setChecked(true);
    else
        ui.chbPrintLastFirst->setChecked(false);

    if (p->colorMode() == QPrinter::Color)
        ui.chbColor->setChecked(true);
    else
        ui.chbColor->setChecked(false);

    ui.chbDuplex->setChecked(p->doubleSidedPrinting());

    QString file = p->outputFileName();
    if (!file.isEmpty()) {
        ui.chbPrintToFile->setChecked(true);
        ui.stackedWidget->setCurrentIndex(1);
        ui.gbDestination->setTitle(QApplication::translate("QPrintDialog","File"));
        ui.leFile->setText(file);
    }
    QString printer = p->printerName();
    if (!printer.isEmpty()) {
        for (int i = 0; i < ui.cbPrinters->count(); ++i) {
            if (ui.cbPrinters->itemText(i) == printer) {
                ui.cbPrinters->setCurrentIndex(i);
                break;
            }
        }
    }
}

void QPrintDialogPrivate::_q_printToFileChanged(int state)
{
    Q_Q(QPrintDialog);
    if (state == Qt::Checked) {
        ui.stackedWidget->setCurrentIndex(1);
        ui.gbDestination->setTitle(QPrintDialog::tr("File"));
        QString fileName = q->printer()->outputFileName();
        if (fileName.isEmpty()) {
            QString home = QString::fromLocal8Bit(::qgetenv("HOME").constData());
            QString cur = QDir::currentPath();
            if (home.at(home.length()-1) != QLatin1Char('/'))
                home += QLatin1Char('/');
            if (cur.at(cur.length()-1) != QLatin1Char('/'))
                cur += QLatin1Char('/');
            if (cur.left(home.length()) != home)
                cur = home;
#ifdef Q_WS_X11
            cur += QLatin1String("print.pdf");
#endif
            ui.leFile->setText(cur);
        } else {
            ui.leFile->setText(fileName);
        }

        ui.leFile->setCursorPosition(ui.leFile->text().length());
        ui.leFile->selectAll();
        ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    } else {
        ui.stackedWidget->setCurrentIndex(0);
        ui.gbDestination->setTitle(QPrintDialog::tr("Printer"));
        if (!ui.cbPrinters->count())
            ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    refreshPageSizes();
}

void QPrintDialogPrivate::_q_rbPrintRangeToggled(bool checked)
{
    if (checked) {
        ui.sbFrom->setEnabled(true);
        ui.sbTo->setEnabled(true);
    } else {
        ui.sbFrom->setEnabled(false);
        ui.sbTo->setEnabled(false);
    }
}

void QPrintDialogPrivate::_q_printerChanged(int index)
{
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    if (QCUPSSupport::isAvailable()) {
        cups->setCurrentPrinter(index);
        cupsPPD = cups->currentPPD();
        //only enable properties if ppd is valid
        ui.btnProperties->setEnabled(cupsPPD != 0);
        // set printer info line
        QString info;
        if (cupsPPD)
            info = QString::fromLocal8Bit(cupsPPD->manufacturer) + QLatin1String(" - ") + QString::fromLocal8Bit(cupsPPD->modelname);
        ui.lbPrinterInfo->setText(info);
    } else {
#endif
        if (lprPrinters.count() > 0) {
            QString info = lprPrinters.at(index).name + QLatin1String("@") + lprPrinters.at(index).host;
            if (!lprPrinters.at(index).comment.isEmpty())
            info += QLatin1String(", ") + lprPrinters.at(index).comment;
            ui.lbPrinterInfo->setText(info);
        }
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    }
#endif

    refreshPageSizes();
}

void QPrintDialogPrivate::_q_chbPrintLastFirstToggled(bool checked)
{
    Q_Q(QPrintDialog);
    if (checked)
        q->printer()->setPageOrder(QPrinter::LastPageFirst);
    else
        q->printer()->setPageOrder(QPrinter::FirstPageFirst);
}

void QPrintDialogPrivate::refreshPageSizes()
{
    ui.cbPaperSize->blockSignals(true);
    ui.cbPaperSize->clear();

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    if (QCUPSSupport::isAvailable() && ui.chbPrintToFile->checkState() !=  Qt::Checked) {
        const ppd_option_t* pageSizes = cups->pageSizes();
        int numChoices = pageSizes ? pageSizes->num_choices : 0;

        for (int i = 0; i < numChoices; ++i) {
            ui.cbPaperSize->addItem(QString::fromLocal8Bit(pageSizes->choices[i].text), QByteArray(pageSizes->choices[i].choice));
            if (static_cast<int>(pageSizes->choices[i].marked) == 1)
                ui.cbPaperSize->setCurrentIndex(i);
        }
    } else {
#endif
        Q_Q(QPrintDialog);
        populatePaperSizes(ui.cbPaperSize);
        ui.cbPaperSize->setCurrentIndex(ui.cbPaperSize->findData(q->printer()->pageSize()));
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    }
#endif

    ui.cbPaperSize->blockSignals(false);
}

#ifndef QT_NO_FILEDIALOG
void QPrintDialogPrivate::_q_btnBrowseClicked()
{
    Q_Q(QPrintDialog);
    QString fileName = QFileDialog::getSaveFileName(q, QPrintDialog::tr("Print To File ..."),
                                                    ui.leFile->text());
    if (!fileName.isNull())
        ui.leFile->setText(fileName);
}
#endif

void QPrintDialogPrivate::_q_btnPropertiesClicked()
{
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    PPDOptionsModel model(cups);
    PPDPropertiesDialog dialog(&model);
    dialog.exec();

    const ppd_option_t* pageSizes = cups->pageSizes();
    int numChoices = pageSizes ? pageSizes->num_choices : 0;
    for (int i = 0; i < numChoices; ++i) {
        if (static_cast<int>(pageSizes->choices[i].marked) == 1)
            ui.cbPaperSize->setCurrentIndex(i);
    }
#endif
}

bool QPrintDialogPrivate::setupPrinter()
{
    Q_Q(QPrintDialog);
    QPrinter* p = q->printer();

    // printer or file name
    if (ui.chbPrintToFile->isChecked()) {
        QString file = ui.leFile->text();
#ifndef QT_NO_MESSAGEBOX
        QFile f(file);
        QFileInfo fi(f);
        bool exists = fi.exists();
        bool opened = false;
        if (exists && fi.isDir()) {
            QMessageBox::warning(q, q->windowTitle(),
			    QPrintDialog::tr("%1 is a directory.\nPlease choose a different file name.").arg(file));
            return false;
        } else if ((exists && !fi.isWritable()) || !(opened = f.open(QFile::Append))) {
            QMessageBox::warning(q, q->windowTitle(),
			    QPrintDialog::tr("File %1 is not writable.\nPlease choose a different file name.").arg(file));
            return false;
        } else if (exists) {
            int ret = QMessageBox::question(q, q->windowTitle(),
                                            QPrintDialog::tr("%1 already exists.\nDo you want to overwrite it?").arg(file),
                                            QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
            if (ret == QMessageBox::No)
                return false;
        }
        if (opened) {
            f.close();
            if (!exists)
                f.remove();
        }
#endif
        p->setOutputFileName(file);
    } else {
        p->setPrinterName(ui.cbPrinters->currentText());
        p->setOutputFileName(QString());
    }

    // print range
    if (ui.rbPrintAll->isChecked()) {
        p->setPrintRange(QPrinter::AllPages);
        p->setFromTo(0,0);
    } else if (ui.rbPrintSelection->isChecked()) {
        p->setPrintRange(QPrinter::Selection);
        p->setFromTo(0,0);
    } else if (ui.rbPrintRange->isChecked()) {
        p->setPrintRange(QPrinter::PageRange);
        p->setFromTo(ui.sbFrom->value(), ui.sbTo->value());
    }

    // copies
    p->setNumCopies(ui.sbNumCopies->value());
    p->setCollateCopies(ui.chbCollate->isChecked());
    p->setDoubleSidedPrinting(ui.chbDuplex->isChecked());

    // paper format
    QVariant val = ui.cbPaperSize->itemData(ui.cbPaperSize->currentIndex());
    int ps = p->pageSize();
    if (val.type() == QVariant::Int) {
        ps = val.toInt();
    }
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    else if (QCUPSSupport::isAvailable() && ui.chbPrintToFile->checkState() !=  Qt::Checked
             && cups->currentPPD()) {
        QByteArray cupsPageSize = val.toByteArray();
        QPrintEngine *engine = p->printEngine();
        engine->setProperty(PPK_CupsStringPageSize, QString::fromLatin1(cupsPageSize));
        engine->setProperty(PPK_CupsOptions, cups->options());

        QRect pageRect = cups->pageRect(cupsPageSize);
        engine->setProperty(PPK_CupsPageRect, pageRect);

        QRect paperRect = cups->paperRect(cupsPageSize);
        engine->setProperty(PPK_CupsPaperRect, paperRect);

        for(ps = 0; ps < QPrinter::NPageSize; ++ps) {
            QPdf::PaperSize size = QPdf::paperSize(QPrinter::PageSize(ps));
            if (size.width == paperRect.width() && size.height == paperRect.height())
                break;
        }
    }
#endif
    p->setPageSize(static_cast<QPrinter::PageSize>(ps));
    p->setOrientation(static_cast<QPrinter::Orientation>(ui.cbPaperLayout->itemData(ui.cbPaperLayout->currentIndex()).toInt()));

    // other
    if (ui.chbColor->isChecked())
        p->setColorMode(QPrinter::Color);
    else
        p->setColorMode(QPrinter::GrayScale);

    return true;
}

void QPrintDialogPrivate::updateWidgets()
{
    Q_Q(QPrintDialog);
    ui.gbPrintRange->setEnabled(q->isOptionEnabled(QPrintDialog::PrintPageRange) ||
                                q->isOptionEnabled(QPrintDialog::PrintSelection));

    ui.rbPrintRange->setEnabled(q->isOptionEnabled(QPrintDialog::PrintPageRange));
    ui.rbPrintSelection->setEnabled(q->isOptionEnabled(QPrintDialog::PrintSelection));
    ui.chbPrintToFile->setEnabled(q->isOptionEnabled(QPrintDialog::PrintToFile));
    ui.chbCollate->setEnabled(q->isOptionEnabled(QPrintDialog::PrintCollateCopies));

    switch (q->printRange()) {
    case QPrintDialog::AllPages:
        ui.gbPrintRange->setChecked(true);
        break;
    case QPrintDialog::Selection:
        ui.rbPrintSelection->setChecked(true);
        break;
    case QPrintDialog::PageRange:
        ui.rbPrintRange->setChecked(true);
        break;
    default:
        break;
    }

    ui.sbFrom->setMinimum(q->minPage());
    ui.sbTo->setMinimum(q->minPage());
    ui.sbFrom->setMaximum(q->maxPage());
    ui.sbTo->setMaximum(q->maxPage());

    ui.sbFrom->setValue(q->fromPage());
    ui.sbTo->setValue(q->toPage());
}

////////////////////////////////////////////////////////////////////////////////

QPrintDialog::QPrintDialog(QPrinter *printer, QWidget *parent) : QAbstractPrintDialog(*(new QPrintDialogPrivate), printer, parent)
{
    Q_D(QPrintDialog);
    d->init();
}

QPrintDialog::~QPrintDialog()
{}

int QPrintDialog::exec()
{
    Q_D(QPrintDialog);

    d->updateWidgets();

  redo:
    int status = QDialog::exec();
    if (status == QDialog::Accepted)
        if (!d->setupPrinter())
            goto redo;
    return status;
}


#ifdef QT3_SUPPORT
QPrinter *QPrintDialog::printer() const
{
    Q_D(const QPrintDialog);
    return d->printer;
}

void QPrintDialog::setPrinter(QPrinter *printer, bool pickupSettings)
{
    if (!printer)
        return;

    Q_D(QPrintDialog);
    d->printer = printer;

    if (pickupSettings)
        d->applyPrinterProperties(printer);
}

void QPrintDialog::addButton(QPushButton *button)
{
    Q_D(QPrintDialog);
    d->ui.buttonBox->addButton(button, QDialogButtonBox::HelpRole);
}
#endif // QT3_SUPPORT

////////////////////////////////////////////////////////////////////////////////
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)

class OptionTreeItem
{
public:
    enum ItemType { Root, Group, Option, Choice };

    OptionTreeItem(ItemType t, int i, const void* p, const char* desc, OptionTreeItem* pi)
        : type(t),
          index(i),
          ptr(p),
          description(desc),
          selected(-1),
          selDescription(0),
          parentItem(pi) {};

    ~OptionTreeItem() {
        while (!childItems.isEmpty())
            delete childItems.takeFirst();
    };

    ItemType type;
    int index;
    const void* ptr;
    const char* description;
    int selected;
    const char* selDescription;
    OptionTreeItem* parentItem;
    QList<OptionTreeItem*> childItems;
};

////////////////////////////////////////////////////////////////////////////////

PPDPropertiesDialog::PPDPropertiesDialog(PPDOptionsModel* model, QWidget* parent, Qt::WindowFlags f) : QDialog(parent, f)
{
    setupUi(this);
    treeView->setItemDelegate(new PPDOptionsEditor(this));
    treeView->setModel(model);

    for (int i = 0; i < model->rowCount(); ++i) {
        treeView->expand(model->index(i,0));
    }

    connect(btnSave, SIGNAL(clicked()), this, SLOT(btnSaveClicked()));
}

PPDPropertiesDialog::~PPDPropertiesDialog()
{}

void PPDPropertiesDialog::showEvent(QShowEvent* event)
{
    treeView->resizeColumnToContents(0);
    event->accept();
}

void PPDPropertiesDialog::btnSaveClicked()
{
    PPDOptionsModel* model = static_cast<PPDOptionsModel*>(treeView->model());
    OptionTreeItem* rootItem = model->rootItem;
    QList<const ppd_option_t*> options;
    QList<const char*> markedOptions;

    addItemToOptions(rootItem, options, markedOptions);
    model->cups->saveOptions(options, markedOptions);
}

void PPDPropertiesDialog::addItemToOptions(OptionTreeItem *parent, QList<const ppd_option_t*>& options, QList<const char*>& markedOptions)
{
    for (int i = 0; i < parent->childItems.count(); ++i) {

        OptionTreeItem *itm = parent->childItems.at(i);
        if (itm->type == OptionTreeItem::Option) {
            const ppd_option_t* opt = reinterpret_cast<const ppd_option_t*>(itm->ptr);
            options << opt;
            if (qstrcmp(opt->defchoice, opt->choices[itm->selected].choice) != 0) {
                markedOptions << opt->keyword << opt->choices[itm->selected].choice;
            }
        } else {
            addItemToOptions(itm, options, markedOptions);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

PPDOptionsModel::PPDOptionsModel(QCUPSSupport *c, QObject *parent)
    : QAbstractItemModel(parent), cups(c), ppd(c->currentPPD())
{
    parseItems();
}

PPDOptionsModel::~PPDOptionsModel()
{}

int PPDOptionsModel::columnCount(const QModelIndex&) const
{
    return 2;
}

int PPDOptionsModel::rowCount(const QModelIndex& parent) const
{
    OptionTreeItem* itm;
    if (!parent.isValid())
        itm = rootItem;
    else
        itm = reinterpret_cast<OptionTreeItem*>(parent.internalPointer());

    if (itm->type == OptionTreeItem::Option)
        return 0;

    return itm->childItems.count();
}

QVariant PPDOptionsModel::data(const QModelIndex& index, int role) const
{
    switch(role) {
        case Qt::FontRole: {
            OptionTreeItem* itm = reinterpret_cast<OptionTreeItem*>(index.internalPointer());
            if (itm && itm->type == OptionTreeItem::Group){
                QFont font = QApplication::font();
                font.setBold(true);
                return QVariant(font);
            }
            return QVariant();
        }
        break;

        case Qt::DisplayRole: {
            OptionTreeItem* itm;
            if (!index.isValid())
                itm = rootItem;
            else
                itm = reinterpret_cast<OptionTreeItem*>(index.internalPointer());

            if (index.column() == 0)
                return cups->unicodeString(itm->description);
            else if (itm->type == OptionTreeItem::Option && itm->selected > -1)
                return cups->unicodeString(itm->selDescription);
            else
                return QVariant();
        }
        break;

        default:
            return QVariant();
    }
    if (role != Qt::DisplayRole)
        return QVariant();
}

QModelIndex PPDOptionsModel::index(int row, int column, const QModelIndex& parent) const
{
    OptionTreeItem* itm;
    if (!parent.isValid())
        itm = rootItem;
    else
        itm = reinterpret_cast<OptionTreeItem*>(parent.internalPointer());

    return createIndex(row, column, itm->childItems.at(row));
}


QModelIndex PPDOptionsModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    OptionTreeItem* itm = reinterpret_cast<OptionTreeItem*>(index.internalPointer());

    if (itm->parentItem && itm->parentItem != rootItem)
        return createIndex(itm->parentItem->index, 0, itm->parentItem);
    else
        return QModelIndex();
}

Qt::ItemFlags PPDOptionsModel::flags(const QModelIndex& index) const
{
    if (!index.isValid() || reinterpret_cast<OptionTreeItem*>(index.internalPointer())->type == OptionTreeItem::Group)
        return Qt::ItemIsEnabled;

    if (index.column() == 1)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void PPDOptionsModel::parseItems()
{
    rootItem = new OptionTreeItem(OptionTreeItem::Root, 0, ppd, "Root Item", 0);
    parseGroups(rootItem);
}

void PPDOptionsModel::parseGroups(OptionTreeItem* parent)
{
    if (parent->type == OptionTreeItem::Root) {

        const ppd_file_t* ppdFile = reinterpret_cast<const ppd_file_t*>(parent->ptr);

        if (ppdFile) {
            for (int i = 0; i < ppdFile->num_groups; ++i) {
                OptionTreeItem* group = new OptionTreeItem(OptionTreeItem::Group, i, &ppdFile->groups[i], ppdFile->groups[i].text, parent);
                parent->childItems.append(group);
                parseGroups(group); // parse possible subgroups
                parseOptions(group); // parse options
            }
        }
    } else if (parent->type == OptionTreeItem::Group) {

        const ppd_group_t* group = reinterpret_cast<const ppd_group_t*>(parent->ptr);

        if (group) {
            for (int i = 0; i < group->num_subgroups; ++i) {
                OptionTreeItem* subgroup = new OptionTreeItem(OptionTreeItem::Group, i, &group->subgroups[i], group->subgroups[i].text, parent);
                parent->childItems.append(subgroup);
                parseGroups(subgroup); // parse possible subgroups
                parseOptions(subgroup); // parse options
            }
        }
    }
}

void PPDOptionsModel::parseOptions(OptionTreeItem* parent)
{
    const ppd_group_t* group = reinterpret_cast<const ppd_group_t*>(parent->ptr);
    for (int i = 0; i < group->num_options; ++i) {
        OptionTreeItem* opt = new OptionTreeItem(OptionTreeItem::Option, i, &group->options[i], group->options[i].text, parent);
        parent->childItems.append(opt);
        parseChoices(opt);
    }
}

void PPDOptionsModel::parseChoices(OptionTreeItem* parent)
{
    const ppd_option_t* option = reinterpret_cast<const ppd_option_t*>(parent->ptr);
    bool marked = false;
    for (int i = 0; i < option->num_choices; ++i) {
        OptionTreeItem* choice = new OptionTreeItem(OptionTreeItem::Choice, i, &option->choices[i], option->choices[i].text, parent);
        if (static_cast<int>(option->choices[i].marked) == 1) {
            parent->selected = i;
            parent->selDescription = option->choices[i].text;
            marked = true;
        } else if (!marked && qstrcmp(option->choices[i].choice, option->defchoice) == 0) {
            parent->selected = i;
            parent->selDescription = option->choices[i].text;
        }
        parent->childItems.append(choice);
    }
}

QVariant PPDOptionsModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch(section){
        case 0:
            return QVariant(QApplication::translate("PPDOptionsModel","Name"));
        case 1:
            return QVariant(QApplication::translate("PPDOptionsModel","Value"));
        default:
            return QVariant();
    }
}

////////////////////////////////////////////////////////////////////////////////

QWidget* PPDOptionsEditor::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const
{
    if (index.column() == 1 && reinterpret_cast<OptionTreeItem*>(index.internalPointer())->type == OptionTreeItem::Option)
        return new QComboBox(parent);
    else
        return 0;
}

void PPDOptionsEditor::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (index.column() != 1)
        return;

    QComboBox* cb = static_cast<QComboBox*>(editor);
    OptionTreeItem* itm = reinterpret_cast<OptionTreeItem*>(index.internalPointer());

    if (itm->selected == -1)
        cb->addItem(QString());

    for (int i = 0; i < itm->childItems.count(); ++i)
        cb->addItem(QString::fromLocal8Bit(itm->childItems.at(i)->description));

    if (itm->selected > -1)
        cb->setCurrentIndex(itm->selected);

    connect(cb, SIGNAL(currentIndexChanged(int)), this, SLOT(cbChanged(int)));
}

void PPDOptionsEditor::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QComboBox* cb = static_cast<QComboBox*>(editor);
    OptionTreeItem* itm = reinterpret_cast<OptionTreeItem*>(index.internalPointer());

    if (itm->selected == cb->currentIndex())
        return;

    const ppd_option_t* opt = reinterpret_cast<const ppd_option_t*>(itm->ptr);
    PPDOptionsModel* m = static_cast<PPDOptionsModel*>(model);

    if (m->cups->markOption(opt->keyword, opt->choices[cb->currentIndex()].choice) == 0) {
        itm->selected = cb->currentIndex();
        itm->selDescription = reinterpret_cast<const ppd_option_t*>(itm->ptr)->choices[itm->selected].text;
    }
}

void PPDOptionsEditor::cbChanged(int)
{
    emit commitData(static_cast<QWidget*>(sender()));
}

#endif

#include "moc_qprintdialog.cpp"
#include "qprintdialog_unix.moc"

#endif // QT_NO_PRINTDIALOG
