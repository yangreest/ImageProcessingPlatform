@echo off
rem 用法: build_run.bat <源文件名(不含扩展名)>
call "D:\SoftWare\vs2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "d:\GitHub\ImageProcessingPlatform\_algtest"
set SRC=%1
cl /nologo /O2 /std:c++17 /EHsc /MD /utf-8 /D NOMINMAX ^
  /I D:\3rd\HammingElectronic\include ^
  %SRC%.cpp ^
  /Fe%SRC%.exe /Fo%SRC%.obj ^
  /link /LIBPATH:D:\3rd\HammingElectronic\lib ImgProcCplusplus.lib
if errorlevel 1 exit /b 1
%SRC%.exe > %SRC%.log 2>&1
echo ---- exit=%errorlevel% log=%SRC%.log ----
