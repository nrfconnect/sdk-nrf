.. _ug_thingy91:

.. _thingy91_ug_intro:

Developing with Thingy:91
#########################

.. contents::
   :local:
   :depth: 2


Nordic Thingy:91 is a battery-operated prototyping platform for cellular IoT systems, designed especially for asset tracking applications and environmental monitoring.
Thingy:91 integrates an nRF9160 SiP that supports LTE-M, NB-IoT, and Global Navigation Satellite System (GNSS) and an nRF52840 SoC that supports Bluetooth® Low Energy, Near Field Communication (NFC) and USB.

You can find more information on the product in the `Thingy:91 product page`_ and in the `Nordic Thingy:91 User Guide`_.
The |NCS| provides support for developing applications on the Thingy:91.
If you are not familiar with the |NCS|, see :ref:`installation` and :ref:`configuration_and_build` documentation to install the |NCS| and learn more about its development environment.

This guide gives you more information on the various aspects of Thingy:91.

.. _thingy91_serialports:

Connecting to Thingy:91
***********************

You can connect to Thingy:91 wirelessly (using the `nRF Toolbox`_ app) or over a serial connection (using `nRF Connect Serial Terminal`_, `Cellular Monitor`_, or a serial terminal).

Using nRF Toolbox
=================

To connect to your Thingy:91 wirelessly, you need to meet the following prerequisites:

* The :ref:`connectivity_bridge` installed on your Thingy:91.
* The :ref:`nus_service_readme` enabled.

  .. note::
     By default, the Bluetooth LE interface is off, as the connection is not encrypted or authenticated.
     To turn it on at runtime, set the appropriate option in the :file:`Config.txt` file located on the USB Mass storage Device.

Using a serial terminal
=======================

If you prefer to use a standard serial terminal, the baud rate has to be specified manually.

Thingy:91 uses the following UART baud rate configuration:

.. list-table::
   :header-rows: 1
   :align: center

   * - UART Interface
     - Baud Rate
   * - UART_0
     - 115200
   * - UART_1
     - 1000000

Using nRF Connect Serial Terminal
=================================

You can use the `nRF Connect Serial Terminal`_ application to get debug output and send AT commands to the Thingy:91.
In the case of nRF Connect Serial Terminal or Cellular Monitor, the baud rate for the communication is set automatically.

To connect to the Thingy:91 using the nRF Connect Serial Terminal app, complete the following steps:

1. Open `nRF Connect for Desktop`_.
#. Find Serial Terminal in the list of applications and click :guilabel:`Install`.
#. Connect the Thingy:91 to a computer with a micro-Universal Serial Bus (USB) cable.
#. Make sure that the Thingy:91 is powered on.
#. Launch the Serial Terminal application.
#. In the navigation bar, click :guilabel:`SELECT DEVICE`.
   A drop-down menu appears.
#. In the menu, select Thingy:91.
#. In the terminal window, send an AT command to the modem.
   If the connection is working, the modem responds with OK.

The terminal view shows all of the Asset Tracker v2 debug output as well as the AT commands and their results.
For information on the available AT commands, see the `nRF9160 AT Commands Reference Guide`_.

Operating modes
***************

Thingy:91 contains RGB indicator LEDs, which indicate the operating state of the device as described in the :ref:`LED indication <led_indication>` section of the :ref:`asset_tracker_v2_ui_module`.

GNSS
****

Thingy:91 has a GNSS receiver, which, if activated, allows the device to be located globally using GNSS signals.
In :ref:`asset_tracker_v2`, GNSS is activated by default.

LTE Band Lock
*************

The modem within Thingy:91 can be configured to use specific LTE bands by using the band lock AT command.
See :ref:`nrf9160_ug_band_lock` and the `band lock section in the AT Commands reference document`_ for additional information.
The preprogrammed firmware configures the modem to use the bands currently certified on the Thingy:91 hardware.
When building the firmware, you can configure which bands must be enabled.

LTE-M / NB-IoT switching
************************

Thingy:91 has a multimode modem, which enables it to support automatic switching between LTE-M and NB-IoT.
A built-in parameter in the Thingy:91 firmware determines whether the modem first attempts to connect in LTE-M or NB-IoT mode.
If the modem fails to connect using this preferred mode within the default timeout period (10 minutes), the modem switches to the other mode.

.. _programming_thingy:

Updating the Thingy:91 firmware using Programmer
************************************************

You can use the Programmer app from `nRF Connect for Desktop`_ to:

* :ref:`Update the Connectivity bridge application firmware in the nRF52840 SoC <updating_the conn_bridge_52840>`.
* :ref:`Update the modem firmware on the nRF9160 SoC <update_modem_fw_nrf9160>`.
* :ref:`Program the application firmware on the nRF9160 SiP <update_nrf9160_application>`.

These operations can be done through USB using MCUboot, or through an external debug probe.
When developing with your Thingy:91, it is recommended to use an external debug probe.

.. note::
   The external debug probe must support Arm Cortex-M33, such as the nRF9160 DK.
   You need a 10-pin 2x5 socket-socket 1.27 mm IDC (:term:`Serial Wire Debug (SWD)`) JTAG cable to connect to the external debug probe.

Download and extract the latest application and modem firmware from the `Thingy:91 Downloads`_ page.

The downloaded ZIP archive contains the following firmware:

Application firmware
  The :file:`img_app_bl` folder contains full firmware images for different applications.
  The guides for programming through an external debug probe in this section use the images in this folder.

Application firmware for Device Firmware Update (DFU)
  The images in the :file:`img_fota_dfu_bin` and :file:`img_fota_dfu_hex` folders contain firmware images for DFU.
  The guides for programming through USB in this section use the images in the :file:`img_fota_dfu_hex` folder.

Modem firmware
  The modem firmware is in a ZIP archive instead of a folder.
  The archive is named :file:`mfw_nrf9160_` followed by the firmware version number.
  Do not unzip this file.

The :file:`CONTENTS.txt` file in the extracted folder contains the location and names of the different firmware images.

The instructions in this section show you how to program the :ref:`connectivity_bridge` and :ref:`asset_tracker_v2` applications, as well as the modem firmware.
Connectivity bridge provides bridge functionality for the hardware, and Asset Tracker v2 simulates sensor data and transmits it to Nordic Semiconductor's cloud solution, `nRF Cloud`_.

The data is transmitted using either LTE-M or NB-IoT.
Asset Tracker v2 first attempts to use LTE-M, then NB-IoT.
Check with your SIM card provider for the mode they support at your location.
For the iBasis SIM card provided with the Thingy:91, see `iBasis IoT network coverage`_.

.. tip::
   For a more compact nRF Cloud firmware application, you can build and install the :ref:`nrf_cloud_multi_service` sample.
   See the :ref:`building_pgming` section for more information.

.. note::
   To update the Thingy:91 through USB, the nRF9160 SiP and nRF52840 SoC bootloaders must be factory-compatible.
   The bootloaders might not be factory-compatible if the nRF9160 SiP or nRF52840 SoC has been updated with an external debug probe.
   To restore the bootloaders, program the nRF9160 SiP or nRF52840 SoC with factory-compatible Thingy:91 firmware files through an external debug probe.

.. note::
   You can also use these precompiled firmware image files for restoring the firmware to its initial image.

.. _updating_the conn_bridge_52840:

Updating the firmware in the nRF52840 SoC
=========================================

.. tabs::

   .. group-tab:: Through USB

      To update the firmware, complete the following steps:

      1. Open `nRF Connect for Desktop`_ and launch the Programmer app.
      #. Scroll down in the menu on the left and make sure **Enable MCUboot** is selected.

         .. figure:: images/programmer_enable_mcuboot.png
            :alt: Programmer - Enable MCUboot

            Programmer - Enable MCUboot

      #. Switch off the Thingy:91.
      #. Press **SW4** while switching **SW1** to the **ON** position.

         .. figure:: images/thingy91_sw1_sw4.svg
            :alt: thingy91_sw1_sw4

            Thingy:91 - SW1 SW4 switch

      #. In the Programmer navigation bar, click :guilabel:`SELECT DEVICE`.
         A drop-down menu appears.

         .. figure:: images/programmer_select_device2.png
            :alt: Programmer - Select device

            Programmer - Select device

      #. In the menu, select the entry corresponding to your device (:guilabel:`MCUBOOT`).

         .. note::
            The device entry might not be the same in all cases and can vary depending on the application version and the operating system.

      #. In the menu on the left, click :guilabel:`Add file` in the **FILE** section, and select :guilabel:`Browse`.
         A file explorer window appears.

         .. figure:: images/programmer_add_file2.png
            :alt: Programmer - Add file

            Programmer - Add file

      #. Navigate to where you extracted the firmware.

      #. Open the folder :file:`img_fota_dfu_hex` that contains the HEX files for updating over USB.
         See the :file:`CONTENTS.txt` file for information on which file you need.

      #. Select the Connectivity bridge firmware file.

      #. Click :guilabel:`Open`.

      #. Scroll down in the menu on the left to the **DEVICE** section and click :guilabel:`Write`.

         .. figure:: images/programmer_hex_write1.png
            :alt: Programmer - Writing of HEX files

            Programmer - Writing of HEX files

         The **MCUboot DFU** window appears.

         .. figure:: images/thingy91_mcuboot_dfu.png
            :alt: Programmer - MCUboot DFU

            Programmer - MCUboot DFU

      #. In the **MCUboot DFU** window, click :guilabel:`Write`.
         When the update is complete, a "Completed successfully" message appears.
      #. Scroll up in the menu on the left to the **FILE** section and click :guilabel:`Clear files`.

   .. group-tab:: Through external debug probe

      To update the firmware using the nRF9160 DK as the external debug probe, complete the following steps:

      1. Open `nRF Connect for Desktop`_ and launch the Programmer app.

      .. _prepare_hw_ext_dp:

      2. Prepare the hardware:

         a. Connect the Thingy:91 to the debug out port on a 10-pin external debug probe using a JTAG cable.

            .. figure:: images/programmer_thingy91_connect_dk_swd_vddio.svg
               :alt: Thingy:91 - Connecting the external debug probe

               Thingy:91 - Connecting the external debug probe

            .. note::
               When using nRF9160 DK as the debug probe, make sure that VDD_IO (SW11) is set to 1.8 V on the nRF9160 DK.

         #. Make sure that the Thingy:91 and the external debug probe are powered on.

            .. note::
               Do not unplug or power off the devices during this process.

         #. Connect the external debug probe to the computer with a micro-USB cable.

            In the Programmer navigation bar, :guilabel:`No devices available` changes to :guilabel:`SELECT DEVICE`.

            .. figure:: ../nrf52/images/programmer_select_device1.png
               :alt: Programmer - Select device

               Programmer - SELECT DEVICE
         #. Click :guilabel:`SELECT DEVICE` and select the appropriate debug probe entry from the drop-down list.

            Select nRF9160 DK from the list.

            .. figure:: images/programmer_com_ports.png
               :alt: Programmer - nRF9160 DK

               Programmer - nRF9160 DK

            The button text changes to the SEGGER ID of the selected device, and the **Device memory layout** section indicates that the device is connected.

      #. Set the SWD selection switch **SW2** to **nRF52** on the Thingy:91.
         See `SWD Select`_ for more information on the switch.

      #. In the menu on the left, click :guilabel:`Add file` in the **FILE** section, and select :guilabel:`Browse`.
         A file explorer window appears.

         .. figure:: images/programmer_add_file1.png
            :alt: Programmer - Add file

            Programmer - Add file

      #. Navigate to where you extracted the firmware.

      #. Open the folder :file:`img_app_bl` that contains the HEX files for flashing with a debugger.
         See the :file:`CONTENTS.txt` file for information on which file you need.

      #. Select the Connectivity bridge firmware file.
      #. Click :guilabel:`Open`.
      #. Scroll down in the menu on the left to the **DEVICE** section and click :guilabel:`Erase & write`.
         The update is completed when the animation in Programmer's **Device memory layout** window ends.

         .. figure:: images/programmer_ext_debug_hex_write.png
            :alt: Programming using an external debug probe

            Programming using an external debug probe

      #. Scroll up in the menu on the left to the **FILE** section and click :guilabel:`Clear files`.

.. _update_modem_fw_nrf9160:

Update the modem firmware on the nRF9160 SiP
============================================

.. tabs::

   .. group-tab:: Through USB

     To update the modem firmware using USB, complete the following steps:

      1. Open `nRF Connect for Desktop`_ and launch the Programmer app if you do not have it open already.
      #. Make sure that **Enable MCUboot** is selected.
      #. Switch off the Thingy:91.
      #. Press **SW3** while switching **SW1** to the **ON** position.

         .. figure:: images/thingy91_sw1_sw3.svg
            :alt: Thingy:91 - SW1 SW3 switch

            Thingy:91 - SW1 SW3 switch

      #. In the menu, select Thingy:91.

      #. In the menu on the left, click :guilabel:`Add file` in the **FILE** section, and select :guilabel:`Browse`.
         A file explorer window appears.

         .. figure:: images/programmer_add_file.png
            :alt: Programmer - Add file

            Programmer - Add file

      #. Navigate to where you extracted the firmware.
      #. Find the modem firmware zip file with the name similar to :file:`mfw_nrf9160_*.zip` and the number of the latest version.

         .. note::
            Do not extract the modem firmware zip file.

      #. Select the zip file and click :guilabel:`Open`.
      #. In the Programmer app, scroll down in the menu on the left to the **DEVICE** section and click :guilabel:`Write`.

         .. figure:: images/programmer_usb_update_modem.png
            :alt: Programmer - Update modem

            Programmer - Update modem

         The **Modem DFU via MCUboot** window appears.

         .. figure:: images/thingy91_modemdfu_mcuboot.png
            :alt: Programmer - Modem DFU via MCUboot

            Programmer - Modem DFU via MCUboot

      #. In the **Modem DFU via MCUboot** window, click :guilabel:`Write`.
         When the update is complete, a **Completed successfully** message appears.

   .. group-tab:: Through external debug probe

      To update the modem firmware using an external debug probe, complete the following steps:

      1. Open `nRF Connect for Desktop`_ and launch the Programmer app and :ref:`prepare the hardware <prepare_hw_ext_dp>` if you have not done it already.
      #. Set the SWD selection switch **SW2** to **nRF91** on the Thingy:91.

      #. In the menu on the left, click :guilabel:`Add file` in the **FILE** section, and select :guilabel:`Browse`.
         A file explorer window appears.

         .. figure:: images/programmer_add_file1.png
            :alt: Programmer - Add file

            Programmer - Add file

      #. Navigate to where you extracted the firmware.
      #. Find the modem firmware zip file with the name similar to :file:`mfw_nrf9160_*.zip` and the number of the latest version and click :guilabel:`Open`.

         .. note::
            Do not extract the modem firmware zip file.

      #. Select the zip file and click :guilabel:`Open`.
      #. In the Programmer app, scroll down in the menu on the left to the **DEVICE** section and click :guilabel:`Write`.

         .. figure:: images/programmer_ext_debug_update_modem.png
            :alt: Programmer - Update modem

            Programmer - Update modem

         The **Modem DFU** window appears.

         .. figure:: images/programmer_modemdfu.png
            :alt: Programmer - Modem DFU

            Programmer - Modem DFU

      #. In the **Modem DFU** window, click :guilabel:`Write`.
         When the update is complete, a "Completed successfully" message appears.

         .. note::
            Before trying to update the modem again, click the :guilabel:`Erase all` button. In this case, the contents of the flash memory are deleted and the applications must be reprogrammed.

.. _update_nrf9160_application:

Program the nRF9160 SiP application
===================================

.. tabs::

   .. group-tab:: Through USB

      To program the application firmware using USB, complete the following steps:

      1. Open `nRF Connect for Desktop`_ and launch the Programmer app if you have not done already.
      #. Make sure that **Enable MCUboot** is selected.
      #. Switch off the Thingy:91.
      #. Press **SW3** while switching **SW1** to the **ON** position.

         .. figure:: images/thingy91_sw1_sw3.svg
            :alt: Thingy:91 - SW1 SW3 switch

            Thingy:91 - SW1 SW3 switch

      #. In the Programmer navigation bar, click :guilabel:`SELECT DEVICE`.
         A drop-down menu appears.

         .. figure:: images/programmer_select_device.png
            :alt: Programmer - Select device

            Programmer - Select device

      #. In the menu, select Thingy:91.

      #. In the menu on the left, click :guilabel:`Add file` in the **FILE** section, and select :guilabel:`Browse`.
         A file explorer window appears.

         .. figure:: images/programmer_add_file.png
            :alt: Programmer - Add file

            Programmer - Add file

      #. Navigate to where you extracted the firmware.

      #. Open the folder :file:`img_fota_dfu_hex` that contains the HEX files for updating over USB.
         See the :file:`CONTENTS.txt` file for information on which file you need.

      #. Select the appropriate Asset Tracker v2 firmware file.

         .. note::

            If you are connecting over NB-IoT and your operator does not support extended Protocol Configuration Options (ePCO), select the file that has legacy Protocol Configuration Options (PCO) mode enabled.

      #. Click :guilabel:`Open`.

      #. Scroll down in the menu on the left to the **DEVICE** section and click :guilabel:`Write`.

         .. figure:: images/programmer_hex_write.png
            :alt: Programmer - Writing of HEX files

            Programmer - Writing of HEX files

         The **MCUboot DFU** window appears.

         .. figure:: images/thingy91_mcuboot_dfu1.png
            :alt: Programmer - MCUboot DFU

            Programmer - MCUboot DFU

      #. In the **MCUboot DFU** window, click :guilabel:`Write`.
         When the update is complete, a **Completed successfully** message appears.
      #. Scroll up in the menu on the left to the **FILE** section and click :guilabel:`Clear files`.

   .. group-tab:: Through external debug probe

      To program the application firmware using an external debug probe, complete the following steps:

      1. Open `nRF Connect for Desktop`_ and launch the Programmer app and :ref:`prepare the hardware <prepare_hw_ext_dp>` if you have not done it already.
      #. Make sure the SWD selection switch **SW2** is set to **nRF91** on the Thingy:91.

      #. In the menu on the left, click :guilabel:`Add file` in the **FILE** section, and select :guilabel:`Browse`.
         A file explorer window appears.

         .. figure:: images/programmer_add_file1.png
            :alt: Programmer - Add file

            Programmer - Add file

      #. Navigate to where you extracted the firmware.

      #. Open the folder :file:`img_app_bl` that contains the HEX files for updating using a debugger.
         See the :file:`CONTENTS.txt` file for information on which file you need.

      #. Select the appropriate Asset Tracker v2 firmware file.

         .. note::

            If you are connecting over NB-IoT and your operator does not support extended Protocol Configuration Options (ePCO), select the file that has legacy Protocol Configuration Options (PCO) mode enabled.

      #. Click :guilabel:`Open`.
      #. Scroll down in the menu on the left to the **DEVICE** section and click :guilabel:`Erase & write`.
         The update is completed when the animation in Programmer's **Device memory layout** window ends.

         .. figure:: images/programmer_ext_debug_hex_write.png
            :alt: Programming using an external debug probe

            Programming using an external debug probe

      #. Scroll up in the menu on the left to the **FILE** section and click :guilabel:`Clear files`.

.. _building_pgming:

Building and programming from the source code
*********************************************

You can also program the Thingy:91 by using the images obtained by building the code in an |NCS| environment.

To set up your system to be able to build a compatible firmware image, follow the :ref:`installation` guide for the |NCS| and read the :ref:`configuration_and_build` documentation.
The build targets of interest for Thingy:91 in |NCS| are as follows:

+---------------+---------------------------------------------------+
|Component      |  Build target                                     |
+===============+===================================================+
|nRF9160 SiP    |``thingy91_nrf9160_ns``                            |
+---------------+---------------------------------------------------+
|nRF52840 SoC   |``thingy91_nrf52840``                              |
+---------------+---------------------------------------------------+

You must use the build target ``thingy91_nrf9160_ns`` when building the application code for the nRF9160 SiP and the build target ``thingy91_nrf52840`` when building the application code for the onboard nRF52840 SoC.

.. note::

   * In |NCS| releases before v1.3.0, these build targets were named ``nrf9160_pca20035``, ``nrf9160_pca20035ns``, and ``nrf52840_pca20035``.
   * In |NCS| releases ranging from v1.3.0 to v1.6.1, the build target ``thingy91_nrf9160_ns`` was named ``thingy91_nrf9160ns``.

.. note::

   LTE/GNSS features can only be used with :ref:`Cortex-M Security Extensions enabled <app_boards_spe_nspe_cpuapp_ns>` (``_ns`` build target).

The table below shows the different types of build files that are generated and the different scenarios in which they are used:

+-----------------------+----------------------------------------+----------------------------------------------------------------+
| File                  | File format                            | Programming scenario                                           |
+=======================+========================================+================================================================+
|:file:`merged.hex`     | Full image, HEX format                 | Using an external debug probe and nRF Connect Programmer.      |
+-----------------------+----------------------------------------+----------------------------------------------------------------+
|:file:`app_signed.hex` | MCUboot compatible image, HEX format   | Using the built-in bootloader and nRF Connect Programmer.      |
+-----------------------+----------------------------------------+----------------------------------------------------------------+
|:file:`app_update.bin` | MCUboot compatible image, binary format|* Using the built-in bootloader and mcumgr command line tool.   |
|                       |                                        |* For FOTA updates.                                             |
+-----------------------+----------------------------------------+----------------------------------------------------------------+

For an overview of different types of build files in the |NCS|, see :ref:`app_build_output_files`.

There are multiple methods of programming a sample or application onto a Thingy:91.
It is recommended to use an external debug probe to program the Thingy:91.

.. note::

   If you do not have an external debug probe available to program the Thingy:91, you can directly program by :ref:`using the USB (MCUboot) method and nRF Connect Programmer <programming_thingy>`.
   In this scenario, use the :file:`app_signed.hex` firmware image file.

.. note::

   While building applications for Thingy:91, the build system changes the signing algorithm of MCUboot so that it uses the default RSA Keys.
   This is to ensure backward compatibility with the MCUboot versions that precede the |NCS| v1.4.0.
   The default RSA keys must only be used for development.
   In a final product, you must use your own, secret keys.
   See :ref:`ug_fw_update_development_keys` for more information.

.. _build_pgm_vsc:

Building and programming using |VSC|
====================================

Complete the following steps to build and program using the |nRFVSC|:

.. |sample_path_vsc| replace:: :file:`ncs/nrf/applications/asset_tracker_v2`

.. |vsc_sample_board_target_line| replace:: you must use the build target ``thingy91_nrf9160_ns`` when building the application code for the nRF9160 SiP and the build target ``thingy91_nrf52840`` when building the application code for the onboard nRF52840 SoC

.. include:: ../../../includes/vsc_build_and_run.txt

3. Program the application:

.. prog_extdebugprobe_start
..

   a. Set the Thingy:91 SWD selection switch (**SW2**) to **nRF91** or **nRF52** depending on whether you want to program the nRF9160 SiP or the nRF52840 SoC component.
   #. Connect the Thingy:91 to the debug out port on a 10-pin external debug probe, for example, nRF9160 DK (Development Kit), using a 10-pin JTAG cable.

      .. note::

         If you are using nRF9160 DK as the debug probe, make sure that **VDD_IO (SW11)** is set to 1.8 V on the nRF9160 DK.

   #. Connect the external debug probe to the PC using a USB cable.
   #. Make sure that the Thingy:91 and the external debug probe are powered on.

.. prog_extdebugprobe_end
..

   e. In |nRFVSC|, click the :guilabel:`Flash` option in the **Actions View**.

      If you have multiple boards connected, you are prompted to pick a device at the top of the screen.

      A small notification banner appears in the bottom-right corner of |VSC| to display the progress and confirm when the flash is complete.

.. _build_pgm_cmdline:

Building and programming on the command line
============================================

.. |cmd_folder_path| replace:: on the nRF9160 SiP component and ``ncs/nrf/applications/connectivity_bridge`` when building the source code for the :ref:`connectivity_bridge` application on the nRF52840 SoC component

.. |cmd_build_target| replace:: ``thingy91_nrf9160_ns`` if building for the nRF9160 SiP component and ``thingy91_nrf52840`` if building for the nRF52840 SoC component

.. include:: ../../../includes/cmd_build_and_run.txt

6. Program the application:

.. include:: thingy91.rst
   :start-after: prog_extdebugprobe_start
   :end-before: prog_extdebugprobe_end
..

   e. Program the sample or application to the device using the following command:

      .. code-block:: console

          west flash

      The device resets and runs the programmed sample or application.

.. _thingy91_partition_layout:

Partition layout
================

When building firmware on the Thingy:91 board, a static partition layout matching the factory layout is used.
This ensures that programming firmware through USB works.
In this case, the MCUboot bootloader will not be updated.
So, to maintain compatibility, it is important that the image partitions do not get moved.
When programming the Thingy:91 through an external debug probe, all partitions, including MCUboot, are programmed.
This enables the possibility of using an updated bootloader or defining an application-specific partition layout.

Configure the partition layout using one of the following configuration options:

* :kconfig:option:`CONFIG_THINGY91_STATIC_PARTITIONS_FACTORY` - This option is the default Thingy:91 partition layout used in the factory firmware.
  This ensures firmware updates are compatible with Thingy:91 when programming firmware through USB.
* :kconfig:option:`CONFIG_THINGY91_STATIC_PARTITIONS_SECURE_BOOT` - This option is similar to the factory partition layout, but also has space for the immutable bootloader and two MCUboot slots.
  A debugger is needed to program Thingy:91 for the first time.
  This is an :ref:`experimental <software_maturity>` feature.
* :kconfig:option:`CONFIG_THINGY91_STATIC_PARTITIONS_LWM2M_CARRIER` - This option uses a partition layout, including a storage partition needed for the :ref:`liblwm2m_carrier_readme` library.
