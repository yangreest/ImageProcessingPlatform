@echo off
call "D:\SoftWare\vs2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "d:\GitHub\ImageProcessingPlatform\_algtest"
cl /nologo /O2 /std:c++17 /EHsc /MD /utf-8 ^
  /I D:\3rd\HammingElectronic\include ^
  hm_probe1.cpp ^
  /Fehm_probe.exe /Fohm_probe.obj ^
  /link /LIBPATH:D:\3rd\HammingElectronic\lib ImgProcCplusplus.lib
if errorlevel 1 exit /b 1
hm_probe.exe > hm_probe1.log 2>&1
type hm_probe1.log
