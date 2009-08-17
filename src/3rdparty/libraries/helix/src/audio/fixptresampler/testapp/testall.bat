@echo off

rem
rem Comprehensive test of resampler modes.
rem

rem All standard conversions, mono

call testone -q0  -i8000 -o48000 
call testone -q0 -i11025 -o48000 
call testone -q0 -i16000 -o48000 
call testone -q0 -i22050 -o48000 
call testone -q0 -i32000 -o48000 
call testone -q0 -i44100 -o48000 

call testone -q0  -i8000 -o44100 
call testone -q0 -i11025 -o44100 
call testone -q0 -i16000 -o44100 
call testone -q0 -i22050 -o44100 
call testone -q0 -i32000 -o44100 

call testone -q0  -i8000 -o32000 
call testone -q0 -i11025 -o32000 
call testone -q0 -i16000 -o32000 
call testone -q0 -i22050 -o32000 
call testone -q0 -i44100 -o32000 

call testone -q0  -i8000 -o22050 
call testone -q0 -i11025 -o22050 
call testone -q0 -i16000 -o22050 
call testone -q0 -i32000 -o22050 
call testone -q0 -i44100 -o22050 

call testone -q0  -i8000 -o16000 
call testone -q0 -i11025 -o16000 
call testone -q0 -i22050 -o16000 
call testone -q0 -i32000 -o16000 
call testone -q0 -i44100 -o16000 

call testone -q0  -i8000 -o11025 
call testone -q0 -i16000 -o11025 
call testone -q0 -i22050 -o11025 
call testone -q0 -i32000 -o11025 
call testone -q0 -i44100 -o11025 

call testone -q0 -i11025 -o8000 
call testone -q0 -i16000 -o8000 
call testone -q0 -i22050 -o8000 
call testone -q0 -i32000 -o8000 
call testone -q0 -i44100 -o8000 

rem All standard conversions, stereo

call testone -q0  -i8000 -o48000 -s
call testone -q0 -i11025 -o48000 -s
call testone -q0 -i16000 -o48000 -s
call testone -q0 -i22050 -o48000 -s
call testone -q0 -i32000 -o48000 -s
call testone -q0 -i44100 -o48000 -s

call testone -q0  -i8000 -o44100 -s
call testone -q0 -i11025 -o44100 -s
call testone -q0 -i16000 -o44100 -s
call testone -q0 -i22050 -o44100 -s
call testone -q0 -i32000 -o44100 -s

call testone -q0  -i8000 -o32000 -s
call testone -q0 -i11025 -o32000 -s
call testone -q0 -i16000 -o32000 -s
call testone -q0 -i22050 -o32000 -s
call testone -q0 -i44100 -o32000 -s

call testone -q0  -i8000 -o22050 -s
call testone -q0 -i11025 -o22050 -s
call testone -q0 -i16000 -o22050 -s
call testone -q0 -i32000 -o22050 -s
call testone -q0 -i44100 -o22050 -s

call testone -q0  -i8000 -o16000 -s
call testone -q0 -i11025 -o16000 -s
call testone -q0 -i22050 -o16000 -s
call testone -q0 -i32000 -o16000 -s
call testone -q0 -i44100 -o16000 -s

call testone -q0  -i8000 -o11025 -s
call testone -q0 -i16000 -o11025 -s
call testone -q0 -i22050 -o11025 -s
call testone -q0 -i32000 -o11025 -s
call testone -q0 -i44100 -o11025 -s

call testone -q0 -i11025 -o8000 -s
call testone -q0 -i16000 -o8000 -s
call testone -q0 -i22050 -o8000 -s
call testone -q0 -i32000 -o8000 -s
call testone -q0 -i44100 -o8000 -s

rem ARB mode, nwing = even/odd

call testone -q0 -i44100 -o48001
call testone -q0 -i44100 -o32001
call testone -q0 -i44100 -o48001 -s
call testone -q0 -i44100 -o32001 -s

rem All filter kernels

call testone -q1 -i44100 -o48000
call testone -q1 -i44100 -o32000
call testone -q1 -i44100 -o48001
call testone -q1 -i44100 -o32001

call testone -q2 -i44100 -o48000
call testone -q2 -i44100 -o32000
call testone -q2 -i44100 -o48001
call testone -q2 -i44100 -o32001

call testone -q3 -i44100 -o48000
call testone -q3 -i44100 -o32000
call testone -q3 -i44100 -o48001
call testone -q3 -i44100 -o32001

rem Constant-input chunks

call testone -q0 -i44100 -o48001 -n1
call testone -q0 -i44100 -o48001 -n7
call testone -q0 -i44100 -o48001 -n9
call testone -q0 -i44100 -o48001 -n2  -s
call testone -q0 -i44100 -o48001 -n16 -s
call testone -q0 -i44100 -o48001 -n18 -s

rem Constant-output chunks

call testone -q0 -i44100 -o48001 -c -n1
call testone -q0 -i44100 -o48001 -c -n7
call testone -q0 -i44100 -o48001 -c -n9
call testone -q0 -i44100 -o48001 -c -n2  -s
call testone -q0 -i44100 -o48001 -c -n16 -s
call testone -q0 -i44100 -o48001 -c -n18 -s

rem Split stereo frames
rem Not supported on ARMv5E (requires word-aligned frames)

rem call testone -q0 -i44100 -o48001    -n1  -s
rem call testone -q0 -i44100 -o48001    -n19 -s
rem call testone -q0 -i44100 -o48001 -c -n1  -s
rem call testone -q0 -i44100 -o48001 -c -n19 -s
