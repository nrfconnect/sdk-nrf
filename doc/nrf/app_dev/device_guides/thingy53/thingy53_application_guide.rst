.. _thingy53_app_guide:

Application guide for Thingy:53
###############################

.. contents::
   :local:
   :depth: 2

The Nordic Thingy:53 does not have a built-in J-Link debug IC.
Because of that, the Thingy:53 board enables MCUboot bootloader with serial recovery support and a fixed devicetree-based memory map by default.
You can also enable FOTA updates manually.
See the following sections for details of what is configured by default and what you can configure by yourself.

.. _thingy53_serialports:

Connecting to Thingy:53 for logs
********************************

Applications and samples for the Nordic Thingy:53 use a serial terminal to provide logs.
By default, the serial terminal is accessible through the USB CDC ACM class handled by application firmware.
The serial port is visible right after the Thingy:53 is connected to the host using a USB cable.
The CDC ACM baudrate is ignored, and transfer goes with USB speed.

For more information, see :ref:`thingy53_app_usb`.

.. _thingy53_app_partition_manager_config:

Memory partitioning
*******************

.. include:: ../../../includes/pm_deprecation.txt

Thingy:53 uses devicetree-based memory partitioning.
Partition Manager is disabled by default for this board, and the flash layout is defined in the board devicetree files.
The layout must stay consistent between builds so that MCUboot can perform proper image updates and clean up the settings storage partition.

Application core (``thingy53/nrf5340/cpuapp``) partitions are defined in :file:`zephyr/boards/nordic/thingy53/thingy53_nrf5340_cpuapp.dts`:

* **mcuboot** — 64 kB at the start of internal flash (``boot_partition``).
* **image-0** — 896 kB primary application slot (``slot0_partition``).
* **storage** — 64 kB settings partition at the end of internal flash (``storage_partition``).
* **image-1** — 896 kB secondary application slot on external flash (``slot1_partition`` on ``mx25r64``).
* **image-3** — 256 kB network-core image staging slot on external flash (``slot3_partition`` on ``mx25r64``).

The non-secure build target (``thingy53/nrf5340/cpuapp/ns``) uses :file:`zephyr/boards/nordic/thingy53/thingy53_nrf5340_cpuapp_ns.dts`, which splits the primary slot into secure and non-secure sub-partitions and defines a dedicated non-secure storage partition.

Network core (``thingy53/nrf5340/cpunet``) partitions are defined in :file:`zephyr/boards/nordic/thingy53/thingy53_nrf5340_cpunet.dts`:

* **b0n** — network-core bootloader (``b0n_partition``).
* **b0-provision-data** — secure boot provisioning data (``bl_storage`` / ``provision_partition``).
* **image-0** — network-core application slot (``s0_partition``).

SRAM layout for inter-core communication and network-core firmware update is provided by :file:`nrf5340_sram_partition.dtsi` and :file:`nrf5340_shared_sram_partition.dtsi`, included from the application core board devicetree.
The shared SRAM region at the start of application SRAM (``sram0_dfu_shared``) is used by MCUboot for the network-core firmware update path and must not be accessed by the application.
Trying to access this region results in an ARM fault.

Some samples and applications use devicetree overlay files to override the default partition layout for a specific use case (for example, Matter factory data).
Do not change the default board layout unless you also update the bootloader and any deployed images accordingly.

.. _thingy53_app_mcuboot_bootloader:

MCUboot bootloader
******************

MCUboot bootloader is enabled by default for Thingy:53 in the :file:`Kconfig.defconfig` file of the board.
This ensures that the sample includes the MCUboot bootloader and that an MCUboot-compatible image is generated when the sample is built.
When using the |NCS| to build the MCUboot bootloader for the Thingy:53, the configuration is applied automatically from the MCUboot repository.

The MCUboot bootloader supports serial recovery and a custom command to erase the settings storage partition.
Erasing the settings partition is needed to ensure that an application is not booted with incompatible content loaded from the settings partition.

In addition, you can set an image version, such as ``"2.3.0+0"``, using the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig option.

.. _thingy53_app_usb:

USB
***

The logs on the Thingy:53 board are provided by default using USB CDC ACM to allow access to them without additional hardware.

Most of the applications and samples for Thingy:53 use only a single instance of USB CDC ACM that works as the logger's backend.
No other USB classes are used.
These samples can share a common USB product name, vendor ID, and product ID.

If a sample supports additional USB classes or more than one instance of USB CDC ACM, it must use a dedicated product name, vendor ID, and product ID.
This sample must also enable USB composite device configuration (:kconfig:option:`CONFIG_USB_COMPOSITE_DEVICE`).

The :kconfig:option:`CONFIG_BOARD_SERIAL_BACKEND_CDC_ACM` Kconfig option (defined in the :file:`zephyr/boards/nordic/thingy53/Kconfig.defconfig` file) automatically sets the default values of USB product name, vendor ID and product ID of Thingy:53.
It also enables the USB device stack and initializes the USB device at boot.
The remote wakeup feature of a USB device is disabled by default as it requires extra action from the application side.
A single USB CDC ACM instance is automatically included in the default board's DTS configuration file (:file:`zephyr/boards/nordic/thingy53/thingy53_nrf5340_common.dtsi`).
The USB CDC instance is used to forward application logs.

If you do not want to use the USB CDC ACM as a backend for logging out of the box, you can disable it as follows:

* Disable the :kconfig:option:`CONFIG_BOARD_SERIAL_BACKEND_CDC_ACM` Kconfig option.
* If USB CDC ACM is not used for anything else, you can disable it in the application's DTS overlay file:

  .. code-block:: none

     &cdc_acm_uart {
         status = "disabled";
     };

.. _thingy53_app_antenna:

Antenna selection
*****************

The Nordic Thingy:53 has an RF front-end with two 2.4 GHz antennas:

* **ANT1** is connected to the nRF5340 through the nRF21540 RF FEM and supports TX gain of up to +20 dBm.
* **ANT2** is connected to the nRF5340 through the RF switch and supports TX output power of up to +3 dBm.

.. figure:: images/thingy53_antenna_connections.svg
   :alt: Nordic Thingy:53 - Antenna connections

   Nordic Thingy:53 - Antenna connections

The samples in the |NCS| use **ANT1** by default, with the nRF21540 gain set to +10 dBm.
You can configure the TX gain with the :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB` Kconfig option to select between +10 dBm or +20 dBm gain.
To use the **ANT2** antenna, disable the :kconfig:option:`CONFIG_MPSL_FEM` Kconfig option in the network core's image configuration.

.. note::
   Transmitting with TX output power above +10 dBM is not permitted in some regions.
   See the `Nordic Thingy:53 Regulatory notices`_ in the `Nordic Thingy:53 Hardware`_ documentation for the applicable regulations in your region before changing the configuration.

.. _thingy53_app_fota_smp:

FOTA over Bluetooth Low Energy
******************************

.. include:: /app_dev/device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_over_ble_intro_start
   :end-before: fota_upgrades_over_ble_intro_end

Thingy:53 supports network core upgrade out of the box.

.. include:: /app_dev/device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_over_ble_additional_information_start
   :end-before: fota_upgrades_over_ble_additional_information_end

.. _thingy53_app_external_flash:

External flash
**************

During a FOTA update, there might not be enough space available in internal flash storage to store the existing application and network core images as well as the incoming images, so the incoming images must be stored in external flash storage.
The Thingy:53 board automatically configures external flash storage and QSPI driver when FOTA updates are implied with the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option.

.. _thingy53_compatible_applications:

Samples and applications for Thingy:53 with FOTA out of the box
***************************************************************

The following samples and applications in the |NCS| enable FOTA for Thingy:53 by default:

* Applications:

  * :ref:`matter_weather_station_app`

* Samples:

  * :ref:`peripheral_lbs`
  * :ref:`peripheral_uart`
  * :ref:`bluetooth_mesh_light`
  * :ref:`bluetooth_mesh_light_lc`
  * :ref:`bluetooth_mesh_light_switch`
  * :ref:`bluetooth_mesh_sensor_server`
