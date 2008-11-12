TEMPLATE = subdirs
contains(gfx-plugins, qvfb)	    :SUBDIRS += qvfb
contains(gfx-plugins, vnc)	    :SUBDIRS += vnc
contains(gfx-plugins, transformed)  :SUBDIRS += transformed
contains(gfx-plugins, hybrid)       :SUBDIRS += hybrid
