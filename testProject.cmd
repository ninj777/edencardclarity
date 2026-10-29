@echo off
rem Run buildProject.cmd first, then this file from the same Native Tools prompt.
setlocal
cd /d "%~dp0"
set cardObjects=build\catalog.obj build\catalogBuild.obj build\catalogText.obj build\catalogWallet.obj build\catalogFiles.obj build\catalogApp.obj build\catalogWindow.obj build\rewardMath.obj build\input.obj build\sqliteWindows.obj
for %%t in (testCatalog testCatalogScale) do (
  cl /nologo /TC /std:c11 /utf-8 /W4 /D_CRT_SECURE_NO_WARNINGS tests\%%t.c %cardObjects% user32.lib gdi32.lib comctl32.lib /Fe:build\%%t.exe /Fo:build\%%t.obj
  if errorlevel 1 exit /b 1
)
for %%t in (testCatalogMenu testGate1 testGate2) do (
  cl /nologo /TC /std:c11 /utf-8 /W4 /D_CRT_SECURE_NO_WARNINGS tests\%%t.c tests\testSupport.c /Fe:build\%%t.exe /Fo:build\
  if errorlevel 1 exit /b 1
)
cl /nologo /TC /std:c11 /utf-8 /W4 /D_CRT_SECURE_NO_WARNINGS tests\testModules.c build\cards.obj build\logger.obj /Fe:build\testModules.exe /Fo:build\testModules.obj
if errorlevel 1 exit /b 1
set checkFolder=build\windowsChecks%RANDOM%%RANDOM%
build\testCatalog.exe data\cardCatalog.db %checkFolder%
if errorlevel 1 exit /b 1
build\testCatalogMenu.exe "%CD%\build\cardClarity.exe" "%CD%\data\cardCatalog.db"
if errorlevel 1 exit /b 1
build\testCatalogScale.exe data\cardCatalog.db %checkFolder%\scale.db
if errorlevel 1 exit /b 1
build\testGate1.exe "%CD%\build\cardClarity.exe"
if errorlevel 1 exit /b 1
build\testGate2.exe "%CD%\build\cardClarity.exe"
if errorlevel 1 exit /b 1
build\testModules.exe
if errorlevel 1 exit /b 1
echo The C checks passed. The desktop checklist still needs a real person.
