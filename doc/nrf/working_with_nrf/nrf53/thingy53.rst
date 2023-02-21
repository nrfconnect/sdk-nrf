.. _ug_thingy53:

Developing with Thingy:53
#########################

.. contents::
   :local:
   :depth: 2

The Nordic Thingy:53 is a battery-operated prototyping platform for IoT Systems.

The Nordic Thingy:53 integrates the nRF5340 SoC that supports Bluetooth® Low Energy, IEEE 802.15.4 based protocols and Near Field Communication (NFC).
The nRF5340 is augmented with the nRF21540 RF FEM (Front-end Module) Range extender that has an integrated power amplifier (PA)/low-noise amplifier (LNA) and nPM1100 Power Management IC (PMIC) that has an integrated dual-mode buck regulator and battery charger.

You can find more information on the `Nordic Thingy:53`_ product page and in the `Nordic Thingy:53 Hardware`_ documentation.
The :ref:`nRF Connect SDK <index>` provides support for developing applications on the Nordic Thingy:53.

.. _thingy53_serialports:

Connecting to Thingy:53
***********************

Applications and samples for the Nordic Thingy:53 use a serial terminal to provide logs.
By default, the serial terminal is accessible through the USB CDC ACM class handled by application firmware.
The serial port is visible right after the Thingy:53 is connected to the host using a USB cable.
The CDC ACM baudrate is ignored, and transfer goes with USB speed.

.. _thingy53_building_pgming:

Building and programming from the source code
*********************************************

You can program the Nordic Thingy:53 by using the images obtained by building the code in the |NCS| environment.

To set up your system to be able to build a firmware image, follow the :ref:`getting_started` guide for the |NCS|.

.. _thingy53_build_pgm_targets:

Build targets
=============

The build targets of interest for Thingy:53 in the |NCS| are listed in the following table:

+--------------------------------+----------------------------------------------------------------------------------------------------------------------------------+
|Component                       |  Build target                                                                                                                    |
+================================+==================================================================================================================================+
| nRF5340 SoC - Application core |``thingy53_nrf5340_cpuapp`` for :ref:`Cortex-M Security Extensions (CMSE) disabled <app_boards_spe_nspe_cpuapp>`                  |
|                                |                                                                                                                                  |
|                                |``thingy53_nrf5340_cpuapp_ns`` for :ref:`CMSE enabled <app_boards_spe_nspe_cpuapp_ns>`                                            |
+--------------------------------+----------------------------------------------------------------------------------------------------------------------------------+
| nRF5340 SoC - Network core     |``thingy53_nrf5340_cpunet``                                                                                                       |
+--------------------------------+----------------------------------------------------------------------------------------------------------------------------------+

The |NCS| uses :ref:`ug_multi_image` for Thingy:53 by default.
When you choose ``thingy53_nrf5340_cpuapp`` or ``thingy53_nrf5340_cpuapp_ns`` as the build target when building a sample or application, you will generate firmware for both the application core and network core:

* The application core firmware consists of MCUboot bootloader and an application image.
* The network core firmware consists of network core bootloader (B0n) and application firmware of the network core.

The build process generates firmware in two formats:

* Intel Hex file (:file:`merged_domains.hex`) - Used with an external debug probe.
  The file contains bootloaders and applications for both cores.
* Binary files (:file:`app_update.bin`, :file:`net_core_app_update.bin`), containing signed application firmwares for the application and network core, respectively.
  For convenience, the binary files are bundled in :file:`dfu_application.zip`, together with a manifest that describes them.
  You can use the binary files or the combined zip archive to update application firmware for both cores, with either MCUboot serial recovery or OTA DFU using Bluetooth LE.

For more information about files generated as output of the build process, see :ref:`app_build_output_files`.

See the following sections for details regarding building and programming the firmware for Thingy:53 in various environments.
See :ref:`thingy53_app_update` for more detailed information about updating firmware on Thingy:53.

.. _thingy53_build_pgm_vscode:

Building and programming using |VSC|
====================================

|vsc_extension_instructions|

Complete the following steps after installing the |nRFVSC|:

.. |sample_path_vsc| replace:: :file:`nrf/samples/bluetooth/peripheral_lbs`

.. |vsc_sample_board_target_line| replace:: select ``thingy53_nrf5340_cpuapp`` as the target board

.. include:: ../../includes/vsc_build_and_run.txt

#. Program the sample or application:

   a. Connect the Nordic Thingy:53 to the debug out port on a 10-pin external debug probe, for example nRF5340 DK, using a 10-pin JTAG cable.
   #. Connect the external debug probe to the PC using a USB cable.
   #. Make sure that the Thingy:53 and the external debug probe are powered on.
   #. Click :guilabel:`Flash` in the :guilabel:`Actions View`.

.. _thingy53_build_pgm_command_line:

Building and programming on the command line
============================================

You must :ref:`build_environment_cli` before you start building an |NCS| project on the command line.

To build and program the source code from the command line, complete the following steps:

1. Open a command line or terminal window.
#. Go to the specific directory for the sample or application.

   For example, the directory path is :file:`ncs/nrf/applications/machine_learning` when building the source code for the :ref:`nrf_machine_learning_app` application.

#. Make sure that you have the required version of the |NCS| repository by pulling the ``sdk-nrf`` repository on GitHub as described in :ref:`dm-wf-get-ncs` and :ref:`dm-wf-update-ncs` sections.
#. Get the rest of the dependencies using west::

      west update

#. Build the sample or application code as follows:

   .. parsed-literal::
      :class: highlight

      west build -b *build_target* -d *destination_directory_name*

   The build target should be ``thingy53_nrf5340_cpuapp`` or ``thingy53_nrf5340_cpuapp_ns`` when building samples for the application core.
   The proper child image for ``thingy53_nrf5340_cpunet`` will be built automatically.
   See :ref:`thingy53_build_pgm_targets` for details.

#. Program the sample or application:

   a. Connect the Nordic Thingy:53 to the debug out port on a 10-pin external debug probe, for example nRF5340 DK, using a 10-pin JTAG cable.
   #. Connect the external debug probe to the PC using a USB cable.
   #. Make sure that the Nordic Thingy:53 and the external debug probe are powered on.
   #. Use the following command to program the sample or application to the device::

         west flash

   The device resets and runs the programmed sample or application.

.. _thingy53_app_update:

Updating firmware image
=======================

You can program the firmware on the Nordic Thingy:53 using an external debug probe and 10-pin JTAG cable, as described in :ref:`thingy53_building_pgming`, using either :ref:`Visual Studio Code <thingy53_build_pgm_vscode>` or :ref:`command line <thingy53_build_pgm_command_line>`.
You can also update applications running on both the network and application core using the built-in MCUboot bootloader and `nRF Connect Programmer`_ or the `nRF Programmer`_ app for Android and iOS.
You can also update the prebuilt application images that way.

See :ref:`thingy53_gs_updating_firmware` for details about updating firmware image.

.. _thingy53_app_update_debug:

Firmware update using external debug probe
------------------------------------------

If you are using an external debug probe, such as the nRF5340 DK, or any J-Link device supporting ARM Cortex-M33, you can program the Thingy:53 the same way as nRF5340 DK.
See :ref:`ug_nrf5340` for details.

.. _thingy53_app_update_mcuboot:

Firmware update using MCUboot bootloader
----------------------------------------

Samples and applications built for Thingy:53 include the MCUboot bootloader that you can use to update the firmware out of the box.
This method uses signed binary files :file:`app_update.bin` and :file:`net_core_app_update.bin` (or :file:`dfu_application.zip`).
You can program the precompiled firmware image using one of the following ways:

* Use the :doc:`mcuboot:index-ncs` feature and the built-in serial recovery mode of Thingy:53.
  In this scenario Thingy is connected directly to your PC through USB.
* Update the firmware over-the-air (OTA) using Bluetooth LE and the nRF Programmer mobile application for Android or iOS.
  To use this method, the application that is currently programmed on Thingy:53 must support it.
  For details, refer to :ref:`thingy53_app_fota_smp` section.
  All precompiled images support OTA using Bluetooth.

See :ref:`thingy53_gs_updating_firmware` for the detailed procedures on how to program a Thingy:53 using `nRF Connect Programmer`_ or the `nRF Programmer`_ for Android or iOS.

.. _thingy53_app_guide:

Thingy:53 application guide
***************************

The Nordic Thingy:53 does not have a built-in J-Link debug IC.
Because of that, the Thingy:53 board enables MCUboot bootloader with serial recovery support and predefined static Partition Manager memory map by default.
You can also enable FOTA updates manually.
See the following sections for details of what is configured by default and what you can configure by yourself.

.. _thingy53_app_partition_manager_config:

Partition manager configuration
===============================

The samples and applications for Nordic Thingy:53 use the :ref:`partition_manager` by default to define memory partitions.
The memory layout must stay consistent, so that MCUboot can perform proper image updates and clean up the settings storage partition.
To ensure that the partition layout does not change between builds, the sample must use a static partition layout that is consistent between all samples in the |NCS|.
The memory partitions are defined in the :file:`pm_static_thingy53_nrf5340_cpuapp.yml` and :file:`pm_static_thingy53_nrf5340_cpuapp_ns.yml` files in the :file:`zephyr/boards/arm/thingy53_nrf5340` directory.

The PCD SRAM partition is locked by the MCUboot bootloader to prevent the application from modifying the network core firmware.
Trying to access data on this partition results in an ARM fault.

The MCUboot bootloader needs a flash controller overlay for the network core image update.
The overlay is applied automatically.

.. _thingy53_app_mcuboot_bootloader:

MCUboot bootloader
==================

MCUboot bootloader is enabled by default for Thingy:53 in the :file:`Kconfig.defconfig` file of the board.
This ensures that the sample includes the MCUboot bootloader and that an MCUboot-compatible image is generated when the sample is built.
When using |NCS| to build the MCUboot bootloader for the Thingy:53, the configuration is applied automatically from the MCUboot repository.

The MCUboot bootloader supports serial recovery and a custom command to erase the settings storage partition.
Erasing the settings partition is needed to ensure that an application is not booted with incompatible content loaded from the settings partition.

In addition, you can set an image version, such as ``"2.3.0+0"``, using the :kconfig:option:`CONFIG_MCUBOOT_IMAGE_VERSION` Kconfig option.

.. _thingy53_app_usb:

USB
===

The logs on the Thingy:53 board are provided by default using USB CDC ACM to allow access to them without additional hardware.

Most of the applications and samples for Thingy:53 use only a single instance of USB CDC ACM that works as the logger's backend.
No other USB classes are used.
These samples can share a common USB product name, vendor ID, and product ID.

If a sample supports additional USB classes or more than one instance of USB CDC ACM, it must use a dedicated product name, vendor ID, and product ID.
This sample must also enable USB composite device configuration (:kconfig:option:`CONFIG_USB_COMPOSITE_DEVICE`).

The :kconfig:option:`CONFIG_BOARD_SERIAL_BACKEND_CDC_ACM` Kconfig option (defined in the :file:`zephyr/boards/arm/thingy53_nrf5340/Kconfig.defconfig` file) automatically sets the default values of USB product name, vendor ID and product ID of Thingy:53.
It also enables the USB device stack and initializes the USB device at boot.
The remote wakeup feature of a USB device is disabled by default as it requires extra action from the application side.
A single USB CDC ACM instance is automatically included in the default board's DTS configuration file (:file:`zephyr/boards/arm/thingy53_nrf5340/thingy53_nrf5340_common.dts`).
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
=================

The Nordic Thingy:53 has an RF front-end with two 2.4 GHz antennas:

* **ANT1** is connected to the nRF5340 through the nRF21540 RF FEM and supports TX gain of up to +20 dBm.
* **ANT2** is connected to the nRF5340 through the RF switch and supports TX output power of up to +3 dBm.

.. figure:: images/thingy53_antenna_connections.svg
   :alt: Nordic Thingy:53 - Antenna connections

   Nordic Thingy:53 - Antenna connections

The samples in the |NCS| use **ANT1** by default, with the nRF21540 gain set to +10 dBm.
You can configure the TX gain with the :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB` Kconfig option to select between +10 dBm or +20 dBm gain.
To use the **ANT2** antenna, disable the :kconfig:option:`CONFIG_MPSL_FEM` Kconfig option in the network core's child image configuration.

.. note::
   Transmitting with TX output power above +10 dBM is not permitted in some regions.
   See the `Nordic Thingy:53 Regulatory notices`_ in the `Nordic Thingy:53 Hardware`_ documentation for the applicable regulations in your region before changing the configuration.

.. _thingy53_app_fota_smp:

.. include:: ../nrf52/developing.rst
   :start-after: fota_upgrades_over_ble_intro_start
   :end-before: fota_upgrades_over_ble_intro_end

Bluetooth buffers configuration introduced by the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU_SPEEDUP` Kconfig option is also automatically applied to the network core child image by the dedicated overlay file.
Thingy:53 supports network core upgrade out of the box.

.. include:: ../nrf52/developing.rst
   :start-after: fota_upgrades_over_ble_additional_information_start
   :end-before: fota_upgrades_over_ble_additional_information_end


.. _thingy53_app_external_flash:

External flash
==============

During a FOTA update, there might not be enough space available in internal flash storage to store the existing application and network core images as well as the incoming images, so the incoming images must be stored in external flash storage.
The Thingy:53 board automatically configures external flash storage and QSPI driver when FOTA updates are implied with the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU` Kconfig option.

.. _thingy53_compatible_applications:

Samples and applications for Thingy:53 with FOTA out of the box
===============================================================

The following samples and applications in the |NCS| enable FOTA for Thingy:53 by default:

* Applications:

  * :ref:`matter_weather_station_app`
  * :ref:`nrf_machine_learning_app`

* Samples:

  * :ref:`peripheral_lbs`
  * :ref:`peripheral_uart`
  * :ref:`bluetooth_mesh_light`
  * :ref:`bluetooth_mesh_light_lc`
  * :ref:`bluetooth_mesh_light_switch`
  * :ref:`bluetooth_mesh_sensor_server`
