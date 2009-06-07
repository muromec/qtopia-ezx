# Microsoft Developer Studio Project File - Name="Vivo Video Decompressor Filter" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Vivo Video Decompressor Filter - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "HvVidFlt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "HvVidFlt.mak" CFG="Vivo Video Decompressor Filter - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Vivo Video Decompressor Filter - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Vivo Video Decompressor Filter - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Vivo Video Decompressor Filter - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MD /W3 /GX /O2 /I "." /I "./include" /I "../include" /I "./common" /I "./env" /I "./video" /I "../../../common/include/" /I "./Filter" /I "./pub" /D "NDEBUG" /D "_MT" /D "_DLL" /D _X86_=1 /D "WIN32" /D "_WINDOWS" /D "COMPILE_MMX" /D "VCM_CODEC" /D "STRICT" /D "DECODE_ONLY" /D "_CODER_IN_EXE" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib version.lib advapi32.lib /nologo /base:"0x1c400000" /subsystem:windows /dll /machine:I386 /out:"Release/drv23260.dll" /libpath:"./DrctShow/lib" /libpath:"/..DrctShow/lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Vivo Video Decompressor Filter - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /I "." /I "../include" /I "./include" /I "./env" /I "./common" /I "../../../common/include/" /I "./video" /I "./Filter" /I "./pub" /D "_DEBUG" /D "_MT" /D "_DLL" /D _X86_=1 /D "WIN32" /D "_WINDOWS" /D "COMPILE_MMX" /D "VCM_CODEC" /D "STRICT" /D "DECODE_ONLY" /D "_CODER_IN_EXE" /FR /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 quartz.lib strmbase.lib msvcrtd.lib winmm.lib vfw32.lib kernel32.lib user32.lib gdi32.lib version.lib comctl32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /out:"Debug/d2633260.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Vivo Video Decompressor Filter - Win32 Release"
# Name "Vivo Video Decompressor Filter - Win32 Debug"
# Begin Group "Video"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\video\clip1.c
# End Source File
# Begin Source File

SOURCE=.\video\clip2.c
# End Source File
# Begin Source File

SOURCE=.\video\dec_blk.c
# End Source File
# Begin Source File

SOURCE=.\video\getimgpa.c
# End Source File
# Begin Source File

SOURCE=.\video\h261dec.c
# End Source File
# Begin Source File

SOURCE=.\video\h261err.c
# End Source File
# Begin Source File

SOURCE=.\video\h263dec.c
# End Source File
# Begin Source File

SOURCE=.\video\h263pdec.c
# End Source File
# Begin Source File

SOURCE=.\video\hvdbg.c
# End Source File
# Begin Source File

SOURCE=.\video\hvdecode.c
# End Source File
# Begin Source File

SOURCE=.\video\idct.c
# End Source File
# Begin Source File

SOURCE=.\video\loopfilt.c
# End Source File
# Begin Source File

SOURCE=.\video\mcomp.c
# End Source File
# Begin Source File

SOURCE=.\common\mmxcpuid.c
# End Source File
# Begin Source File

SOURCE=.\video\Mmxdeblo.c
# End Source File
# Begin Source File

SOURCE=.\video\mmxmotio.c
# End Source File
# Begin Source File

SOURCE=.\video\Recongob.c
# End Source File
# Begin Source File

SOURCE=.\video\vdecopcl.c
# End Source File
# Begin Source File

SOURCE=.\video\vld.c
# End Source File
# Begin Source File

SOURCE=.\video\vldecode.c
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common\hxcinst.cpp
# End Source File
# Begin Source File

SOURCE=.\common\hxcinst.h
# End Source File
# Begin Source File

SOURCE=.\common\hxcodec.cpp
# End Source File
# Begin Source File

SOURCE=.\common\hxcodecf.h
# End Source File
# Begin Source File

SOURCE=.\common\hxcodeci.h
# End Source File
# Begin Source File

SOURCE=.\common\iowriter.cpp
# End Source File
# Begin Source File

SOURCE=.\common\iowriter.h
# End Source File
# End Group
# Begin Group "Filter"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\d263.ver
# End Source File
# Begin Source File

SOURCE=.\filter\hvviddec.cpp
# End Source File
# Begin Source File

SOURCE=.\filter\hvviddec.h
# End Source File
# Begin Source File

SOURCE=.\env\hxh263d.cpp
# End Source File
# Begin Source File

SOURCE=.\env\hxvamain.cpp
# End Source File
# Begin Source File

SOURCE=.\include\hxvamain.h
# End Source File
# Begin Source File

SOURCE=.\pub\hxvdec.h
# End Source File
# End Group
# End Target
# End Project
