.. _nrf9160_external_flash:

Configuring external flash memory on the nRF9160 DK
###################################################

.. contents::
   :local:
   :depth: 2

The nRF9160 DK version 0.14.0 and later versions contain an 8-MB external flash (MX25R6435F).
The external flash can be used to store data that does not fit on the internal flash.
To free up GPIO when the external flash is not needed, the nRF9160 DK board controller (nRF52840 SoC) controls a switch to enable or disable routing between the external flash and the nRF9160 SIP.

See the `nRF9160 DK board controller <nRF9160 DK board control section in the nRF9160 DK User Guide_>`_ and `External memory <External memory section in the nRF9160 DK User Guide_>`_ sections in the nRF9160 DK user guides for more details.

Programming the board controller firmware
*****************************************

To use the external flash memory on the nRF9160 DK v0.14.0 or later versions, the board controller firmware must be of version v2.0.1.
This is the factory firmware version.
If you need to program the board controller firmware again, complete the following steps:

#. Download the nRF9160 DK board controller firmware from the `nRF9160 DK downloads`_ page.
#. Make sure the **PROG/DEBUG SW10** switch on the nRF9160 DK is set to **nRF52**.
#. Program the board controller firmware (:file:`nrf9160_dk_board_controller_fw_2.0.1.hex`) using the `Programmer app <Programming a Development Kit_>`_ in nRF Connect for Desktop.
   By default, this enables pin routing to external flash.

Adding the configuration and devicetree option
**********************************************

To use external flash, you also need to enable configuration and devicetree options by completing the following steps:

#. Enable the following Kconfig options by setting each to ``y`` in your project configuration:

   * :kconfig:option:`CONFIG_FLASH`
   * :kconfig:option:`CONFIG_FLASH_MAP`
   * :kconfig:option:`CONFIG_SPI`
   * :kconfig:option:`CONFIG_SPI_NOR`
   * :kconfig:option:`CONFIG_SPI_NOR_SFDP_DEVICETREE`
   * :kconfig:option:`CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK`

#. Specify the board revision parameter when :ref:`building` to automatically include the devicetree overlay with the external flash.
   The board revision is printed on the label of your DK, just below the PCA number.

#. Then add the following relevant devicetree blocks to the application`s devicetree overlay file, depending on your application`s needs:

   .. code-block:: devicetree

      /* Enable the external flash device (required) */
      &mx25r64 {
	   status = "okay";
      };

      /* Configure partition manager to use mx25r64 as the external flash device */
      / {
          chosen {
              nordic,pm-ext-flash = &mx25r64;
          };
      };

      /* Enable high performance mode to increase write/erase performance */
      &mx25r64 {
          mxicy,mx25r-power-mode = "high-performance";
      };

   For more information about devicetree overlays, see :ref:`zephyr:use-dt-overlays`.

Using Partition Manager
***********************

If your application was built using the |NCS|, you must define partitions using :ref:`partition_manager`.
The built-in partition definitions can be found in the :file:`nrf/subsys/partition_manager` folder, and the file names start with ``pm.yml``.
The files that have ``external_flash`` as their region support storing partitions in the external flash.
You can also find the configuration option required to place the partition in the external flash region in those files.
For example, the :file:`pm.yml.pgps` file has an option (:kconfig:option:`CONFIG_PM_PARTITION_REGION_PGPS_EXTERNAL`) to place the P-GPS partition in external flash:

.. code-block:: c

   #ifdef CONFIG_PM_PARTITION_REGION_PGPS_EXTERNAL
     region: external_flash
   #else
     inside: [nonsecure_storage]
   #endif
     size: CONFIG_NRF_CLOUD_PGPS_PARTITION_SIZE

You can also change the size of the partition with the :kconfig:option:`CONFIG_NRF_CLOUD_PGPS_PARTITION_SIZE` Kconfig option.
