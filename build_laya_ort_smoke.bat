@echo off
setlocal
set "ROOT=%~dp0"
set "ORT=%ROOT%work\ort-package"
set "LAYA=%ROOT%work\laya-onnx"
if not exist "%ORT%\build\native\include\onnxruntime_c_api.h" (
  echo Missing work\ort-package native headers.
  exit /b 2
)
if not exist "%LAYA%\laya.onnx" (
  echo Missing work\laya-onnx\laya.onnx.
  exit /b 3
)
if not exist build mkdir build
copy /Y "%ORT%\runtimes\win-x64\native\onnxruntime.dll" build\onnxruntime.dll >nul
copy /Y "%ORT%\runtimes\win-x64\native\onnxruntime_providers_shared.dll" build\onnxruntime_providers_shared.dll >nul 2>nul
set "CC=C:\TDM-GCC-64\bin\gcc.exe"
%CC% -std=c11 -O2 -Wall -Wextra -Werror -Iinclude -I"%ORT%\build\native\include" tests\test_laya_onnx_c.c src\katali_route.c src\katali_route_tokenizer.c src\katali_route_laya_ort.c -o build\test-laya-onnx-c.exe -lm
if errorlevel 1 exit /b 1
build\test-laya-onnx-c.exe
endlocal
