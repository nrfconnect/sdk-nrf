.. _thingy53_building_pgming:

Building and programming with Thingy:53
#######################################

.. contents::
   :local:
   :depth: 2

You can program the Nordic Thingy:53 by using the images obtained by building the code in the |NCS| environment.

To set up your system to be able to build a firmware image, follow the :ref:`installation` guide for the |NCS|.

.. _thingy53_build_pgm_targets:

Board targets
*************

The board targets of interest for Thingy:53 in the |NCS| are listed in the following table:

+--------------------------------+----------------------------------------------------------------------------------------------------------------------------------+
|Component                       |  Board target                                                                                                                    |
+================================+==================================================================================================================================+
| nRF5340 SoC - Application core |``thingy53/nrf5340/cpuapp`` for :ref:`Cortex-M Security Extensions (CMSE) disabled <app_boards_spe_nspe_cpuapp>`                  |
|                                |                                                                                                                                  |
|                                |``thingy53/nrf5340/cpuapp/ns`` for :ref:`CMSE enabled <app_boards_spe_nspe_cpuapp_ns>`                                            |
+--------------------------------+----------------------------------------------------------------------------------------------------------------------------------+
| nRF5340 SoC - Network core     |``thingy53/nrf5340/cpunet``                                                                                                       |
+--------------------------------+----------------------------------------------------------------------------------------------------------------------------------+

The |NCS| uses :ref:`configuration_system_overview_sysbuild` by default.
When you choose ``thingy53/nrf5340/cpuapp`` or ``thingy53/nrf5340/cpuapp/ns`` as the board target when building a sample or application, you will generate firmware for both the application core and network core:

* The application core firmware consists of MCUboot bootloader and an application image.
* The network core firmware consists of network core bootloader (B0n) and application firmware of the network core.

The build process generates firmware in two formats:

* Intel Hex file (:file:`merged.hex` and :file:`merged_CPUNET.hex`) - Used with an external debug probe.
  These file contains bootloaders and applications for each core.
* Binary files (:file:`zephyr.signed.bin`), containing signed application firmwares for the application and network core, respectively.
  For convenience, the binary files are bundled in :file:`dfu_application.zip`, together with a manifest that describes them.
  You can use the binary files or the combined zip archive to update application firmware for both cores, with either MCUboot serial recovery or OTA DFU using Bluetooth® LE.

For more information about files generated as output of the build process, see :ref:`app_build_output_files`.

See the following sections for details regarding building and programming the firmware for Thingy:53 in various environments.

.. _thingy53_build_pgm_targets_wifi:

Building Wi-Fi applications on Thingy:53
========================================

You can use the Nordic Thingy:53 with the nRF7002 Expansion Board (EB) for Wi-Fi development.
Connect the nRF7002 EB to the **P9** connector on Thingy:53.

To build for the nRF7002 EB with Thingy:53, use the ``thingy53/nrf5340/cpuapp`` board target with the CMake ``SHIELD`` variable set to ``nrf7002eb``.
For example, you can use the following command when building on the command line:

.. code-block::

   west build -b thingy53/nrf5340/cpuapp -- -DSHIELD=nrf7002eb

For the compatible Wi-Fi samples in the |NCS|, see the :ref:`wifi_samples` section.

.. _thingy53_app_update:

Programming methods for Thingy:53
*********************************

You can program the firmware on the Nordic Thingy:53 using an external debug probe and a 10-pin JTAG cable, using :ref:`Visual Studio Code <thingy53_build_pgm_vscode>`, :ref:`command line <thingy53_build_pgm_command_line>`, or the `Programmer app <Programming Nordic Thingy prototyping platforms_>`_ from nRF Connect for Desktop.
You can also program applications running on both the network and application core using the built-in MCUboot serial recovery mode, using the `Programmer app`_ from nRF Connect for Desktop or `nRF Util <Programming application firmware using MCUboot serial recovery_>`_.

Finally, you can use the `Programmer app`_ in nRF Connect for Desktop, the `nRF Programmer mobile app`_ for Android and iOS, or nRF Util to update the :ref:`preloaded application images <thingy53_precompiled>`.

.. _thingy53_app_update_debug:

Firmware update using external debug probe
==========================================

You can program the firmware on the Nordic Thingy:53 using an external debug probe and a 10-pin JTAG cable.
In such cases, you can program the Thingy:53 the same way as the nRF5340 DK.

The external debug probe must support Arm Cortex-M33 (such as the nRF5340 DK).
You need a 10-pin 2x5 socket-to-socket 1.27 mm IDC (:term:`Serial Wire Debug (SWD)`) JTAG cable to connect to the external debug probe.

This method is supported when programming with :ref:`Visual Studio Code <thingy53_build_pgm_vscode>`, :ref:`command line <thingy53_build_pgm_command_line>`, or the `Programmer app <Programming Nordic Thingy prototyping platforms_>`_ from nRF Connect for Desktop.

See also :ref:`ug_nrf5340` for additional information.

.. _thingy53_app_update_mcuboot:

Firmware update using MCUboot bootloader
========================================

Samples and applications built for Thingy:53 include the MCUboot bootloader that you can use to update the firmware out of the box.
This method uses signed binary files :file:`app_update.bin` and :file:`net_core_app_update.bin` (or :file:`dfu_application.zip`).
You can program the precompiled firmware image in one of the following ways:

* Use the :doc:`MCUboot<mcuboot:index-ncs>` feature and the built-in serial recovery mode of Thingy:53.
  In this scenario, the Thingy is connected directly to your PC through USB.
  For details, refer to the :ref:`thingy53_app_mcuboot_bootloader` section.

  See `Programming Nordic Thingy prototyping platforms`_ for details on how to program the Thingy:53 using the Programmer app from nRF Connect for Desktop.

* Update the firmware over-the-air (OTA) using Bluetooth LE and the nRF Programmer mobile application for Android or iOS.
  To use this method, the application that is currently programmed on Thingy:53 must support it.
  For details, refer to the :ref:`thingy53_app_fota_smp` section.
  All precompiled images support OTA using Bluetooth.

  See the :ref:`thingy53_gs_updating_ble` section for the detailed procedures on how to program a Thingy:53 using the `nRF Programmer mobile app`_ for Android or iOS.

.. _thingy53_build_pgm_vscode:

Building and programming using |VSC|
************************************

Complete the following steps to build and program using |nRFVSC|:

.. |sample_path_vsc| replace:: :file:`nrf/samples/bluetooth/peripheral_lbs`

.. |vsc_sample_board_target_line| replace:: select ``thingy53/nrf5340/cpuapp`` as the board target

.. include:: /includes/vsc_build_and_run.txt

3. Program the sample or application:

   a. Connect the Nordic Thingy:53 to the debug out port on a 10-pin external debug probe, for example nRF5340 DK, using a 10-pin JTAG cable.

      .. figure:: ./images/thingy53_nrf5340_dk.webp
         :alt: Nordic Thingy:53 connected to the debug port on a 10-pin external debug probe

         Nordic Thingy:53 connected to the debug port on a 10-pin external debug probe

   #. Connect the external debug probe to the PC using a micro-USB cable.
   #. Make sure that the Thingy:53 and the external debug probe are powered on.
      (On the Thingy:53, move the power switch **SW1** to the **ON** position.)
   #. Click :guilabel:`Flash` in the `Actions View`_.

.. _thingy53_build_pgm_command_line:

Building and programming on the command line
********************************************

You must :ref:`build_environment_cli` before you start building an |NCS| project on the command line.

To build and program the source code from the command line, complete the following steps:

1. |open_terminal_window_with_environment|
#. Go to the specific directory for the sample or application.

   For example, the directory path is :file:`ncs/nrf/applications/machine_learning` when building the source code for the :ref:`nrf_machine_learning_app` application.

#. Pull the ``sdk-nrf`` repository on GitHub as described in the :ref:`cloning_the_repositories` and :ref:`updating_repos` sections.
   This is needed to make sure that you have the required version of the |NCS| repository.
#. Get the rest of the dependencies using west:

   .. code-block:: console

      west update

#. Build the sample or application code as follows:

   .. parsed-literal::
      :class: highlight

      west build -b *board_target* -d *destination_directory_name*

   The board target should be ``thingy53/nrf5340/cpuapp`` or ``thingy53/nrf5340/cpuapp/ns`` when building samples for the application core.
   The image for ``thingy53/nrf5340/cpunet`` will be built automatically.
   See :ref:`thingy53_build_pgm_targets` for details.

#. Program the sample or application:

   a. Connect the Nordic Thingy:53 to the debug out port on a 10-pin external debug probe, for example nRF5340 DK, using a 10-pin JTAG cable.

      .. figure:: ./images/thingy53_nrf5340_dk.webp
         :alt: Nordic Thingy:53 connected to the debug port on a 10-pin external debug probe

         Nordic Thingy:53 connected to the debug port on a 10-pin external debug probe

   #. Connect the external debug probe to the PC using a micro-USB cable.
   #. Make sure that the Nordic Thingy:53 and the external debug probe are powered on.
      (On the Thingy:53, move the power switch **SW1** to the **ON** position.)
   #. Use the following command to program the sample or application to the device:

      .. code-block:: console

         west flash

   The device resets and runs the programmed sample or application.

.. _thingy53_gs_updating_usb:
.. _thingy53_gs_updating_external_probe:

Programming using the Programmer app
************************************

You can program the Nordic Thingy:53 using the `Programmer app`_ from nRF Connect for Desktop and MCUboot's serial recovery feature.
You can use this application to also program precompiled firmware packages.

You can program the Thingy:53 using the Programmer app with either USB-C or an external debug probe.
See the `Programming Nordic Thingy prototyping platforms`_ in the tool documentation for detailed steps.

Programming using nRF Util
**************************

You can use the nRF Util tool to program Thingy:53, including programming precompiled firmware packages.
See `Programming application firmware using MCUboot serial recovery`_ in the tool documentation for detailed steps.

Using the nRF Programmer mobile app
***********************************

You can use the `nRF Programmer mobile app`_ on your Android or iOS device to program precompiled firmware packages.
For detailed steps, see :ref:`thingy53_gs_updating_ble`.
