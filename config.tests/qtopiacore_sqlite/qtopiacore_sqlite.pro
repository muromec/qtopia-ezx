qconfig=$$(QTEDIR)/.qmake.cache
exists($$qconfig):include($$qconfig)
else:error(Missing $$qconfig!)
!contains(CONFIG,system-sqlite):error(system-sqlite was not enabled!)
