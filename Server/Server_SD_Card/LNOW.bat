@echo off

color 0a
DEL /AH LAUN_NET.txt
@echo --------------------------------------------------------------------------------

set /p ssid="what is the networks ssid? "

@echo:%ssid%>>LAUN_NET.txt
attrib LAUN_NET.txt +h 

@echo --------------------------------------------------------------------------------

set /p pass=" what is the networks passowrd? "

@echo %pass%>>LAUN_NET.txt

@echo --------------------------------------------------------------------------------

@echo thank you,
@echo text text text
@echo --------------------------------------------------------------------------------
pause