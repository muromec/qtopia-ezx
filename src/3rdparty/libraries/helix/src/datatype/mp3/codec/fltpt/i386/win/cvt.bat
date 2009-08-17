REM rename functions in .obj files
objcopy --redefine-sym _fdct32=_xmp3_fdct32 --redefine-sym _coef32=_xmp3_coef32 --redefine-sym _fdct32_dual=_xmp3_fdct32_dual --redefine-sym _fdct32_dual_mono=_xmp3_fdct32_dual_mono --redefine-sym _dct_coef_addr=_xmp3_dct_coef_addr dct.obj dct_new.obj 
objcopy --redefine-sym _window=_xmp3_window --redefine-sym _window_dual=_xmp3_window_dual winm.obj winm_new.obj
