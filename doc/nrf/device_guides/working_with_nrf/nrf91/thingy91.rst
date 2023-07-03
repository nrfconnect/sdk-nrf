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

You can connect to Thingy:91 wirelessly (using the `nRF Toolbox`_ app) or over a serial connection (using `LTE Link Monitor`_, `Trace Collector`_, or a serial terminal).

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

Using LTE Link Monitor
======================

You can use the `LTE Link Monitor`_ application to get debug output and send AT commands to the Thingy:91.
In the case of LTE Link Monitor or Trace Collector, the baud rate for the communication is set automatically.

To connect to the Thingy:91 using LTE Link Monitor, complete the following steps:

1. Open `nRF Connect for Desktop`_.
#. Find LTE Link Monitor in the list of applications and click :guilabel:`Install`.
#. Connect the Thingy:91 to a computer with a micro-Universal Serial Bus (USB) cable.
#. Make sure that the Thingy:91 is powered on.
#. Launch the LTE Link Monitor application.
#. In the navigation bar, click :guilabel:`SELECT DEVICE`.
   A drop-down menu appears.
#. In the menu, select Thingy:91.
#. In the LTE Link Monitor terminal, send an AT command to the modem.
   If the connection is working, the modem responds with OK.

The terminal view shows all of the Asset Tracker v2 debug output as well as the AT commands and their results.
For information on the available AT commands, see `nRF91 AT Commands Reference Guide <AT Commands Reference Guide_>`_.


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

#. Program the application:

.. include:: thingy91.rst
   :start-after: prog_extdebugprobe_start
   :end-before: prog_extdebugprobe_end
..

   e. Program the sample or application to the device using the following command:

      .. code-block:: console

          west flash

      The device resets and runs the programmed sample or application.
