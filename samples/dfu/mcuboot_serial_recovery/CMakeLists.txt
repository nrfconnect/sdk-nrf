cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mcuboot_serial_recovery)

target_sources(app PRIVATE
  src/execution_context.c
  ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/main.c
)

if(CONFIG_NRF_MCUBOOT_BOOT_REQUEST)
  target_sources(app PRIVATE src/boot_request_shell.c)
  target_link_libraries(app PRIVATE MCUBOOT_BOOTUTIL)
endif()
