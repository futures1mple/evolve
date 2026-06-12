@echo off
setlocal enabledelayedexpansion

REM ================================================================
REM  EVOLVE Experiment Runner
REM  Запускать из корневой папки проекта (где находятся src\ и include\)
REM
REM  Требования:
REM    - gcc в PATH (MinGW / MSYS2 / TDM-GCC)
REM    - src\main.c заменён на обновлённую версию (с аргументами CLI)
REM    - configs\ папка с конфиг-файлами рядом с include\
REM
REM  Запуск: run_experiments.bat
REM ================================================================

set GCC=gcc
set SRC=src\main.c src\voter.c src\authority.c src\commitment.c src\zkproof.c src\recommitment.c src\tally.c src\poly.c src\utils.c
set FLAGS=-O2 -std=c99 -Wall
set IDIR=-Iinclude

REM --- Очищаем старый results.csv (создаём backup) ---
if exist results.csv (
    echo [INFO] Backing up old results.csv to results_backup.csv
    copy results.csv results_backup.csv >nul
    del results.csv
)

echo.
echo ================================================================
echo  EVOLVE EXPERIMENT SUITE
echo ================================================================
echo.

REM ================================================================
REM  СЦЕНАРИЙ 1: Влияние N
REM  Фиксировано: Voters=100, Auth=4, Bucket=30
REM ================================================================
echo [SCENARIO 1] N influence (Voters=100, Auth=4, Bucket=30)
echo ----------------------------------------------------------------

for %%N in (128 512 1024 2048) do (
    echo.
    echo  [BUILD] N=%%N Auth=4 ...
    copy /Y configs\config_N%%N_A4.h include\config.h >nul
    %GCC% %FLAGS% %IDIR% %SRC% -o evolve_tmp.exe
    if errorlevel 1 (
        echo  [ERROR] Compilation failed for N=%%N
        goto :end
    )

    for /L %%R in (1,1,3) do (
        echo  [RUN %%R/3] N=%%N Voters=100 Bucket=30
        evolve_tmp.exe 100 30
    )
)


REM ================================================================
REM  СЦЕНАРИЙ 2: Масштабируемость по числу избирателей
REM  Фиксировано: N=1024, Auth=4, Bucket=30
REM  (бинарник уже скомпилирован как N=1024 A=4 в конце сценария 1)
REM ================================================================
echo.
echo [SCENARIO 2] Voters scalability (N=1024, Auth=4, Bucket=30)
echo ----------------------------------------------------------------

echo  [BUILD] N=1024 Auth=4 (for voters scalability) ...
copy /Y configs\config_N1024_A4.h include\config.h >nul
%GCC% %FLAGS% %IDIR% %SRC% -o evolve_tmp.exe
if errorlevel 1 (
    echo  [ERROR] Compilation failed
    goto :end
)

REM V=100 и V=1000 уже есть в старом results.csv, но перезапускаем
REM для единообразия формата (17 колонок)
for %%V in (100 500 1000 2000) do (
    echo.
    echo  [VOTERS=%%V] 2 runs ...
    for /L %%R in (1,1,2) do (
        echo  [RUN %%R/2] N=1024 Voters=%%V Bucket=30
        evolve_tmp.exe %%V 30
    )
)

REM --- V=5000 и V=10000: ОПЦИОНАЛЬНО, очень долго (~30-54 мин каждый) ---
REM Раскомментировать если нужно:
REM echo  [OPTIONAL] Voters=5000 (est ~35 min) ...
REM evolve_tmp.exe 5000 30
REM echo  [OPTIONAL] Voters=10000 (est ~54 min) ...
REM evolve_tmp.exe 10000 30


REM ================================================================
REM  СЦЕНАРИЙ 3: Влияние bucket size
REM  Фиксировано: N=1024, Auth=4, Voters=100
REM  (тот же бинарник)
REM ================================================================
echo.
echo [SCENARIO 3] Bucket size (N=1024, Auth=4, Voters=100)
echo ----------------------------------------------------------------

for %%B in (8 15 30 50 100) do (
    echo.
    echo  [BUCKET=%%B] 3 runs ...
    for /L %%R in (1,1,3) do (
        echo  [RUN %%R/3] N=1024 Voters=100 Bucket=%%B
        evolve_tmp.exe 100 %%B
    )
)


REM ================================================================
REM  СЦЕНАРИЙ 4: Влияние числа authorities
REM  Фиксировано: N=1024, Voters=100, Bucket=30
REM ================================================================
echo.
echo [SCENARIO 4] Authorities (N=1024, Voters=100, Bucket=30)
echo ----------------------------------------------------------------

for %%A in (2 4 8) do (
    echo.
    echo  [BUILD] N=1024 Auth=%%A ...
    copy /Y configs\config_N1024_A%%A.h include\config.h >nul
    %GCC% %FLAGS% %IDIR% %SRC% -o evolve_tmp.exe
    if errorlevel 1 (
        echo  [ERROR] Compilation failed for Auth=%%A
        goto :end
    )

    for /L %%R in (1,1,3) do (
        echo  [RUN %%R/3] N=1024 Auth=%%A Voters=100 Bucket=30
        evolve_tmp.exe 100 30
    )
)


REM ================================================================
REM  Финал: восстанавливаем базовый config
REM ================================================================
echo.
echo ----------------------------------------------------------------
copy /Y configs\config_N1024_A4.h include\config.h >nul
del evolve_tmp.exe 2>nul

echo.
echo ================================================================
echo  ALL EXPERIMENTS DONE
echo  Results written to: results.csv
echo ================================================================

:end
endlocal
