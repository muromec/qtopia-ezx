# This file contains projects that are eligable for inclusion in the Opensource Edition.

# Qtopia Platform projects (required for every edition)
PROJECTS*=\
    # A placeholder for installing Qt files
    qt \
    server \
    applications/helpbrowser \
    libraries/handwriting \
    libraries/qtopiabase \
    libraries/qtopia \
    libraries/qtopiaaudio \
    libraries/qtopiacomm \
    libraries/qtopiapim \
    libraries/qtopiaprinting \
    settings/appearance \
    settings/handwriting \
    settings/homescreen \
    settings/language \
    settings/light-and-power \
    settings/logging \
    settings/network \
    settings/packagemanager \
    settings/security \
    settings/systemtime \
    settings/worldtime \
    tools/content_installer \
    tools/dbmigrate \
    tools/dbmigrateservice \
    tools/device_updater \
    tools/device_updater/plugin \
    tools/printserver \
    tools/qcop \
    tools/qdawggen \
    tools/qdsync/app \
    tools/qdsync/base \
    tools/qdsync/common \
    tools/qdsync/pim \
    tools/vsexplorer \
    plugins/network/lan \
    plugins/network/dialing \
    plugins/qtopiacore/iconengines/qtopiaiconengine \
    plugins/qtopiacore/iconengines/qtopiasvgiconengine \
    plugins/qtopiacore/iconengines/qtopiapiciconengine \
    plugins/qtopiacore/imageformats/picture \
    3rdparty/applications/micro_httpd \
    3rdparty/libraries/alsa \
    3rdparty/libraries/crypt \
    3rdparty/libraries/inputmatch \
    3rdparty/libraries/md5 \
    3rdparty/libraries/openobex \
    3rdparty/libraries/pthread \
    3rdparty/libraries/tar\
    3rdparty/libraries/zlib\
    3rdparty/libraries/sqlite\
    3rdparty/libraries/dlmalloc\
    3rdparty/libraries/vobject\
    3rdparty/libraries/g711
    

build_qtopia_sqlite:PROJECTS*=\
    3rdparty/applications/sqlite

# Dummy entries (finegrained dependencies, retained in case libqtopiacomm is ever split up)
PROJECTS*=\
    libraries/qtopiacomm/bluetooth\
    libraries/qtopiacomm/ir\
    libraries/qtopiacomm/network\
    libraries/qtopiacomm/obex\
    libraries/qtopiacomm/serial\
    libraries/qtopiacomm/vpn

# Projects used in all non-platform builds
!platform {
    PROJECTS*=\
        applications/addressbook \
        applications/calculator \
        applications/camera \
        applications/clock \
        applications/datebook \
        applications/mediarecorder \
        applications/photoedit \
        applications/sysinfo \
        applications/textedit \
        applications/todo \
        applications/gqsync \
        games/qasteroids \
        games/fifteen \
        games/snake \
        games/minesweep \
        settings/words \
        plugins/content/id3 \
        3rdparty/plugins/content/vorbis \
        plugins/content/exif \
        plugins/content/threegpp \
        # Qtmail stuff
        applications/qtmail \
        libraries/qtopiamail \
        plugins/composers/email \
        plugins/composers/generic \
        plugins/composers/mms \
        plugins/viewers/generic

    enable_modem:PROJECTS*=\    
        plugins/viewers/smil
}

# Projects used in phone builds
phone {
    PROJECTS*=\
        libraries/qtopiaphone \
        settings/ringprofile \
        settings/speeddial \
        tools/atinterface \
        3rdparty/libraries/gsm \
        plugins/codecs/wavrecord

    enable_modem {
        PROJECTS*=\
            libraries/qtopiaphonemodem \
            libraries/qtopiasmil
        for(p,PHONEVENDORS) {
            exists(plugins/phonevendors/$$p/$$tail($$p).pro):PROJECTS*=plugins/phonevendors/$$p
        }
        for(m,MULTIPLEXERS) {
            exists(plugins/multiplexers/$$m/$$tail($$m).pro):PROJECTS*=plugins/multiplexers/$$m
        }
    }

    enable_cell {
        PROJECTS*=\
            settings/callforwarding\
            settings/phonenetworks\
            settings/phonesettings
    }

    # This isn't supported but it's included anyway
    PROJECTS*=tools/phonesim tools/phonesim/lib tools/phonesim_target
    CONFIG+=qtopiatest_use_phonesim


}

!x11 {
    # input methods that are available in all editions, except X11
    PROJECTS*=\
        plugins/inputmethods/predictivekeyboard \
        plugins/inputmethods/keyboard \
        plugins/inputmethods/dockedkeyboard \
        3rdparty/plugins/inputmethods/pkim
    
    contains(LANGUAGES,zh_CN):PROJECTS*=plugins/inputmethods/pinyin
}

# qtopiatest
!x11:qtopiatest {
    # Qtopiatest core and plugin interfaces
    PROJECTS*=\
        libraries/qtopiacore/qtestlib\
        libraries/qtopiatest

    # Qtopiatest reference implementation (plugins)
    PROJECTS*=\
        plugins/qtopiatest/application \
        plugins/qtopiatest/server \
        plugins/qtopiatest/widgets

    # Qtopiatest script interpreter
    PROJECTS*=\
        tools/qtopiatestrunner/lib \
        tools/qtopiatestrunner/liboverrides \
        tools/qtopiatestrunner

    # performance test helpers
    PROJECTS*=\
        plugins/qtopiacore/gfxdrivers/perftestqvfb \
        plugins/qtopiacore/gfxdrivers/perftestlinuxfb
}

# Projects used in media-enabled builds
enable_qtopiamedia {
    PROJECTS*=\
        libraries/qtopiamedia \
        tools/mediaserver \
        applications/mediaplayer

        isEmpty(DEVICE_CONFIG_PATH) {
            PROJECTS*=\
                plugins/audiohardware/desktop
        }

    contains(QTOPIAMEDIA_ENGINES,helix) {
        PROJECTS*=\
            3rdparty/libraries/helix \
            plugins/mediaengines/helix
    }

    contains(QTOPIAMEDIA_ENGINES,gstreamer) {
        PROJECTS*=\
            3rdparty/libraries/gstreamer \
            plugins/mediaengines/gstreamer
    }

    contains(QTOPIAMEDIA_ENGINES,cruxus) {
        PROJECTS*=plugins/mediaengines/cruxus
        # MIDI support
        PROJECTS*=\
            3rdparty/libraries/libtimidity\
            3rdparty/plugins/codecs/libtimidity
        # MP3 support
        # Removed for now, due to licensing
        PROJECTS*=\
            3rdparty/plugins/codecs/libmad
        # OGG Vorbis support
        PROJECTS*=\
            3rdparty/libraries/tremor\
            3rdparty/plugins/codecs/tremor
        # WAV support
        PROJECTS*=\
            plugins/codecs/wavplay
    }
}

# Basic sound capability for non-media-enabled builds
!enable_qtopiamedia:!x11 {
    PROJECTS*=\
        tools/qss
}

!no_quicklaunch|enable_singleexec:PROJECTS*=tools/quicklauncher

enable_bluetooth:PROJECTS*=\
    settings/btsettings \
    applications/bluetooth \
    plugins/network/bluetooth \
    plugins/qtopiaprinting/bluetooth

enable_infrared:PROJECTS*=\
    settings/beaming

enable_dbus {
    PROJECTS*=\
    3rdparty/libraries/dbus \
    3rdparty/libraries/qtdbus
}

enable_dbusipc {
    PROJECTS*=\
    3rdparty/applications/dbus \
    tools/qtopia-dbus-launcher
}

THEMES *= smart

phone:THEMES*=\
    classic \
    crisp \
    finxi \
    qtopia

enable_samples:PROJECTS+=settings/serverwidgets

drmagent {
    PROJECTS*=\
        3rdparty/libraries/drmagent\
        plugins/drmagent/bscidrmagent \
        settings/drmbrowser
}

enable_sxe {
    PROJECTS*=\
        libraries/qtopiasecurity\
        tools/sxe_installer \
        tools/sxe_policy_runner \
        tools/sxemonitor \
        tools/rlimiter \
        tools/sysmessages
}

