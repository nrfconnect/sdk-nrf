include(${ZEPHYR_NRF_MODULE_DIR}/sysbuild/extensions.cmake)
add_overlay_config(mcuboot
  ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/template/mcuboot_minimal_overrides.conf)
