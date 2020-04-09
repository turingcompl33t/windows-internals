:: _setup.bat
:: Hacky setup script to set workspace directory.

@echo off

FOR /F "tokens=* USEBACKQ" %%F IN (`cd`) DO (
SET WIN_WORKSPACE=%%F
)

echo Set WIN_WORKSPACE = %WIN_WORKSPACE%