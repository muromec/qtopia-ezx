TEMPLATE = subdirs

!contains(QT_CONFIG, no-jpeg):SUBDIRS += jpeg
!contains(QT_CONFIG, no-gif):SUBDIRS += gif
!contains(QT_CONFIG, no-mng):SUBDIRS += mng
!contains(QT_CONFIG, no-svg):SUBDIRS += svg
!contains(QT_CONFIG, no-tiff):SUBDIRS += tiff
