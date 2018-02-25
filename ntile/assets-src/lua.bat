@echo off

set PATH=..\app\win32;..\dependencies\bleb;%PATH%

blebtool put -R ../app/ntile1.bleb -i ntileStartup.lua ntile/scripts/startup.lua

pause
