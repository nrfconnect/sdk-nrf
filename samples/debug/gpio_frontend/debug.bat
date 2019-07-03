@echo off
:: Run zephyrparser.py with .elf file from gpio_frontend.
if exist .\build_nrf52_pca10040\zephyr\zephyr.elf (
    set build=build_nrf52_pca10040
) else if exist .\build_nrf52840_pca10056\zephyr\zephyr.elf (
    set build=build_nrf52840_pca10056
) else if exist .\build_nrf9160_pca10090\zephyr\zephyr.elf (
    set build=build_nrf9160_pca10090
) else if exist .\build_nrf9160_pca10090ns\zephyr\zephyr.elf (
    set build=build_nrf9160_pca10090ns
) else if exist .\build\zephyr\zephyr.elf (
    set build=build
) else (
    echo No build directory found.
    echo Please build the sample first.
    pause
    exit
)
    echo Starting ZephyrLogParser...
    echo When prompt appears, type "gpio init" to start GPIO backend.
    echo Type "gpio start" to begin log capture, remember to restart the board immediately after.
    cd ..\..\..\scripts\logger_pc_app
    py -3 zephyrlogparser.py --elf ..\..\samples\debug\gpio_frontend\%build%\zephyr\zephyr.elf --gpio-on
pause
