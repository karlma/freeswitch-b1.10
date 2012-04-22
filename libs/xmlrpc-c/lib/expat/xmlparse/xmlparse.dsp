# Microsoft Developer Studio Project File - Name="xmlparse" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=xmlparse - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xmlparse.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xmlparse.mak" CFG="xmlparse - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xmlparse - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "xmlparse - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "xmlparse - Win32 Release DLL" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "xmlparse - Win32 Debug DLL" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "xmlparse - Win32 MinSize DLL" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "xmlparse"
# PROP Scc_LocalPath ".."

!IF  "$(CFG)" == "xmlparse - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release\xmlparse"
# PROP Intermediate_Dir "Release\xmlparse"
# PROP Target_Dir "."
LINK32=link.exe -lib
MTL=midl.exe
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\xmltok" /I "..\xmlwf" /I "..\..\.." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "XML_DTD" /D "_MBCS" /D "_LIB" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "xmlparse - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug\xmlparse"
# PROP Intermediate_Dir "Debug\xmlparse"
# PROP Target_Dir "."
LINK32=link.exe -lib
MTL=midl.exe
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\xmltok" /I "..\xmlwf" /I "..\..\.." /D "WIN32" /D "_WINDOWS" /D "XML_DTD" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "xmlparse - Win32 Release DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\ReleaseDLL"
# PROP BASE Intermediate_Dir ".\ReleaseDLL"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseDLL\xmlparse"
# PROP Intermediate_Dir "ReleaseDLL\xmlparse"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "."
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\xmltok" /I "..\xmlwf" /I "..\..\.." /D XMLTOKAPI=__declspec(dllimport) /D XMLPARSEAPI=__declspec(dllexport) /D "NDEBUG" /D "XML_NS" /D "WIN32" /D "_WINDOWS" /D "XML_DTD" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x20000000" /subsystem:windows /dll /machine:I386 /link50compat
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "xmlparse - Win32 Debug DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\DebugDLL"
# PROP BASE Intermediate_Dir ".\DebugDLL"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugDLL\xmlparse"
# PROP Intermediate_Dir "DebugDLL\xmlparse"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "."
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\xmltok" /I "..\xmlwf" /I "..\..\.." /D "_DEBUG" /D XMLTOKAPI=__declspec(dllimport) /D XMLPARSEAPI=__declspec(dllexport) /D "WIN32" /D "_WINDOWS" /D "XML_DTD" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x20000000" /subsystem:windows /dll /debug /machine:I386

!ELSEIF  "$(CFG)" == "xmlparse - Win32 MinSize DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\ReleaseMinSizeDLL"
# PROP BASE Intermediate_Dir ".\ReleaseMinSizeDLL"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinSizeDLL\xmlparse"
# PROP Intermediate_Dir "ReleaseMinSizeDLL\xmlparse"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\xmltok" /I "..\xmlwf" /D XMLTOKAPI=__declspec(dllimport) /D XMLPARSEAPI=__declspec(dllexport) /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "XML_NS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\xmltok" /I "..\xmlwf" /I "..\..\.." /D "XML_MIN_SIZE" /D "XML_WINLIB" /D XMLPARSEAPI=__declspec(dllexport) /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x20000000" /subsystem:windows /dll /machine:I386
# SUBTRACT BASE LINK32 /profile
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x20000000" /entry:"DllMain" /subsystem:windows /dll /machine:I386
# SUBTRACT LINK32 /profile /nodefaultlib

!ENDIF 

# Begin Target

# Name "xmlparse - Win32 Release"
# Name "xmlparse - Win32 Debug"
# Name "xmlparse - Win32 Release DLL"
# Name "xmlparse - Win32 Debug DLL"
# Name "xmlparse - Win32 MinSize DLL"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\xmltok\dllmain.c

!IF  "$(CFG)" == "xmlparse - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "xmlparse - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "xmlparse - Win32 Release DLL"

!ELSEIF  "$(CFG)" == "xmlparse - Win32 Debug DLL"

!ELSEIF  "$(CFG)" == "xmlparse - Win32 MinSize DLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xmlparse.c
# End Source File
# Begin Source File

SOURCE=..\xmltok\xmlrole.c

!IF  "$(CFG)" == "xmlparse - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "xmlparse - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "xmlparse - Win32 Release DLL"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "xmlparse - Win32 Debug DLL"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "xmlparse - Win32 MinSize DLL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\xmltok\xmltok.c

!IF  "$(CFG)" == "xmlparse - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "xmlparse - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "xmlparse - Win32 Release DLL"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "xmlparse - Win32 Debug DLL"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "xmlparse - Win32 MinSize DLL"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\xmlparse.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
