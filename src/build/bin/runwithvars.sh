#!/bin/sh

getvar()
{
    gv_varname="$1"
    shift
    while [ $# -gt 0 ]; do
        if [ "${1##$gv_varname=}" != "$1" ]; then
            echo "${1##$gv_varname=}"
        fi
        shift
    done
}

# NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE
#
# The environment file handling logic is duplicated in configure
#
# NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE

DEVICE_CONFIG_PATH=$(getvar DEVICE_CONFIG_PATH "$@")
DEFAULT_DEVICE_PATH=$(getvar DEFAULT_DEVICE_PATH "$@")
DEVICE_BIN=$(getvar DEVICE_BIN "$@")

# Only pull in these files if we're actually building for a device
if [ -n "$DEVICE_CONFIG_PATH" ]; then
    . "$DEFAULT_DEVICE_PATH/environment"
    [ -f "$DEVICE_CONFIG_PATH/environment" ] && . "$DEVICE_CONFIG_PATH/environment"
    setup_path
fi

exec env "$@"

