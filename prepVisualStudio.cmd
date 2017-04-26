@echo off & setlocal enableextensions enabledelayedexpansion

echo "%~1" | findstr /c:"Visual Studio" >nul 
if not %errorlevel% == 0  goto args_count_wrong 
goto args_count_ok

:args_count_wrong
echo Usage %0 "visual studio cmake generator" [project dir] [build dir]  
echo The available Visual Studio cmake generators templates are : 
cmake --help | FINDSTR "Visual Optional"
exit /b 1

:args_count_ok

set _FB_GEN="%~1"

set cur_dir=%~d0%~p0
set RESTVAR=
shift
:loop1
if "%~1"=="" goto after_loop
set RESTVAR=%RESTVAR% "%~1"
shift
goto loop1

:after_loop

call "%cur_dir%\common.cmd" %RESTVAR%
if %errorlevel% == 2 exit /b 1
call "%cur_dir%\winprep.cmd"


