qtopia_project(qtopia app)
TARGET=benchmark

depends(libraries/qtopiagfx)

# Input
SOURCES += main.cpp

# Set this to trusted for full privileges
target.hint=sxe
target.domain=untrusted

