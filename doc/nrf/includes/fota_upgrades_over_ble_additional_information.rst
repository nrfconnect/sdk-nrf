.. fota_upgrades_over_ble_additional_information_start

If necessary, you can modify any of the implied options or defaulted values introduced by the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option.

You can either add these Kconfig options to the configuration files of your application or have them inline in a project build command.
Here is an example of how you can build for the :ref:`peripheral_lbs` sample:

.. parsed-literal::
   :class: highlight

    west build -b *board_target* -- -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU=y

When you connect to the device after the build has completed and the firmware has been programmed to it, the SMP Service is enabled with the ``UUID 8D53DC1D-1DB7-4CD3-868B-8A527460AA84``.
If you want to add SMP Service to advertising data, refer to the :zephyr:code-sample:`smp-svr`.

.. fota_upgrades_over_ble_additional_information_end
