TEMPLATE = subdirs
contains(style-plugins, windows)	:SUBDIRS += windows
contains(style-plugins, motif)		:SUBDIRS += motif
contains(style-plugins, plastique)	:SUBDIRS += plastique
contains(style-plugins, cde)		:SUBDIRS += cde
mac:contains(style-plugins, mac)	:SUBDIRS += mac
win32:contains(style-plugins, windowsxp):SUBDIRS += windowsxp
