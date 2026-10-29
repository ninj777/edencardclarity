@echo off
rem Run this from an x64 Native Tools Command Prompt for Visual Studio.
setlocal
cd /d "%~dp0"
if not exist build mkdir build
cl /nologo /TC /std:c11 /O1 /w /D_CRT_SECURE_NO_WARNINGS /DSQLITE_THREADSAFE=0 /DSQLITE_OMIT_LOAD_EXTENSION /DSQLITE_ENABLE_FTS5 /c thirdParty\sqlite3.c /Fobuild\sqliteWindows.obj
if errorlevel 1 exit /b 1
for %%f in (main cards input wallet storage logger window catalog catalogBuild catalogText catalogWallet catalogFiles catalogApp catalogWindow rewardMath) do (
  cl /nologo /TC /std:c11 /utf-8 /W4 /D_CRT_SECURE_NO_WARNINGS /DUNICODE /D_UNICODE /c CreditCardThing\%%f.c /Fobuild\%%f.obj
  if errorlevel 1 exit /b 1
)
link /nologo build\main.obj build\cards.obj build\input.obj build\wallet.obj build\storage.obj build\logger.obj build\window.obj build\catalog.obj build\catalogBuild.obj build\catalogText.obj build\catalogWallet.obj build\catalogFiles.obj build\catalogApp.obj build\catalogWindow.obj build\rewardMath.obj build\sqliteWindows.obj user32.lib gdi32.lib comctl32.lib /OUT:build\cardClarity.exe /MANIFEST:EMBED /MANIFESTINPUT:CreditCardThing\appManifest.xml
if errorlevel 1 exit /b 1
build\cardClarity.exe --build-catalog data data\cardCatalog.db
if errorlevel 1 exit /b 1
echo Build finished. Run build\cardClarity.exe --gui
