# Microsoft Developer Studio Project File - Name="GREG2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=GREG2 - Win32 Uni Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GREG2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GREG2.mak" CFG="GREG2 - Win32 Uni Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GREG2 - Win32 Uni Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "GREG2 - Win32 Uni Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GREG2 - Win32 Uni Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Win32_Un"
# PROP BASE Intermediate_Dir ".\Win32_Un"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UREL"
# PROP Intermediate_Dir "\Symbian\6.1\Series60\EPOC32\BUILD\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2\WINS\UREL"
# ADD CPP /nologo /Zp4 /MD /W4 /O1 /Op /X /I "\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP" /I "\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\INC" /I "\HELIXSYM\AUDIO\DEVICE\TEST" /I "\Symbian\6.1\Series60\EPOC32\INCLUDE" /I "\Symbian\6.1\Series60\EPOC32\INCLUDE\LIBC" /D "__SYMBIAN32__" /D "__VC32__" /D "__WINS__" /D "__AVKON_ELAF__" /D "__DLL__" /D "NDEBUG" /D "_UNICODE" /GF /c
# ADD MTL /nologo /mktyplib203 /D /win32
# ADD BASE RSC /l 0x809
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /machine:IX86
# ADD LINK32 "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UREL\EDLL.LIB" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\euser.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\apparc.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\cone.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\eikcore.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\eikcoctl.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\avkon.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\estlib.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\mediaclientaudiostream.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\testauddevice.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\auddevlib.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\contlib.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\debuglib.lib" "\Symbian\6.1\Series60\EPOC32\BUILD\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2\WINS\UREL\GREG2.exp" /nologo /entry:"_E32Dll" /subsystem:windows /dll /machine:IX86 /nodefaultlib /include:"?_E32Dll@@YGHPAXI0@Z" /out:"\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UREL\Z\SYSTEM\APPS\GREG2\GREG2.APP" /WARN:3
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=echo Doing first-stage link by name 	nmake -nologo -f "\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2.SUP.MAKE" PRELINKUREL 	if errorlevel 1 nmake -nologo -f "\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2.SUP.MAKE" STOPLINKUREL
PostBuild_Cmds=nmake -nologo -f "\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2.SUP.MAKE" POSTBUILDUREL
# End Special Build Tool

!ELSEIF  "$(CFG)" == "GREG2 - Win32 Uni Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Win32_U0"
# PROP BASE Intermediate_Dir ".\Win32_U0"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB"
# PROP Intermediate_Dir "\Symbian\6.1\Series60\EPOC32\BUILD\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2\WINS\UDEB"
# ADD CPP /nologo /Zp4 /MDd /W4 /Zi /Od /X /I "\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP" /I "\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\INC" /I "\HELIXSYM\AUDIO\DEVICE\TEST" /I "\Symbian\6.1\Series60\EPOC32\INCLUDE" /I "\Symbian\6.1\Series60\EPOC32\INCLUDE\LIBC" /D "__SYMBIAN32__" /D "__VC32__" /D "__WINS__" /D "__AVKON_ELAF__" /D "__DLL__" /D "_DEBUG" /D "_UNICODE" /FR /Fd"\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\Z\SYSTEM\APPS\GREG2\GREG2.PDB" /GF /c
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /machine:IX86
# ADD LINK32 "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\EDLL.LIB" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\euser.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\apparc.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\cone.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\eikcore.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\eikcoctl.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\avkon.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\estlib.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\mediaclientaudiostream.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\testauddevice.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\auddevlib.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\contlib.lib" "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\debuglib.lib" "\Symbian\6.1\Series60\EPOC32\BUILD\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2\WINS\UDEB\GREG2.exp" /nologo /entry:"_E32Dll" /subsystem:windows /dll /pdb:"\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\Z\SYSTEM\APPS\GREG2\GREG2.pdb" /debug /machine:IX86 /nodefaultlib /include:"?_E32Dll@@YGHPAXI0@Z" /out:"\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\Z\SYSTEM\APPS\GREG2\GREG2.APP" /WARN:3
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=echo Doing first-stage link by name 	nmake -nologo -f "\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2.SUP.MAKE" PRELINKUDEB 	if errorlevel 1 nmake -nologo -f "\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2.SUP.MAKE" STOPLINKUDEB
PostBuild_Cmds=nmake -nologo -f "\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2.SUP.MAKE" POSTBUILDUDEB
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "GREG2 - Win32 Uni Release"
# Name "GREG2 - Win32 Uni Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\Greg2.mmp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\SRC\..\data\greg2.rss

!IF  "$(CFG)" == "GREG2 - Win32 Uni Release"

# PROP Intermediate_Dir "\Symbian\6.1\Series60\EPOC32\BUILD\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2\WINS"
USERDEP__GREG2="\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\INC\GREG2.HRH"	"\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\INC\GREG2.LOC"	
# Begin Custom Build - Building resources from ..\data\greg2.rss
InputPath=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\SRC\..\data\greg2.rss

"\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UREL\Z\SYSTEM\APPS\GREG2\GREG2.rSC.dummy" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nmake -nologo -f "\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2.SUP.MAKE"   "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UREL\Z\SYSTEM\APPS\GREG2\GREG2.r"

# End Custom Build

!ELSEIF  "$(CFG)" == "GREG2 - Win32 Uni Debug"

# PROP Intermediate_Dir "\Symbian\6.1\Series60\EPOC32\BUILD\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2\WINS"
USERDEP__GREG2="\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\INC\GREG2.HRH"	"\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\INC\GREG2.LOC"	
# Begin Custom Build - Building resources from ..\data\greg2.rss
InputPath=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\SRC\..\data\greg2.rss

"\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\Z\SYSTEM\APPS\GREG2\GREG2.rSC.dummy" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nmake -nologo -f "\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2.SUP.MAKE"   "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\Z\SYSTEM\APPS\GREG2\GREG2.r"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\Greg2.uid.cpp
DEP_CPP_GREG2_=\
	"..\..\..\EPOC32\INCLUDE\e32def.h"\
	"..\..\..\EPOC32\INCLUDE\e32des16.h"\
	"..\..\..\EPOC32\INCLUDE\e32des8.h"\
	"..\..\..\EPOC32\INCLUDE\e32std.h"\
	"..\..\..\EPOC32\INCLUDE\e32std.inl"\
	
# End Source File
# Begin Source File

SOURCE=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\SRC\..\data\greg2_caption.rss

!IF  "$(CFG)" == "GREG2 - Win32 Uni Release"

# PROP Intermediate_Dir "\Symbian\6.1\Series60\EPOC32\BUILD\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2\WINS"
USERDEP__GREG2_C="\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\INC\GREG2.LOC"	
# Begin Custom Build - Building resources from ..\data\greg2_caption.rss
InputPath=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\SRC\..\data\greg2_caption.rss

"\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UREL\Z\SYSTEM\APPS\GREG2\GREG2_CAPTION.rSC.dummy" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nmake -nologo -f "\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2.SUP.MAKE"   "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UREL\Z\SYSTEM\APPS\GREG2\GREG2_CAPTION.r"

# End Custom Build

!ELSEIF  "$(CFG)" == "GREG2 - Win32 Uni Debug"

# PROP Intermediate_Dir "\Symbian\6.1\Series60\EPOC32\BUILD\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2\WINS"
USERDEP__GREG2_C="\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\INC\GREG2.LOC"	
# Begin Custom Build - Building resources from ..\data\greg2_caption.rss
InputPath=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\SRC\..\data\greg2_caption.rss

"\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\Z\SYSTEM\APPS\GREG2\GREG2_CAPTION.rSC.dummy" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nmake -nologo -f "\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\GROUP\GREG2.SUP.MAKE"   "\Symbian\6.1\Series60\EPOC32\RELEASE\WINS\UDEB\Z\SYSTEM\APPS\GREG2\GREG2_CAPTION.r"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\SRC\Greg2app.cpp
DEP_CPP_GREG2A=\
	"..\..\..\EPOC32\INCLUDE\aknapp.h"\
	"..\..\..\EPOC32\INCLUDE\akndoc.h"\
	"..\..\..\EPOC32\INCLUDE\apadef.h"\
	"..\..\..\EPOC32\INCLUDE\apaflrec.h"\
	"..\..\..\EPOC32\INCLUDE\apaid.h"\
	"..\..\..\EPOC32\INCLUDE\apparc.h"\
	"..\..\..\EPOC32\INCLUDE\e32base.h"\
	"..\..\..\EPOC32\INCLUDE\e32base.inl"\
	"..\..\..\EPOC32\INCLUDE\e32def.h"\
	"..\..\..\EPOC32\INCLUDE\e32des16.h"\
	"..\..\..\EPOC32\INCLUDE\e32des8.h"\
	"..\..\..\EPOC32\INCLUDE\e32hal.h"\
	"..\..\..\EPOC32\INCLUDE\e32keys.h"\
	"..\..\..\EPOC32\INCLUDE\e32pccd.h"\
	"..\..\..\EPOC32\INCLUDE\e32std.h"\
	"..\..\..\EPOC32\INCLUDE\e32std.inl"\
	"..\..\..\EPOC32\INCLUDE\e32svr.h"\
	"..\..\..\EPOC32\INCLUDE\eikapp.h"\
	"..\..\..\EPOC32\INCLUDE\eikdoc.h"\
	"..\..\..\EPOC32\INCLUDE\f32file.h"\
	"..\..\..\EPOC32\INCLUDE\f32file.inl"\
	"..\..\..\EPOC32\INCLUDE\gdi.h"\
	"..\..\..\EPOC32\INCLUDE\gdi.inl"\
	"..\..\..\EPOC32\INCLUDE\s32std.h"\
	"..\..\..\EPOC32\INCLUDE\s32std.inl"\
	"..\..\..\EPOC32\INCLUDE\s32strm.h"\
	"..\..\..\EPOC32\INCLUDE\s32strm.inl"\
	"..\..\..\EPOC32\INCLUDE\savenotf.h"\
	"..\INC\Greg2app.h"\
	"..\INC\Greg2document.h"\
	
# End Source File
# Begin Source File

SOURCE=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\SRC\Greg2appui.cpp
DEP_CPP_GREG2AP=\
	"..\..\..\..\..\..\HELIXSYM\AUDIO\DEVICE\TEST\Testauddevice.h"\
	"..\..\..\EPOC32\INCLUDE\aknappui.h"\
	"..\..\..\EPOC32\INCLUDE\aknscbut.h"\
	"..\..\..\EPOC32\INCLUDE\aknscrlb.h"\
	"..\..\..\EPOC32\INCLUDE\AknWsEventObserver.h"\
	"..\..\..\EPOC32\INCLUDE\apacmdln.h"\
	"..\..\..\EPOC32\INCLUDE\apadef.h"\
	"..\..\..\EPOC32\INCLUDE\apaflrec.h"\
	"..\..\..\EPOC32\INCLUDE\apaid.h"\
	"..\..\..\EPOC32\INCLUDE\apgtask.h"\
	"..\..\..\EPOC32\INCLUDE\apparc.h"\
	"..\..\..\EPOC32\INCLUDE\avkon.hrh"\
	"..\..\..\EPOC32\INCLUDE\badesca.h"\
	"..\..\..\EPOC32\INCLUDE\baerrhan.h"\
	"..\..\..\EPOC32\INCLUDE\bamdesca.h"\
	"..\..\..\EPOC32\INCLUDE\barsc.h"\
	"..\..\..\EPOC32\INCLUDE\basched.h"\
	"..\..\..\EPOC32\INCLUDE\bitbase.h"\
	"..\..\..\EPOC32\INCLUDE\bitdev.h"\
	"..\..\..\EPOC32\INCLUDE\bitdev.inl"\
	"..\..\..\EPOC32\INCLUDE\bitmap.h"\
	"..\..\..\EPOC32\INCLUDE\bitstd.h"\
	"..\..\..\EPOC32\INCLUDE\coeaui.h"\
	"..\..\..\EPOC32\INCLUDE\coeauib.h"\
	"..\..\..\EPOC32\INCLUDE\coeccntx.h"\
	"..\..\..\EPOC32\INCLUDE\coecntrl.h"\
	"..\..\..\EPOC32\INCLUDE\coecobs.h"\
	"..\..\..\EPOC32\INCLUDE\coedef.h"\
	"..\..\..\EPOC32\INCLUDE\coehelp.h"\
	"..\..\..\EPOC32\INCLUDE\coeinput.h"\
	"..\..\..\EPOC32\INCLUDE\coemain.h"\
	"..\..\..\EPOC32\INCLUDE\coemop.h"\
	"..\..\..\EPOC32\INCLUDE\coeview.h"\
	"..\..\..\EPOC32\INCLUDE\e32base.h"\
	"..\..\..\EPOC32\INCLUDE\e32base.inl"\
	"..\..\..\EPOC32\INCLUDE\e32def.h"\
	"..\..\..\EPOC32\INCLUDE\e32des16.h"\
	"..\..\..\EPOC32\INCLUDE\e32des8.h"\
	"..\..\..\EPOC32\INCLUDE\e32hal.h"\
	"..\..\..\EPOC32\INCLUDE\e32keys.h"\
	"..\..\..\EPOC32\INCLUDE\e32pccd.h"\
	"..\..\..\EPOC32\INCLUDE\e32std.h"\
	"..\..\..\EPOC32\INCLUDE\e32std.inl"\
	"..\..\..\EPOC32\INCLUDE\e32svr.h"\
	"..\..\..\EPOC32\INCLUDE\eikamnt.h"\
	"..\..\..\EPOC32\INCLUDE\eikapp.h"\
	"..\..\..\EPOC32\INCLUDE\eikappui.h"\
	"..\..\..\EPOC32\INCLUDE\eikaufty.h"\
	"..\..\..\EPOC32\INCLUDE\eikbctrl.h"\
	"..\..\..\EPOC32\INCLUDE\eikbtgrp.h"\
	"..\..\..\EPOC32\INCLUDE\eikcba.h"\
	"..\..\..\EPOC32\INCLUDE\eikcmobs.h"\
	"..\..\..\EPOC32\INCLUDE\eikcolor.hrh"\
	"..\..\..\EPOC32\INCLUDE\eikctgrp.h"\
	"..\..\..\EPOC32\INCLUDE\eikdef.h"\
	"..\..\..\EPOC32\INCLUDE\eikdgfty.h"\
	"..\..\..\EPOC32\INCLUDE\eikdoc.h"\
	"..\..\..\EPOC32\INCLUDE\eikenv.h"\
	"..\..\..\EPOC32\INCLUDE\eikfctry.h"\
	"..\..\..\EPOC32\INCLUDE\eiklibry.h"\
	"..\..\..\EPOC32\INCLUDE\eikmobs.h"\
	"..\..\..\EPOC32\INCLUDE\eiksbfrm.h"\
	"..\..\..\EPOC32\INCLUDE\eiksbobs.h"\
	"..\..\..\EPOC32\INCLUDE\eikscrlb.h"\
	"..\..\..\EPOC32\INCLUDE\Eikspane.h"\
	"..\..\..\EPOC32\INCLUDE\eikspmod.h"\
	"..\..\..\EPOC32\INCLUDE\eiksrv.h"\
	"..\..\..\EPOC32\INCLUDE\eiksrv.pan"\
	"..\..\..\EPOC32\INCLUDE\eiksrvc.h"\
	"..\..\..\EPOC32\INCLUDE\eiksrvs.h"\
	"..\..\..\EPOC32\INCLUDE\eikunder.h"\
	"..\..\..\EPOC32\INCLUDE\eikvcurs.h"\
	"..\..\..\EPOC32\INCLUDE\f32file.h"\
	"..\..\..\EPOC32\INCLUDE\f32file.inl"\
	"..\..\..\EPOC32\INCLUDE\fbs.h"\
	"..\..\..\EPOC32\INCLUDE\fldbase.h"\
	"..\..\..\EPOC32\INCLUDE\fldbltin.h"\
	"..\..\..\EPOC32\INCLUDE\fldinfo.h"\
	"..\..\..\EPOC32\INCLUDE\fldset.h"\
	"..\..\..\EPOC32\INCLUDE\fntstore.h"\
	"..\..\..\EPOC32\INCLUDE\frmlaydt.h"\
	"..\..\..\EPOC32\INCLUDE\frmparam.h"\
	"..\..\..\EPOC32\INCLUDE\frmtlay.h"\
	"..\..\..\EPOC32\INCLUDE\frmvis.h"\
	"..\..\..\EPOC32\INCLUDE\gdi.h"\
	"..\..\..\EPOC32\INCLUDE\gdi.inl"\
	"..\..\..\EPOC32\INCLUDE\greg2.rsg"\
	"..\..\..\EPOC32\INCLUDE\gulalign.h"\
	"..\..\..\EPOC32\INCLUDE\gulbordr.h"\
	"..\..\..\EPOC32\INCLUDE\gulcolor.h"\
	"..\..\..\EPOC32\INCLUDE\guldef.h"\
	"..\..\..\EPOC32\INCLUDE\lafmain.h"\
	"..\..\..\EPOC32\INCLUDE\lafpublc.h"\
	"..\..\..\EPOC32\INCLUDE\openfont.h"\
	"..\..\..\EPOC32\INCLUDE\s32buf.h"\
	"..\..\..\EPOC32\INCLUDE\s32buf.inl"\
	"..\..\..\EPOC32\INCLUDE\s32page.h"\
	"..\..\..\EPOC32\INCLUDE\s32page.inl"\
	"..\..\..\EPOC32\INCLUDE\s32share.h"\
	"..\..\..\EPOC32\INCLUDE\s32share.inl"\
	"..\..\..\EPOC32\INCLUDE\s32std.h"\
	"..\..\..\EPOC32\INCLUDE\s32std.inl"\
	"..\..\..\EPOC32\INCLUDE\s32stor.h"\
	"..\..\..\EPOC32\INCLUDE\s32stor.inl"\
	"..\..\..\EPOC32\INCLUDE\s32strm.h"\
	"..\..\..\EPOC32\INCLUDE\s32strm.inl"\
	"..\..\..\EPOC32\INCLUDE\savenotf.h"\
	"..\..\..\EPOC32\INCLUDE\tagma.h"\
	"..\..\..\EPOC32\INCLUDE\txtetext.h"\
	"..\..\..\EPOC32\INCLUDE\txtetext.inl"\
	"..\..\..\EPOC32\INCLUDE\txtfmlyr.h"\
	"..\..\..\EPOC32\INCLUDE\txtfmlyr.inl"\
	"..\..\..\EPOC32\INCLUDE\txtfmstm.h"\
	"..\..\..\EPOC32\INCLUDE\txtfrmat.h"\
	"..\..\..\EPOC32\INCLUDE\txtfrmat.inl"\
	"..\..\..\EPOC32\INCLUDE\txtlaydc.h"\
	"..\..\..\EPOC32\INCLUDE\txtstyle.h"\
	"..\..\..\EPOC32\INCLUDE\txtstyle.inl"\
	"..\..\..\EPOC32\INCLUDE\uikon.hrh"\
	"..\..\..\EPOC32\INCLUDE\vwsdef.h"\
	"..\..\..\EPOC32\INCLUDE\w32std.h"\
	"..\INC\Greg2.hrh"\
	"..\INC\Greg2appui.h"\
	"..\INC\Greg2container.h"\
	
# End Source File
# Begin Source File

SOURCE=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\SRC\Greg2container.cpp
DEP_CPP_GREG2C=\
	"..\..\..\EPOC32\INCLUDE\badesca.h"\
	"..\..\..\EPOC32\INCLUDE\bamdesca.h"\
	"..\..\..\EPOC32\INCLUDE\bitbase.h"\
	"..\..\..\EPOC32\INCLUDE\bitdev.h"\
	"..\..\..\EPOC32\INCLUDE\bitdev.inl"\
	"..\..\..\EPOC32\INCLUDE\bitmap.h"\
	"..\..\..\EPOC32\INCLUDE\bitstd.h"\
	"..\..\..\EPOC32\INCLUDE\coeccntx.h"\
	"..\..\..\EPOC32\INCLUDE\coecntrl.h"\
	"..\..\..\EPOC32\INCLUDE\coecobs.h"\
	"..\..\..\EPOC32\INCLUDE\coedef.h"\
	"..\..\..\EPOC32\INCLUDE\coehelp.h"\
	"..\..\..\EPOC32\INCLUDE\coeinput.h"\
	"..\..\..\EPOC32\INCLUDE\coemop.h"\
	"..\..\..\EPOC32\INCLUDE\e32base.h"\
	"..\..\..\EPOC32\INCLUDE\e32base.inl"\
	"..\..\..\EPOC32\INCLUDE\e32def.h"\
	"..\..\..\EPOC32\INCLUDE\e32des16.h"\
	"..\..\..\EPOC32\INCLUDE\e32des8.h"\
	"..\..\..\EPOC32\INCLUDE\e32hal.h"\
	"..\..\..\EPOC32\INCLUDE\e32keys.h"\
	"..\..\..\EPOC32\INCLUDE\e32pccd.h"\
	"..\..\..\EPOC32\INCLUDE\e32std.h"\
	"..\..\..\EPOC32\INCLUDE\e32std.inl"\
	"..\..\..\EPOC32\INCLUDE\e32svr.h"\
	"..\..\..\EPOC32\INCLUDE\eikalign.h"\
	"..\..\..\EPOC32\INCLUDE\eiklabel.h"\
	"..\..\..\EPOC32\INCLUDE\f32file.h"\
	"..\..\..\EPOC32\INCLUDE\f32file.inl"\
	"..\..\..\EPOC32\INCLUDE\fbs.h"\
	"..\..\..\EPOC32\INCLUDE\fntstore.h"\
	"..\..\..\EPOC32\INCLUDE\gdi.h"\
	"..\..\..\EPOC32\INCLUDE\gdi.inl"\
	"..\..\..\EPOC32\INCLUDE\gulalign.h"\
	"..\..\..\EPOC32\INCLUDE\gulbordr.h"\
	"..\..\..\EPOC32\INCLUDE\guldef.h"\
	"..\..\..\EPOC32\INCLUDE\gulftflg.hrh"\
	"..\..\..\EPOC32\INCLUDE\gulutil.h"\
	"..\..\..\EPOC32\INCLUDE\Mda\Client\Base.h"\
	"..\..\..\EPOC32\INCLUDE\Mda\Client\Base.inl"\
	"..\..\..\EPOC32\INCLUDE\Mda\Client\Controller.h"\
	"..\..\..\EPOC32\INCLUDE\Mda\Client\Port.h"\
	"..\..\..\EPOC32\INCLUDE\Mda\Client\Utility.h"\
	"..\..\..\EPOC32\INCLUDE\Mda\Common\Audio.h"\
	"..\..\..\EPOC32\INCLUDE\Mda\Common\Audio.hrh"\
	"..\..\..\EPOC32\INCLUDE\Mda\Common\AudioStream.hrh"\
	"..\..\..\EPOC32\INCLUDE\Mda\common\Base.h"\
	"..\..\..\EPOC32\INCLUDE\Mda\Common\Base.hrh"\
	"..\..\..\EPOC32\INCLUDE\Mda\Common\Base.inl"\
	"..\..\..\EPOC32\INCLUDE\Mda\common\controller.h"\
	"..\..\..\EPOC32\INCLUDE\Mda\Common\Controller.hrh"\
	"..\..\..\EPOC32\INCLUDE\Mda\Common\Port.h"\
	"..\..\..\EPOC32\INCLUDE\Mda\Common\Port.hrh"\
	"..\..\..\EPOC32\INCLUDE\Mda\Common\Resource.h"\
	"..\..\..\EPOC32\INCLUDE\Mda\Common\Resource.hrh"\
	"..\..\..\EPOC32\INCLUDE\mdaaudiooutputstream.h"\
	"..\..\..\EPOC32\INCLUDE\openfont.h"\
	"..\..\..\EPOC32\INCLUDE\s32std.h"\
	"..\..\..\EPOC32\INCLUDE\s32std.inl"\
	"..\..\..\EPOC32\INCLUDE\s32strm.h"\
	"..\..\..\EPOC32\INCLUDE\s32strm.inl"\
	"..\..\..\EPOC32\INCLUDE\w32std.h"\
	"..\INC\Greg2container.h"\
	
# End Source File
# Begin Source File

SOURCE=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\SRC\Greg2document.cpp
DEP_CPP_GREG2D=\
	"..\..\..\EPOC32\INCLUDE\aknappui.h"\
	"..\..\..\EPOC32\INCLUDE\akndoc.h"\
	"..\..\..\EPOC32\INCLUDE\aknscbut.h"\
	"..\..\..\EPOC32\INCLUDE\aknscrlb.h"\
	"..\..\..\EPOC32\INCLUDE\AknWsEventObserver.h"\
	"..\..\..\EPOC32\INCLUDE\apacmdln.h"\
	"..\..\..\EPOC32\INCLUDE\apadef.h"\
	"..\..\..\EPOC32\INCLUDE\apaflrec.h"\
	"..\..\..\EPOC32\INCLUDE\apaid.h"\
	"..\..\..\EPOC32\INCLUDE\apgtask.h"\
	"..\..\..\EPOC32\INCLUDE\apparc.h"\
	"..\..\..\EPOC32\INCLUDE\avkon.hrh"\
	"..\..\..\EPOC32\INCLUDE\badesca.h"\
	"..\..\..\EPOC32\INCLUDE\baerrhan.h"\
	"..\..\..\EPOC32\INCLUDE\bamdesca.h"\
	"..\..\..\EPOC32\INCLUDE\barsc.h"\
	"..\..\..\EPOC32\INCLUDE\basched.h"\
	"..\..\..\EPOC32\INCLUDE\bitbase.h"\
	"..\..\..\EPOC32\INCLUDE\bitdev.h"\
	"..\..\..\EPOC32\INCLUDE\bitdev.inl"\
	"..\..\..\EPOC32\INCLUDE\bitmap.h"\
	"..\..\..\EPOC32\INCLUDE\bitstd.h"\
	"..\..\..\EPOC32\INCLUDE\coeaui.h"\
	"..\..\..\EPOC32\INCLUDE\coeauib.h"\
	"..\..\..\EPOC32\INCLUDE\coeccntx.h"\
	"..\..\..\EPOC32\INCLUDE\coecntrl.h"\
	"..\..\..\EPOC32\INCLUDE\coecobs.h"\
	"..\..\..\EPOC32\INCLUDE\coedef.h"\
	"..\..\..\EPOC32\INCLUDE\coehelp.h"\
	"..\..\..\EPOC32\INCLUDE\coeinput.h"\
	"..\..\..\EPOC32\INCLUDE\coemain.h"\
	"..\..\..\EPOC32\INCLUDE\coemop.h"\
	"..\..\..\EPOC32\INCLUDE\coeview.h"\
	"..\..\..\EPOC32\INCLUDE\e32base.h"\
	"..\..\..\EPOC32\INCLUDE\e32base.inl"\
	"..\..\..\EPOC32\INCLUDE\e32def.h"\
	"..\..\..\EPOC32\INCLUDE\e32des16.h"\
	"..\..\..\EPOC32\INCLUDE\e32des8.h"\
	"..\..\..\EPOC32\INCLUDE\e32hal.h"\
	"..\..\..\EPOC32\INCLUDE\e32keys.h"\
	"..\..\..\EPOC32\INCLUDE\e32pccd.h"\
	"..\..\..\EPOC32\INCLUDE\e32std.h"\
	"..\..\..\EPOC32\INCLUDE\e32std.inl"\
	"..\..\..\EPOC32\INCLUDE\e32svr.h"\
	"..\..\..\EPOC32\INCLUDE\eikamnt.h"\
	"..\..\..\EPOC32\INCLUDE\eikapp.h"\
	"..\..\..\EPOC32\INCLUDE\eikappui.h"\
	"..\..\..\EPOC32\INCLUDE\eikaufty.h"\
	"..\..\..\EPOC32\INCLUDE\eikbctrl.h"\
	"..\..\..\EPOC32\INCLUDE\eikbtgrp.h"\
	"..\..\..\EPOC32\INCLUDE\eikcba.h"\
	"..\..\..\EPOC32\INCLUDE\eikcmobs.h"\
	"..\..\..\EPOC32\INCLUDE\eikcolor.hrh"\
	"..\..\..\EPOC32\INCLUDE\eikctgrp.h"\
	"..\..\..\EPOC32\INCLUDE\eikdef.h"\
	"..\..\..\EPOC32\INCLUDE\eikdgfty.h"\
	"..\..\..\EPOC32\INCLUDE\eikdoc.h"\
	"..\..\..\EPOC32\INCLUDE\eikenv.h"\
	"..\..\..\EPOC32\INCLUDE\eikfctry.h"\
	"..\..\..\EPOC32\INCLUDE\eiklibry.h"\
	"..\..\..\EPOC32\INCLUDE\eikmobs.h"\
	"..\..\..\EPOC32\INCLUDE\eiksbfrm.h"\
	"..\..\..\EPOC32\INCLUDE\eiksbobs.h"\
	"..\..\..\EPOC32\INCLUDE\eikscrlb.h"\
	"..\..\..\EPOC32\INCLUDE\Eikspane.h"\
	"..\..\..\EPOC32\INCLUDE\eikspmod.h"\
	"..\..\..\EPOC32\INCLUDE\eiksrv.h"\
	"..\..\..\EPOC32\INCLUDE\eiksrv.pan"\
	"..\..\..\EPOC32\INCLUDE\eiksrvc.h"\
	"..\..\..\EPOC32\INCLUDE\eiksrvs.h"\
	"..\..\..\EPOC32\INCLUDE\eikunder.h"\
	"..\..\..\EPOC32\INCLUDE\eikvcurs.h"\
	"..\..\..\EPOC32\INCLUDE\f32file.h"\
	"..\..\..\EPOC32\INCLUDE\f32file.inl"\
	"..\..\..\EPOC32\INCLUDE\fbs.h"\
	"..\..\..\EPOC32\INCLUDE\fldbase.h"\
	"..\..\..\EPOC32\INCLUDE\fldbltin.h"\
	"..\..\..\EPOC32\INCLUDE\fldinfo.h"\
	"..\..\..\EPOC32\INCLUDE\fldset.h"\
	"..\..\..\EPOC32\INCLUDE\fntstore.h"\
	"..\..\..\EPOC32\INCLUDE\frmlaydt.h"\
	"..\..\..\EPOC32\INCLUDE\frmparam.h"\
	"..\..\..\EPOC32\INCLUDE\frmtlay.h"\
	"..\..\..\EPOC32\INCLUDE\frmvis.h"\
	"..\..\..\EPOC32\INCLUDE\gdi.h"\
	"..\..\..\EPOC32\INCLUDE\gdi.inl"\
	"..\..\..\EPOC32\INCLUDE\gulalign.h"\
	"..\..\..\EPOC32\INCLUDE\gulbordr.h"\
	"..\..\..\EPOC32\INCLUDE\gulcolor.h"\
	"..\..\..\EPOC32\INCLUDE\guldef.h"\
	"..\..\..\EPOC32\INCLUDE\lafmain.h"\
	"..\..\..\EPOC32\INCLUDE\lafpublc.h"\
	"..\..\..\EPOC32\INCLUDE\openfont.h"\
	"..\..\..\EPOC32\INCLUDE\s32buf.h"\
	"..\..\..\EPOC32\INCLUDE\s32buf.inl"\
	"..\..\..\EPOC32\INCLUDE\s32page.h"\
	"..\..\..\EPOC32\INCLUDE\s32page.inl"\
	"..\..\..\EPOC32\INCLUDE\s32share.h"\
	"..\..\..\EPOC32\INCLUDE\s32share.inl"\
	"..\..\..\EPOC32\INCLUDE\s32std.h"\
	"..\..\..\EPOC32\INCLUDE\s32std.inl"\
	"..\..\..\EPOC32\INCLUDE\s32stor.h"\
	"..\..\..\EPOC32\INCLUDE\s32stor.inl"\
	"..\..\..\EPOC32\INCLUDE\s32strm.h"\
	"..\..\..\EPOC32\INCLUDE\s32strm.inl"\
	"..\..\..\EPOC32\INCLUDE\savenotf.h"\
	"..\..\..\EPOC32\INCLUDE\tagma.h"\
	"..\..\..\EPOC32\INCLUDE\txtetext.h"\
	"..\..\..\EPOC32\INCLUDE\txtetext.inl"\
	"..\..\..\EPOC32\INCLUDE\txtfmlyr.h"\
	"..\..\..\EPOC32\INCLUDE\txtfmlyr.inl"\
	"..\..\..\EPOC32\INCLUDE\txtfmstm.h"\
	"..\..\..\EPOC32\INCLUDE\txtfrmat.h"\
	"..\..\..\EPOC32\INCLUDE\txtfrmat.inl"\
	"..\..\..\EPOC32\INCLUDE\txtlaydc.h"\
	"..\..\..\EPOC32\INCLUDE\txtstyle.h"\
	"..\..\..\EPOC32\INCLUDE\txtstyle.inl"\
	"..\..\..\EPOC32\INCLUDE\uikon.hrh"\
	"..\..\..\EPOC32\INCLUDE\vwsdef.h"\
	"..\..\..\EPOC32\INCLUDE\w32std.h"\
	"..\INC\Greg2appui.h"\
	"..\INC\Greg2document.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\..\Epoc32\Release\wins\UDEB\auddevlib.lib
# End Source File
# Begin Source File

SOURCE=..\..\..\Epoc32\Release\wins\UDEB\testauddevice.lib
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\INC\Greg2.hrh
# End Source File
# Begin Source File

SOURCE=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\INC\Greg2app.h
# End Source File
# Begin Source File

SOURCE=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\INC\Greg2appui.h
# End Source File
# Begin Source File

SOURCE=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\INC\Greg2container.h
# End Source File
# Begin Source File

SOURCE=\SYMBIAN\6.1\SERIES60\EPOC32EX\GREG2\INC\Greg2document.h
# End Source File
# Begin Source File

SOURCE=\HELIXSYM\AUDIO\DEVICE\TEST\Testauddevice.h

!IF  "$(CFG)" == "GREG2 - Win32 Uni Release"

!ELSEIF  "$(CFG)" == "GREG2 - Win32 Uni Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
