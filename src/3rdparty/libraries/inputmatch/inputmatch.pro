qtopia_project(qtopia lib)
TARGET=inputmatch
license(FREEWARE)

CONFIG+=no_tr

HEADERS = pkimmatcher.h symbolpicker.h wordpicker.h picker.h inputmatchglobal.h

SOURCES = pkimmatcher.cpp symbolpicker.cpp wordpicker.cpp picker.cpp
sdk_inputmatch_headers.files=$${HEADERS}
sdk_inputmatch_headers.path=/include/qtopia/inputmatch
sdk_inputmatch_headers.hint=sdk headers
INSTALLS+=sdk_inputmatch_headers

idep(LIBS+=-l$$TARGET)
qt_inc($$TARGET)
