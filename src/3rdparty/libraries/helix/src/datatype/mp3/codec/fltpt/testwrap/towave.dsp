# Microsoft Developer Studio Project File - Name="towave" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=towave - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "towave.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "towave.mak" CFG="towave - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "towave - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "towave - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "towave - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "rel_obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\..\..\..\common\include" /I "..\..\..\..\..\common\runtime\pub" /I "..\..\..\common\pub" /I "..\pub" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "MP3_CONCEAL_LOSS" /D "_WINDOWS" /D "HELIX_FEATURE_AUDIO_MPA_LAYER1" /D "HELIX_FEATURE_AUDIO_MPA_LAYER2" /D "HELIX_FEATURE_AUDIO_MPA_LAYER3" /D "DISABLE_WAVE_HEADER" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "towave - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\..\..\common\include" /I "..\..\..\..\..\common\runtime\pub" /I "..\..\..\common\pub" /I "..\pub" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "MP3_CONCEAL_LOSS" /D "_WINDOWS" /D "HELIX_FEATURE_AUDIO_MPA_LAYER1" /D "HELIX_FEATURE_AUDIO_MPA_LAYER2" /D "HELIX_FEATURE_AUDIO_MPA_LAYER3" /D "DISABLE_WAVE_HEADER" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "towave - Win32 Release"
# Name "towave - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "codec_base"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\common\mhead.cpp
# End Source File
# Begin Source File

SOURCE=..\mpadec.cpp
# End Source File
# Begin Source File

SOURCE=..\mpadecobj.cpp
# End Source File
# Begin Source File

SOURCE=..\sbt.c
# End Source File
# Begin Source File

SOURCE=..\sconceal.cpp
# End Source File
# Begin Source File

SOURCE=..\trigtabs.c
# End Source File
# End Group
# Begin Group "codec_layer_1_2"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\mpadecl1.cpp
# End Source File
# Begin Source File

SOURCE=..\mpadecl2.cpp
# End Source File
# End Group
# Begin Group "codec_layer_3"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\bitget.c
# End Source File
# Begin Source File

SOURCE=..\hwin.c
# End Source File
# Begin Source File

SOURCE=..\l3dq.c
# End Source File
# Begin Source File

SOURCE=..\mdct.c
# End Source File
# Begin Source File

SOURCE=..\mpadecl3.cpp
# End Source File
# Begin Source File

SOURCE=..\msis.c
# End Source File
# Begin Source File

SOURCE=..\sbtl3.c
# End Source File
# Begin Source File

SOURCE=..\uph.c
# End Source File
# Begin Source File

SOURCE=..\upsf.c
# End Source File
# End Group
# Begin Group "optimizations"

# PROP Default_Filter ""
# Begin Group "assembly"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\i386\win\winm.obj
# End Source File
# Begin Source File

SOURCE=..\i386\win\dct.obj
# End Source File
# End Group
# Begin Group "conly"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\dct.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\winm.c
# PROP Exclude_From_Build 1
# End Source File
# End Group
# End Group
# Begin Group "testwrap"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\towavemt.cpp
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
