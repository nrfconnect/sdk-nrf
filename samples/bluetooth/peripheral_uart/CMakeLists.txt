set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE
  src/main.c
  ../gatt/nus.c
)

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth/)
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
