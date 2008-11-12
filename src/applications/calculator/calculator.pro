qtopia_project(qtopia app)
TARGET=calculator
CONFIG+=qtopia_main

# library files
HEADERS = calculator.h engine.h data.h instruction.h display.h \
            interfaces/stdinputwidgets.h integerdata.h integerinstruction.h
SOURCES = calculator.cpp engine.cpp data.cpp instruction.cpp \
            display.cpp interfaces/stdinputwidgets.cpp main.cpp

# double type
HEADERS += doubledata.h
SOURCES += doubledata.cpp

FRACTION_HEADERS = fractiondata.h fractioninstruction.h doubleinstruction.h
FRACTION_SOURCES = fractiondata.cpp fractioninstruction.cpp doubleinstruction.cpp
PHONE_HEADERS = interfaces/phone.h phoneinstruction.h \
                doubleinstruction.h interfaces/simple.h
PHONE_SOURCES = interfaces/phone.cpp phoneinstruction.cpp \
                doubleinstruction.cpp interfaces/simple.cpp
PHONE_FORMS=helperpanel.ui

SIMPLEUI_SOURCES = interfaces/simple.cpp
SIMPLEUI_HEADERS = interfaces/simple.h

FRACTIONUI_SOURCES = interfaces/fraction.cpp
FRACTIONUI_HEADERS = interfaces/fraction.h

SCIENCEUI_SOURCES = interfaces/advanced.cpp
SCIENCEUI_HEADERS = interfaces/advanced.h

CONVERSIONUI_SOURCES = interfaces/conversion.cpp
CONVERSIONUI_HEADERS = interfaces/conversion.h
                
TRANSLATABLES = $${FRACTION_SOURCES} $${FRACTION_HEADERS} \
                $${PHONE_SOURCES} $${PHONE_HEADERS} \
                $${SIMPLEUI_SOURCES} $${SIMPLEUI_HEADERS} \
                $${FRACTIONUI_SOURCES} $${FRACTIONUI_HEADERS} \
                $${SCIENCEUI_SOURCES} $${SCIENCEUI_HEADERS} \
                $${CONVERSIONUI_SOURCES} $${CONVERSIONUI_HEADERS} 

                
!phone {
#removing these components will save app size:
#ENABLE_FRACTION                - 320 kB
#ENABLE_SCIENCE                 - 340 kB
#ENABLE_CONVERSION              - 250 kB + size of conversion files
DEFINES+=ENABLE_FRACTION ENABLE_SCIENCE ENABLE_CONVERSION
        
    HEADERS += $${FRACTION_HEADERS}
    SOURCES += $${FRACTION_SOURCES}

    HEADERS += $${FRACTIONUI_HEADERS} $${SIMPLEUI_HEADERS} \
                $${SCIENCEUI_HEADERS} $${CONVERSIONUI_HEADERS}
    SOURCES += $${FRACTIONUI_SOURCES} $${SIMPLEUI_SOURCES} \
                $${SCIENCEUI_SOURCES} $${CONVERSIONUI_SOURCES}
} else {
    HEADERS+= $${PHONE_HEADERS}
    SOURCES+= $${PHONE_SOURCES}
    FORMS+=$${PHONE_FORMS}
}

desktop.files=$$QTOPIA_DEPOT_PATH/apps/Applications/calculator.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop
help.source=$$QTOPIA_DEPOT_PATH/help
help.files=calculator*
help.hint=help
INSTALLS+=help
pics.files=$$QTOPIA_DEPOT_PATH/pics/calculator/*
pics.path=/pics/calculator
pics.hint=pics
INSTALLS+=pics

pkg.name=qpe-calculator
pkg.desc=A simple calculator for Qtopia.
pkg.multi=libraries/qtopiacalc
pkg.domain=trusted

#idep(LIBS+=-l$$TARGET)
