@echo off
setlocal
set "ROOT=%~dp0"
set "OUT=%ROOT%outputs\katali-route-package"
if not exist "%OUT%" mkdir "%OUT%"
if not exist "%OUT%\config" mkdir "%OUT%\config"
if not exist "%OUT%\work\laya-onnx\tokenizer" mkdir "%OUT%\work\laya-onnx\tokenizer"
copy /Y "%ROOT%build\laya-gguf-pool.exe" "%OUT%\laya-gguf-pool.exe" >nul
copy /Y "%ROOT%build\katali_gguf.dll" "%OUT%\katali_gguf.dll" >nul
copy /Y "%ROOT%build\onnxruntime.dll" "%OUT%\onnxruntime.dll" >nul
copy /Y "%ROOT%build\onnxruntime_providers_shared.dll" "%OUT%\onnxruntime_providers_shared.dll" >nul
copy /Y "%ROOT%work\laya-onnx\laya.onnx" "%OUT%\work\laya-onnx\laya.onnx" >nul
copy /Y "%ROOT%work\laya-onnx\laya.onnx.data" "%OUT%\work\laya-onnx\laya.onnx.data" >nul
copy /Y "%ROOT%work\laya-onnx\tokenizer\tokenizer.json" "%OUT%\work\laya-onnx\tokenizer\tokenizer.json" >nul
copy /Y "%ROOT%config\route.conf.example" "%OUT%\config\route.conf.example" >nul
copy /Y "%ROOT%README.md" "%OUT%\README-project.md" >nul
copy /Y "%ROOT%docs\gguf-backend.md" "%OUT%\GGUF-backend.md" >nul
copy /Y "C:\Users\joanr\Desktop\elastic\src-tauri\icons\icon.ico" "%OUT%\icon.ico" >nul
copy /Y "C:\Users\joanr\Desktop\elastic\src-tauri\icons\icon_head_950.png" "%OUT%\icon_head_950.png" >nul
echo package_created=%OUT%
endlocal
