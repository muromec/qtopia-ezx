HEADERS   = filelistmodel.h \
            window.h
SOURCES   = filelistmodel.cpp \
            main.cpp \
            window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/itemviews/stardelegate
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/itemviews/stardelegate
INSTALLS += target sources

