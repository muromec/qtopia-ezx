@echo off

rem
rem This example compares the ARM output against the x86 reference.
rem In this case, the ARM testapp is executed using the ARMulator.
rem
rem The output should match, exactly.
rem

SET TSTAPP=armsd -cpu ARM7TDMI -exec testapp.axf
SET REFAPP=testapp.exe
SET ARGS=%1 %2 %3 %4 %5 %6 %7 %8 %9

echo TESTING %ARGS%
%TSTAPP% %ARGS% in.pcm tst.pcm > NUL
%REFAPP% %ARGS% in.pcm ref.pcm > NUL

fc /b tst.pcm ref.pcm
del tst.pcm
del ref.pcm
