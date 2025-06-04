# The order of the Kconfig files appied:
# Merged configuration '/work/nrf/samples/bluetooth/iso_time_sync/sysbuild/ipc_radio/prj.conf'
# Merged configuration '/work/nrf/samples/bluetooth/iso_time_sync/sysbuild/ipc_radio/boards/nrf5340dk_nrf5340_cpunet.conf'
# Merged configuration '/work/nrf/applications/ipc_radio/overlay-bt_hci_ipc.conf'

# Overwrite configs from overlay-bt_hci_ipc.conf by applying board-specific config one more time.

if(SB_CONFIG_BOARD_NRF5340DK)
  set(ipc_radio_EXTRA_CONF_FILE "${ipc_radio_EXTRA_CONF_FILE};boards/nrf5340dk_nrf5340_cpunet.conf" CACHE INTERNAL "sysbuild controlled")
endif()
