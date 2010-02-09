; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
;AppId={{088C2C50-0D5F-4276-8771-FF54CAA14E70}
AppName=FreeSWITCH
AppVerName=FreeSWITCH svn 16456
AppPublisher=FreeSWITCH
AppPublisherURL=http://www.freeswitch.org
AppSupportURL=http://www.freeswitch.org
AppUpdatesURL=http://www.freeswitch.org
DefaultDirName=C:\FreeSWITCH
DefaultGroupName=FreeSWITCH
OutputBaseFilename=freeswitch_freepbx
SetupIconFile=freeswitch.ico
Compression=lzma
SolidCompression=true
AppCopyright=Anthony Minessale II
AllowUNCPath=false
OutputDir=Output
ArchitecturesInstallIn64BitMode=x64

; The define statements below require ISPP (Inno Setup Preprocesso) to be installed.
; It's part of the QuickStart Pack (http://www.jrsoftware.org/isdl.php

#define FreeSWITCH_32bit "..\..\..\release"
#define FreeSWITCH_64bit "..\..\..\x64\release"

[Languages]
Name: english; MessagesFile: compiler:Default.isl

[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked
Name: quicklaunchicon; Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked

[Files]
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

Source: freeswitch.ico; DestDir: {app}; Components: FreeSWITCH
Source: vcredist_x86.exe; Flags: 32bit; Components: FreeSWITCH; DestDir: {tmp}; Check: not Is64BitInstallMode
Source: vcredist_x64.exe; Flags: 64bit; Components: FreeSWITCH; DestDir: {tmp}; Check: Is64BitInstallMode
Source: freepbx.ico; DestDir: {app}; Components: FreePBX_WAMP
Source: expsound.bat; DestDir: {app}; Components: FreeSWITCH
Source: FreePBX.url; DestDir: {app}; Components: FreePBX_WAMP
Source: create_freepbx.sql; DestDir: {app}; Components: FreePBX_WAMP
Source: create_freepbx.bat; DestDir: {app}; Components: FreePBX_WAMP

; 32 bit release
Source: {#FreeSWITCH_32bit}\*.exe; DestDir: {app}; Flags: ignoreversion  onlyifdoesntexist 32bit; Components: FreeSWITCH; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\*.dll; DestDir: {app}; Flags: ignoreversion  onlyifdoesntexist 32bit; Components: FreeSWITCH; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\*.lib; DestDir: {app}; Flags: ignoreversion  onlyifdoesntexist 32bit; Components: FreeSWITCH; Check: not Is64BitInstallMode
Source: {#FreeSWITCH_32bit}\mod\*; DestDir: {app}\mod; Flags: ignoreversion  onlyifdoesntexist 32bit; Components: FreeSWITCH; Check: not Is64BitInstallMode
Source: ..\..\..\Release_CLR\*; DestDir: {app}; Flags: ignoreversion recursesubdirs createallsubdirs onlyifdoesntexist 32bit; Components: FreeSWITCH; Check: not Is64BitInstallMode
; 64 bit release
Source: {#FreeSWITCH_64bit}\*.exe; DestDir: {app}; Flags: ignoreversion  onlyifdoesntexist 64bit; Components: FreeSWITCH; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\*.dll; DestDir: {app}; Flags: ignoreversion  onlyifdoesntexist 64bit; Components: FreeSWITCH; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\*.lib; DestDir: {app}; Flags: ignoreversion  onlyifdoesntexist 64bit; Components: FreeSWITCH; Check: Is64BitInstallMode
Source: {#FreeSWITCH_64bit}\mod\*; DestDir: {app}\mod; Flags: ignoreversion  onlyifdoesntexist 64bit; Components: FreeSWITCH; Check: Is64BitInstallMode
Source: ..\..\..\x64\Release_CLR\*; DestDir: {app}; Flags: ignoreversion recursesubdirs createallsubdirs onlyifdoesntexist 64bit; Components: FreeSWITCH; Check: Is64BitInstallMode

; shared by 32 and 64 bit install
Source: {#FreeSWITCH_32bit}\conf\*; DestDir: {app}\conf; Flags: ignoreversion recursesubdirs createallsubdirs  onlyifdoesntexist; Components: FreeSWITCH
Source: ..\..\..\contrib\jmesquita\fsgui\bin\*; DestDir: {app}; Flags: ignoreversion recursesubdirs createallsubdirs onlyifdoesntexist; Components: fsgui

; WAMP
Source: G:\wamp_source\close\*.*; DestDir: {app}\wamp\; Flags: ignoreversion recursesubdirs; AfterInstall: close(); Components: FreePBX_WAMP; Tasks: 
Source: G:\wamp_source\wamp\*.*; DestDir: {app}\wamp\; Flags: ignoreversion recursesubdirs onlyifdoesntexist; Components: FreePBX_WAMP
Source: G:\wamp_source\wamp\license.txt; DestDir: {app}\wamp\; AfterInstall: install_pbx(); Components: FreePBX_WAMP

Source: {#FreeSWITCH_32bit}\php_ESL.dll; DestDir: {app}\wamp\bin\php\php5.3.0\ext; Flags: ignoreversion recursesubdirs; Components: FreePBX_WAMP

; FreePBX w/ svn folders
Source: G:\freepbx-v3-dev\*; DestDir: {app}\wamp\www\freepbx-v3; Flags: ignoreversion recursesubdirs onlyifdoesntexist createallsubdirs; Components: FreePBX_WAMP
Source: G:\freepbx-v3-dev\INSTALL.TXT; DestDir: {app}\wamp\www\freepbx-v3; AfterInstall: fix_freepbx_paths(); Components: FreePBX_WAMP

; These two files are required for TLS/SSL support
; 32 bit version
Source: C:\OpenSSL\libeay32.dll; DestDir: {app}; Check: not Is64BitInstallMode; Flags: 32bit
Source: C:\OpenSSL\ssleay32.dll; DestDir: {app}; Check: not Is64BitInstallMode; Flags: 32bit
; 64 bit version
Source: C:\OpenSSL64\libeay32.dll; DestDir: {app}; Check: Is64BitInstallMode; Flags: 64bit
Source: C:\OpenSSL64\ssleay32.dll; DestDir: {app}; Check: Is64BitInstallMode; Flags: 64bit


[Dirs]
Name: {app}\log
Name: {app}\db
Name: {app}\scripts
Name: {app}\htdocs
Name: {app}\grammar
Name: {app}\backup

[Icons]
Name: {group}\FreeSWITCH; Filename: {app}\FreeSwitch.exe; Components: FreeSWITCH; IconFilename: {app}\freeswitch.ico; IconIndex: 0; WorkingDir: {app}
Name: {commondesktop}\FreeSWITCH; Filename: {app}\FreeSwitch.exe; Tasks: desktopicon; Components: FreeSWITCH; IconFilename: {app}\freeswitch.ico; IconIndex: 0; WorkingDir: {app}
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\FreeSWITCH; Filename: {app}\FreeSwitch.exe; Tasks: quicklaunchicon; Components: FreeSWITCH; IconFilename: {app}\freeswitch.ico; IconIndex: 0; WorkingDir: {app}
Name: {commondesktop}\fsgui; Filename: {app}\fsgui.exe; Tasks: desktopicon; Components: fsgui; IconFilename: {app}\freeswitch.ico; IconIndex: 0; WorkingDir: {app}\plugins
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\fsgui; Filename: {app}\fsgui.exe; Tasks: quicklaunchicon; Components: fsgui; IconFilename: {app}\freeswitch.ico; IconIndex: 0; WorkingDir: {app}\plugins

Name: {commondesktop}\FreePBX.url; Filename: {app}\FreePBX.url; Tasks: desktopicon; Components: FreePBX_WAMP; IconFilename: {app}\freepbx.ico; IconIndex: 0
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\FreePBX.url; Filename: {app}\FreePBX.url; Tasks: quicklaunchicon; Components: FreePBX_WAMP; IconFilename: {app}\freepbx.ico; IconIndex: 0

Name: {group}\start WampServer; Filename: {app}\wamp\wampmanager.exe; WorkingDir: {app}\wamp; Components: FreePBX_WAMP; Flags: runminimized
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\WampServer; Filename: {app}\wamp\wampmanager.exe; Tasks: quicklaunchicon; Components: FreePBX_WAMP
Name: {commondesktop}\WampServer; Filename: {app}\wamp\wampmanager.exe; Tasks: desktopicon; Components: FreePBX_WAMP

[Run]
Filename: {tmp}\vcredist_x86.exe; Description: Microsoft Visual C++ 2008 Redistributable Package (x86); Components: FreeSWITCH; Parameters: /q; Flags: 32bit; Check: not Is64BitInstallMode
Filename: {tmp}\vcredist_x64.exe; Description: Microsoft Visual C++ 2008 Redistributable Package (x64); Components: FreeSWITCH; Parameters: /q; Flags: 64bit; Check: Is64BitInstallMode
Filename: {app}\wamp\uninstall_services.bat; Components: FreePBX_WAMP; Flags: runhidden
Filename: {app}\wamp\install_services.bat; Components: FreePBX_WAMP; Flags: runhidden
Filename: {app}\wamp\wampmanager.exe; Description: Launch WampServer 2 now; Components: FreePBX_WAMP; Flags: shellexec postinstall skipifsilent runhidden
Filename: {app}\freeswitch.exe; Description: Launch FreeSWITCH now; Flags: shellexec postinstall skipifsilent; Components: FreeSWITCH

Filename: {app}\create_freepbx.bat; Description: Create FreePBX MySQL user; Flags: runhidden; Components: FreePBX_WAMP; Parameters: {app}; WorkingDir: {app}

Filename: {app}\expsound.bat; Parameters: {tmp} {app} freeswitch-sounds-en-us-callie-8000-1.0.11.tar.gz; Components: sound_files_8khz; WorkingDir: {tmp}
Filename: {app}\expsound.bat; Parameters: {tmp} {app} freeswitch-sounds-music-8000-1.0.8.tar.gz; Components: sound_files_8khz; WorkingDir: {tmp}; Tasks: ; Languages: 

Filename: {app}\expsound.bat; Parameters: {tmp} {app} freeswitch-sounds-en-us-callie-16000-1.0.11.tar.gz; Components: sound_files_16khz; WorkingDir: {tmp}; Tasks: ; Languages: 
Filename: {app}\expsound.bat; Parameters: {tmp} {app} freeswitch-sounds-music-16000-1.0.8.tar.gz; Components: sound_files_16khz; WorkingDir: {tmp}; Tasks: ; Languages: 

Filename: {app}\expsound.bat; Parameters: {tmp} {app} freeswitch-sounds-en-us-callie-32000-1.0.11.tar.gz; Components: sound_files_32khz; WorkingDir: {tmp}; Tasks: ; Languages: 
Filename: {app}\expsound.bat; Parameters: {tmp} {app} freeswitch-sounds-music-32000-1.0.8.tar.gz; Components: sound_files_32khz; WorkingDir: {tmp}; Tasks: ; Languages: 

Filename: {app}\expsound.bat; Parameters: {tmp} {app} freeswitch-sounds-en-us-callie-48000-1.0.11.tar.gz; Components: sound_files_48khz; WorkingDir: {tmp}; Tasks: ; Languages: 
Filename: {app}\expsound.bat; Parameters: {tmp} {app} freeswitch-sounds-music-48000-1.0.8.tar.gz; Components: sound_files_48khz; WorkingDir: {tmp}; Tasks: ; Languages: 

[UninstallDelete]
Type: filesandordirs; Name: {app}\sounds
Type: files; Name: {app}\wamp\*.*
Type: filesandordirs; Name: {app}\wamp\apps
Type: filesandordirs; Name: {app}\wamp\bin\apache
Type: filesandordirs; Name: {app}\wamp\bin\php
Type: filesandordirs; Name: {app}\wamp\help
Type: filesandordirs; Name: {app}\wamp\lang
Type: filesandordirs; Name: {app}\wamp\logs
Type: filesandordirs; Name: {app}\wamp\scripts
Type: filesandordirs; Name: {app}\wamp\tmp
Type: filesandordirs; Name: {app}

[UninstallRun]
Filename: {app}\wamp\uninstall_services.bat; Components: FreePBX_WAMP; Flags: runhidden
Filename: {app}\freeswitch.exe; Flags: runhidden; Parameters: -stop; WorkingDir: {app}

[_ISToolDownload]
Source: http://files.freeswitch.org/freeswitch-sounds-en-us-callie-8000-1.0.12.tar.gz; DestDir: {tmp}; DestName: freeswitch-sounds-en-us-callie-8000-1.0.11.tar.gz; Components: sound_files_8khz
Source: http://files.freeswitch.org/freeswitch-sounds-en-us-callie-16000-1.0.12.tar.gz; DestDir: {tmp}; DestName: freeswitch-sounds-en-us-callie-16000-1.0.11.tar.gz; Components: sound_files_16khz
Source: http://files.freeswitch.org/freeswitch-sounds-en-us-callie-32000-1.0.12.tar.gz; DestDir: {tmp}; DestName: freeswitch-sounds-en-us-callie-32000-1.0.11.tar.gz; Components: sound_files_32khz
Source: http://files.freeswitch.org/freeswitch-sounds-en-us-callie-48000-1.0.12.tar.gz; DestDir: {tmp}; DestName: freeswitch-sounds-en-us-callie-48000-1.0.11.tar.gz; Components: sound_files_48khz

Source: http://files.freeswitch.org/freeswitch-sounds-music-8000-1.0.8.tar.gz; DestDir: {tmp}; DestName: freeswitch-sounds-music-8000-1.0.8.tar.gz; Components: sound_files_8khz
Source: http://files.freeswitch.org/freeswitch-sounds-music-16000-1.0.8.tar.gz; DestDir: {tmp}; DestName: freeswitch-sounds-music-16000-1.0.8.tar.gz; Components: sound_files_16khz
Source: http://files.freeswitch.org/freeswitch-sounds-music-32000-1.0.8.tar.gz; DestDir: {tmp}; DestName: freeswitch-sounds-music-32000-1.0.8.tar.gz; Components: sound_files_32khz
Source: http://files.freeswitch.org/freeswitch-sounds-music-48000-1.0.8.tar.gz; DestDir: {tmp}; DestName: freeswitch-sounds-music-48000-1.0.8.tar.gz; Components: sound_files_48khz

Source: http://files.freeswitch.org/downloads/win32/7za.exe; DestDir: {tmp}; DestName: 7za.exe; Components: sound_files_8khz sound_files_16khz sound_files_32khz sound_files_48khz
[Components]
Name: FreeSWITCH; Description: FreeSWITCH core components; Flags: fixed; Types: custom compact full; Languages: 
Name: FreePBX_WAMP; Description: FreePBX interface along with Apache, Mysql and PHP; Types: custom full
Name: fsgui; Description: fsgui (Joao Mesquita's QT based console); Types: custom full
Name: sound_files_8khz; Description: sound files 8khz (G711); ExtraDiskSpaceRequired: 21000000; Types: custom compact full; Languages: 
Name: sound_files_16khz; Description: sound files 16khz (G722); ExtraDiskSpaceRequired: 42000000; Types: custom full; Languages: 
Name: sound_files_32khz; Description: sound files 32khz (CELT); ExtraDiskSpaceRequired: 82000000; Types: custom full; Languages: 
Name: sound_files_48khz; Description: sound files 48khz (CELT); ExtraDiskSpaceRequired: 119000000; Types: custom full; Languages: 
[Code]
//variables globales
var phpVersion: String;
var apacheVersion: String;
var path: String;
var pathWithSlashes: String;
var page: TInputQueryWizardPage;
var email: String;
var smtp: String;

//-----------------------------------------------

procedure expand_sound();
var execFile: String;
var myResult: Integer;
var temppath: String;

begin
temppath := ExpandConstant('{tmp}');
execFile := temppath+'\7za.exe';
Exec(execFile, '',temppath+'\', SW_HIDE, ewWaitUntilTerminated, myResult);
end;

procedure fix_freepbx_paths();
var srcFile: String;
var destFile: String;
var srcContents: String;
var fspath: String;
var fspathWithSlashes: String;

begin
fspath := ExpandConstant('{app}');
fspathWithSlashes := fspath;
StringChange (fspathWithSlashes, '\','/');

destFile := fspathWithSlashes+'/wamp/www/freepbx-v3/modules/freeswitch/config/freeswitch.php';
//destFile := fspathWithSlashes+'/wamp/www/freepbx-v3/freepbx/config/telephony.php';
//srcFile := fspathWithSlashes+'/wamp/www/freepbx-v3/freepbx/config/telephony.php.install';

//if not FileExists (destFile) then
//begin

  LoadStringFromFile (destFile, srcContents);

  StringChange (srcContents, '/opt/freeswitch', fspathWithSlashes);
  StringChange (srcContents, '/usr/local/freeswitch', fspathWithSlashes);

  SaveStringToFile(destFile,srcContents, False);
//end

//DeleteFile(SrcFile);

end;

//-----------------------------------------------

//procedure qui ferme les eteind les services et ferme wampmanager.exe (si ils existent)

procedure close();
var batFile: String;
var myResult: Integer;

begin
path := ExpandConstant('{app}') + '\wamp';
batFile := path+'\closewamp.bat';
Exec(batFile, '',path+'\', SW_HIDE, ewWaitUntilTerminated, myResult);


end;

//----------------------------------------------

//procedure qui adapte WAMP aux choix effectu�s lors de l'installation

procedure install_pbx();
var srcFile: String;
var destFile: String;
var srcContents: String;
var browser: String;
var winPath: String;
var mysqlVersion: String;
var wampserverVersion: String;
var phpmyadminVersion: String;
//var sqlitemanagerVersion: String;
var tmp: String;
var phpDllCopy: String;


begin

//version des applis, � modifier pour chaque version de WampServer 2
apacheVersion := '2.2.11';
phpVersion := '5.3.0' ;
mysqlVersion := '5.1.36';
wampserverVersion := '2.0';
phpmyadminVersion := '3.2.0.1';
//sqlitemanagerVersion := '1.2.0';


path := ExpandConstant('{app}') + '\wamp';
winPath := ExpandConstant('{win}');

pathWithSlashes := path;
StringChange (pathWithSlashes, '\','/');

//----------------------------------------------
// renommage du fichier c:/windows/php.ini
//----------------------------------------------

if FileExists ('c:/windows/php.ini') then
begin
  if MsgBox('A previous php.ini file has been detected in your Windows directory. Do you want WampServer2 to rename it to php_old.ini to avoid conflicts?',mbConfirmation,MB_YESNO) = IDYES then
  begin
    RenameFile('c:/windows/php.ini','c:/windows/php_old.ini');
  end
end




//----------------------------------------------
// Fichier install_services.bat
//----------------------------------------------

destFile := pathWithSlashes+'/install_services.bat';
srcFile := pathWithSlashes+'/install_services.bat.install';

if not FileExists (pathWithSlashes+'/wampmanager.conf') then
begin

  LoadStringFromFile (srcFile, srcContents);

  //version de apache et mysql
  StringChange (srcContents, 'WAMPMYSQLVERSION', mysqlVersion);
  StringChange (srcContents, 'WAMPAPACHEVERSION', apacheVersion);

  SaveStringToFile(destFile,srcContents, False);
end
else
begin
  //dans le cas d'une upgrade on d�truit le fichier pour qu'il ne soit pas execut�
  DeleteFile(destFile);
end
DeleteFile(SrcFile);








//----------------------------------------------
// Fichier install_services_auto.bat
//----------------------------------------------

destFile := pathWithSlashes+'/install_services_auto.bat';
srcFile := pathWithSlashes+'/install_services_auto.bat.install';

if not FileExists (pathWithSlashes+'/wampmanager.conf') then
begin

  LoadStringFromFile (srcFile, srcContents);

  //version de apache et mysql
  StringChange (srcContents, 'WAMPMYSQLVERSION', mysqlVersion);
  StringChange (srcContents, 'WAMPAPACHEVERSION', apacheVersion);

  SaveStringToFile(destFile,srcContents, False);
end
else
begin
  //dans le cas d'une upgrade on d�truit le fichier pour qu'il ne soit pas execut�
  DeleteFile(destFile);
end
DeleteFile(SrcFile);







//----------------------------------------------
// Fichier uninstall_services_auto.bat
//----------------------------------------------

destFile := pathWithSlashes+'/uninstall_services.bat';
srcFile := pathWithSlashes+'/uninstall_services.bat.install';

if not FileExists (pathWithSlashes+'/wampmanager.conf') then
begin

  LoadStringFromFile (srcFile, srcContents);

  //version de apache et mysql
  StringChange (srcContents, 'WAMPMYSQLVERSION', mysqlVersion);
  StringChange (srcContents, 'WAMPAPACHEVERSION', apacheVersion);

  SaveStringToFile(destFile,srcContents, False);
end
else
begin
  //dans le cas d'une upgrade on d�truit le fichier pour qu'il ne soit pas execut�
  DeleteFile(destFile);
end
DeleteFile(SrcFile);




//----------------------------------------------
// Fichier wampmanager.conf
//----------------------------------------------

destFile := pathWithSlashes+'/wampmanager.conf';
srcFile := pathWithSlashes+'/wampmanager.conf.install';

if not FileExists (destFile) then
begin

  LoadStringFromFile (srcFile, srcContents);


  //installDir et versions
  StringChange (srcContents, 'WAMPROOT', pathWithSlashes);
  StringChange (srcContents, 'WAMPSERVERVERSION', wampserverVersion);
  StringChange (srcContents, 'WAMPPHPVERSION', phpVersion);
  StringChange (srcContents, 'WAMPMYSQLVERSION', mysqlVersion);
  StringChange (srcContents, 'WAMPAPACHEVERSION', apacheVersion);
  StringChange (srcContents, 'WAMPPHPMYADMINVERSION', phpmyadminVersion);
//  StringChange (srcContents, 'WAMPSQLITEMANAGERVERSION', sqlitemanagerVersion);



  //navigateur
  browser := 'explorer.exe';
  if FileExists ('C:/Program Files/Mozilla Firefox/firefox.exe')  then
  begin
    if MsgBox('Firefox has been detected on your computer. Would you like to use it as the default browser with WampServer2?',mbConfirmation,MB_YESNO) = IDYES then
    begin
      browser := 'C:/Program Files/Mozilla Firefox/firefox.exe';
    end
  end
  if browser = 'explorer.exe' then
  begin
    GetOpenFileName('Please choose your default browser. If you are not sure, just click Open :', browser, winPath,'exe files (*.exe)|*.exe|All files (*.*)|*.*' ,'exe');
  end
  StringChange (srcContents, 'WAMPBROWSER', browser);
  SaveStringToFile(destFile,srcContents, False);
end

else
begin
// changement de wampserverVersion et WampserverVersion pour les upgrades
end

DeleteFile(SrcFile);









//----------------------------------------------
// Fichier wampmanager.ini
//----------------------------------------------

destFile := pathWithSlashes+'/wampmanager.ini';
srcFile := pathWithSlashes+'/wampmanager.ini.install';

if not FileExists (destFile) then
begin

  LoadStringFromFile (srcFile, srcContents);


  //installDir et versions
  StringChange (srcContents, 'WAMPROOT', pathWithSlashes);
  StringChange (srcContents, 'WAMPPHPVERSION', phpVersion);
  SaveStringToFile(destFile,srcContents, False);
end

DeleteFile(SrcFile);



//----------------------------------------------
// Fichier wampmanager.tpl
//----------------------------------------------

destFile := pathWithSlashes+'/wampmanager.tpl';
srcFile := pathWithSlashes+'/wampmanager.tpl.install';

if not FileExists (destFile) then
begin

  LoadStringFromFile (srcFile, srcContents);
  SaveStringToFile(destFile,srcContents, False);
end

DeleteFile(SrcFile);









//----------------------------------------------
// Fichier alias phpmyadmin
//----------------------------------------------

destFile := pathWithSlashes+'/alias/phpmyadmin.conf';
srcFile := pathWithSlashes+'/alias/phpmyadmin.conf.install';

if not FileExists (destFile) then
begin

  LoadStringFromFile (srcFile, srcContents);

  //installDir et version de phpmyadmin
  StringChange (srcContents, 'WAMPROOT', pathWithSlashes);
  StringChange (srcContents, 'WAMPPHPMYADMINVERSION', phpmyadminVersion);

  SaveStringToFile(destFile,srcContents, False);
end
else
begin
  LoadStringFromFile (srcFile, srcContents);

  //installDir et version de phpmyadmin
  StringChange (srcContents, 'WAMPROOT', pathWithSlashes);
  StringChange (srcContents, 'WAMPPHPMYADMINVERSION', phpmyadminVersion);

  SaveStringToFile(destFile,srcContents, False);


  //mise � jour de la version de phpmyadmin
  tmp := GetIniString('apps','phpmyadminVersion',phpmyadminVersion,pathWithSlashes+'/wampmanager.conf');
  if not CompareText(tmp,phpmyadminVersion) = 0  then
  begin
    SetIniString('apps','phpmyadminVersion',phpmyadminVersion,pathWithSlashes+'/wampmanager.conf');
    LoadStringFromFile (destFile, srcContents);
    StringChange (srcContents, tmp, phpmyadminVersion);
    SaveStringToFile(destFile,srcContents, False);
  end
end

DeleteFile(SrcFile);









//----------------------------------------------
// Fichier de configuration de phpmyadmin
//----------------------------------------------

destFile := pathWithSlashes+'/apps/phpmyadmin'+phpmyadminVersion+'/config.inc.php';
srcFile := pathWithSlashes+'/apps/phpmyadmin'+phpmyadminVersion+'/config.inc.php.install';

if not FileExists (destFile) then
begin

// si un fichier existe pour une version precedente de phpmyadmin, on le recupere
  if FileExists (pathWithSlashes+'/apps/phpmyadmin'+tmp+'/config.inc.php') then
  begin
    LoadStringFromFile (pathWithSlashes+'/apps/phpmyadmin'+tmp+'/config.inc.php', srcContents);
    SaveStringToFile(destFile,srcContents, False);
  end
  else
  begin

// sinon on prends le fichier par defaut
    LoadStringFromFile (srcFile, srcContents);
    SaveStringToFile(destFile,srcContents, False);
  end
end

DeleteFile(SrcFile);









//----------------------------------------------
// Fichier httpd.conf
//----------------------------------------------

destFile := pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/conf/httpd.conf';
srcFile := pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/conf/httpd.conf.install';

if not FileExists (destFile) then
begin

  LoadStringFromFile (srcFile, srcContents);

  //installDir et version de php
  StringChange (srcContents, 'WAMPROOT', pathWithSlashes);
  StringChange (srcContents, 'WAMPPHPVERSION', phpVersion);

  SaveStringToFile(destFile,srcContents, False);
end
DeleteFile(SrcFile);




//----------------------------------------------
// Fichier my.ini
//----------------------------------------------

destFile := pathWithSlashes+'/bin/mysql/mysql'+mysqlVersion+'/my.ini';
srcFile := pathWithSlashes+'/bin/mysql/mysql'+mysqlVersion+'/my.ini.install';

if not FileExists (destFile) then
begin

  LoadStringFromFile (srcFile, srcContents);

  //installDir et version de php
  StringChange (srcContents, 'WAMPROOT', pathWithSlashes);

  SaveStringToFile(destFile,srcContents, False);
end
DeleteFile(SrcFile);











//----------------------------------------------
// Fichier index.php
//----------------------------------------------

destFile := pathWithSlashes+'/www/index.php';
srcFile := pathWithSlashes+'/www/index.php.install';

if not FileExists (destFile) then
begin
  LoadStringFromFile (srcFile, srcContents);
  SaveStringToFile(destFile,srcContents, False);
end
else
begin
  if MsgBox('Would you like to install the new WampServer 2 homepage? (this will overwrite exisiting index.php file)',mbConfirmation,MB_YESNO) = IDYES then
  begin
    DeleteFile (destFile);
    LoadStringFromFile (srcFile, srcContents);
    SaveStringToFile(destFile,srcContents, False);
  end
end
DeleteFile(SrcFile);





//----------------------------------------------
// Fichier php.ini
//----------------------------------------------
destFile := pathWithSlashes+'/bin/php/php'+phpVersion+'/php.ini';

if not FileExists (destFile) then
begin
   page := CreateInputQueryPage(wpInstalling,
   'PHP mail parameters', '',
   'Please specify the SMTP server and the adresse mail to be used by PHP when using the function mail(). If you are not sure, just leave the default values.');

   page.Add('SMTP:', False);
   page.Add('Email:', False);

   // Valeurs par defaut
   page.Values[0] := 'localhost';
   page.Values[1] := 'you@yourdomain';
   SaveStringToFile(pathWithSlashes+'/mailtag','tag', False);
end



//----------------------------------------------
// copie des dll de php vers apache
//----------------------------------------------

phpDllCopy := 'fdftk.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);
phpDllCopy := 'fribidi.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);
phpDllCopy := 'gds32.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);
phpDllCopy := 'libeay32.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);
phpDllCopy := 'libmhash.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);
phpDllCopy := 'libmysql.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);
phpDllCopy := 'msql.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);
phpDllCopy := 'libmysqli.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);
phpDllCopy := 'ntwdblib.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);
phpDllCopy := 'php5activescript.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);
phpDllCopy := 'php5isapi.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);
phpDllCopy := 'php5nsapi.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);
phpDllCopy := 'ssleay32.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);
phpDllCopy := 'yaz.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);
phpDllCopy := 'libmcrypt.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);
phpDllCopy := 'php5ts.dll';
filecopy (pathWithSlashes+'/bin/php/php'+phpVersion+'/'+phpDllCopy, pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/'+phpDllCopy, False);

end;




//-----------------------------------------------------------
// gestion des fichiers php.ini

procedure CurPageChanged(CurPageID: Integer);
var destFile: String;
var srcFile: String;
var srcContents: String;
begin
  if CurPageID = 14 then
  begin
    if FileExists (pathWithSlashes+'/mailtag') then
    begin
      smtp := page.Values[0];
      email := page.Values[1];
      DeleteFile(pathWithSlashes+'/mailtag');
    end
    destFile := pathWithSlashes+'/bin/php/php'+phpVersion+'/php.ini';
    srcFile := pathWithSlashes+'/bin/php/php'+phpVersion+'/php.ini.install';
    LoadStringFromFile (srcFile, srcContents);
    StringChange (srcContents, 'WAMPROOT', pathWithSlashes);
    StringChange (srcContents, 'WAMPSMTP', smtp);
    StringChange (srcContents, 'WAMPEMAIL', email);

//----------------------------------------------
// fichier php.ini dans php
//----------------------------------------------



    if not FileExists (destFile) then
    begin
      SaveStringToFile(destFile,srcContents, False);
    end









//----------------------------------------------
// fichier phpForApache.ini dans php
//----------------------------------------------



    destFile := pathWithSlashes+'/bin/php/php'+phpVersion+'/phpForApache.ini';
    if not FileExists (destFile) then
    begin
      SaveStringToFile(destFile,srcContents, False);
    end










//----------------------------------------------
// fichier php.ini dans apache
//----------------------------------------------



    destFile := pathWithSlashes+'/bin/apache/apache'+apacheVersion+'/bin/php.ini';
    if not FileExists (destFile) then
    begin
      SaveStringToFile(destFile,srcContents, False);
    end

  end

  DeleteFile(SrcFile);






//MsgBox(tmp,mbConfirmation,MB_YESNO);
end;





//-----------------------------------------------

//procedure lanc�e � la fin de l'installation, elle supprime les fichiers d'installation

procedure DeinitializeSetup();

begin
  DeleteFile(path+'\install_services.bat');
  DeleteFile(path+'\install_services_auto.bat');
  DeleteFile(path+'\expsound.bat');
  DeleteFile(path+'..\create_freepbx.bat');
end;



//-----------------------------------------------

//procedure lanc�e au d�but de l'installation, elle alerte sur les upgrades de WampServer

//function InitializeSetup(): Boolean;
//begin
//  Result := MsgBox('Important Information:' #13#13 'Please do not try to upgrade from WAMP5 1.x.' #13 ' If you have WAMP5 1.x installed, save your data, uninstall WAMP5 ' #13 'and delete the wamp folder before installing this new release. ' #13#13 'Do you want to continue install?', mbConfirmation, MB_YESNO) = idYes;
//end;
// Function generated by ISTool.
function NextButtonClick(CurPage: Integer): Boolean;
begin
	Result := istool_download(CurPage);
end;
