; greenphone-sdk.nsi
;
; This script installs greenphone sdk on windows.
!include "Sections.nsh"
!include "LogicLib.nsh"
!include "TextFunc.nsh"

SetCompress off

;--------------------------------

Name "Qtopia SDK Version: !QPE_VERSION! Build: !CHANGENO!"
Caption "Qtopia SDK"
Icon "install.ico"
OutFile "autorun.exe"

SetDateSave on
SetDatablockOptimize on
CRCCheck on
;SilentInstall silent
XPStyle on

InstallDir "$PROFILE\Trolltech\Qtopia"
InstallDirRegKey HKLM "Software\Trolltech\Qtopia" ""

CheckBitmap "${NSISDIR}\Contrib\Graphics\Checks\classic-cross.bmp"

;--------------------------------

LicenseData "license.txt"

Page license
;Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

ShowInstDetails nevershow

;--------------------------------

Section "Qtopia SDK"
  SetOutPath $INSTDIR
  DetailPrint "Writing Reg entries..."
  ; write reg info
  WriteRegStr HKLM SOFTWARE\Trolltech\Qtopia "Install_Dir" "$INSTDIR"
  ; write uninstall strings
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Qtopia" "DisplayName" "Qtopia (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Qtopia" "UninstallString" '"$INSTDIR\bt-uninst.exe"'
  WriteUninstaller "bt-uninst.exe"
  CreateDirectory "$SMPROGRAMS\Trolltech"
  CreateDirectory "$SMPROGRAMS\Trolltech\Qtopia"
  CreateShortCut "$SMPROGRAMS\Trolltech\Qtopia\Uninstall Qtopia SDK.lnk" "$INSTDIR\bt-uninst.exe"
  CreateShortCut "$SMPROGRAMS\Trolltech\Qtopia\Qtopia SDK.lnk" "$INSTDIR\sdk.vmx"
  CreateShortCut "$SMPROGRAMS\Trolltech\Qtopia\Getting Started.lnk" "$INSTDIR\Qtopia_gettingstarted.pdf"
  CreateShortCut "$SMPROGRAMS\Trolltech\Qtopia\User Guide.lnk" "$INSTDIR\Qtopia_userguide.pdf"
  CreateShortCut "$SMPROGRAMS\Trolltech\Qtopia\Dev Quick Start.lnk" "$INSTDIR\Qtopia_quickstart.pdf"
  CreateShortCut "$SMPROGRAMS\Trolltech\Qtopia\Release Notes.lnk" "$INSTDIR\release.html"
  CreateShortCut "$DESKTOP\Qtopia SDK.lnk" "$INSTDIR\sdk.vmx"
  ;File Qtopia.vmx
  File install.ico
  File license.txt

  CopyFiles "$EXEDIR\Qtopia_gettingstarted.pdf" "$INSTDIR"
  CopyFiles "$EXEDIR\Qtopia_quickstart.pdf" "$INSTDIR"
  CopyFiles "$EXEDIR\Qtopia_userguide.pdf" "$INSTDIR"
  CopyFiles "$EXEDIR\qtopia_greenphone-thm2.png" "$INSTDIR"
  CopyFiles "$EXEDIR\trolltech.png" "$INSTDIR"
  CopyFiles "$EXEDIR\vmware_player.png" "$INSTDIR"
  CopyFiles "$EXEDIR\release.html" "$INSTDIR"
  CopyFiles "$EXEDIR\qtopia.iso" "$INSTDIR"
  CopyFiles "$EXEDIR\sdk.vmx" "$INSTDIR"


  DetailPrint "" 
  IfFileExists "$INSTDIR\qtopiasrc.vmdk" 0 doit1
  MessageBox MB_YESNO "Overwrite Qtopia SDK source partition?" IDYES doit1 IDNO next1
  doit1:
  DetailPrint "Installing qtopia src... Step 1/5"
  untgz::extract -d "$INSTDIR" -zbz2 "$EXEDIR\qtopiasrc.dat"
  AddSize 10000
  next1:

  DetailPrint "" 
  IfFileExists "$INSTDIR\home.vmdk" 0 doit2
  MessageBox MB_YESNO "Overwrite Qtopia SDK home partition?" IDYES doit2 IDNO next2
  doit2:
  DetailPrint "Installing home......... Step 2/5"
  untgz::extract -d "$INSTDIR" -zbz2 "$EXEDIR\home.dat"
  AddSize 50000
  next2:

  DetailPrint "" 
  DetailPrint "Installing qtopia SDK... Step 3/5"
  untgz::extract -d "$INSTDIR" -zbz2 "$EXEDIR\qtopia.dat"  
  AddSize 500000

  DetailPrint "" 
  DetailPrint "Installing toolchain.... Step 4/5"
  untgz::extract -d "$INSTDIR" -zbz2 "$EXEDIR\toolchain.dat"
  AddSize 220000

  DetailPrint "" 
  IfFileExists "$INSTDIR\rootfs.vmdk" 0 doit3
  MessageBox MB_YESNO "Overwrite Qtopia SDK root filesystem partition?" IDYES doit3 IDNO next3
  doit3:
  DetailPrint "Installing root......... Step 5/5"
  untgz::extract -d "$INSTDIR" -zbz2 "$EXEDIR\rootfs.dat"
  AddSize 1100000
  next3:

SectionEnd

;--------------------------------
  
Function .onInit
 System::Call 'kernel32::CreateMutexA(i 0, i 0, t "gpMutex") i .r1 ?e'
 Pop $R0
 
 StrCmp $R0 0 +3
   MessageBox MB_OK|MB_ICONEXCLAMATION "The installer is already running."
   Abort

  SetOutPath $TEMP
  File /oname=spltmp.bmp "splash.bmp"
  ;File /oname=spltmp.wav "splash.wav"
  splash::show 5000 $TEMP\spltmp
  Pop $0
  Delete $TEMP\spltmp.bmp
  ;Delete $TEMP\spltmp.wav
  ExecShell "open" "$EXEDIR\release.html"
  sleep 3000
FunctionEnd

;--------------------------------

; Uninstaller

UninstallText "This will uninstall Qtopia SDK. Hit next to continue."
UninstallIcon "install.ico"

Section "Uninstall"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Qtopia"
  DeleteRegKey HKLM "SOFTWARE\Trolltech\Qtopia"
  Delete "$INSTDIR\bt-uninst.exe"
  Delete "$INSTDIR\license.txt"
  Delete "$INSTDIR\qtopia.vmdk"
  Delete "$INSTDIR\qtopiasrc.vmdk"
  Delete "$INSTDIR\Qtopia.vmx"
  Delete "$INSTDIR\toolchain.vmdk"
  Delete "$INSTDIR\home.vmdk"
  Delete "$INSTDIR\rootfs.vmdk"
  Delete "$INSTDIR\qtopia.iso"
  Delete "$INSTDIR\*.vmsd"
  Delete "$INSTDIR\*.log"
  Delete "$INSTDIR\*.nvram"
  Delete "$INSTDIR\*.vmxf"
  Delete "$INSTDIR\*.vmss"
  Delete "$INSTDIR\*.vmem"
  Delete "$INSTDIR\*.lck"
  Delete "$INSTDIR\*.pdf"
  Delete "$INSTDIR\*.html"
  Delete "$INSTDIR\*.png"
  Delete "$INSTDIR\install.ico"
  Delete "$SMPROGRAMS\Trolltech\Qtopia\Uninstall Qtopia SDK.lnk"
  Delete "$SMPROGRAMS\Trolltech\Qtopia\Qtopia SDK.lnk"
  Delete "$SMPROGRAMS\Trolltech\Qtopia\Getting Started.lnk"
  Delete "$SMPROGRAMS\Trolltech\Qtopia\User Guide.lnk"
  Delete "$SMPROGRAMS\Trolltech\Qtopia\Dev Quick Start.lnk"
  Delete "$SMPROGRAMS\Trolltech\Qtopia\Release Notes.lnk"
  Delete "$DESKTOP\Qtopia SDK.lnk"
  RMDir  "$SMPROGRAMS\Trolltech\Qtopia"
  RMDir  "$INSTDIR"
  IfFileExists "$INSTDIR" 0 NoErrorMsg
    MessageBox MB_OK "Note: $INSTDIR could not be removed!" IDOK 0 ; skipped if file doesn't exist
  NoErrorMsg:

SectionEnd
