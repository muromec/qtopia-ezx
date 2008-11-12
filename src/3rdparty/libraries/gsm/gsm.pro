qtopia_project(external lib)
license(FREEWARE)
TARGET		=   gsm
VERSION		=   1.0.0
CONFIG-=warn_on

HEADERS		=   gsm.h
SOURCES		=   add.c \
		    code.c \
		    decode.c \
		    gsm_create.c \
		    gsm_decode.c \
		    gsm_destroy.c \
		    gsm_encode.c \
		    gsm_option.c \
		    long_term.c \
		    lpc.c \
		    preprocess.c \
		    rpe.c \
		    short_term.c \
		    table.c
DEFINES    +=  FAST SASR WAV49

pkg.desc=GSM library
pkg.domain=trusted

# FIXME "make syncqtopia"
dep(INCLUDEPATH+=$$PWD)
idep(LIBS+=-l$$TARGET)
