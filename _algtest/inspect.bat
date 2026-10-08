@echo off
call "D:\SoftWare\vs2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
echo ================ EXPORTS ImgProcCplusplus.dll ================
dumpbin /exports "D:\3rd\HammingElectronic\bin\ImgProcCplusplus.dll"
echo ================ DEPENDENTS ImgProcCplusplus.dll ================
dumpbin /dependents "D:\3rd\HammingElectronic\bin\ImgProcCplusplus.dll"
echo ================ HEADERS(machine) ================
dumpbin /headers "D:\3rd\HammingElectronic\bin\ImgProcCplusplus.dll" | findstr /i "machine time date"
echo ================ LIB symbols ================
dumpbin /symbols "D:\3rd\HammingElectronic\lib\ImgProcCplusplus.lib" | findstr /i "External"
