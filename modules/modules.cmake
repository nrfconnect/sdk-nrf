set(ZEPHYR_MCUBOOT_CMAKE_DIR  ${CMAKE_CURRENT_LIST_DIR}/mcuboot)
set(ZEPHYR_MCUBOOT_KCONFIG    ${CMAKE_CURRENT_LIST_DIR}/mcuboot/Kconfig)
set(ZEPHYR_NRFXLIB_CMAKE_DIR  ${CMAKE_CURRENT_LIST_DIR}/nrfxlib)
set(ZEPHYR_CJSON_CMAKE_DIR    ${CMAKE_CURRENT_LIST_DIR}/cjson)
set(ZEPHYR_CJSON_KCONFIG      ${CMAKE_CURRENT_LIST_DIR}/cjson/Kconfig)
set(ZEPHYR_COREMARK_KCONFIG   ${CMAKE_CURRENT_LIST_DIR}/coremark/Kconfig)
set(ZEPHYR_TRUSTED_FIRMWARE_M_KCONFIG ${CMAKE_CURRENT_LIST_DIR}/trusted-firmware-m/Kconfig)
set(ZEPHYR_AZURE_SDK_FOR_C_KCONFIG ${CMAKE_CURRENT_LIST_DIR}/azure-sdk-for-c/Kconfig)
set(ZEPHYR_AZURE_SDK_FOR_C_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/azure-sdk-for-c)

# Those are modules with Kconfig tree's inside the module repo but where
# nRF Connect SDK extend those trees.
# This means the extended Kconfig module file refers to external symbols causing
# Kconfig parsing to fail if referred module is not available.
# Thus we set the module to `-NOTFOUND` if unavailable to avoid sourcing the
# Kconfig file having undefined symbols.

set(NCS_KCONFIG_EXTENDED_MODULES
    memfault-firmware-sdk
)

foreach(module_name ${NCS_KCONFIG_EXTENDED_MODULES})
  zephyr_string(SANITIZE TOUPPER MODULE_NAME_UPPER ${module_name})
  if(${module_name} IN_LIST ZEPHYR_MODULE_NAMES)
    set(NCS_${MODULE_NAME_UPPER}_KCONFIG
        "${CMAKE_CURRENT_LIST_DIR}/${module_name}/Kconfig"
    )
  else()
    set(NCS_${MODULE_NAME_UPPER}_KCONFIG
        "${CMAKE_CURRENT_LIST_DIR}/${module_name}/Kconfig-NOTFOUND"
    )
  endif()
  list(APPEND ZEPHYR_KCONFIG_MODULES_DIR
       "NCS_${MODULE_NAME_UPPER}_KCONFIG=${NCS_${MODULE_NAME_UPPER}_KCONFIG}"
  )
endforeach()
