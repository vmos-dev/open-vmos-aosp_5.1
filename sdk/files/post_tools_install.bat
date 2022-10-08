@echo off
rem Copyright (C) 2010 The Android Open Source Project
rem
rem Licensed under the Apache License, Version 2.0 (the "License");
rem you may not use this file except in compliance with the License.
rem You may obtain a copy of the License at
rem
rem      http://www.apache.org/licenses/LICENSE-2.0
rem
rem Unless required by applicable law or agreed to in writing, software
rem distributed under the License is distributed on an "AS IS" BASIS,
rem WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
rem See the License for the specific language governing permissions and
rem limitations under the License.

rem This script is called by the SDK Manager once a new version of the tools
rem package as been installed.

rem don't modify the caller's environment
setlocal

rem Set up prog to be the path of this script, including following symlinks,
rem and set up progdir to be the fully-qualified pathname of its directory.
set prog=%~f0

rem Grab current directory before we change it
set work_dir=%cd%

rem Change current directory and drive to where the script is, to avoid
rem issues with directories containing whitespaces.
cd /d %~dp0

:Step1
set src=SDK Manager.exe
set dst=..\..\%src%

if not exist "%src%" goto Step2
  echo Updating %src%
  copy /V /Y "%src%" "%dst%"

:Step2
set src=AVD Manager.exe
set dst=..\..\%src%

if not exist "%src%" goto Cleanup
  echo Updating %src%
  copy /V /Y "%src%" "%dst%"

:Cleanup
set old_dst=..\..\SDK Setup.exe
if not exist "%old_dst%" goto :EOF
  echo Removing obsolete %old_dst%
  del /F /Q "%old_dst%"
