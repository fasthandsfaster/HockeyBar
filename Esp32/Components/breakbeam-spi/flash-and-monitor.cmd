@echo off
setlocal

pushd %~dp0

call e:\udv\esp32\esp-idf-v4.4\export.bat

idf.py -p com3 flash monitor

popd
