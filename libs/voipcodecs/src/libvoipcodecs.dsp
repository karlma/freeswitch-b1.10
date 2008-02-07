# Microsoft Developer Studio Project File - Name="voipcodecs" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=voipcodecs - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "voipcodecs.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "voipcodecs.mak" CFG="voipcodecs - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "voipcodecs - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "voipcodecs - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "voipcodecs - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D HAVE_TGMATH_H /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /I "." /I "..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D HAVE_TGMATH_H /D "_WINDLL" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib ws2_32.lib winmm.lib /nologo /dll /map /debug /machine:I386 /out:"Release/libvoipcodecs.dll"

!ELSEIF  "$(CFG)" == "voipcodecs - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D HAVE_TGMATH_H /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "." /I "..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D HAVE_TGMATH_H /FR /FD /GZ /c
# SUBTRACT CPP /WX /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib ws2_32.lib winmm.lib /nologo /dll /incremental:no /map /debug /machine:I386 /out:"Debug/libvoipcodecs.dll" /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "voipcodecs - Win32 Release"
# Name "voipcodecs - Win32 Debug"
# Begin Group "Source Files"
# Begin Source File

SOURCE=.\bit_operations.c
# End Source File
# Begin Source File

SOURCE=.\bitstream.c
# End Source File
# Begin Source File

SOURCE=.\g711.c
# End Source File
# Begin Source File

SOURCE=.\g722_encode.c
# End Source File
# Begin Source File

SOURCE=.\g722_decode.c
# End Source File
# Begin Source File

SOURCE=.\g726.c
# End Source File
# Begin Source File

SOURCE=.\gsm0610_decode.c
# End Source File
# Begin Source File

SOURCE=.\gsm0610_encode.c
# End Source File
# Begin Source File

SOURCE=.\gsm0610_long_term.c
# End Source File
# Begin Source File

SOURCE=.\gsm0610_lpc.c
# End Source File
# Begin Source File

SOURCE=.\gsm0610_preprocess.c
# End Source File
# Begin Source File

SOURCE=.\gsm0610_rpe.c
# End Source File
# Begin Source File

SOURCE=.\gsm0610_short_term.c
# End Source File
# Begin Source File

SOURCE=.\ima_adpcm.c
# End Source File
# Begin Source File

SOURCE=.\lpc10_analyse.c
# End Source File
# Begin Source File

SOURCE=.\lpc10_decode.c
# End Source File
# Begin Source File

SOURCE=.\lpc10_encode.c
# End Source File
# Begin Source File

SOURCE=.\lpc10_placev.c
# End Source File
# Begin Source File

SOURCE=.\lpc10_voicing.c
# End Source File
# Begin Source File

SOURCE=.\oki_adpcm.c
# End Source File
# Begin Source File

SOURCE=.\vector_int.c
# End Source File
# Begin Source File

SOURCE=.\voipcodecs/bit_operations.h
# End Source File
# Begin Source File

SOURCE=.\voipcodecs/bitstream.h
# End Source File
# Begin Source File

SOURCE=.\voipcodecs/dc_restore.h
# End Source File
# Begin Source File

SOURCE=.\voipcodecs/g711.h
# End Source File
# Begin Source File

SOURCE=.\voipcodecs/g722.h
# End Source File
# Begin Source File

SOURCE=.\voipcodecs/g726.h
# End Source File
# Begin Source File

SOURCE=.\voipcodecs/gsm0610.h
# End Source File
# Begin Source File

SOURCE=.\voipcodecs/ima_adpcm.h
# End Source File
# Begin Source File

SOURCE=.\voipcodecs/lpc10.h
# End Source File
# Begin Source File

SOURCE=.\voipcodecs/oki_adpcm.h
# End Source File
# Begin Source File

SOURCE=.\voipcodecs/telephony.h
# End Source File
# Begin Source File

SOURCE=.\voipcodecs/vector_int.h
# End Source File
# Begin Source File

SOURCE=.\voipcodecs.h
# End Source File
# Begin Source File

SOURCE=.\voipcodecs.h
# End Source File
# End Group

# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
