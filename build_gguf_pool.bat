@echo off
setlocal
set "ROOT=%~dp0"
set "SDK=%ROOT%..\..\katali-gguf"
set "CC=C:\TDM-GCC-64\bin\gcc.exe"
if not exist build mkdir build
if not exist "%SDK%\katali_gguf.lib" exit /b 2
%CC% -std=c11 -O2 -Wall -Wextra -Werror -Iinclude -I"%SDK%\include" src\katali_route.c src\katali_route_config.c src\katali_route_gguf_sdk.c tests\test_route_gguf_pool.c -L"%SDK%" -lkatali_gguf -o build\test-route-gguf-pool.exe -lm
if errorlevel 1 exit /b 1
%CC% -std=c11 -O2 -Wall -Wextra -Werror -Iinclude -I"%SDK%\include" src\katali_route.c src\katali_route_config.c src\katali_route_gguf_sdk.c tests\test_route_gguf_lazy.c -L"%SDK%" -lkatali_gguf -o build\test-route-gguf-lazy.exe -lm
if errorlevel 1 exit /b 1
%CC% -std=c11 -O2 -Wall -Wextra -Werror -Iinclude -I"%SDK%\include" src\katali_route.c src\katali_route_config.c src\katali_route_gguf_sdk.c tests\benchmark_gguf_pool.c -L"%SDK%" -lkatali_gguf -o build\benchmark-gguf-pool.exe -lm
if errorlevel 1 exit /b 1
%CC% -std=c11 -O2 -Wall -Wextra -Werror -Iinclude -I"%SDK%\include" src\katali_route.c src\katali_route_config.c src\katali_route_gguf_sdk.c examples\route_gguf_pool.c -L"%SDK%" -lkatali_gguf -o build\route-gguf-pool-example.exe -lm
if errorlevel 1 exit /b 1
copy /Y "%SDK%\katali_gguf.dll" build\katali_gguf.dll >nul
set "PATH=%SDK%;%PATH%"
build\test-route-gguf-pool.exe
if errorlevel 1 exit /b 1
build\test-route-gguf-lazy.exe
if errorlevel 1 exit /b 1
build\route-gguf-pool-example.exe
endlocal
