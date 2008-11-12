TEMPLATE = subdirs

SUBDIRS	*= accessible imageformats sqldrivers iconengines
unix {
        contains(QT_CONFIG,iconv)|contains(QT_CONFIG,gnu-libiconv):SUBDIRS *= codecs
} else {
        SUBDIRS *= codecs
}
embedded:SUBDIRS *=  gfxdrivers decorations mousedrivers
!win32:!embedded:!mac:SUBDIRS *= inputmethods
