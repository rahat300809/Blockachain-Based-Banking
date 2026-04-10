@echo off
set PATH=C:\msys64\mingw64\bin;%PATH%
g++.exe logissue.cpp -o blockchain_app.exe -I"C:/msys64/mingw64/include" -L"C:/msys64/mingw64/lib" -lssl -lcrypto
if %errorlevel% equ 0 (
    echo Compilation successful!
) else (
    echo Compilation failed.
)
pause
