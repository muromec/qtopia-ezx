qconfig=$$(QTEDIR)/mkspecs/qconfig.pri
exists($$qconfig):include($$qconfig)
else:error(Missing $$qconfig!)
!contains(QT_CONFIG,glib):error(No Glib!)
