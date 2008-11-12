TEMPLATE=app
CONFIG-=qt x11 app_bundle
SINGLEEXEC=$$(SINGLEEXEC)
equals(SINGLEEXEC,1) {
    QMAKE_BEHAVIORS=
    foo="bar"
    bar=bar
    !equals(foo,$$bar):QMAKE_BEHAVIORS+=keep_quotes
    # Deal with qmake behaviors
    contains(QMAKE_BEHAVIORS,keep_quotes) {
        LITERAL_ESCAPED_QUOTE=\"
    } else {
        LITERAL_ESCAPED_QUOTE=\\\"
    }
    SOURCES+=main.cpp
    DEFINES+=MAIN_FILE=$$LITERAL_ESCAPED_QUOTE$$(QT_DEPOT)/config.tests/unix/openssl/openssl.cpp$$LITERAL_ESCAPED_QUOTE
    LIBS+=-lssl -lcrypto
} else {
    SOURCES+=$$(QT_DEPOT)/config.tests/unix/openssl/openssl.cpp
}

