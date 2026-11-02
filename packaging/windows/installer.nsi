; NSIS installer for the Veyon Internet Access Control plugin
;
; makensis -DVERSION=0.0.1 -DPLUGIN_DLL=build/internetaccesscontrol.dll -DOUTFILE=setup.exe packaging/windows/installer.nsi
;
; Silent installation: setup.exe /S
; Silent removal:      "C:\Program Files\Veyon Internet Access Control\uninstall.exe" /S
;
; SPDX-License-Identifier: MIT

Unicode true

!ifndef VERSION
	!error "VERSION is not defined"
!endif
!ifndef PLUGIN_DLL
	!error "PLUGIN_DLL is not defined"
!endif
!ifndef OUTFILE
	!define OUTFILE "veyon-internet-access-control-${VERSION}-win64-setup.exe"
!endif

!define APP_NAME "Veyon Internet Access Control"
!define PUBLISHER "Veyon Internet Access Control contributors"
!define WEB_SITE "https://github.com/elmodos/veyon-internet-access-control"
!define PLUGIN_FILE "internetaccesscontrol.dll"
!define REG_ROOT "HKLM"
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\VeyonInternetAccessControl"
; written by the Veyon installer (32 bit registry view, as the Veyon installer doesn't use SetRegView)
!define VEYON_UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Veyon"
!define VEYON_MINOR_SERIES "4.11."
; makensis changes into the directory of this script
!define SRC "..\.."

Name "${APP_NAME}"
Caption "${APP_NAME} ${VERSION}"
BrandingText "${APP_NAME} ${VERSION}"
OutFile "${OUTFILE}"
; the uninstaller and documentation are kept outside of the Veyon directory, as installing or
; updating Veyon removes its plugins directory
InstallDir "$PROGRAMFILES64\${APP_NAME}"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
ShowInstDetails show
ShowUninstDetails show

VIProductVersion "${VERSION}.0"
VIAddVersionKey /LANG=0 "ProductName" "${APP_NAME}"
VIAddVersionKey /LANG=0 "CompanyName" "${PUBLISHER}"
VIAddVersionKey /LANG=0 "LegalCopyright" "MIT License"
VIAddVersionKey /LANG=0 "FileDescription" "${APP_NAME} installer"
VIAddVersionKey /LANG=0 "FileVersion" "${VERSION}"
VIAddVersionKey /LANG=0 "ProductVersion" "${VERSION}"

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "FileFunc.nsh"
!include "x64.nsh"

Var VeyonDir

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_TEXT "$(FinishText)"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${SRC}\LICENSE"
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; the language matching the Windows UI language is chosen automatically
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Ukrainian"

LangString AdminRequired ${LANG_ENGLISH} "Please run the installer with administrative rights."
LangString AdminRequired ${LANG_UKRAINIAN} "Запустіть інсталятор із правами адміністратора."
LangString Requires64Bit ${LANG_ENGLISH} "This plugin requires 64-bit Windows and the 64-bit version of Veyon."
LangString Requires64Bit ${LANG_UKRAINIAN} "Цей плагін потребує 64-бітної Windows і 64-бітної версії Veyon."
LangString VeyonNotFound ${LANG_ENGLISH} "Veyon was not found. Please install Veyon ${VEYON_MINOR_SERIES}x first."
LangString VeyonNotFound ${LANG_UKRAINIAN} "Veyon не знайдено. Спершу встановіть Veyon ${VEYON_MINOR_SERIES}x."
LangString VeyonVersionMismatch ${LANG_ENGLISH} "This plugin was built for Veyon ${VEYON_MINOR_SERIES}x, but Veyon $0 is installed. The plugin will probably not be loaded.$\n$\nInstall anyway?"
LangString VeyonVersionMismatch ${LANG_UKRAINIAN} "Цей плагін зібрано для Veyon ${VEYON_MINOR_SERIES}x, а встановлено Veyon $0. Найімовірніше, плагін не завантажиться.$\n$\nВсе одно встановити?"
LangString StoppingService ${LANG_ENGLISH} "Stopping the Veyon service..."
LangString StoppingService ${LANG_UKRAINIAN} "Зупинка служби Veyon..."
LangString StartingService ${LANG_ENGLISH} "Starting the Veyon service..."
LangString StartingService ${LANG_UKRAINIAN} "Запуск служби Veyon..."
LangString RemovingRules ${LANG_ENGLISH} "Removing firewall rules of the plugin..."
LangString RemovingRules ${LANG_UKRAINIAN} "Видалення правил брандмауера, створених плагіном..."
LangString FinishText ${LANG_ENGLISH} "${APP_NAME} has been installed into Veyon.$\n$\nOn the supervisor computer, restart Veyon Master. After updating Veyon, install the plugin again, as the Veyon installer removes all plugins."
LangString FinishText ${LANG_UKRAINIAN} "${APP_NAME} встановлено у Veyon.$\n$\nНа комп’ютері супервайзера перезапустіть Veyon Master. Після оновлення Veyon встановіть плагін знову, бо інсталятор Veyon видаляє всі плагіни."


!macro StopVeyonService
	DetailPrint "$(StoppingService)"
	nsExec::ExecToLog '"$VeyonDir\veyon-wcli.exe" service stop'
	Pop $0
	; make sure the service released the plugin file
	Sleep 2000
!macroend

!macro StartVeyonService
	DetailPrint "$(StartingService)"
	nsExec::ExecToLog '"$VeyonDir\veyon-wcli.exe" service start'
	Pop $0
!macroend


Function .onInit
	UserInfo::GetAccountType
	Pop $0
	${If} $0 != "admin"
		MessageBox MB_ICONSTOP "$(AdminRequired)" /SD IDOK
		SetErrorLevel 740 ; ERROR_ELEVATION_REQUIRED
		Quit
	${EndIf}

	${IfNot} ${RunningX64}
		MessageBox MB_ICONSTOP "$(Requires64Bit)" /SD IDOK
		SetErrorLevel 2
		Quit
	${EndIf}

	; the Veyon uninstaller lives in the Veyon installation directory
	ReadRegStr $0 ${REG_ROOT} "${VEYON_UNINSTALL_KEY}" "UninstallString"
	${If} $0 != ""
		${GetParent} $0 $VeyonDir
	${Else}
		StrCpy $VeyonDir "$PROGRAMFILES64\Veyon"
	${EndIf}

	${IfNot} ${FileExists} "$VeyonDir\veyon-core.dll"
		MessageBox MB_ICONSTOP "$(VeyonNotFound)" /SD IDOK
		SetErrorLevel 2
		Quit
	${EndIf}

	ReadRegStr $0 ${REG_ROOT} "${VEYON_UNINSTALL_KEY}" "DisplayVersion"
	StrLen $1 "${VEYON_MINOR_SERIES}"
	StrCpy $2 $0 $1
	${If} $0 != ""
	${AndIf} $2 != "${VEYON_MINOR_SERIES}"
		; silent installations are aborted, as the plugin would not work anyway
		MessageBox MB_ICONEXCLAMATION|MB_YESNO "$(VeyonVersionMismatch)" /SD IDNO IDYES versionAccepted
		SetErrorLevel 3
		Quit
		versionAccepted:
	${EndIf}
FunctionEnd


Section "Install"
	SetShellVarContext all

	!insertmacro StopVeyonService

	SetOutPath "$VeyonDir\plugins"
	SetOverwrite on
	File "/oname=${PLUGIN_FILE}" "${PLUGIN_DLL}"

	SetOutPath "$INSTDIR"
	File "${SRC}\LICENSE"
	File "${SRC}\README.md"
	File "${SRC}\README.uk.md"
	File "${SRC}\scripts\windows\diagnose.ps1"
	WriteUninstaller "$INSTDIR\uninstall.exe"

	WriteRegStr ${REG_ROOT} "${UNINSTALL_KEY}" "DisplayName" "${APP_NAME}"
	WriteRegStr ${REG_ROOT} "${UNINSTALL_KEY}" "DisplayVersion" "${VERSION}"
	WriteRegStr ${REG_ROOT} "${UNINSTALL_KEY}" "Publisher" "${PUBLISHER}"
	WriteRegStr ${REG_ROOT} "${UNINSTALL_KEY}" "URLInfoAbout" "${WEB_SITE}"
	WriteRegStr ${REG_ROOT} "${UNINSTALL_KEY}" "DisplayIcon" "$VeyonDir\veyon-master.exe"
	WriteRegStr ${REG_ROOT} "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
	WriteRegStr ${REG_ROOT} "${UNINSTALL_KEY}" "VeyonDirectory" "$VeyonDir"
	WriteRegStr ${REG_ROOT} "${UNINSTALL_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegStr ${REG_ROOT} "${UNINSTALL_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
	WriteRegDWORD ${REG_ROOT} "${UNINSTALL_KEY}" "NoModify" 1
	WriteRegDWORD ${REG_ROOT} "${UNINSTALL_KEY}" "NoRepair" 1
	${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
	WriteRegDWORD ${REG_ROOT} "${UNINSTALL_KEY}" "EstimatedSize" $0

	!insertmacro StartVeyonService
SectionEnd


Function un.onInit
	ReadRegStr $VeyonDir ${REG_ROOT} "${UNINSTALL_KEY}" "VeyonDirectory"
	${If} $VeyonDir == ""
		StrCpy $VeyonDir "$PROGRAMFILES64\Veyon"
	${EndIf}
FunctionEnd


Section "Uninstall"
	SetShellVarContext all

	!insertmacro StopVeyonService

	Delete "$VeyonDir\plugins\${PLUGIN_FILE}"

	; rules are normally removed when internet access is unblocked or the Veyon Server stops,
	; this catches rules left behind e.g. by a crash
	DetailPrint "$(RemovingRules)"
	nsExec::ExecToLog 'powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command "Get-NetFirewallRule -DisplayName Veyon_IAC_* -ErrorAction SilentlyContinue | Remove-NetFirewallRule"'
	Pop $0

	${If} ${FileExists} "$VeyonDir\veyon-wcli.exe"
		!insertmacro StartVeyonService
	${EndIf}

	Delete "$INSTDIR\LICENSE"
	Delete "$INSTDIR\README.md"
	Delete "$INSTDIR\README.uk.md"
	Delete "$INSTDIR\diagnose.ps1"
	Delete "$INSTDIR\uninstall.exe"
	RMDir "$INSTDIR"

	DeleteRegKey ${REG_ROOT} "${UNINSTALL_KEY}"
SectionEnd
