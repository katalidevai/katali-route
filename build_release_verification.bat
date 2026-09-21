@echo off
setlocal
set "ROOT=%~dp0"
call "%ROOT%build_route.bat"
if errorlevel 1 exit /b 1
call "%ROOT%build_gguf_pool.bat"
if errorlevel 1 exit /b 1
call "%ROOT%build_laya_gguf_pool.bat"
if errorlevel 1 exit /b 1
set "SDK=%ROOT%..\..\katali-gguf"
set "PATH=%SDK%;%ROOT%build;%PATH%"
"%ROOT%build\benchmark-gguf-pool.exe" "%ROOT%config\route.conf.example" "%ROOT%build\benchmark-gguf-pool.jsonl"
if errorlevel 1 exit /b 1
echo release_verification=passed
endlocal
