@echo off
setlocal

pushd %~dp0

call e:\udv\esp32\esp-idf-v4.4\export.bat

idf.py app
rem idf.py -p com3 flash

rem echo idf.py -p com3 monitor
rem echo idf.py -p com3 menuconfig

popd
