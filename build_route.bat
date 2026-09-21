@echo off
setlocal
if not exist build mkdir build
set "CC=gcc"
where gcc >nul 2>&1 || set "CC=C:\TDM-GCC-64\bin\gcc.exe"
%CC% -std=c11 -O2 -Wall -Wextra -Werror -Iinclude src\katali_route.c src\katali_route_laya.c src\route_main.c -o build\katali-route.exe -lm
if errorlevel 1 exit /b 1
%CC% -std=c11 -O2 -Wall -Wextra -Werror -Iinclude src\katali_route.c src\katali_route_laya.c tests\test_route.c -o build\test-route.exe -lm
if errorlevel 1 exit /b 1
%CC% -std=c11 -O2 -Wall -Wextra -Werror -Iinclude src\katali_route_tokenizer.c tests\test_tokenizer.c -o build\test-tokenizer.exe -lm
if errorlevel 1 exit /b 1
%CC% -std=c11 -O2 -Wall -Wextra -Werror -Iinclude src\katali_route_config.c tests\test_route_config.c -o build\test-route-config.exe -lm
if errorlevel 1 exit /b 1
%CC% -std=c11 -O2 -Wall -Wextra -Werror -Iinclude src\katali_route.c src\katali_route_laya.c tests\benchmark_route.c -o build\benchmark-route.exe -lm
if errorlevel 1 exit /b 1
build\test-route.exe
build\test-tokenizer.exe
build\test-route-config.exe
build\katali-route.exe "Summarize this request"
build\benchmark-route.exe
