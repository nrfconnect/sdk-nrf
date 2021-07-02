#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
if (NOT CONFIG_SB_FOTA_SRC)

if(CONFIG_FPU)
  if(CONFIG_FP_HARDABI)
    set(float_dir hard-float)
  elseif(CONFIG_FP_SOFTABI)
    set(float_dir softfp-float)
  else()
    assert(0 "Unreachable code")
  endif()
else()
  set(float_dir soft-float)
endif()

assert(GCC_M_CPU "GCC_M_CPU must be set to find correct lib.")

set(lib_path lib/${GCC_M_CPU}/${float_dir})

set(SB_FOTA_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${lib_path})

if(NOT EXISTS ${SB_FOTA_LIB_PATH})
  message(WARNING "This combination of SoC and floating point ABI is not supported by libsb_fota."
                  "(${SB_FOTA_LIB_PATH} doesn't exist.)")
endif()

set(                    SB_FOTA_TARGET libsb_fota)
zephyr_library_import(${SB_FOTA_TARGET} ${SB_FOTA_LIB_PATH}/libsb_fota.a)
target_link_libraries(${SB_FOTA_TARGET} INTERFACE modem -lc)

zephyr_include_directories(include)
zephyr_library()
zephyr_library_sources(
  os/sb_fota_os.c
  os/sb_fota_settings.c
)

endif()