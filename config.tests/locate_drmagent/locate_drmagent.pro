TEMPLATE=app
CONFIG-=qt
QMAKE_LINK=@true
printsearchdirs.commands=$(CC) -print-search-dirs
QMAKE_EXTRA_TARGETS+=printsearchdirs
ALL_DEPS+=printsearchdirs
