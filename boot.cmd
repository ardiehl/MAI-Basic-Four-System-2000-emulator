@echo off
:: telnet and putty only for now, ai translated, needs to be cleaned
setlocal enabledelayedexpansion

:: Initial configuration
set "IMAGE=wd/bossix_micropolis_2011.dsk"

:: parameters for xterm
set "XT_GEOMETRY=80x24+50+100"
set "XT_FONT=Monospace"
set "XT_FONTSIZE=12"
set "XT_BACKGROUND=black"
set "XT_FOREGROUND=white"

set "TELNET=telnet"
set "PORT_BASE=4000"
set "HOST=localhost"

set "XT1=xterm -fa %XT_FONT% -fs %XT_FONTSIZE% -bg %XT_BACKGROUND% -fg %XT_FOREGROUND% -geometry %XT_GEOMETRY% -T "
set "XT2= -e "

set "GT1=gnome-terminal --geometry %XT_GEOMETRY% -t "
set "GT2= -- "

set "PU1=putty -telnet -P"
set "PU2= "

set "CR1=cool-retro-term -p 'Monochrome Green' -T "
set "CR2= -e "
set "CR3= 2>nul"

set "TERMCMD=%XT1%"
set "TERMEXEC=%XT2%"

:: socket numbers to start
set "SOCKNUMS="
set "POSITIONAL_ARGS="
set "GO="
set "ARGS=eagleemu"

:: Parse Arguments Loop
:args_loop
if "%~1"=="" goto args_done

:: Match single terminal option switches (-0 to -9)
echo %~1| findstr /R "^-[0-9]$" >nul
if %errorlevel%==0 (
    set "OPT=%~1"
    set "SOCKNUMS=!SOCKNUMS! !OPT:~1!"
    shift
    goto args_loop
)

if "%~1"=="-g" set "GO=g" & shift & goto args_loop
if "%~1"=="--go" set "GO=g" & shift & goto args_loop

if "%~1"=="-gt" goto set_gnome
if "%~1"=="--gnome" goto set_gnome

if "%~1"=="-pt" goto set_putty
if "%~1"=="--putty" goto set_putty

if "%~1"=="-cr" goto set_cool
if "%~1"=="--cool" goto set_cool

if "%~1"=="-t" set "TELNET=%~2" & shift & shift & goto args_loop
if "%~1"=="--telnet" set "TELNET=%~2" & shift & shift & goto args_loop

if "%~1"=="-a" set "SOCKNUMS=0 1 2 3 4 5 6 7 8 9" & shift & goto args_loop
if "%~1"=="--all" set "SOCKNUMS=0 1 2 3 4 5 6 7 8 9" & shift & goto args_loop

if "%~1"=="-i" goto set_image
if "%~1"=="--image" goto set_image

if "%~1"=="-w" call :addarg "dev nv wd" & shift & goto args_loop
if "%~1"=="--wd" call :addarg "dev nv wd" & shift & goto args_loop

if "%~1"=="-c" call :addarg "dev nv cs" & shift & goto args_loop
if "%~1"=="--cs" call :addarg "dev nv cs" & shift & goto args_loop

if "%~1"=="-f" call :addarg "dev nv fd" & shift & goto args_loop
if "%~1"=="--fd" call :addarg "dev nv fd" & shift & goto args_loop

:: Catch-all for other switches
echo %~1| findstr /R "^-" >nul
if %errorlevel%==0 (
    echo Unknown option %1
    call :usage
    exit /b 1
)

:: Treat as positional arguments
set "POSITIONAL_ARGS=%POSITIONAL_ARGS% "%~1""
shift
goto args_loop

:set_gnome
set "TERMCMD=%GT1%"
set "TERMEXEC=%GT2%"
shift
goto args_loop

:set_putty
set "TERMCMD=%PU1%"
set "TERMEXEC=%PU2%"
shift
goto args_loop

:set_cool
set "TERMCMD=%CR1%"
set "TERMEXEC=%CR2%"
shift
goto args_loop

:set_image
set "IMAGE=%~2"
if "%IMAGE%"=="-" set "IMAGE="
shift & shift
goto args_loop

:args_done

:: Validate Image Existence
if not "%IMAGE%"=="" (
    if not exist "%IMAGE%" (
        echo %~nx0: unable to open %IMAGE%
        exit /b 1
    )
    call :addarg "dev wd image %IMAGE%"
)

:: Generate socket configurations
for %%i in (%SOCKNUMS%) do (
    call :terminalPort %%i
    if "%%i"=="0" (
        call :addarg "dev scc socketio 0 1"
    ) else if "%%i"=="1" (
        call :addarg "dev scc socketio 1 1"
    ) else (
        call :addarg "dev sock terminal %%i 1"
    )
    set /a PORT=PORT_BASE + %%i
    
    if "!TERMCMD!"=="%XT1%" (
        call :addarg "execa %TELNET% %HOST% !PORT!"
    ) else (
    if "!TERMCMD!"=="%PU1%" (
        call :addarg "execa %PU1% !PORT! %HOST%"
    ) else (
        call :addarg "exec !TERMCMD!!TP!!TERMEXEC!%TELNET% %HOST% !PORT!%TERMEND%"
      )
    )
)

:: when executing the 32 bit version on 64 bit systems, telnet is not in the search path
set "PATH=%PATH%;%WINDIR%\sysnative"

:: Execute the program string directly
::echo ARGS: %ARGS% %POSITIONAL_ARGS% %GO%
%ARGS% %POSITIONAL_ARGS% %GO%
goto :eof

:: ==============================================================================
:: SUBROUTINES
:: ==============================================================================

:usage
echo Usage: %~nx0 [options]
echo  -i, --image     wd boot image or - to set no wd image
echo  -w  --wd        boot from wd0
echo  -c  --cs        boot from cs0
echo  -f  --fd        boot from fd0
::echo  -t, --telnet    telnet program to use
::echo  -gt,--gnome     use gnome-terminal instead of xterm
echo  -pt,--putty     use putty instead of xterm
::echo  -cr,--cool      use cool retro terminal instead of xterm
echo  -0              start first terminal -0 to -9 are supported
echo  -a, --all       start all terminals
echo  -g              go, start the emulation
goto :eof

:terminalPort
if "%~1"=="0" set "TP=scc0"
if "%~1"=="1" set "TP=scc1"
if "%~1"=="2" set "TP=fourway_1A"
if "%~1"=="3" set "TP=fourway_1B"
if "%~1"=="4" set "TP=fourway_1C"
if "%~1"=="5" set "TP=fourway_1D"
if "%~1"=="6" set "TP=fourway_2A"
if "%~1"=="7" set "TP=fourway_2B"
if "%~1"=="8" set "TP=fourway_2C"
if "%~1"=="9" set "TP=fourway_2D"
goto :eof

:addarg
set "ARGS=%ARGS% %1"
goto :eof

