PREFIX=IR
VPATH+=ir

IR_HEADERS+=\
    qiriasdatabase.h\
    qirlocaldevice.h\
    qirnamespace.h\
    qirremotedevice.h \
    qirremotedevicewatcher.h \
    qirserver.h \
    qirsocket.h \
    qirglobal.h

IR_PRIVATE_HEADERS+=\
    qirnamespace_p.h \
    qirsocketengine_p.h

IR_SOURCES+=\
    qiriasdatabase.cpp\
    qirlocaldevice.cpp\
    qirnamespace.cpp\
    qirremotedevice.cpp \
    qirremotedevicewatcher.cpp \
    qirserver.cpp \
    qirsocket.cpp

unix {
    IR_SOURCES += qirsocketengine_unix.cpp
}

sdk_ir_headers.files=$$IR_HEADERS
sdk_ir_headers.path=/include/qtopia/comm
sdk_ir_headers.hint=sdk headers
INSTALLS+=sdk_ir_headers

sdk_ir_private_headers.files=$$IR_PRIVATE_HEADERS
sdk_ir_private_headers.path=/include/qtopiacomm/private
sdk_ir_private_headers.hint=sdk headers
INSTALLS+=sdk_ir_private_headers

