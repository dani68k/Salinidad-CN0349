@echo off
setlocal
cd /d "%~dp0\..\.."
if not defined VSCMD_VER call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
if not exist .pio\host_cn0349 mkdir .pio\host_cn0349
cl /nologo /EHsc /std:c++14 /W3 /Itest\host_cn0349 /Iinclude /Ilib\AD5934 /Ilib\ADG715 /Fo.pio\host_cn0349\ /Fe.pio\host_cn0349\tests.exe test\host_cn0349\test_main.cpp src\cn0349\CN0349Manager.cpp src\cn0349\CN0349Impedance.cpp src\cn0349\CN0349Temperature.cpp src\cn0349\CN0349Ec.cpp src\impedance\ImpedanceApp.cpp src\mux\MuxApp.cpp src\terminal\Terminal.cpp lib\AD5934\AD5934.cpp lib\ADG715\ADG715.cpp
if errorlevel 1 exit /b 1
.pio\host_cn0349\tests.exe
exit /b %errorlevel%
