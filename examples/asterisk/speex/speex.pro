qtopia_project(external lib)
license(FREEWARE)
TARGET		=   speex
VERSION		=   1.0.0
CONFIG         +=   warn_off
requires(enable_voip)

INCLUDEPATH+=$$QTOPIA_DEPOT_PATH/examples/asterisk/speex/include

DESTDIR = $$QPEDIR/lib

HEADERS		= arch.h \
                  cb_search_arm4.h \
                  cb_search.h \
                  cb_search_sse.h \
                  filters_arm4.h \
                  filters.h \
                  filters_sse.h \
                  fixed_arm4.h \
                  fixed_arm5e.h \
                  fixed_debug.h \
                  fixed_generic.h \
                  include/speex/speex_bits.h \
                  include/speex/speex_callbacks.h \
                  include/speex/speex_config_types.h \
                  include/speex/speex_echo.h \
                  include/speex/speex.h \
                  include/speex/speex_header.h \
                  include/speex/speex_jitter.h \
                  include/speex/speex_noglobals.h \
                  include/speex/speex_preprocess.h \
                  include/speex/speex_stereo.h \
                  include/speex/speex_types.h \
                  lpc.h \
                  lsp.h \
                  ltp_arm4.h \
                  ltp.h \
                  ltp_sse.h \
                  math_approx.h \
                  medfilter.h \
                  misc.h \
                  modes.h \
                  nb_celp.h \
                  quant_lsp.h \
                  sb_celp.h \
                  smallft.h \
                  stack_alloc.h \
                  vbr.h \
                  vq_arm4.h \
                  vq.h
SOURCES		= bits.c \
                  cb_search.c \
                  exc_10_16_table.c \
                  exc_10_32_table.c \
                  exc_20_32_table.c \
                  exc_5_256_table.c \
                  exc_5_64_table.c \
                  exc_8_128_table.c \
                  filters.c \
                  gain_table.c \
                  gain_table_lbr.c \
                  hexc_10_32_table.c \
                  hexc_table.c \
                  high_lsp_tables.c \
                  jitter.c \
                  lbr_48k_tables.c \
                  lpc.c \
                  lsp.c \
                  lsp_tables_nb.c \
                  ltp.c \
                  math_approx.c \
                  mdf.c \
                  medfilter.c \
                  misc.c \
                  modes.c \
                  nb_celp.c \
                  preprocess.c \
                  quant_lsp.c \
                  sb_celp.c \
                  smallft.c \
                  speex.c \
                  speex_callbacks.c \
                  speex_header.c \
                  stereo.c \
                  vbr.c \
                  vq.c
DEFINES         += FIXED_POINT

pkg.desc=Speex codec library
pkg.domain=trusted

# FIXME "make syncqtopia"
dep(INCLUDEPATH+=$$PWD)
idep(LIBS+=-l$$TARGET)
