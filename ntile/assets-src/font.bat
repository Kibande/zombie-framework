@echo off

set PATH=..\app\win32;..\dependencies\bleb;%PATH%

mkdir ..\build

ntile mkfont ntile/font.png ../build/thin.bleb "#$-:ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz0123456789.,!?()[]" 2,0,999,12,1,4
blebtool merge -R ../app/ntile1.bleb -p ntile/font/thin. ../build/thin.bleb

ntile.exe mkfont ntile/font.png ../build/fat.bleb ABCDEFGHIJKLMNOPQRSTUVWXYZ 2,15,999,23,1,5
blebtool merge -R ../app/ntile1.bleb -p ntile/font/fat. ../build/fat.bleb

pause
