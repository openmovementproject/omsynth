@echo off
rem by Dan Jackson

if "%~1"=="" (
	echo WARNING: No input files specified
	goto end
)

:loop
if "%~1"=="" goto end
for %%f in ("%~1") do (
    call :process "%%~f"
)
shift
goto loop


:process
if /i not "%~x1"==".csv" (
	echo ERROR: Input file not expected type: %~x1
	goto end
)

echo Processing: "%~n1"
omsynth "%~1" -out "%~dp1%~n1.synth.cwa"
echo.
goto :eof


:end
echo Done!
pause
