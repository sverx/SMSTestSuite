@echo off
echo Build assets
assets2banks assets --bank1size=21000 --compile
@if %errorlevel% NEQ 0 goto :EOF

echo Build Main
sdcc -c -mz80 --peep-file ..\SMSlib\peep-rules.txt main.c
@if %errorlevel% NEQ 0 goto :EOF

echo Linking
REM sdcc -o output.ihx -mz80 --data-loc 0xC000 -Wl-b_BANK2=0x8000 -Wl-b_BANK3=0x8000 -Wl-b_BANK4=0x8000 --no-std-crt0 -L ..\SMSlib ..\crt0\crt0_sms.rel main.rel SMSlib.lib ..\PSGlib\PSGlib.rel bank2.rel bank3.rel bank4.rel
sdcc -o output.ihx -mz80 --data-loc 0xC000 --no-std-crt0 -L ..\SMSlib ..\crt0\crt0_sms.rel main.rel SMSlib_MD_VDP.lib ..\PSGlib\PSGlib.rel bank1.rel
@if %errorlevel% NEQ 0 goto :EOF

ihx2sms output.ihx output.sms
