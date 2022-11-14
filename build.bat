@echo off
echo Build assets
assets2banks assets --firstbank=1,20000
@if %errorlevel% NEQ 0 goto :EOF

echo Build Bank1
sdcc -c -mz80 bank1.c
@if %errorlevel% NEQ 0 goto :EOF

echo Build Main
sdcc -c -mz80 --peep-file ..\SMSlib\peep-rules.txt main.c
@if %errorlevel% NEQ 0 goto :EOF

echo Linking
sdcc -o output.ihx -mz80 --data-loc 0xC000 --no-std-crt0 -L ..\SMSlib ..\crt0\crt0_sms.rel main.rel bank1.rel SMSlib_MD_VDP.lib ..\PSGlib\PSGlib.rel
@if %errorlevel% NEQ 0 goto :EOF

ihx2sms output.ihx output.sms

