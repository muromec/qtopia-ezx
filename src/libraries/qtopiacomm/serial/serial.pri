PREFIX=SERIAL
VPATH+=serial

SERIAL_HEADERS+=\
    qgsm0710multiplexer.h\
    qserialiodevicemultiplexer.h\
    qserialiodevicemultiplexerplugin.h\
    qmultiportmultiplexer.h\
    qserialiodevice.h\
    qserialport.h\
    qserialsocket.h\
    qatchat.h\
    qatresult.h\
    qatresultparser.h\
    qatchatscript.h\
    qretryatchat.h\
    qatutils.h\
    qgsmcodec.h

SERIAL_PRIVATE_HEADERS+=\
    qserialiodevice_p.h\
    qprefixmatcher_p.h\
    qatchat_p.h\
    qpassthroughserialiodevice_p.h\
    gsm0710_p.h

SERIAL_SOURCES+=\
    qgsm0710multiplexer.cpp\
    qserialiodevicemultiplexer.cpp\
    qserialiodevicemultiplexerplugin.cpp\
    qmultiportmultiplexer.cpp\
    qserialiodevice.cpp\
    qserialport.cpp\
    qserialsocket.cpp\
    qatchat.cpp\
    qatresult.cpp\
    qatresultparser.cpp\
    qatchatscript.cpp\
    qprefixmatcher.cpp\
    qretryatchat.cpp\
    qatutils.cpp\
    qgsmcodec.cpp\
    qpassthroughserialiodevice.cpp\
    gsm0710.c

sdk_serial_headers.files=$$SERIAL_HEADERS
sdk_serial_headers.path=/include/qtopia/serial
sdk_serial_headers.hint=sdk headers
INSTALLS+=sdk_serial_headers

sdk_serial_private_headers.files=$$SERIAL_PRIVATE_HEADERS
sdk_serial_private_headers.path=/include/qtopiacomm/private
sdk_serial_private_headers.hint=sdk headers
INSTALLS+=sdk_serial_private_headers

