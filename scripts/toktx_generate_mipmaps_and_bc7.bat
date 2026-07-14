@echo off
setlocal EnableDelayedExpansion

set "INPUT=%CD%\"
set "OUTPUT=%CD%\ktx2"

if not exist "%OUTPUT%" mkdir "%OUTPUT%"

for /R %%F in (*.png *.jpg *.jpeg *.tga *.bmp) do (

    set "FULL=%%~dpF"
    set "REL=!FULL:%INPUT%=!"

    if "!REL:~0,1!"=="\" set "REL=!REL:~1!"

    if not exist "%OUTPUT%\!REL!" mkdir "%OUTPUT%\!REL!"

    set "NAME=%%~nF"

    echo Converting %%~nxF
    echo Output folder: %OUTPUT%\!REL!

    if /I "!NAME:~-7!"=="_normal" (
        toktx ^
            --t2 ^
            --genmipmap ^
            --normal_mode ^
            --encode uastc ^
            --uastc_quality 2 ^
            --uastc_rdo_l 1 ^
            --assign_oetf linear ^
            "%OUTPUT%\!REL!%%~nF.ktx2" ^
            "%%F"
    ) else (
        toktx ^
            --t2 ^
            --genmipmap ^
            --encode uastc ^
            --uastc_quality 2 ^
            --uastc_rdo_l 1 ^
            "%OUTPUT%\!REL!%%~nF.ktx2" ^
            "%%F"
    )
)

echo Done.
pause