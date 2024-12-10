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

The |NCS| uses :ref:`ug_multi_image` for Thingy:53 by default.
When you choose ``thingy53/nrf5340/cpuapp`` or ``thingy53/nrf5340/cpuapp/ns`` as the board target when building a sample or application, you will generate firmware for both the application core and network core:

* The application core firmware consists of MCUboot bootloader and an application image.
* The network core firmware consists of network core bootloader (B0n) and application firmware of the network core.

The build process generates firmware in two formats:

* Intel Hex file (:file:`merged.hex` and :file:`merged_CPUNET.hex`) - Used with an external debug probe.
  These file contains bootloaders and applications for each core.
* Binary files (:file:`zephyr.signed.bin`), containing signed application firmwares for the application and network core, respectively.
  For convenience, the binary files are bundled in :file:`dfu_application.zip`, together with a manifest that describes them.
  You can use the binary files or the combined zip archive to update application firmware for both cores, with either MCUboot serial recovery or OTA DFU using Bluetooth LE.

For more information about files generated as output of the build process, see :ref:`app_build_output_files`.

See the following sections for details regarding building and programming the firmware for Thingy:53 in various environments.
See :ref:`thingy53_app_update` for more detailed information about updating firmware on Thingy:53.

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

.. _thingy53_build_pgm_vscode:

Building and programming using |VSC|
************************************

Complete the following steps to build and program using the |nRFVSC|:

.. |sample_path_vsc| replace:: :file:`nrf/samples/bluetooth/peripheral_lbs`

.. |vsc_sample_board_target_line| replace:: select ``thingy53/nrf5340/cpuapp`` as the board target

.. include:: /includes/vsc_build_and_run.txt

3. Program the sample or application:

   a. Connect the Nordic Thingy:53 to the debug out port on a 10-pin external debug probe, for example nRF5340 DK, using a 10-pin JTAG cable.
   #. Connect the external debug probe to the PC using a USB cable.
   #. Make sure that the Thingy:53 and the external debug probe are powered on.
   #. Click :guilabel:`Flash` in the :guilabel:`Actions View`.

.. _thingy53_build_pgm_command_line:

Building and programming on the command line
********************************************

You must :ref:`build_environment_cli` before you start building an |NCS| project on the command line.

To build and program the source code from the command line, complete the following steps:

1. |open_terminal_window_with_environment|
#. Go to the specific directory for the sample or application.

   For example, the directory path is :file:`ncs/nrf/applications/machine_learning` when building the source code for the :ref:`nrf_machine_learning_app` application.

#. Make sure that you have the required version of the |NCS| repository by pulling the ``sdk-nrf`` repository on GitHub as described in :ref:`dm-wf-get-ncs` and :ref:`dm-wf-update-ncs` sections.
#. Get the rest of the dependencies using west:

   .. code-block:: console

      west update

#. Build the sample or application code as follows:

   .. parsed-literal::
      :class: highlight

      west build -b *board_target* -d *destination_directory_name*

   The board target should be ``thingy53/nrf5340/cpuapp`` or ``thingy53/nrf5340/cpuapp/ns`` when building samples for the application core.
   The proper child image for ``thingy53/nrf5340/cpunet`` will be built automatically.
   See :ref:`thingy53_build_pgm_targets` for details.

#. Program the sample or application:

   a. Connect the Nordic Thingy:53 to the debug out port on a 10-pin external debug probe, for example nRF5340 DK, using a 10-pin JTAG cable.
   #. Connect the external debug probe to the PC using a USB cable.
   #. Make sure that the Nordic Thingy:53 and the external debug probe are powered on.
   #. Use the following command to program the sample or application to the device:

      .. code-block:: console

         west flash

   The device resets and runs the programmed sample or application.
