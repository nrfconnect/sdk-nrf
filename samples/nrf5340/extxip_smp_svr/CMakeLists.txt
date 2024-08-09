cmake_minimum_required(VERSION 3.20.0)

macro(app_set_runner_args)
  if(CONFIG_BOARD_THINGY53_NRF5340_CPUAPP)
    # Use alternative QSPI configuration file when flashing Thingy53
    board_runner_args(nrfjprog "--qspiini=${CMAKE_CURRENT_SOURCE_DIR}/Qspi_thingy53.ini")
  endif()
endmacro()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(smp_svr_ext_xip)

# This project uses orginal sdk-zephyr C source code
target_sources(app PRIVATE ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/main.c)
target_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_BT app PRIVATE ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/bluetooth.c)

# This places the application bluetooth.c file and some MCUmgr libraries into QSPI XIP
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/bluetooth.c LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__mgmt LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__transport LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__util LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__smp LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__grp__img_mgmt LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__grp__os_mgmt LOCATION EXTFLASH_TEXT NOCOPY)

zephyr_code_relocate(FILES ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/bluetooth.c LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__mgmt LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__transport LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__util LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__smp LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__grp__img_mgmt LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__grp__os_mgmt LOCATION RAM_DATA)
