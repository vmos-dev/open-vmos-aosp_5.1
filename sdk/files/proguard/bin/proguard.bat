@ECHO OFF

REM Start-up script for ProGuard -- free class file shrinker, optimizer,
REM obfuscator, and preverifier for Java bytecode.

rem Change current directory and drive to where the script is, to avoid
rem issues with directories containing whitespaces.
cd /d %~dp0

IF EXIST "%PROGUARD_HOME%" GOTO home
SET PROGUARD_HOME=..
:home

set java_exe=
call "%PROGUARD_HOME%"\..\lib\find_java.bat

call %java_exe% -jar "%PROGUARD_HOME%"\lib\proguard.jar %*
