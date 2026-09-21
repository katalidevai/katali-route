@echo off
setlocal
set "ROOT=%~dp0"
set "SDK=%ROOT%..\..\katali-gguf"
set "ORT=%ROOT%work\ort-package"
set "TOK=%ROOT%work\tokenizers-cpp"
set "TOKLIB=%TOK%\rust\target\x86_64-pc-windows-gnu\release\libtokenizers_c.a"
set "CC=C:\TDM-GCC-64\bin\gcc.exe"
if not exist build mkdir build
if not exist "%SDK%\katali_gguf.lib" exit /b 2
if not exist "%ORT%\build\native\include\onnxruntime_c_api.h" exit /b 3
if not exist "%ROOT%work\laya-onnx\laya.onnx" exit /b 4
if not exist "%TOKLIB%" exit /b 5
copy /Y "%ORT%\runtimes\win-x64\native\onnxruntime.dll" build\onnxruntime.dll >nul
%CC% -std=c11 -O2 -Wall -Wextra -Werror -Iinclude -I"%SDK%\include" -I"%ORT%\build\native\include" -I"%TOK%\include" tests\test_laya_end_to_end_c.c src\katali_route.c src\katali_route_config.c src\katali_route_tokenizer.c src\katali_route_laya_tokenizer.c src\katali_route_laya_ort.c src\katali_route_laya_host.c src\katali_route_gguf_sdk.c "%TOKLIB%" -L"%SDK%" -lkatali_gguf -o build\test-laya-gguf-e2e.exe -lm -lws2_32 -luserenv -lbcrypt -ladvapi32 -lntdll -lmswsock 2>build\laya-gguf-build.err
if errorlevel 1 exit /b 1
%CC% -std=c11 -O2 -Wall -Wextra -Werror -Iinclude -I"%SDK%\include" -I"%ORT%\build\native\include" -I"%TOK%\include" examples\laya_gguf_pool.c src\katali_route.c src\katali_route_config.c src\katali_route_tokenizer.c src\katali_route_laya_tokenizer.c src\katali_route_laya_ort.c src\katali_route_laya_host.c src\katali_route_gguf_sdk.c "%TOKLIB%" -L"%SDK%" -lkatali_gguf -o build\laya-gguf-pool.exe -lm -lws2_32 -luserenv -lbcrypt -ladvapi32 -lntdll -lmswsock 2>build\laya-gguf-example-build.err
if errorlevel 1 exit /b 1
set "PATH=%SDK%;build;%PATH%"
build\test-laya-gguf-e2e.exe
if errorlevel 1 exit /b 1
build\laya-gguf-pool.exe
endlocal
