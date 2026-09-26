@echo off
setlocal
set "APP_ROOT=%~dp0"
pushd "%APP_ROOT%"
if not exist "%APP_ROOT%bin\canvas-x.exe" (
  echo [ERROR] Missing canvas-x.exe in app\bin
  popd
  exit /b 1
)
call "%APP_ROOT%bin\canvas-x.exe"
set "EXIT_CODE=%ERRORLEVEL%"
popd
exit /b %EXIT_CODE%
