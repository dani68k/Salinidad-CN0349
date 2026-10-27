@echo off
setlocal
cd /d "%~dp0\..\.."
if not defined VSCMD_VER call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
if not exist .pio\host_math mkdir .pio\host_math
set "UNITY_SRC=.pio\libdeps\native\Unity\src"
if not exist "%UNITY_SRC%\unity.c" exit /b 1
cl /nologo /EHsc /std:c++14 /DUNITY_INCLUDE_DOUBLE /Iinclude /I"%UNITY_SRC%" /Fo.pio\host_math\ /Fe.pio\host_math\pt100.exe test\test_pt100\test_main.cpp "%UNITY_SRC%\unity.c"
if errorlevel 1 exit /b 1
.pio\host_math\pt100.exe
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++14 /DUNITY_INCLUDE_DOUBLE /Iinclude /I"%UNITY_SRC%" /Fo.pio\host_math\ /Fe.pio\host_math\ad5934_math.exe test\test_ad5934_math\test_main.cpp test\host_cn0349\unity_hooks.c "%UNITY_SRC%\unity.c"
if errorlevel 1 exit /b 1
.pio\host_math\ad5934_math.exe
exit /b %errorlevel%
