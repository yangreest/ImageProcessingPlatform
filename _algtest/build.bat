@echo off
call "D:\SoftWare\vs2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d "d:\GitHub\ImageProcessingPlatform"
cl /nologo /O2 /std:c++17 /EHsc /MD /utf-8 ^
  /I D:\3rd\opencv_4_12_0\include ^
  /I ImageProcessingPlatform\ImageProcessing ^
  _algtest\main_test.cpp ImageProcessingPlatform\ImageProcessing\ImageProcessByOpenCV.cpp ^
  /Fe_algtest\algtest.exe ^
  /link /LIBPATH:D:\3rd\opencv_4_12_0\lib opencv_world4120.lib
