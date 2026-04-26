; TeAr VST3 Installer for Windows
; Usage: makensis -DVERSION=1.0.0 installer.nsi
; Expects TeAr.vst3 to be present in the same directory as this script.

!define APPNAME    "TeAr"
!define PUBLISHER  "FX-Mechanics"
!define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

Name            "${APPNAME} ${VERSION}"
OutFile         "TeAr-Windows-Setup.exe"
InstallDir      "$PROGRAMFILES64\FX-Mechanics\${APPNAME}"
RequestExecutionLevel admin

Page instfiles

Section "VST3 Plugin"
    SetOutPath "$COMMONFILES64\VST3"
    File /r "TeAr.vst3"

    SetOutPath "$INSTDIR"
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    WriteRegStr   HKLM "${UNINST_KEY}" "DisplayName"     "${APPNAME}"
    WriteRegStr   HKLM "${UNINST_KEY}" "Publisher"       "${PUBLISHER}"
    WriteRegStr   HKLM "${UNINST_KEY}" "DisplayVersion"  "${VERSION}"
    WriteRegStr   HKLM "${UNINST_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegDWORD HKLM "${UNINST_KEY}" "NoModify"        1
    WriteRegDWORD HKLM "${UNINST_KEY}" "NoRepair"        1
SectionEnd

Section "Uninstall"
    RMDir /r "$COMMONFILES64\VST3\TeAr.vst3"
    Delete   "$INSTDIR\Uninstall.exe"
    RMDir    "$INSTDIR"
    RMDir    "$PROGRAMFILES64\FX-Mechanics"
    DeleteRegKey HKLM "${UNINST_KEY}"
SectionEnd
