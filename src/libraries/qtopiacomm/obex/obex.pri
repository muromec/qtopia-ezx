PREFIX=OBEX
VPATH+=obex

DEFINES+=QTOPIA_OBEX

OBEX_HEADERS+=\
    qobexglobal.h \
    qobexpushclient.h\
    qobexpushservice.h\
    qobexnamespace.h\
    qobexheader.h\
    qobexserversession.h\
    qobexclientsession.h\
    qobexauthenticationchallenge.h \
    qobexauthenticationresponse.h \
    qobexfolderlistingentryinfo.h\
    qobexftpclient.h

OBEX_SOURCES+=\
    qobexpushclient.cpp\
    qobexpushservice.cpp\
    qobexsocket.cpp\
    qobexcommand_p.cpp\
    qobexheader.cpp\
    qobexserversession.cpp\
    qobexclientsession.cpp\
    qobexauthenticationchallenge.cpp \
    qobexauthenticationresponse.cpp \
    qobexauthentication_p.cpp \
    qobexfolderlistingentryinfo.cpp \
    qobexfolderlisting.cpp \
    qobexftpclient.cpp 

OBEX_PRIVATE_HEADERS+=\
    qobexcommand_p.h\
    qobexheader_p.h\
    qobexauthentication_p.h \
    qobexauthenticationchallenge_p.h \
    qobexauthenticationresponse_p.h \
    qobexsocket_p.h \
    qobexclientsession_p.h \
    qobexserversession_p.h \
    qobexfolderlisting_p.h

sdk_obex_headers.files=$$OBEX_HEADERS
sdk_obex_headers.path=/include/qtopia/comm
sdk_obex_headers.hint=sdk headers
INSTALLS+=sdk_obex_headers

sdk_obex_private_headers.files=$$OBEX_PRIVATE_HEADERS
sdk_obex_private_headers.path=/include/qtopiacomm/private
sdk_obex_private_headers.hint=sdk headers
INSTALLS+=sdk_obex_private_headers
