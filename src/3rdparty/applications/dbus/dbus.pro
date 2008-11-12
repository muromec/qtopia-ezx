qtopia_project(stub)

session_conf.commands = $$COMMAND_HEADER\
	perl -pe $${LITERAL_SQUOTE}s,\@DBUS_SESSION_SOCKET_DIR\@,$$QTOPIA_PREFIX/dbus/session,;s,\@EXPANDED_DATADIR\@,$$QTOPIA_PREFIX/dbus/session,$${LITERAL_SQUOTE} \
        $$SRCDIR/session.conf.in >$(INSTALL_ROOT)/dbus/session.conf
session_conf.path = /dbus
INSTALLS+=session_conf

dbus_session_services.path = /dbus/session/services
dbus_session_services.commands = true
INSTALLS+=dbus_session_services

qtopia_dbus_script.path = /bin
qtopia_dbus_script.commands = $$COMMAND_HEADER\
	perl -pe $${LITERAL_SQUOTE}s,\@DBUSEXEC\@,$$DBUS_PREFIX/bin/dbus-daemon,;s,\@DBUSSESSIONCONFIG\@,$$QTOPIA_PREFIX/dbus/session.conf,$${LITERAL_SQUOTE} \
	$$SRCDIR/qtopia-dbus-daemon.in >$(INSTALL_ROOT)/bin/qtopia-dbus-daemon && \
	chmod a+x $(INSTALL_ROOT)/bin/qtopia-dbus-daemon
INSTALLS+=qtopia_dbus_script

