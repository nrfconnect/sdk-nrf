cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(smp_svr_ext_xip)

# This project uses orginal sdk-zephyr C source code
target_sources(app PRIVATE ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/main.c)
target_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_BT app PRIVATE ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/bluetooth.c)

# This places the application bluetooth.c file and some MCUmgr libraries into QSPI XIP
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/bluetooth.c LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY mcumgr_mgmt LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY mcumgr_transport LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY mcumgr_util LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY mcumgr_smp LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY mcumgr_grp_img_mgmt LOCATION EXTFLASH_TEXT NOCOPY)
zephyr_code_relocate(LIBRARY mcumgr_grp_os_mgmt LOCATION EXTFLASH_TEXT NOCOPY)

zephyr_code_relocate(FILES ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/bluetooth.c LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY mcumgr_mgmt LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY mcumgr_transport LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY mcumgr_util LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY mcumgr_smp LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY mcumgr_grp_img_mgmt LOCATION RAM_DATA)
zephyr_code_relocate(LIBRARY mcumgr_grp_os_mgmt LOCATION RAM_DATA)
