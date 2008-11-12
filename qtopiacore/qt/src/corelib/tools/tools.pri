# Qt tools module

HEADERS +=  \
	tools/qalgorithms.h \
	tools/qbitarray.h \
	tools/qbytearray.h \
	tools/qbytearraymatcher.h \
	tools/qcache.h \
	tools/qchar.h \
	tools/qcontainerfwd.h \
	tools/qcryptographichash.h \
	tools/qdatetime.h \
	tools/qdatetime_p.h \
	tools/qhash.h \
        tools/qline.h \
	tools/qlinkedlist.h \
	tools/qlist.h \
	tools/qlocale.h \
	tools/qlocale_p.h \
	tools/qlocale_data_p.h \
	tools/qmap.h \
        tools/qpodlist_p.h \
        tools/qpoint.h \
	tools/qqueue.h \
        tools/qrect.h \
	tools/qregexp.h \
	tools/qringbuffer_p.h \
	tools/qshareddata.h \
	tools/qset.h \
        tools/qsize.h \
	tools/qstack.h \
	tools/qstring.h \
	tools/qstringlist.h \
	tools/qstringmatcher.h \
	tools/qtimeline.h \
	tools/qunicodetables_p.h \
	tools/qvarlengtharray.h \
	tools/qvector.h


SOURCES += \
	tools/qbitarray.cpp \
	tools/qbytearray.cpp \
	tools/qbytearraymatcher.cpp \
	tools/qcryptographichash.cpp \
	tools/qdatetime.cpp \
	tools/qhash.cpp \
        tools/qline.cpp \
	tools/qlinkedlist.cpp \
	tools/qlistdata.cpp \
	tools/qlocale.cpp \
        tools/qpoint.cpp \
	tools/qmap.cpp \
        tools/qrect.cpp \
	tools/qregexp.cpp \
	tools/qshareddata.cpp \
        tools/qsize.cpp \
	tools/qstring.cpp \
	tools/qstringlist.cpp \
	tools/qtimeline.cpp \
	tools/qvector.cpp \
        tools/qvsnprintf.cpp


#zlib support
contains(QT_CONFIG, zlib) {
   INCLUDEPATH += ../3rdparty/zlib
   SOURCES+= \
	../3rdparty/zlib/adler32.c \
	../3rdparty/zlib/compress.c \
	../3rdparty/zlib/crc32.c \
	../3rdparty/zlib/deflate.c \
	../3rdparty/zlib/gzio.c \
	../3rdparty/zlib/inffast.c \
	../3rdparty/zlib/inflate.c \
	../3rdparty/zlib/inftrees.c \
	../3rdparty/zlib/trees.c \
	../3rdparty/zlib/uncompr.c \
	../3rdparty/zlib/zutil.c
} else:!contains(QT_CONFIG, no-zlib) {
   unix:LIBS += -lz
#  win32:LIBS += libz.lib
}

!macx-icc:unix:LIBS += -lm
