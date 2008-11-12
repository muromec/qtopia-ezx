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

#ifndef QPRINTENGINE_QWS_P_H
#define QPRINTENGINE_QWS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtGui/qprinter.h"

#ifndef QT_NO_PRINTER

#include "QtGui/qprintengine.h"
#include "QtCore/qbytearray.h"
#include "private/qpaintengine_p.h"

class QtopiaPrintEnginePrivate;
class QRasterPaintEngine;
class QPrinterPrivate;
class QImage;

class QtopiaPrintEngine : public QPaintEngine, public QPrintEngine
{
    Q_DECLARE_PRIVATE(QtopiaPrintEngine)
public:
    QtopiaPrintEngine(QPrinter::PrinterMode mode);

    // override QWSPaintEngine
    bool begin(QPaintDevice *dev);
    bool end();
    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr);
    void drawTextItem(const QPointF &p, const QTextItem &ti);
    QPaintEngine::Type type() const { return QPaintEngine::X11; }

    QPaintEngine *paintEngine() const;

    void updateState(const QPaintEngineState &state);

    QRect paperRect() const;
    QRect pageRect() const;

    bool newPage();
    bool abort();

    QPrinter::PrinterState printerState() const;

    int metric(QPaintDevice::PaintDeviceMetric metricType) const;

    QVariant property(PrintEnginePropertyKey key) const;
    void setProperty(PrintEnginePropertyKey key, const QVariant &value);

private:
    friend class QPrintDialog;
    friend class QPageSetupDialog;

    void clearPage();
    void flushPage();
};

class QtopiaPrintBuffer
{
public:
    QtopiaPrintBuffer( bool bigEndian=FALSE ) { _bigEndian = bigEndian; }
    ~QtopiaPrintBuffer() {}

    const QByteArray& data() const { return _data; }

    int size() const { return _data.size(); }

    void clear() { _data.clear(); }

    void append( char value ) { _data.append( value ); }
    void append( short value );
    void append( int value );
    void append( const QByteArray& array ) { _data.append( array ); }

    void patch( int posn, int value );

    void pad();

private:
    QByteArray _data;
    bool _bigEndian;
};

#define	QT_QWS_PRINTER_DEFAULT_DPI	   200

class QtopiaPrintEnginePrivate : public QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QtopiaPrintEngine)
public:
    QtopiaPrintEnginePrivate(QPrinter::PrinterMode m) :
	mode(m),
	printerState(QPrinter::Idle),
	orientation(QPrinter::Portrait),
	pageSize(QPrinter::A4),
	pageOrder(QPrinter::FirstPageFirst),
	colorMode(QPrinter::GrayScale),
	paperSource(QPrinter::OnlyOne),
        resolution(QT_QWS_PRINTER_DEFAULT_DPI),
        paintEngine(0),
	numCopies(1),
        outputToFile(false),
        fullPage(false),
        collateCopies(false),
	pageNumber(0),
	pageImage(0),
	partialByte(0),
	partialBits(0)
    {
    }
    ~QtopiaPrintEnginePrivate();

    void initialize();

    QPrinter::PrinterMode mode;

    QString printerName;
    QString outputFileName;
    QString printProgram;
    QString docName;
    QString creator;

    QPrinter::PrinterState printerState;

    QPrinter::Orientation orientation;
    QPrinter::PageSize pageSize;
    QPrinter::PageOrder pageOrder;
    QPrinter::ColorMode colorMode;
    QPrinter::PaperSource paperSource;

    int resolution;
    QPaintEngine *paintEngine;
    int numCopies;

    bool outputToFile;
    bool fullPage;
    bool collateCopies;

    int pageNumber;

    QImage *pageImage;

    QtopiaPrintBuffer buffer;

    // Definitions that are only relevant to G3FAX output.
    int ifdPatch;
    int partialByte;
    int partialBits;
    void writeG3FaxHeader();
    void writeG3FaxPage();
    int writeG3IFDEntry( int tag, int type, int count, int value );
    void writeG3Code( int code, int bits );
    void writeG3WhiteRun( int len );
    void writeG3BlackRun( int len );
    void writeG3EOL();
};

#endif // QT_NO_PRINTER

#endif // QPRINTENGINE_QWS_P_H
