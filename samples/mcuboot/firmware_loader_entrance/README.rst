.. _firmware_loader_entrance:

Firmware loader entrance
########################

.. contents::
   :local:
   :depth: 2

The firmware loader entrance sample demonstrates how to enter the firmware loader application using MCUboot's firmware updater mode with boot mode entrance.

This sample shows how to configure a minimal application that can trigger entry into a dedicated firmware loader application for performing firmware updates over Bluetooth® Low Energy using MCUmgr.
This approach allows you to have a larger main application while keeping firmware update functionality separate in a dedicated loader application.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

This sample demonstrates MCUboot's firmware updater mode, which enables a configuration with three components:

.. list-table:: Components in firmware updater mode
   :header-rows: 1

   * - Component
     - Description
   * - **MCUboot bootloader**
     - Handles boot decisions and image management
   * - **Main application**
     - Your primary application (not intended for firmware updates)
   * - **Firmware loader application**
     - A dedicated application for loading firmware updates over Bluetooth LE using MCUmgr

The benefit of this configuration is having a dedicated application for loading firmware updates, which allows the main application to be larger in comparison to symmetric size dual-bank mode updates.
This helps on devices with limited flash or RAM.

The sample application itself is a minimal Bluetooth LE peripheral that advertises using the MCUmgr service UUID.
When configured with GPIO entrance mode, MCUboot can boot into the firmware loader application based on GPIO state at boot time.
The firmware loader application (configured as ``BLE_MCUMGR``) provides MCUmgr over Bluetooth LE transport for performing firmware updates.

The sample uses the boot mode entrance method, in which MCUboot checks the boot mode using the retention subsystem to determine whether to boot the main application or the firmware loader application.

Configuration
*************

|config|

Additional configuration
========================

Check and configure the following Kconfig options:

.. list-table:: Key configuration options
   :header-rows: 1

   * - Option
     - Description
   * - :kconfig:option:`CONFIG_BOOT_FIRMWARE_LOADER_ENTRANCE_GPIO`
     - Enables GPIO-based entrance to firmware loader mode
   * - :kconfig:option:`CONFIG_BOOT_FIRMWARE_LOADER_NO_APPLICATION`
     - Allows booting firmware loader when no valid main application is present
   * - :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT`
     - Enables Bluetooth LE transport for MCUmgr
   * - :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_REASSEMBLY`
     - Enables packet reassembly for large MCUmgr packets over Bluetooth LE
   * - :kconfig:option:`CONFIG_MCUMGR_GRP_OS`
     - Enables OS management group with bootloader information and boot mode support
   * - :kconfig:option:`CONFIG_MCUMGR_GRP_OS_RESET_BOOT_MODE`
     - Enables reset with boot mode support for entering firmware loader

Sysbuild configuration
======================

The sample uses sysbuild to configure MCUboot and the firmware loader image.
The following options are set in :file:`sysbuild.conf`:

.. list-table:: Sysbuild configuration options
   :header-rows: 1

   * - Option
     - Description
   * - :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER`
     - Enables MCUboot firmware updater mode.
   * - :kconfig:option:`SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER_BOOT_MODE_ENTRANCE`
     - Enables boot mode entrance method.
   * - :kconfig:option:`SB_CONFIG_FIRMWARE_LOADER_IMAGE_BLE_MCUMGR`
     - Selects the BLE MCUmgr sample as the firmware loader image.

Partition Manager configuration
===============================

The sample uses static Partition Manager files to define the memory layout:

.. list-table:: Partition Manager static files
   :header-rows: 1

   * - File
     - Description
   * - :file:`pm_static_nrf52840dk_nrf52840.yml`
     - Partition layout for nRF52840 DK
   * - :file:`pm_static_nrf54l15dk_nrf54l15_cpuapp.yml`
     - Partition layout for nRF54L15 DK

These files define the following partitions:

.. list-table:: Key partitions
   :header-rows: 1

   * - Partition
     - Description
   * - ``mcuboot``
     - MCUboot bootloader partition
   * - ``app``
     - Main application partition
   * - ``firmware_loader``
     - Firmware loader application partition (located in ``mcuboot_secondary``)
   * - ``settings_storage``
     - Settings storage partition

Building and running
********************

.. |sample path| replace:: :file:`samples/mcuboot/firmware_loader_entrance`

.. include:: /includes/build_and_run.txt

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Build and program the sample to your development kit.
#. Observe the terminal output.

The sample starts advertising Bluetooth LE with the MCUmgr service UUID.
You will see "Advertising successfully started" in the terminal output.

To test entering the firmware loader application:

#. Configure the GPIO entrance method (if using GPIO entrance).
#. Reset the device with the GPIO configured to enter firmware loader mode.

MCUboot will boot into the firmware loader application instead of the main application.
Then, the firmware loader application starts advertising Bluetooth LE and can be used to perform firmware updates using MCUmgr.

Sample output
=============

The following output is logged on the terminal:

.. code-block:: console

   [00:00:00.000,000] <inf> firmware_entrance_loader: Bluetooth initialized
   [00:00:00.100,000] <inf> firmware_entrance_loader: Advertising successfully started, device name: firmware_loader_entrance

Dependencies
************

This sample uses the following libraries:

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/zephyr/bluetooth/bluetooth.h`
  * :file:`include/zephyr/bluetooth/conn.h`
  * :file:`include/zephyr/bluetooth/gatt.h`

* :ref:`zephyr:kernel_api`:

  * :file:`include/zephyr/kernel.h`

* :file:`include/zephyr/mgmt/mcumgr/transport/smp_bt.h`
* :file:`include/zephyr/settings/settings.h`
* :file:`include/zephyr/logging/log.h`
