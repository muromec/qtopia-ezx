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
#ifdef ENABLE_SCIENCE

#include "advanced.h"
#include "../doubleinstruction.h"

#include <QLayout>
#include <QPainter>
#include <QRadioButton>
#include <QDesktopWidget>

#include <qtopiaapplication.h>

static void drawRootOfX(QPainter *p, int x, int y, int fontSize)
{
    int fontSize05 = fontSize / 2;
    p->drawText(x+fontSize05-2,y,"x");
    p->drawLine(x-fontSize,y-fontSize05,x-fontSize05,y);
    p->drawLine(x-fontSize05,y,x,y-fontSize);
    p->drawLine(x,y-fontSize,x+fontSize+3,y-fontSize);
}

class PowerButton : public AdvancedButton {
public:
    PowerButton(QWidget *p = 0) : AdvancedButton(p) { }
    void advancedDrawLabel( QPainter *p, int x, int y ) {
        if ( inv ) {
            p->drawText(x - 5, y - fontSize05 - 2, "y");
            drawRootOfX(p, x + fontSize05 - 2, y + 2, fontSize10);
        } else {
            p->drawText(x, y, "x");
            p->drawText(x + fontSize05, y - fontSize05, "y");
        }
    }
};

class SquareButton : public AdvancedButton {
public:
    SquareButton(QWidget *p = 0) : AdvancedButton(p) { }
    void advancedDrawLabel( QPainter *p, int x, int y ) {
        if ( inv ) {
            drawRootOfX(p, x, y, fontSize10);
        } else {
            p->drawText(x, y, "x");
            p->drawText(x+fontSize05, y-fontSize05, "2");
        }
    }
};

class LnButton : public AdvancedButton {
public:
    LnButton(QWidget *p = 0) : AdvancedButton(p) { }
    void advancedDrawLabel( QPainter *p, int x, int y ) {
        if ( inv ) {
            p->drawText(x, y, "e");
            p->drawText(x+fontSize05, y-fontSize05, "x");
        } else {
            p->drawText(x, y, "Ln");
        }
    }
};

class OneOverButton : public AdvancedButton {
public:
    OneOverButton(QWidget *p = 0) : AdvancedButton(p) { }
    void advancedDrawLabel( QPainter *p, int x, int y ) {
        p->drawText(x + fontSize05, y - fontSize05, "1");
        int between = y + (fontSize05 / 2) - fontSize05;
        p->drawLine(x, between, x + fontSize10 + fontSize05, between);
        p->drawText(x + fontSize05, y + fontSize05, "x");
    }
};

FormAdvanced::FormAdvanced(QWidget *parent) : DecimalInputWidget(parent), IsInverse(false){

    setWhatsThis(tr("The scientific calculator plugin provides the most comprehensive "
                "interface. Many buttons have a second mode which can be activated via <b>INV</b>."));

    setObjectName( "FormAdvanced" );

    InputWidgetLayout = new QGridLayout(this);
    InputWidgetLayout->setSpacing( 0 );
    InputWidgetLayout->setMargin( 0 );

    init(0, 0);
}

void FormAdvanced::init(int fromRow, int fromCol)
{
    QWidget * gridWidget = new QWidget(this);
    QHBoxLayout *hlayout = new QHBoxLayout( gridWidget );
    hlayout->setMargin( 0 );
    hlayout->setSpacing( 0 );
    gridWidget->setSizePolicy(sizePolicy());

    degree = new QRadioButton(tr("Deg", "Degree mode"));
    hlayout->addWidget( degree );
    degree->setWhatsThis(tr("Calculate in degree mode"));
    radians = new QRadioButton(tr("Rad", "Radian mode"));
    hlayout->addWidget( radians );
    radians->setWhatsThis(tr("Calculate in radian mode"));
    gradians = new QRadioButton(tr("Gra", "Gradian mode"));
    hlayout->addWidget( gradians );
    gradians->setWhatsThis(tr("Calculate in gradian mode"));
    degree->setChecked(true);

    connect(degree, SIGNAL(toggled(bool)), this, SLOT(DegreeMode(bool)));
    connect(radians, SIGNAL(toggled(bool)), this, SLOT(RadiansMode(bool)));
    connect(gradians, SIGNAL(toggled(bool)), this, SLOT(GradiansMode(bool)));

    QRect screenRect = QtopiaApplication::desktop()->availableGeometry();
    bool portrait = screenRect.width() < screenRect.height();
    if ( portrait) { //portrait
        InputWidgetLayout->addWidget(gridWidget, fromRow, fromCol, 1, 4);
        DecimalInputWidget::init(fromRow+6, fromCol);
    } else { //landscape
        InputWidgetLayout->addWidget(gridWidget, fromRow, fromCol, 1, 4);
        DecimalInputWidget::init(fromRow, fromCol+4);
    }

    PBMPlus = new QPushButton;
    PBMPlus->setSizePolicy(sizePolicy());
    PBMPlus->setText(tr("M+"));

    PBMR = new QPushButton;
    PBMR->setSizePolicy(sizePolicy());
    PBMR->setText(tr("MR"));

    PBMC = new QPushButton;
    PBMC->setSizePolicy(sizePolicy());
    PBMC->setText(tr("MC"));

    PBC = new QPushButton;
    PBC->setSizePolicy(sizePolicy());
    PBC->setText(tr("CE/C"));

    if (portrait) {
        InputWidgetLayout->addWidget(PBMPlus, fromRow+1, fromCol);
        InputWidgetLayout->addWidget(PBMR, fromRow + 1, fromCol+1);
        InputWidgetLayout->addWidget(PBMC, fromRow+1, fromCol+2);
        InputWidgetLayout->addWidget(PBC, fromRow+1, fromCol+3);
    } else {
        InputWidgetLayout->addWidget(PBMPlus, fromRow+3, fromCol+3);
        InputWidgetLayout->addWidget(PBMR, fromRow+4, fromCol+3);
        InputWidgetLayout->addWidget(PBMC, fromRow+2, fromCol+3);
        InputWidgetLayout->addWidget(PBC, fromRow+1, fromCol+3);
    }

    PBPi = new QPushButton;
    PBPi->setSizePolicy(sizePolicy());
    PBPi->setText(tr("Pi"));

    PBE = new QPushButton;
    PBE->setSizePolicy(sizePolicy());
    PBE->setText(tr("e"));

    PBSin = new QPushButton;
    PBSin->setSizePolicy(sizePolicy());
    PBSin->setText(tr("Sin"));

    PBCos = new QPushButton;
    PBCos->setSizePolicy(sizePolicy());
    PBCos->setText(tr("Cos"));

    PBTan = new QPushButton;
    PBTan->setSizePolicy(sizePolicy());
    PBTan->setText(tr("Tan"));

    PBFactorial = new QPushButton;
    PBFactorial->setSizePolicy(sizePolicy());
    PBFactorial->setText(tr("n!"));

    PBLn = (AdvancedButton*)new LnButton;
    PBLn->setSizePolicy(sizePolicy());

    PBInverse = new QPushButton;
    PBInverse->setSizePolicy(sizePolicy());
    PBInverse->setText(tr("INV", "inverse mode"));

    PBSquare = (AdvancedButton*)new SquareButton;
    PBSquare->setSizePolicy(sizePolicy());

    PBPow = (AdvancedButton*)new PowerButton;
    PBPow->setSizePolicy(sizePolicy());

    PBOneOverX = (QPushButton*)new OneOverButton;
    PBOneOverX->setSizePolicy(sizePolicy());

    PBLog = new QPushButton;
    PBLog->setSizePolicy(sizePolicy());
    PBLog->setText(tr("Log"));

    if (portrait) {
        InputWidgetLayout->addWidget(PBPi, fromRow+2, fromCol+2);
        InputWidgetLayout->addWidget(PBSin, fromRow+3, fromCol);
        InputWidgetLayout->addWidget(PBCos, fromRow+3, fromCol+1);
        InputWidgetLayout->addWidget(PBTan, fromRow+3, fromCol+2);
        InputWidgetLayout->addWidget(PBFactorial, fromRow+2, fromCol);
        InputWidgetLayout->addWidget(PBLn, fromRow+4, fromCol+3);
        InputWidgetLayout->addWidget(PBInverse, fromRow+2, fromCol+3);
        InputWidgetLayout->addWidget(PBE, fromRow+2, fromCol+1);
        InputWidgetLayout->addWidget(PBSquare, fromRow+4, fromCol);
        InputWidgetLayout->addWidget(PBPow, fromRow+4, fromCol+1);
        InputWidgetLayout->addWidget(PBLog, fromRow+3, fromCol+3);
        InputWidgetLayout->addWidget(PBOneOverX, fromRow+4, fromCol+2);
    } else {
        InputWidgetLayout->addWidget(PBPi, fromRow+1, fromCol+1);
        InputWidgetLayout->addWidget(PBSin, fromRow+2, fromCol);
        InputWidgetLayout->addWidget(PBCos, fromRow+2, fromCol+1);
        InputWidgetLayout->addWidget(PBTan, fromRow+2, fromCol+2);
        InputWidgetLayout->addWidget(PBFactorial, fromRow+4, fromCol+2);
        InputWidgetLayout->addWidget(PBLn, fromRow+3, fromCol+1);
        InputWidgetLayout->addWidget(PBInverse, fromRow+1, fromCol+2);
        InputWidgetLayout->addWidget(PBE, fromRow+1, fromCol);
        InputWidgetLayout->addWidget(PBSquare, fromRow+4, fromCol);
        InputWidgetLayout->addWidget(PBPow, fromRow+4, fromCol+1);
        InputWidgetLayout->addWidget(PBLog, fromRow+3, fromCol);
        InputWidgetLayout->addWidget(PBOneOverX, fromRow+3, fromCol+2);
    }

    connect (PBMR, SIGNAL(clicked()), this, SLOT(MRClicked()));
    connect (PBMC, SIGNAL(clicked()), this, SLOT(MCClicked()));
    connect (PBMPlus, SIGNAL(clicked()), this, SLOT(MPlusClicked()));
    connect (PBC, SIGNAL(clicked()), this, SLOT(CClicked()));

    connect (PBPi, SIGNAL(clicked()), this, SLOT(PiClicked()));
    connect (PBE, SIGNAL(clicked()), this, SLOT(EClicked()));
    connect (PBInverse, SIGNAL(clicked()), this, SLOT(InverseClicked()));

    connect (PBSin, SIGNAL(clicked()), this, SLOT(SinDegClicked()));
    connect (PBCos, SIGNAL(clicked()), this, SLOT(CosDegClicked()));
    connect (PBTan, SIGNAL(clicked()), this, SLOT(TanDegClicked()));
    connect (PBOneOverX, SIGNAL(clicked()), this, SLOT(OneOverXClicked()));

    connect (PBLn, SIGNAL(clicked()), this, SLOT(LnClicked()));
    connect (PBLog, SIGNAL(clicked()), this, SLOT(LogClicked()));
    connect (PBFactorial, SIGNAL(clicked()), this, SLOT(FactorialClicked()));

    connect (PBSquare, SIGNAL(clicked()), this, SLOT(SquareClicked()));
    connect (PBPow, SIGNAL(clicked()), this, SLOT(PowClicked()));
}

FormAdvanced::~FormAdvanced() {
    delete PBPi;
    delete PBC;
    delete PBMR;
    delete PBMC;
    delete PBMPlus;
    delete PBSin;
    delete PBCos;
    delete PBTan;
    delete PBOneOverX;
    delete PBLn;
    delete PBFactorial;
    delete PBSquare;
    delete PBPow;
    delete PBLog;
    delete degree;
    delete gradians;
    delete radians;
}

void FormAdvanced::showEvent ( QShowEvent *e ) {
    systemEngine->setAccType("Double"); // No tr

    QWidget::showEvent(e);
}
void FormAdvanced::MCClicked() {
    systemEngine->memoryReset();
}
void FormAdvanced::MRClicked() {
    systemEngine->memoryRecall();
}
void FormAdvanced::MPlusClicked() {
    systemEngine->memorySave();
}
void FormAdvanced::CClicked() {
    systemEngine->dualReset();
}

void FormAdvanced::DelClicked() {
    systemEngine->delChar();
}

void FormAdvanced::SinDegClicked() {
    if (IsInverse) {
        systemEngine->pushInstruction("aSinDeg"); // no tr
        InverseClicked();
    }
    else
        systemEngine->pushInstruction("SinDeg"); // No tr
}
void FormAdvanced::CosDegClicked() {
    if (IsInverse) {
        systemEngine->pushInstruction("aCosDeg"); // no tr
        InverseClicked();
    }
    else
        systemEngine->pushInstruction("CosDeg"); // No tr
}
void FormAdvanced::TanDegClicked() {
    if (IsInverse) {
        systemEngine->pushInstruction("aTanDeg"); // no tr
        InverseClicked();
    }
    else
        systemEngine->pushInstruction("TanDeg"); // No tr
}
void FormAdvanced::SinRadClicked() {
    if (IsInverse) {
        systemEngine->pushInstruction("aSinRad"); // no tr
        InverseClicked();
    }
    else
        systemEngine->pushInstruction("SinRad"); // No tr
}
void FormAdvanced::CosRadClicked() {
    if (IsInverse) {
        systemEngine->pushInstruction("aCosRad"); // no tr
        InverseClicked();
    }
    else
        systemEngine->pushInstruction("CosRad"); // No tr
}
void FormAdvanced::TanRadClicked() {
    if (IsInverse) {
        systemEngine->pushInstruction("aTanRad"); // no tr
        InverseClicked();
    }
    else
        systemEngine->pushInstruction("TanRad"); // No tr
}
void FormAdvanced::SinGraClicked() {
    if (IsInverse) {
        systemEngine->pushInstruction("aSinGra"); // no tr
        InverseClicked();
    }
    else
        systemEngine->pushInstruction("SinGra"); // No tr
}
void FormAdvanced::CosGraClicked() {
    if (IsInverse) {
        systemEngine->pushInstruction("aCosGra"); // no tr
        InverseClicked();
    }
    else
        systemEngine->pushInstruction("CosGra"); // No tr
}
void FormAdvanced::TanGraClicked() {
    if (IsInverse) {
        systemEngine->pushInstruction("aTanGra"); // no tr
        InverseClicked();
    }
    else
        systemEngine->pushInstruction("TanGra"); // No tr
}
void FormAdvanced::OneOverXClicked() {
    systemEngine->pushInstruction("One over x"); // No tr
}
void FormAdvanced::LnClicked() {
    if (IsInverse) {
        systemEngine->pushInstruction("Exp"); //no tr
        InverseClicked();
    }
    else
        systemEngine->pushInstruction("Ln"); // No tr
}
void FormAdvanced::LogClicked() {
    systemEngine->pushInstruction("Log"); //no tr
}
void FormAdvanced::FactorialClicked() {
    systemEngine->pushInstruction("Factorial"); // No tr
}
void FormAdvanced::SquareClicked() {
    if (IsInverse) {
        systemEngine->pushInstruction("Square root"); // No tr
        InverseClicked();
    }
    else
        systemEngine->pushInstruction("Square"); // No tr
}
void FormAdvanced::PowClicked() {
    if (IsInverse) {
        systemEngine->pushInstruction("X root Y"); //no tr
        InverseClicked();
    }
    else
        systemEngine->pushInstruction("Pow"); // No tr
}
void FormAdvanced::PiClicked() {
    systemEngine->push(QString("3.14159265358979323846")); // No tr
}
void FormAdvanced::EClicked() {
    systemEngine->push(QString("2.71828182845904523536")); // No tr
}
void FormAdvanced::InverseClicked() {
    IsInverse = ! IsInverse;
    if (IsInverse) {
        PBSin->setText(tr("ASin"));
        PBCos->setText(tr("ACos"));
        PBTan->setText(tr("ATan"));
    } else {
        PBSin->setText(tr("Sin"));
        PBCos->setText(tr("Cos"));
        PBTan->setText(tr("Tan"));
    }

    PBSquare->setInversed( IsInverse );
    PBPow->setInversed( IsInverse );
    PBLn->setInversed( IsInverse );
}

void FormAdvanced::DegreeMode(bool on) {
    if (on) {
        radians->setChecked(false);
        gradians->setChecked(false);
        PBSin->disconnect();
        PBCos->disconnect();
        PBTan->disconnect();
        connect (PBSin, SIGNAL(clicked()), this, SLOT(SinDegClicked()));
        connect (PBCos, SIGNAL(clicked()), this, SLOT(CosDegClicked()));
        connect (PBTan, SIGNAL(clicked()), this, SLOT(TanDegClicked()));
    }
}
void FormAdvanced::RadiansMode(bool on) {
    if (on) {
        degree->setChecked(false);
        gradians->setChecked(false);
        PBSin->disconnect();
        PBCos->disconnect();
        PBTan->disconnect();
        connect (PBSin, SIGNAL(clicked()), this, SLOT(SinRadClicked()));
        connect (PBCos, SIGNAL(clicked()), this, SLOT(CosRadClicked()));
        connect (PBTan, SIGNAL(clicked()), this, SLOT(TanRadClicked()));
    }
}
void FormAdvanced::GradiansMode(bool on) {
    if (on) {
        radians->setChecked(false);
        degree->setChecked(false);
        PBSin->disconnect();
        PBCos->disconnect();
        PBTan->disconnect();
        connect (PBSin, SIGNAL(clicked()), this, SLOT(SinGraClicked()));
        connect (PBCos, SIGNAL(clicked()), this, SLOT(CosGraClicked()));
        connect (PBTan, SIGNAL(clicked()), this, SLOT(TanGraClicked()));
    }
}
#endif //ENABLE_SCIENCE
