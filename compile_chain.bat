@echo off
set PATH=C:\msys64\mingw64\bin;%PATH%
g++.exe chain_stack.cpp -o chain_stack.exe -I"C:/msys64/mingw64/include" -L"C:/msys64/mingw64/lib" -lssl -lcrypto > error.txt 2>&1
if %errorlevel% equ 0 (
    echo Compilation successful!
) else (
    echo Compilation failed. See error.txt
)
