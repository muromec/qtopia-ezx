qtopia_project(subdirs)

SUBDIRS=$$PROJECTS
build_qtopia:SUBDIRS+=../etc/themes build/extra
#else:build_qtopiadesktop:SUBDIRS+=qtopiadesktop
enable_singleexec:SUBDIRS-=server
