@ECHO OFF
REM *******************************************************
REM * In an attempt to keep the commandos happy, this
REM * CMD file will compress all the dols in the current
REM * directory.
REM *******************************************************
@FOR /F "delims=/" %%D in ('dir /b *.dol') do dollz3 "%%~nD".dol "%%~nD"-dl3.dol
