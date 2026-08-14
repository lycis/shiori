@echo off
setlocal

git add .

git diff --cached --quiet
if %errorlevel%==0 (
    exit /b 0
)

git commit -m "Shiori: %SHIORI_COMMAND% %SHIORI_COMMAND_ARGS%"

if %errorlevel% neq 0 (
    exit /b %errorlevel%
)

git push

exit /b %errorlevel%