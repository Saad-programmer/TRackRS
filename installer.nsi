; NSIS Installer Script for TrackRS Demo
; Build this script using NSIS (makensis) on Windows to generate trackrs-demo-installer.exe

!define APP_NAME "TrackRS Demo"
!define COMP_NAME "TrackRS Dev Team"
!define VERSION "0.6.7"
!define INSTALLER_NAME "trackrs-demo-setup.exe"

Name "${APP_NAME}"
OutFile "${INSTALLER_NAME}"
InstallDir "$PROGRAMFILES\TrackRS Demo"
InstallDirRegKey HKLM "Software\${APP_NAME}" "Install_Dir"

RequestExecutionLevel admin

; Modern UI 2
!include "MUI2.nsh"
!define MUI_ABORTWARNING
!define MUI_ICON "data\icon\trigger-rally-win.ico"
!define MUI_UNICON "data\icon\trigger-rally-win.ico"

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; Languages
!insertmacro MUI_LANGUAGE "English"

Section "Install"
  SetOutPath "$INSTDIR"
  
  ; Copy primary game executable and config definitions
  File "bin\trackrs.exe"
  File "bin\trackrs.config.defs"
  
  ; Copy Windows DLL dependencies (ensure these are in the bin folder)
  File /nonfatal "bin\*.dll"
  
  ; Copy game data files recursively
  SetOutPath "$INSTDIR\data"
  File /r "data\*.*"
  
  ; Write install directory to registry
  WriteRegStr HKLM "Software\${APP_NAME}" "Install_Dir" "$INSTDIR"
  
  ; Registry keys for Add/Remove Programs
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayName" "${APP_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayIcon" '"$INSTDIR\trackrs.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "Publisher" "${COMP_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}" "DisplayVersion" "${VERSION}"
  
  ; Create uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
  ; Create shortcuts
  CreateDirectory "$SMPROGRAMS\${APP_NAME}"
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\trackrs.exe" "" "$INSTDIR\trackrs.exe" 0
  CreateShortcut "$SMPROGRAMS\${APP_NAME}\Uninstall ${APP_NAME}.lnk" "$INSTDIR\uninstall.exe"
  CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\trackrs.exe" "" "$INSTDIR\trackrs.exe" 0
SectionEnd

Section "Uninstall"
  ; Remove Registry Keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
  DeleteRegKey HKLM "Software\${APP_NAME}"
  
  ; Remove Shortcuts
  Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
  Delete "$SMPROGRAMS\${APP_NAME}\Uninstall ${APP_NAME}.lnk"
  RMDir "$SMPROGRAMS\${APP_NAME}"
  Delete "$DESKTOP\${APP_NAME}.lnk"
  
  ; Remove Files and Subfolders
  Delete "$INSTDIR\trackrs.exe"
  Delete "$INSTDIR\trackrs.config.defs"
  Delete "$INSTDIR\*.dll"
  Delete "$INSTDIR\uninstall.exe"
  RMDir /r "$INSTDIR\data"
  RMDir "$INSTDIR"
SectionEnd
