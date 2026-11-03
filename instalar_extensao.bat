@echo off
chcp 65001 >nul
echo ============================================================================
echo   INSTALADOR DO PLUGIN BIMKERNEL v3.0.0 - ARCHICAD 30 / 29
echo ============================================================================
echo.

REM Check for Administrator privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [!] Solicitando privilegios de Administrador para copiar para Program Files...
    powershell -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

set "SRC_AC30=%~dp0BimKernel_AC30.apx"
set "SRC_AC29=%~dp0BimKernel_AC29.apx"
set "SRC_DEFAULT=%~dp0BimKernel.apx"

if not exist "%SRC_AC30%" (
    if exist "%~dp0Build\BimKernel\30\INT\Release\BimKernel.apx" (
        set "SRC_AC30=%~dp0Build\BimKernel\30\INT\Release\BimKernel.apx"
    ) else (
        set "SRC_AC30=%SRC_DEFAULT%"
    )
)

if not exist "%SRC_AC29%" (
    if exist "%~dp0Build\BimKernel\29\INT\Release\BimKernel.apx" (
        set "SRC_AC29=%~dp0Build\BimKernel\29\INT\Release\BimKernel.apx"
    ) else (
        set "SRC_AC29=%SRC_DEFAULT%"
    )
)

set "INSTALLED_ANY=0"

if exist "C:\Program Files\Graphisoft\Archicad 30\Add-Ons\Extras" (
    echo [*] Detectado Archicad 30 instalado no PC!
    if exist "C:\Program Files\Graphisoft\Archicad 30\Add-Ons\Extras\SuperquadraAutomation.apx" (
        del /F /Q "C:\Program Files\Graphisoft\Archicad 30\Add-Ons\Extras\SuperquadraAutomation.apx" >nul 2>&1
    )
    echo [*] Copiando BimKernel.apx ^(AC30 v3.0.0^) para C:\Program Files\Graphisoft\Archicad 30\Add-Ons\Extras...
    copy /Y "%SRC_AC30%" "C:\Program Files\Graphisoft\Archicad 30\Add-Ons\Extras\BimKernel.apx"
    if %errorLevel% equ 0 (
        echo [OK] BimKernel.apx instalado com sucesso no Archicad 30!
        set "INSTALLED_ANY=1"
    ) else (
        echo [ERRO] Falha ao copiar para Archicad 30. Feche o Archicad 30 se estiver aberto e tente novamente.
    )
    echo.
)

if exist "C:\Program Files\Graphisoft\Archicad 29\Add-Ons\Extras" (
    echo [*] Detectado Archicad 29 instalado no PC!
    if exist "C:\Program Files\Graphisoft\Archicad 29\Add-Ons\Extras\SuperquadraAutomation.apx" (
        del /F /Q "C:\Program Files\Graphisoft\Archicad 29\Add-Ons\Extras\SuperquadraAutomation.apx" >nul 2>&1
    )
    echo [*] Copiando BimKernel.apx ^(AC29 v3.0.0^) para C:\Program Files\Graphisoft\Archicad 29\Add-Ons\Extras...
    copy /Y "%SRC_AC29%" "C:\Program Files\Graphisoft\Archicad 29\Add-Ons\Extras\BimKernel.apx"
    if %errorLevel% equ 0 (
        echo [OK] BimKernel.apx instalado com sucesso no Archicad 29!
        set "INSTALLED_ANY=1"
    ) else (
        echo [ERRO] Falha ao copiar para Archicad 29. Feche o Archicad 29 se estiver aberto e tente novamente.
    )
    echo.
)

if "%INSTALLED_ANY%"=="0" (
    echo [!] Nenhuma pasta padrao do Archicad 30 ou 29 encontrada ou erro de copia.
) else (
    echo ============================================================================
    echo   SUCESSO! BimKernel.apx v3.0.0 (22 Comandos Nativos) instalado!
    echo   Reinicie o Archicad 30 (ou carregue pelo Gestor de Extensoes) para ativar.
    echo ============================================================================
)
pause
