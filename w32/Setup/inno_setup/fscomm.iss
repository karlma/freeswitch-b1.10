; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
;AppId={{ED55656F-D567-4B3D-A0B9-617CAB13D519}
AppName=FSComm
AppVerName=FSComm svn 16235
AppPublisherURL=http://wiki.freeswitch.org/wiki/FSComm
AppSupportURL=http://wiki.freeswitch.org/wiki/FSComm
AppUpdatesURL=http://wiki.freeswitch.org/wiki/FSComm
DefaultDirName=C:\FSComm
DefaultGroupName=FSComm
SetupIconFile=freeswitch.ico
OutputBaseFilename=FSComm
Compression=lzma
SolidCompression=yes
OutputDir=Output
ArchitecturesInstallIn64BitMode=x64

; The define statements below require ISPP (Inno Setup Preprocesso) to be installed.
; It's part of the QuickStart Pack (http://www.jrsoftware.org/isdl.php

#define QT32Bit_Build "D:\Qt\win32\bin"
#define QT64Bit_Build "D:\Qt\x64\bin"

#define FreeSWITCH_32bit "..\..\..\release"
#define FreeSWITCH_64bit "..\..\..\x64\release"

#define FSComm_32bit "..\..\..\fscomm\release"
#define FSComm_64bit "..\..\..\fscomm\x64\release"

[Languages]
Name: english; MessagesFile: compiler:Default.isl

[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked

[Files]

Source: vcredist_x86.exe; Flags: 32bit; DestDir: {tmp}; Check: not Is64BitInstallMode
Source: vcredist_x64.exe; Flags: 64bit; DestDir: {tmp}; Check: Is64BitInstallMode

; 32 bit
Source: freeswitch.ico; DestDir: {app}
Source: {#FSComm_32bit}\fscomm.exe; DestDir: {app}; Flags: 32bit ignoreversion; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\fs_cli.exe; DestDir: {app}; Flags: 32bit ignoreversion; Check: not Is64BitInstallMode
Source: {#FSComm_32bit}\..\conf\*; DestDir: {app}\conf; Flags: 32bit ignoreversion  onlyifdoesntexist recursesubdirs; Check: not Is64BitInstallMode

; 32 bit QT libraries
Source: {#QT32Bit_Build}\QtCore4.dll; DestDir: {app}; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#QT32Bit_Build}\QtGui4.dll; DestDir: {app}; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#QT32Bit_Build}\QtXml4.dll; DestDir: {app}; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode

; required FreeSWICTCH modules for 32bit build
Source: {#FreeSWITCH_32bit}\mod\mod_console.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_logfile.dll; DestDir: {app}\mod; Flags: 32bit  ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_cdr_csv.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_commands.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_dialplan_xml.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_dptools.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_enum.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_event_socket.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_ilbc.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_local_stream.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_loopback.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_PortAudio.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_siren.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_sndfile.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_sofia.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_speex.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_tone_stream.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\mod_voipcodecs.dll; DestDir: {app}\mod; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode

; required FreeSWICTCH dlls for 32bit build
Source: {#FreeSWITCH_32bit}\pthreadVC2.dll; DestDir: {app}; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\libspandsp.dll; DestDir: {app}; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\libbroadvoice.dll; DestDir: {app}; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\libapr.dll; DestDir: {app}; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\libaprutil.dll; DestDir: {app}; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\libteletone.dll; DestDir: {app}; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\FreeSwitch.dll; DestDir: {app}; Flags: 32bit ignoreversion  onlyifdoesntexist; Check: not Is64BitInstallMode

; 64 bit FSComm build
Source: freeswitch.ico; DestDir: {app}
Source: {#FSComm_64bit}\fscomm.exe; DestDir: {app}; Flags: 64bit ignoreversion; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\fs_cli.exe; DestDir: {app}; Flags: 64bit ignoreversion; Check: Is64BitInstallMode
Source: {#FSComm_32bit}\..\conf\*; DestDir: {app}\conf; Flags: 64bit ignoreversion onlyifdoesntexist recursesubdirs; Check: Is64BitInstallMode

; 64 bit QT libraries
Source: {#QT64Bit_Build}\QtCore4.dll; DestDir: {app}; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#QT64Bit_Build}\QtGui4.dll; DestDir: {app}; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#QT64Bit_Build}\QtXml4.dll; DestDir: {app}; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode

; required FreeSWICTCH modules for 64bit build
Source: {#FreeSWITCH_64bit}\mod\mod_console.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_logfile.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_cdr_csv.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_commands.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_dialplan_xml.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_dptools.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_enum.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_event_socket.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_ilbc.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_local_stream.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_loopback.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_PortAudio.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_siren.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_sndfile.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_sofia.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_speex.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_tone_stream.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\mod_voipcodecs.dll; DestDir: {app}\mod; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode

; required FreeSWICTCH dlls for 64bit build
Source: {#FreeSWITCH_64bit}\pthreadVC2.dll; DestDir: {app}; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\libspandsp.dll; DestDir: {app}; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\libbroadvoice.dll; DestDir: {app}; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\libapr.dll; DestDir: {app}; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\libaprutil.dll; DestDir: {app}; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\libteletone.dll; DestDir: {app}; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\FreeSwitch.dll; DestDir: {app}; Flags: 64bit ignoreversion  onlyifdoesntexist; Check: Is64BitInstallMode

[Icons]
Name: {group}\FSComm; Filename: {app}\FSComm.exe; IconFilename: {app}\freeswitch.ico; IconIndex: 0; WorkingDir: {app}\plugins
Name: {commondesktop}\FSComm; Filename: {app}\FSComm.exe; Tasks: desktopicon; IconFilename: {app}\freeswitch.ico; IconIndex: 0; WorkingDir: {app}\plugins

[Run]
Filename: {app}\FSComm.exe; Description: {cm:LaunchProgram,FSComm}; Flags: nowait postinstall skipifsilent
Filename: {tmp}\vcredist_x86.exe; Description: Microsoft Visual C++ 2008 Redistributable Package (x86); Parameters: /q; Flags: 32bit; Check: not Is64BitInstallMode
Filename: {tmp}\vcredist_x64.exe; Description: Microsoft Visual C++ 2008 Redistributable Package (x64); Parameters: /q; Flags: 64bit; Check: Is64BitInstallMode
