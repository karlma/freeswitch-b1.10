'
' Contributor(s):
' Michael Jerris <mike@jerris.com>
' David A. Horner http://dave.thehorners.com
'----------------------------------------------

'On Error Resume Next
' **************
' Initialization
' **************

Set WshShell = CreateObject("WScript.Shell")
Set FSO = CreateObject("Scripting.FileSystemObject")
Set WshSysEnv = WshShell.Environment("SYSTEM")
Set xml = CreateObject("Microsoft.XMLHTTP")
Dim UseWgetEXE

On Error Resume Next
Set oStream = CreateObject("Adodb.Stream")
On Error Goto 0

If Not IsObject(oStream)  Then
	wscript.echo("Failed to create Adodb.Stream, using alternative download method.")
	UseWgetEXE=true
Else
	UseWgetEXE=false
End If

Set objArgs = WScript.Arguments
quote=Chr(34)
ScriptDir=Left(WScript.ScriptFullName,Len(WScript.ScriptFullName)-Len(WScript.ScriptName))
UtilsDir=Showpath(ScriptDir)
ToolsBase="http://svn.freeswitch.org/downloads/win32/"

If UseWgetEXE Then
	GetWgetEXE UtilsDir
End If

GetCompressionTools UtilsDir


If objArgs.Count >=3 Then
	Select Case objArgs(0)
		Case "Get"		
			Wget objArgs(1), Showpath(objArgs(2))
		Case "GetUnzip"		
			WgetUnCompress objArgs(1), Showpath(objArgs(2))
		Case "Version"					
			'CreateVersion(tmpFolder, VersionDir, includebase, includedest)
			CreateVersion Showpath(objArgs(1)), Showpath(objArgs(2)), objArgs(3), objArgs(4)
	End Select
End If


' *******************
' Utility Subroutines
' *******************


Sub WgetUnCompress(URL, DestFolder)
	If Right(DestFolder, 1) <> "\" Then DestFolder = DestFolder & "\" End If
	StartPos = InstrRev(URL, "/", -1, 1) 
	strlength = Len(URL)
	filename=Right(URL,strlength-StartPos)
	NameEnd = InstrRev(filename, ".",-1, 1)
	filestrlength = Len(filename)
	filebase = Left(filename,NameEnd)
	fileext = Right(filename, Len(filename) - NameEnd)
	Wget URL, DestFolder
	If fileext = "zip" Then
		UnCompress Destfolder & filename, DestFolder & filebase
	Else
		UnCompress Destfolder & filename, DestFolder	
	End If
End Sub

Sub GetCompressionTools(DestFolder)
	Dim oExec
	If Right(DestFolder, 1) <> "\" Then DestFolder = DestFolder & "\" End If
	If Not FSO.FileExists(DestFolder & "7za.exe") Then 
		Wget ToolsBase & "7za.exe", DestFolder
	End If	
End Sub

Sub GetWgetEXE(DestFolder)
	Dim oExec
	If Right(DestFolder, 1) <> "\" Then DestFolder = DestFolder & "\" End If
	If Not FSO.FileExists(DestFolder & "wget.exe") Then 
		Slow_Wget ToolsBase & "wget.exe", DestFolder
	End If	
End Sub

Sub UnCompress(Archive, DestFolder)
	wscript.echo("Extracting: " & Archive)
	Set MyFile = fso.CreateTextFile(UtilsDir & "tmpcmd.Bat", True)
	MyFile.WriteLine("@" & quote & UtilsDir & "7za.exe" & quote & " x " & quote & Archive & quote & " -y -o" & quote & DestFolder & quote )
	MyFile.Close
	Set oExec = WshShell.Exec(UtilsDir & "tmpcmd.Bat")
	Do
		WScript.Echo OExec.StdOut.ReadLine()
	Loop While Not OExec.StdOut.atEndOfStream
	If FSO.FileExists(Left(Archive, Len(Archive)-3))Then  
		Set MyFile = fso.CreateTextFile(UtilsDir & "tmpcmd.Bat", True)
		MyFile.WriteLine("@" & quote & UtilsDir & "7za.exe" & quote & " x " & quote & Left(Archive, Len(Archive)-3) & quote & " -y -o" & quote & DestFolder & quote )
		MyFile.Close
		Set oExec = WshShell.Exec(UtilsDir & "tmpcmd.Bat")
		Do
			WScript.Echo OExec.StdOut.ReadLine()
		Loop While Not OExec.StdOut.atEndOfStream
		WScript.Sleep(500)
		FSO.DeleteFile Left(Archive, Len(Archive)-3) ,true 
	End If
	If FSO.FileExists(Left(Archive, Len(Archive)-3) & "tar")Then  
		Set MyFile = fso.CreateTextFile(UtilsDir & "tmpcmd.Bat", True)
		MyFile.WriteLine("@" & quote & UtilsDir & "7za.exe" & quote & " x " & quote & Left(Archive, Len(Archive)-3) & "tar" & quote & " -y -o" & quote & DestFolder & quote )
		MyFile.Close
		Set oExec = WshShell.Exec(UtilsDir & "tmpcmd.Bat")
		Do
			WScript.Echo OExec.StdOut.ReadLine()
		Loop While Not OExec.StdOut.atEndOfStream
		WScript.Sleep(500)
		FSO.DeleteFile Left(Archive, Len(Archive)-3) & "tar",true 
	End If
	
	WScript.Sleep(500)
End Sub

Sub Wget(URL, DestFolder)
	StartPos = InstrRev(URL, "/", -1, 1)   
	strlength = Len(URL)
	filename=Right(URL,strlength-StartPos)
	If Right(DestFolder, 1) <> "\" Then DestFolder = DestFolder & "\" End If

	Wscript.echo("Downloading: " & URL)
	
If UseWgetEXE Then
	Set MyFile = fso.CreateTextFile(UtilsDir & "tmpcmd.Bat", True)
	MyFile.WriteLine("@cd " & quote & DestFolder & quote)
	MyFile.WriteLine("@" & quote & UtilsDir & "wget.exe" & quote & " " & URL)
	MyFile.Close
	Set oExec = WshShell.Exec(UtilsDir & "tmpcmd.Bat")
	Do
		WScript.Echo OExec.StdOut.ReadLine()
	Loop While Not OExec.StdOut.atEndOfStream

Else
	xml.Open "GET", URL, False
	xml.Send
	
	Const adTypeBinary = 1
	Const adSaveCreateOverWrite = 2
	Const adSaveCreateNotExist = 1 

	oStream.type = adTypeBinary
	oStream.open
	oStream.write xml.responseBody
	oStream.savetofile DestFolder & filename, adSaveCreateOverWrite
	oStream.close
End If

End Sub

Sub Slow_Wget(URL, DestFolder)
	StartPos = InstrRev(URL, "/", -1, 1)   
	strlength = Len(URL)
	filename=Right(URL,strlength-StartPos)
	If Right(DestFolder, 1) <> "\" Then DestFolder = DestFolder & "\" End If

	Wscript.echo("Downloading: " & URL)
	xml.Open "GET", URL, False
	xml.Send
	
	const ForReading = 1 , ForWriting = 2 , ForAppending = 8 
Set MyFile = fso.OpenTextFile(DestFolder & filename ,ForWriting, True)
For i = 1 to lenb(xml.responseBody)
 MyFile.write Chr(Ascb(midb(xml.responseBody,i,1)))
Next
MyFile.Close()

End Sub

Function Showpath(folderspec)
	Set f = FSO.GetFolder(folderspec)
	showpath = f.path & "\"
End Function

Sub FindReplaceInFile(FileName, sFind, sReplace)
	Const OpenAsASCII = 0  ' Opens the file as ASCII (TristateFalse) 
	Const OpenAsUnicode = -1  ' Opens the file as Unicode (TristateTrue) 
	Const OpenAsDefault = -2  ' Opens the file using the system default 
	
	Const OverwriteIfExist = -1 
	Const FailIfNotExist   =  0 
	Const ForReading       =  1 
	
	Set fOrgFile = FSO.OpenTextFile(FileName, ForReading, FailIfNotExist, OpenAsASCII)
	sText = fOrgFile.ReadAll
	fOrgFile.Close
	sText = Replace(sText, sFind, sReplace)
	Set fNewFile = FSO.CreateTextFile(FileName, OverwriteIfExist, OpenAsASCII)
	fNewFile.WriteLine sText
	fNewFile.Close
End Sub

Sub CreateVersion(tmpFolder, VersionDir, includebase, includedest)
	Dim oExec
	If Right(tmpFolder, 1) <> "\" Then tmpFolder = tmpFolder & "\" End If
	If Not FSO.FileExists(tmpFolder & "svnversion.exe") Then 
		Wget ToolsBase & "svnversion.exe", tmpFolder
	End If	
	Dim sLastFile
	Const OverwriteIfExist = -1 
	Const ForReading       =  1 
	VersionCmd="svnversion " & quote & VersionDir & "." & quote &  " -n"
	Set MyFile = fso.CreateTextFile(tmpFolder & "tmpVersion.Bat", True)
	MyFile.WriteLine("@" & "cd " & quote & tmpFolder & quote )
	MyFile.WriteLine("@" & VersionCmd)
	MyFile.Close
	Set oExec = WshShell.Exec(tmpFolder & "tmpVersion.Bat")
	Do
		strFromProc = OExec.StdOut.ReadLine()
		VERSION=strFromProc
	Loop While Not OExec.StdOut.atEndOfStream
	sLastVersion = ""
	Set sLastFile = FSO.OpenTextFile(tmpFolder & "lastversion", ForReading, true, OpenAsASCII)
	If Not sLastFile.atEndOfStream Then
		sLastVersion = sLastFile.ReadLine()
	End If
	sLastFile.Close
	
	If VERSION = "" Then
		VERSION = "UNKNOWN"
	End If

	If VERSION <> sLastVersion Then
		Set MyFile = fso.CreateTextFile(tmpFolder & "lastversion", True)
		MyFile.WriteLine(VERSION)
		MyFile.Close
	
		FSO.CopyFile includebase, includedest, true
		FindReplaceInFile includedest, "@SVN_VERSION@", VERSION
	End If
	
End Sub
