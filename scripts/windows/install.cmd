@echo off
rem Runs install.ps1 with elevated privileges and without changing the execution policy
powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process powershell -Verb RunAs -ArgumentList '-NoExit -NoProfile -ExecutionPolicy Bypass -File \"%~dp0install.ps1\"'"
