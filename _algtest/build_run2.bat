@echo off
rem 用法: build_run2.bat <源文件名(不含扩展名)>   (带 OpenCV)
call "D:\SoftWare\vs2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "d:\GitHub\ImageProcessingPlatform\_algtest"
set SRC=%1
cl /nologo /O2 /std:c++17 /EHsc /MD /utf-8 /D NOMINMAX ^
  /I D:\3rd\HammingElectronic\include ^
  /I D:\3rd\opencv_4_12_0\include ^
  %SRC%.cpp ^
  /Fe%SRC%.exe /Fo%SRC%.obj ^
  /link /LIBPATH:D:\3rd\HammingElectronic\lib ImgProcCplusplus.lib /LIBPATH:D:\3rd\opencv_4_12_0\lib opencv_world4120.lib
if errorlevel 1 exit /b 1
%SRC%.exe > %SRC%.log 2>&1
echo ---- exit=%errorlevel% log=%SRC%.log ----
