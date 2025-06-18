
zephyr_library_sources(crc32.c)
zephyr_library_sources(vendor_radio_power_map.c)
zephyr_library_sources(vendor_radio_power_map_common.c)
zephyr_library_sources(vendor_radio_power_map_psemi.c)
zephyr_library_sources(vendor_radio_power_map_psemi_flash_traversal.c)
zephyr_library_sources(vendor_radio_power_limit.c)
zephyr_library_sources(vendor_radio_power_settings.c)

if (CONFIG_PARTITION_MANAGER_ENABLED)
        ncs_add_partition_manager_config(pm.yml.power_map)
endif()
