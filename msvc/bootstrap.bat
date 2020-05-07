@echo off
SETLOCAL EnableDelayedExpansion

if "%1"=="" (SET arch=x64) ELSE (SET arch=%1)

set triplet=%arch%-windows-static
set INC_DIR=%arch%\include
set LIB_DIR=%arch%\lib

if exist %INC_DIR% goto part2
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
mklink /j %INC_DIR% %src%include
mklink /j %LIB_DIR% %src%lib

:part2

if exist %INC_DIR%\portaudio.h goto quit
echo Bootstraping portaudio ..
SET PA_SRC=./%arch%/portaudio
SET PA_BLD=./%PA_SRC%/custombuild
SET PA_DEST=./%arch%

git clone https://git.assembla.com/portaudio.git %PA_SRC%
cmake -S %PA_SRC% -B %PA_BLD% -DCMAKE_INSTALL_PREFIX=%PA_DEST% -DPA_BUILD_EXAMPLES=OFF -DPA_BUILD_SHARED=OFF -DPA_BUILD_STATIC=ON -DPA_BUILD_TESTS=OFF -DPA_USE_ASIO=OFF -DPA_USE_DS=OFF -DPA_DISABLE_INSTALL=OFF
cmake --build %PA_BLD% --config release
cmake --install %PA_BLD% --config release

:quit