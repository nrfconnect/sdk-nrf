if(BOARD MATCHES "nrf91")
  set(mcuboot_DTC_OVERLAY_FILE "${APP_DIR}/nrf91dk_mcuboot.overlay" CACHE STRING "" FORCE)
endif()
