qtopia_project(qtopia plugin)
TARGET=bluetoothprinting

HEADERS = bluetoothplugin.h qbluetoothobexagent.h
SOURCES = bluetoothplugin.cpp qbluetoothobexagent.cpp

depends(libraries/qtopiaprinting)
depends(libraries/qtopiacomm/bluetooth)
