TEMPLATE    = subdirs
SUBDIRS     = delayedencoding \
              draggableicons \
              draggabletext \
              dropsite \
              fridgemagnets \
              puzzle

wince*: SUBDIRS -= dropsite
# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/draganddrop
INSTALLS += sources
