'On Error Resume Next
Set WshShell = CreateObject("WScript.Shell")
Set FSO = CreateObject("Scripting.FileSystemObject")
Set WshSysEnv = WshShell.Environment("SYSTEM")
Set xml = CreateObject("Microsoft.XMLHTTP")
Set oStream = createobject("Adodb.Stream")
Set objArgs = WScript.Arguments
Dim vcver, DevEnv
BuildRelease=False
BuildDebug=False
BuildCore=False
BuildModExosip=false

ScriptDir=Left(WScript.ScriptFullName,Len(WScript.ScriptFullName)-Len(WScript.ScriptName))

LibDestDir=Showpath(ScriptDir & "..\..\libs")
UtilsDir=Showpath(ScriptDir & "Tools")
GetTarGZObjects UtilsDir
GetDevEnv
Wscript.echo "Detected Visual Studio DevEnv: " & DevEnv

If objArgs.Count >=2 Then
	Select Case objArgs(1)
		Case "Release"		
			BuildRelease=True
		Case "Debug"   
			BuildDebug=True
		Case "All"   
			BuildRelease=True
			BuildDebug=True
	End Select
End If

If objArgs.Count >=1 Then
	Select Case objArgs(0)
		Case "Core"		
			BuildCore=True
		Case "Mod_Exosip"   
			BuildModExosip=True
		Case Else
			BuildCore=True
			BuildModExosip=True
	End Select
Else
	BuildCore=True
	BuildModExosip=True
End If


If BuildCore Then
	BuildLibs_Core BuildDebug, BuildRelease
End If

If BuildModExosip Then
	BuildLibs_ModExosip BuildDebug, BuildRelease
End If

WScript.Echo "Complete"

Sub BuildLibs_Core(BuildDebug, BuildRelease)
	If Not FSO.FolderExists(LibDestDir & "apr") Then 
		WgetUnTarGz "ftp://ftp.wayne.edu/apache/apr/apr-1.2.2.tar.gz", LibDestDir
		If Not FSO.FolderExists(LibDestDir & "apr-1.2.2") Then
			Wscript.echo "Unable to get SQLite from default download location, Trying backup location:"
			WgetUnTarGz "http://www.sofaswitch.org/mikej/apr-1.2.2.tar.gz", LibDestDir
		End If
		RenameFolder LibDestDir & "apr-1.2.2", "apr"
		Unix2dos LibDestDir & "apr\libapr.dsp"
		Upgrade LibDestDir & "apr\libapr.dsp", LibDestDir & "apr\libapr.vcproj"
	End If 
	If FSO.FolderExists(LibDestDir & "apr") Then 
		If BuildDebug Then
			If Not FSO.FileExists(LibDestDir & "apr\Debug\libapr-1.lib") Then 
				BuildViaDevEnv LibDestDir & "apr\libapr.vcproj", "Debug"
			End If
		End If
		If BuildRelease Then
			If Not FSO.FileExists(LibDestDir & "apr\Release\libapr-1.lib") Then 
				BuildViaDevEnv LibDestDir & "apr\libapr.vcproj", "Release"
			End If
		End If
	Else
		Wscript.echo "Unable to download APR"
	End If 
	
	If Not FSO.FolderExists(LibDestDir & "sqlite") Then 
		WgetUnZip "http://www.sqlite.org/sqlite-source-3_2_7.zip", LibDestDir 
		If Not FSO.FolderExists(LibDestDir & "sqlite-source-3_2_7") Then
			Wscript.echo "Unable to get SQLite from default download location, Trying backup location:"
			WgetUnTarGz "http://www.sofaswitch.org/mikej/sqlite-source-3_2_7.zip", LibDestDir
		End If
		RenameFolder LibDestDir & "sqlite-source-3_2_7", "sqlite"
		FSO.CopyFile Utilsdir & "sqlite.vcproj", LibDestDir & "sqlite\", True
	'	Upgrade Utilsdir & "sqlite.vcproj", LibDestDir & "sqlite\sqlite.vcproj"
	End If
	If FSO.FolderExists(LibDestDir & "sqlite") Then 
		If BuildDebug Then
			If Not FSO.FileExists(LibDestDir & "sqlite\Debug\sqlite.lib") Then 
				UpgradeViaDevEnv LibDestDir & "sqlite\sqlite.vcproj"
				BuildViaDevEnv LibDestDir & "sqlite\sqlite.vcproj", "Debug"
			End If
		End If
		If BuildRelease Then
			If Not FSO.FileExists(LibDestDir & "sqlite\Release\sqlite.lib") Then 
				UpgradeViaDevEnv LibDestDir & "sqlite\sqlite.vcproj"
				BuildViaDevEnv LibDestDir & "sqlite\sqlite.vcproj", "Release"
			End If
		End If
	Else
		Wscript.echo "Unable to download SQLite"
	End If 
End Sub

Sub BuildLibs_ModExosip(BuildDebug, BuildRelease)

	If Not FSO.FolderExists(LibDestDir & "osip") Then
		WgetUnTarGz "http://www.antisip.com/download/libosip2-2.2.1.tar.gz", LibDestDir
		If Not FSO.FolderExists(LibDestDir & "libosip2-2.2.1") Then
			Wscript.echo "Unable to get osip from default download location, Trying backup location:"
			WgetUnTarGz "http://www.sofaswitch.org/mikej/libosip2-2.2.1.tar.gz", LibDestDir
		End If
		RenameFolder LibDestDir & "libosip2-2.2.1", "osip"
		FSO.CopyFile Utilsdir & "osipparser2.vcproj", LibDestDir & "osip\platform\vsnet\", True
	End If
	If FSO.FolderExists(LibDestDir & "osip") Then 
		If BuildDebug Then
			If Not FSO.FileExists(LibDestDir & "osip\platform\vsnet\Debug\osip2.lib") Then 
				UpgradeViaDevEnv LibDestDir & "osip\platform\vsnet\osip.sln"
				BuildViaDevEnv LibDestDir & "osip\platform\vsnet\osip.sln", "Debug"
			End If
		End If
		If BuildRelease Then
			If Not FSO.FileExists(LibDestDir & "osip\platform\vsnet\Release\osip2.lib") Then 
				UpgradeViaDevEnv LibDestDir & "osip\platform\vsnet\osip.sln"
				BuildViaDevEnv LibDestDir & "osip\platform\vsnet\osip.sln", "Release"
			End If
		End If
	Else
		Wscript.echo "Unable to download Osip"
	End If 

	If Not FSO.FolderExists(LibDestDir & "libeXosip2") Then 
		WgetUnTarGz "http://www.antisip.com/download/libeXosip2-1.9.1-pre17.tar.gz", LibDestDir
		If Not FSO.FolderExists(LibDestDir & "libeXosip2-1.9.1-pre17") Then
			Wscript.echo "Unable to get eXosip from default download location, Trying backup location:"
			WgetUnTarGz "http://www.sofaswitch.org/mikej/libeXosip2-1.9.1-pre17.tar.gz", LibDestDir
		End If
		RenameFolder LibDestDir & "libeXosip2-1.9.1-pre17", "libeXosip2"
		FSO.CopyFile Utilsdir & "eXosip.vcproj", LibDestDir & "libeXosip2\platform\vsnet\", True
	End If
	If FSO.FolderExists(LibDestDir & "libeXosip2") Then 
		If BuildDebug Then
			If Not FSO.FileExists(LibDestDir & "libeXosip2\platform\vsnet\Debug\exosip.lib") Then 
				UpgradeViaDevEnv LibDestDir & "libeXosip2\platform\vsnet\exosip.vcproj"
				BuildViaDevEnv LibDestDir & "libeXosip2\platform\vsnet\exosip.vcproj", "Debug"
			End If
		End If
		If BuildRelease Then
			If Not FSO.FileExists(LibDestDir & "libeXosip2\platform\vsnet\Release\exosip.lib") Then 
				UpgradeViaDevEnv LibDestDir & "libeXosip2\platform\vsnet\exosip.vcproj"
				BuildViaDevEnv LibDestDir & "libeXosip2\platform\vsnet\exosip.vcproj", "Release"
			End If
		End If
	Else
		Wscript.echo "Unable to download exosip"
	End If 

	If Not FSO.FolderExists(LibDestDir & "jthread-1.1.2") Then 
		WgetUnTarGz "http://research.edm.luc.ac.be/jori/jthread/jthread-1.1.2.tar.gz", LibDestDir
		If Not FSO.FolderExists(LibDestDir & "jthread-1.1.2") Then
			Wscript.echo "Unable to get JThread from default download location, Trying backup location:"
			WgetUnTarGz "http://www.sofaswitch.org/mikej/jthread-1.1.2.tar.gz", LibDestDir
		End If
	End If
	
	If Not FSO.FolderExists(LibDestDir & "jrtplib") Then 
		WgetUnTarGz "http://research.edm.luc.ac.be/jori/jrtplib/jrtplib-3.3.0.tar.gz", LibDestDir
		If Not FSO.FolderExists(LibDestDir & "jrtplib-3.3.0") Then
			Wscript.echo "Unable to get JRTPLib from default download location, Trying backup location:"
			WgetUnTarGz "http://www.sofaswitch.org/mikej/jrtplib-3.3.0.tar.gz", LibDestDir
		End If
		RenameFolder LibDestDir & "jrtplib-3.3.0", "jrtplib"
	End If
	If FSO.FolderExists(LibDestDir & "jrtplib") And FSO.FolderExists(LibDestDir & "jthread-1.1.2") And FSO.FolderExists(LibDestDir & "jrtp4c")Then 
		If BuildDebug Then
			If Not FSO.FileExists(LibDestDir & "jrtp4c\w32\Debug\jrtp4c.lib") Then 
				UpgradeViaDevEnv LibDestDir & "jrtp4c\w32\jrtp4c.sln"
				BuildViaDevEnv LibDestDir & "jrtp4c\w32\jrtp4c.sln", "Debug"
			End If
		End If
		If BuildRelease Then
			If Not FSO.FileExists(LibDestDir & "jrtp4c\w32\Release\jrtp4c.lib") Then 
				UpgradeViaDevEnv LibDestDir & "jrtp4c\w32\jrtp4c.sln"
				BuildViaDevEnv LibDestDir & "jrtp4c\w32\jrtp4c.sln", "Release"
			End If
		End If
	Else
		Wscript.echo "Unable to download JRtplib"
	End If 


End Sub

Sub UpgradeViaDevEnv(ProjectFile)
	Set oExec = WshShell.Exec(Chr(34) & DevEnv & Chr(34) & " " & Chr(34) & ProjectFile & Chr(34) & " /Upgrade ")
	Do While oExec.Status <> 1
	WScript.Sleep 100
	Loop
End Sub

Sub BuildViaDevEnv(ProjectFile, BuildType)
	Wscript.echo "Building : " & ProjectFile & " Config type: " & BuildType
	Set oExec = WshShell.Exec(Chr(34) & DevEnv & Chr(34) & " " & Chr(34) & ProjectFile & Chr(34) & " /Build " & BuildType)
	Do While oExec.Status <> 1
	WScript.Sleep 100
	Loop
End Sub

Sub GetDevEnv()
	If WshSysEnv("VS80COMNTOOLS")<> "" Then 
		vcver = "8"
		DevEnv=Showpath(WshSysEnv("VS80COMNTOOLS")&"..\IDE\") & "devenv"
	Else If WshSysEnv("VS71COMNTOOLS")<> "" Then
		vcver = "7"
		DevEnv=Showpath(WshSysEnv("VS71COMNTOOLS")&"..\IDE\") & "devenv"
	Else
		Wscript.Echo("Did not find any Visual Studio .net 2003 or 2005 on your machine")
		WScript.Quit(1)
	End If
	End If
End Sub


Sub RenameFolder(FolderName, NewFolderName)
On Error Resume Next
	Set Folder=FSO.GetFolder(FolderName)
	Folder.Name = NewFolderName
On Error GoTo 0
End Sub

Sub Upgrade(OldFileName, NewFileName)
On Error Resume Next
	If WshSysEnv("VS80COMNTOOLS")<> "" Then 
		Set vcProj = CreateObject("VisualStudio.VCProjectEngine.8.0")
	Else If WshSysEnv("VS71COMNTOOLS")<> "" Then
		Set vcProj = CreateObject("VisualStudio.VCProjectEngine.7.1")
	Else
		Wscript.Echo("Did not find any Visual Studio .net 2003 or 2005 on your machine")
		WScript.Quit(1)
	End If
	End If
	
	
'	WScript.Echo("Converting: "+ OldFileName)
	
	Set vcProject = vcProj.LoadProject(OldFileName)
	If Not FSO.FileExists(vcProject.ProjectFile) Then
		'   // specify name and location of new project file
		vcProject.ProjectFile = NewFileName
	'   // call the project engine to save this off. 
	'   // when no name is shown, it will create one with the .vcproj name
	vcProject.Save()
	End If
'	WScript.Echo("New Project Name: "+vcProject.ProjectFile+"")
On Error GoTo 0
End Sub


Sub Unix2dos(FileName)
	Const OpenAsASCII = 0  ' Opens the file as ASCII (TristateFalse) 
	Const OpenAsUnicode = -1  ' Opens the file as Unicode (TristateTrue) 
	Const OpenAsDefault = -2  ' Opens the file using the system default 
	
	Const OverwriteIfExist = -1 
	Const FailIfNotExist   =  0 
	Const ForReading       =  1 
	
	Set fOrgFile = FSO.OpenTextFile(FileName, ForReading, FailIfNotExist, OpenAsASCII)
	sText = fOrgFile.ReadAll
	fOrgFile.Close
	sText = Replace(sText, vbLf, vbCrLf)
	Set fNewFile = FSO.CreateTextFile(FileName, OverwriteIfExist, OpenAsASCII)
	fNewFile.WriteLine sText
	fNewFile.Close
End Sub
	
Sub WgetUnTarGZ(URL, DestFolder)
	If Right(DestFolder, 1) <> "\" Then DestFolder = DestFolder & "\" End If
	StartPos = InstrRev(URL, "/", -1, 1)   
	strlength = Len(URL)
	filename=Right(URL,strlength-StartPos)
	Wget URL, DestFolder
	UnTarGZ Destfolder & filename, DestFolder
End Sub

Sub WgetUnZip(URL, DestFolder)
	If Right(DestFolder, 1) <> "\" Then DestFolder = DestFolder & "\" End If
	StartPos = InstrRev(URL, "/", -1, 1) 
	strlength = Len(URL)
	filename=Right(URL,strlength-StartPos)
	NameEnd = InstrRev(filename, ".",-1, 1)
	filestrlength = Len(filename)
	filebase = Left(filename,NameEnd)
	Wget URL, DestFolder
	UnZip Destfolder & filename, DestFolder & filebase
End Sub

Sub GetTarGZObjects(DestFolder)
	Dim oExec

	If Right(DestFolder, 1) <> "\" Then DestFolder = DestFolder & "\" End If

	If Not FSO.FileExists(DestFolder & "XTar.dll") Then 
		Wget "http://www.sofaswitch.org/mikej/XTar.dll", DestFolder
	End If

	If Not FSO.FileExists(DestFolder & "XGZip.dll") Then 
		Wget "http://www.sofaswitch.org/mikej/XGZip.dll", DestFolder
	End If
	
	If Not FSO.FileExists(DestFolder & "XZip.dll") Then 
		Wget "http://www.sofaswitch.org/mikej/XZip.dll", DestFolder
	End If
	
	WshShell.Run "regsvr32 /s " & DestFolder & "XTar.dll", 6, True
	
	WshShell.Run "regsvr32 /s " & DestFolder & "XGZip.dll", 6, True

	WshShell.Run "regsvr32 /s " & DestFolder & "XZip.dll", 6, True
	
End Sub

Sub UnTarGZ(TGZfile, DestFolder)

	Set objTAR = WScript.CreateObject("XStandard.TAR")
	Set objGZip = WScript.CreateObject("XStandard.GZip")
	wscript.echo("Extracting: " & TGZfile)
	objGZip.Decompress TGZfile, Destfolder
	objTAR.UnPack Left(TGZfile, Len(TGZfile)-3), Destfolder
	
	Set objTAR = Nothing
	Set objGZip = Nothing
End Sub


Sub UnZip(Zipfile, DestFolder)
Dim objZip
Set objZip = WScript.CreateObject("XStandard.Zip")
objZip.UnPack Zipfile, DestFolder
'Set objZip = Nothing
End Sub


Sub Wget(URL, DestFolder)

	StartPos = InstrRev(URL, "/", -1, 1)   
	strlength = Len(URL)
	filename=Right(URL,strlength-StartPos)
	If Right(DestFolder, 1) <> "\" Then DestFolder = DestFolder & "\" End If

	Wscript.echo("Downloading: " & URL)
	xml.Open "GET", URL, False
	xml.Send
	
	Const adTypeBinary = 1
	Const adSaveCreateOverWrite = 2
	Const adSaveCreateNotExist = 1 
	
	oStream.type = adTypeBinary
	oStream.open
	oStream.write xml.responseBody
	
	' Do not overwrite an existing file
	'oStream.savetofile DestFolder & filename, adSaveCreateNotExist
	
	' Use this form to overwrite a file if it already exists
	 oStream.savetofile DestFolder & filename, adSaveCreateOverWrite
	
	oStream.close
	
End Sub


Function Showpath(folderspec)
	Set f = FSO.GetFolder(folderspec)
	showpath = f.path & "\"
End Function