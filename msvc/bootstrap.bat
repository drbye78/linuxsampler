@echo off
SETLOCAL EnableDelayedExpansion
if "%1"=="" (SET arch=x64) ELSE (SET arch=%1)
set triplet=%arch%-windows-static

if exist %arch%\include goto quit
   echo Bootstraping dependencies for %arch% ...
   if not exist "vcpkg\vcpkg.exe" (
	   echo.
	   echo Bootstraping vcpkg ...
	   echo.
	   rmdir /s /q vcpkg >nul
	   git clone https://github.com/Microsoft/vcpkg.git 
	   call vcpkg\bootstrap-vcpkg.bat -disableMetrics
	)
	echo.
	echo Building dependencies ...
	echo.
	vcpkg\vcpkg install --triplet %triplet% sqlite3 libsndfile asiosdk pthread
	
	if not exist %arch% mkdir %arch%
	set src=vcpkg\installed\%triplet%\
	mklink /j %arch%\include %src%include
	mklink /j %arch%\lib %src%lib
:quit