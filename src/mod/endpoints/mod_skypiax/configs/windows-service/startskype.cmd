echo off

REM
REM you MUST use the new Skype BETA (4.x) for Windows, older versions (3.x) cannot be started this way
REM
REM you have to adjust PATH to where the Skype executable is
set PATH=%PATH%;C:\Program Files\Skype\Phone\

REM echo %PATH%

REM start a Skype client instance that will login to the Skype network using the "username password" you give to it. 
REM Here xxxx would be the password and skypiax1 the username

start Skype.exe /secondary /username:skypiax20 /password:xxxx
call C:\wait.cmd 20

start Skype.exe /secondary /username:skypiax19 /password:xxxx
call C:\wait.cmd 5

REM 
REM Following Skype client instances are commented out
REM

start Skype.exe /secondary /username:skypiax18 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax17 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax16 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax15 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax14 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax13 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax12 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax11 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax10 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax9 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax8 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax7 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax6 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax5 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax4 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax3 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax2 /password:xxxx
call C:\wait.cmd 5
start Skype.exe /secondary /username:skypiax1 /password:xxxx
call C:\wait.cmd 120
NET start AICCU2
pause 
