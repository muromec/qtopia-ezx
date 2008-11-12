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
#ifdef ENABLE_CONVERSION

#include "conversion.h"
#include "../doubleinstruction.h"

#include <qtopiaapplication.h>
#include <qsettings.h>

#include <qlayout.h>
#include <qfile.h>
#include <qdir.h>
#include <QDesktopWidget>

double UnitConversionInstruction::from = 0;
double UnitConversionInstruction::to = 0;
char UnitConversionInstruction::tempFrom = 'n';
char UnitConversionInstruction::tempTo = 'n';

FormConversion::FormConversion(QWidget *parent) : DecimalInputWidget(parent),
currentButton(0)
{
    systemEngine->registerInstruction(new UnitConversionInstruction());

    setObjectName( "ConversionForm" ); //no tr

    InputWidgetLayout = new QGridLayout(this);
    InputWidgetLayout->setSpacing( 3 );
    InputWidgetLayout->setMargin( 0 );

    init(0, 0);
}

void FormConversion::init(int fromRow, int fromCol)
{
    typeSelector = new QComboBox(this);
    typeSelector->setSizePolicy(sizePolicy());
    InputWidgetLayout->addWidget(typeSelector, fromRow, fromCol, 1, 2 );

    PBC = new QPushButton(this);
    PBC->setText(tr("CE/C"));
    PBC->setSizePolicy(sizePolicy());
    InputWidgetLayout->addWidget(PBC, fromRow, fromCol+2, 1, 2);

    QRect screenRect = QtopiaApplication::desktop()->availableGeometry();
    bool portrait = screenRect.width() < screenRect.height();

    modeList = new QList<conversionMode*>();
    conversionMode *nextMode;
    QWidget *w;
    conversionData *nextData;
    conversionStack = new QStackedWidget(this);
    conversionStack->setSizePolicy(sizePolicy());

    // temperature conversions are builtin
    nextMode = new conversionMode;
    nextMode->name = QString( tr("Temperature") );
    nextMode->dataList = new QList<conversionData*>();
    typeSelector->addItem(nextMode->name);
    w = new QWidget();
    QBoxLayout *gl2;
    if (portrait)
        gl2 = new QHBoxLayout(w);
    else
        gl2 = new QVBoxLayout(w);
    gl2->setSpacing( 3 );
    gl2->setMargin( 0 );
    //InputWidgetLayout->addLayout(gl2, fromRow, fromCol, 1, 4);
    conversionStack->addWidget(w);

    nextData = new conversionData;
    nextData->name = QString( tr("fahrenheit") );
    nextData->button = new QPushButton(w);
    nextData->button->setCheckable(true);
    nextData->button->setText(nextData->name);
    nextData->button->setSizePolicy(sizePolicy());
    connect(nextData->button,SIGNAL(clicked()),this,SLOT(fahrenheitButtonClicked()));
    nextMode->dataList->append(nextData);
    gl2->addWidget(nextData->button);

    nextData = new conversionData;
    nextData->name = QString( tr("celcius") );
    nextData->button = new QPushButton(w);
    nextData->button->setCheckable(true);
    nextData->button->setText(nextData->name);
    nextData->button->setSizePolicy(sizePolicy());
    connect(nextData->button,SIGNAL(clicked()),this,SLOT(celciusButtonClicked()));
    nextMode->dataList->append(nextData);
    gl2->addWidget(nextData->button);

    nextData = new conversionData;
    nextData->name = QString( tr("kelvin") );
    nextData->button = new QPushButton(w);
    nextData->button->setCheckable(true);
    nextData->button->setText(nextData->name);
    nextData->button->setSizePolicy(sizePolicy());
    connect(nextData->button,SIGNAL(clicked()),this,SLOT(kelvinButtonClicked()));
    nextMode->dataList->append(nextData);
    gl2->addWidget(nextData->button);

    modeList->append(nextMode);
    // end setting up temperature conversion

    // read conversion data files
    QString path = Qtopia::qtopiaDir() + "etc/calculator/";
    QDir directory;
    directory.setPath( path );
    QStringList fileList = directory.entryList(QStringList("*.conf"));

    if ( !fileList.count() )
        qWarning("No conversion data files are available.");
    else {
        QFile myFile;
        QGridLayout *gl;
        //iterate over found conf files
        for (int i = 0; i < fileList.count(); i++) {
            QString fileName = fileList[i];
            QSettings cfg(path+fileName, QSettings::IniFormat);
            QStringList groups = cfg.childGroups();
            nextMode = new conversionMode;
            nextMode->dataList = new QList<conversionData*>();
            w = new QWidget();
            gl = new QGridLayout(w);
            //gl = new FlowLayout(w, 5, 3);
            gl->setSizeConstraint( QLayout::SetDefaultConstraint );
            gl->setSpacing( 3 );
            gl->setMargin( 0 );
            conversionStack->addWidget(w);
            for (int j = 0; j < groups.count(); j++) {
                QString group = groups[j];
                if (group == "Translation")
                    continue;
                cfg.beginGroup(group);
                if (group == "Name") {
                    nextMode->name = cfg.value("Name").toString();
                    typeSelector->addItem(nextMode->name);
                } else {
                    nextData = new conversionData;
                    nextData->name = cfg.value("Name").toString();
                    nextData->factor = cfg.value("Factor").toString().toDouble();
                    if (nextData->factor == 1)
                       nextMode->defaultType = nextData;
                    nextMode->dataList->append(nextData);

                    //create the button for this data conversion
                    QPushButton *pb = new QPushButton(w);
                    pb->setCheckable(true);
                    pb->setText(nextData->name);
                    pb->setSizePolicy(sizePolicy());
                    int cRow = cfg.value("Row").toInt();
                    int cCol = cfg.value("Column").toInt();
                    if (cRow < 0 || cCol < 0) {
                        qWarning(QString("Missing grid position in config file: %1 group: %2").arg(fileName).arg(nextData->name).toLocal8Bit().constData());
                    } else if (portrait)
                        gl->addWidget(pb, cRow, cCol);
                    else
                        gl->addWidget(pb, cCol, cRow);


                    //gl->addWidget(pb);
                    connect(pb, SIGNAL(clicked()), this, SLOT(conversionButtonClicked()));
                    nextData->button = pb;
                }
            }
            modeList->append(nextMode);

        }
    }


    connect (PBC, SIGNAL(clicked()), this, SLOT(CClicked()));
    connect (typeSelector, SIGNAL(activated(int)), this, SLOT(selectType(int)));

    // Remember last selection
    QSettings config("Trolltech","calculator"); // No tr
    config.beginGroup("Conversion plugin");
    int lastView = config.value("lastView",0).toInt();
    if (lastView > (int)modeList->count()+1 || lastView < 0)
        lastView = 0;

    typeSelector->setCurrentIndex(lastView);
    conversionStack->setCurrentIndex(lastView);

    if (portrait) {
        InputWidgetLayout->addWidget(conversionStack, fromRow+1, fromCol, 2, 4);
        DecimalInputWidget::init(fromRow+3, fromCol);
    } else {
        InputWidgetLayout->addWidget(conversionStack, fromRow+1, fromCol, 5, 2);
        DecimalInputWidget::init(fromRow+1, fromCol+2);
    }

}
FormConversion::~FormConversion() {
    QSettings config("Trolltech","calculator"); // No tr
    config.beginGroup("Conversion plugin"); // No tr
    config.setValue("lastView", conversionStack->currentIndex());
}

void FormConversion::showEvent ( QShowEvent *e ) {
    systemEngine->setAccType("Double"); // No tr

    QWidget::showEvent(e);
}

void FormConversion::selectType(int i) {
    UnitConversionInstruction::tempFrom = UnitConversionInstruction::tempTo = 'n';

    int currentMode = conversionStack->currentIndex();
    QList<conversionData*> dataList = *(modeList->at(currentMode))->dataList;

    for (int c = 0;c < dataList.count(); c++) {
        QPushButton *b = (*(dataList.at(c))).button;
        if (b->isChecked())
            b->toggle();
    }
    currentButton = 0;
    conversionStack->setCurrentIndex(i);
}

void FormConversion::conversionButtonClicked() {
    doConversion();
}

void FormConversion::doConversion() {
    int currentMode = conversionStack->currentIndex();
    QList<conversionData*> dataList = *(modeList->at(currentMode))->dataList;

    bool newButtonFound = false;
    for (int c = 0;c < dataList.count(); c++) {
        QPushButton *b = (*(dataList.at(c))).button;
        if (b->isChecked()) {
            if (b == currentButton) {
                b->toggle();
            } else {
                newButton = b;
                newFactor = (*(dataList.at(c))).factor;
                newButtonFound = true;
            }
        }
    }
    if (newButtonFound) {
        if (currentButton) { // temp convert ignores these anyway
            UnitConversionInstruction::from = currentFactor;
            UnitConversionInstruction::to = newFactor;
            systemEngine->pushInstruction("UnitConversion");
        }
        currentButton = newButton;
        currentFactor = newFactor;
    } else { // current button toggled off
        currentButton = 0;
    }
}

void FormConversion::kelvinButtonClicked() {
    UnitConversionInstruction::tempFrom = UnitConversionInstruction::tempTo;
    UnitConversionInstruction::tempTo = 'k';
    doConversion();
}
void FormConversion::celciusButtonClicked() {
    UnitConversionInstruction::tempFrom = UnitConversionInstruction::tempTo;
    UnitConversionInstruction::tempTo = 'c';
    doConversion();
}
void FormConversion::fahrenheitButtonClicked() {
    UnitConversionInstruction::tempFrom = UnitConversionInstruction::tempTo;
    UnitConversionInstruction::tempTo = 'f';
    doConversion();
}

void FormConversion::CClicked() {
    systemEngine->hardReset();
}

#endif //ENABLE_CONVERSION
