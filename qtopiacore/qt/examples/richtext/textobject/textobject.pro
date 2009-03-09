HEADERS         = svgtextobject.h \
                  window.h
SOURCES         = main.cpp \
                  svgtextobject.cpp \
                  window.cpp

QT += svg

# install
target.path = $$[QT_INSTALL_EXAMPLES]/itemviews/stardelegate
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/itemviews/stardelegate
INSTALLS += target sources

