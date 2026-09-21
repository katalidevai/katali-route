@echo off
setlocal
set "ROOT=%~dp0"
set "ORT=%ROOT%work\ort-package"
set "TOK=%ROOT%work\tokenizers-cpp"
set "TOKLIB=%TOK%\rust\target\x86_64-pc-windows-gnu\release\libtokenizers_c.a"
if not exist "%ORT%\build\native\include\onnxruntime_c_api.h" exit /b 2
if not exist "%ROOT%work\laya-onnx\laya.onnx" exit /b 3
if not exist "%TOKLIB%" exit /b 4
if not exist build mkdir build
copy /Y "%ORT%\runtimes\win-x64\native\onnxruntime.dll" build\onnxruntime.dll >nul
set "CC=C:\TDM-GCC-64\bin\gcc.exe"
%CC% -std=c11 -O2 -Wall -Wextra -Werror -Iinclude -I"%ORT%\build\native\include" -I"%TOK%\include" tests\test_laya_end_to_end_c.c src\katali_route.c src\katali_route_tokenizer.c src\katali_route_laya_tokenizer.c src\katali_route_laya_ort.c "%TOKLIB%" -o build\test-laya-e2e.exe -lm -lws2_32 -luserenv -lbcrypt -ladvapi32 -lntdll -lmswsock 2>build\laya-tokenizer-build.err
if errorlevel 1 exit /b 1
build\test-laya-e2e.exe
endlocal
