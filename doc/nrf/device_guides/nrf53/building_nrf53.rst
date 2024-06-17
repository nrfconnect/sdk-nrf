.. _ug_nrf5340_building:

Building and programming with nRF53 Series
##########################################

.. contents::
   :local:
   :depth: 2

Building and programming with nRF53 Series application for the nRF53 Series devices follows the processes described in the following sections.

Building and programming with nRF5340 DK
****************************************

Depending on the sample, you must program only the application core (for example, when using NFC samples) or both the network and the application core.

The steps differ depending on whether you work with |VSC| or on the command line and whether you are doing a single or multi-image build.

Using |VSC|
===========

You can build and program separate images or combined images using the |nRFVSC|.

.. tabs::

   .. group-tab:: Separate images

      To build and program the application core, follow the instructions in `How to build an application`_ and use ``nrf5340dk/nrf5340/cpuapp`` or ``nrf5340dk/nrf5340/cpuapp/ns`` as the board target.

      To build and program the network core, follow the instructions in `How to build an application`_ and use ``nrf5340dk/nrf5340/cpunet`` as the board target.

   .. group-tab:: Multi-image build

      If you are working with Bluetooth LE, Thread, Zigbee, or Matter samples, the network core sample is built as a child image when you build the application core image (see :ref:`ug_nrf5340_multi_image` above).

      Complete the following steps to build and program a multi-image build to the nRF5340 application core and network core:

	   .. include:: ../../includes/vsc_build_and_run.txt

Program the sample or application
---------------------------------

Complete the following steps to program the sample or application onto Thingy:91:

#. Connect the nRF5340 development kit to your PC using a USB cable.
#. Make sure that the nRF5340 DK and the external debug probe are powered on.
#. Click :guilabel:`Build` in the :guilabel:`Actions View` to start the build process.
#. Click :guilabel:`Flash` in the :guilabel:`Actions View` to program the resulting image to your device.

Using the command line
======================

To build nRF5340 samples from the command line, use :ref:`west <zephyr:west>`.
To program the nRF5340 DK from the command line, use either west (which uses nrfjprog that is part of the `nRF Command Line Tools`_) or :ref:`nRF Util <toolchain_management_tools>`.

.. note::
   Programming the nRF5340 DK from the command line with west requires the `nRF Command Line Tools`_ v10.12.0 or later.

.. tabs::

   .. group-tab:: Separate images

      To build and program the application sample and the network sample as separate images, follow the instructions in :ref:`programming_cmd` for each of the samples.

      See the following instructions for programming the images separately:

      .. tabs::

         .. group-tab:: west

            1. Open a command prompt in the build folder of the network sample and enter the following command to erase the flash memory of the network core and program the network sample::

                west flash --erase

            2. Navigate to the build folder of the application sample and enter the same command to erase the flash memory of the application core and program the application sample::

                west flash --erase

         .. group-tab:: nRF Util

            1. Open a command prompt in the build folder of the network sample and enter the following command to erase the flash memory of the network core and program the network sample::

                nrfutil device program --firmware zephyr.hex --options chip_erase_mode=ERASE_ALL --core Network

            .. note::
               If you cannot locate the build folder of the network sample, look for a folder with one of these names inside the build folder of the application sample:

               * :file:`rpc_host`
               * :file:`hci_rpsmg`
               * :file:`802154_rpmsg`
               * :file:`multiprotocol_rpmsg`

            2. Navigate to the build folder of the application sample and enter the following command to erase the flash memory of the application core and program the application sample::

                nrfutil device program --firmware zephyr.hex  --options chip_erase_mode=ERASE_ALL

            .. note::
               The application build folder will be in a sub-directory which is the name of the folder of the application

            3. Reset the development kit::

                nrfutil device reset --reset-kind=RESET_PIN

      See :ref:`readback_protection_error` if you encounter an error.

   .. group-tab:: Sysbuild

      To build and program a sysbuild HEX file, follow the instructions in :ref:`programming_cmd` for the application core sample.

      To program the multi-image HEX file, you can use west or nRF Util.

      .. tabs::

         .. group-tab:: west

            Enter the following command to program multi-image builds for different cores::

             west flash

			   .. note::
               The minimum supported version of nrfjprog for multi-image builds for different cores is 10.21.0.

         .. group-tab:: nRF Util

            Enter the following commands to program multiple image builds for different cores::

             nrfutil device program --firmware merged_CPUNET.hex --options verify=VERIFY_READ,chip_erase_mode=ERASE_CTRL_AP
             nrfutil device program --firmware merged.hex --options verify=VERIFY_READ,chip_erase_mode=ERASE_CTRL_AP

            .. note::
               The ``--verify`` command confirms that the writing operation has succeeded.

.. _readback_protection_error:

Readback protection
===================

When programming the device, you might get an error similar to the following message::

    ERROR: The operation attempted is unavailable due to readback protection in
    ERROR: your device. Please use --recover to unlock the device.

This error occurs when readback protection is enabled.
To disable the readback protection, you must *recover* your device.
See the following instructions.

.. tabs::

   .. group-tab:: west

      Use the ``--recover`` parameter with ``west flash``, as described in :ref:`programming_params`.

   .. group-tab:: nRF Util

      Enter the following commands to recover first the network core and then the application core::

        nrfutil device recover --core Network
        nrfutil device recover

      .. note::
         Make sure to recover the network core before you recover the application core.

         The ``nrfutil device recover`` command erases the flash memory and then writes a small binary into the recovered flash memory.
         This binary prevents the readback protection from enabling itself again after a pin reset or power cycle.

         Recovering the network core erases the flash memory of both cores.
         Recovering the application core erases only the flash memory of the application core.
         Therefore, you must recover the network core first.
         Otherwise, if you recover the application core first and the network core last, the binary written to the application core is deleted and readback protection is enabled again after a reset.

.. _thingy53_building_pgming:

Building and programming with Thingy:53
***************************************

You can program the Nordic Thingy:53 by using the images obtained by building the code in the |NCS| environment.

To set up your system to be able to build a firmware image, follow the :ref:`installation` guide for the |NCS|.

.. _thingy53_build_pgm_targets:

Board targets
=============

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
----------------------------------------

.. building_wi_fi_applications_on_thingy_53_start

You can use the Nordic Thingy:53 with the nRF7002 Expansion Board (EB) for Wi-Fi development.
Connect the nRF7002 EB to the **P9** connector on Thingy:53.

To build for the nRF7002 EB with Thingy:53, use the ``thingy53/nrf5340/cpuapp`` board target with the CMake ``SHIELD`` variable set to ``nrf7002eb``.
For example, you can use the following command when building on the command line:

.. code-block::

   west build -b thingy53/nrf5340/cpuapp -- -DSHIELD=nrf7002eb

.. building_wi_fi_applications_on_thingy_53_end

For the compatible Wi-Fi samples in the |NCS|, see the :ref:`wifi_samples` section.

.. _thingy53_build_pgm_vscode:

Building and programming using |VSC|
====================================

Complete the following steps to build and program using the |nRFVSC|:

.. |sample_path_vsc| replace:: :file:`nrf/samples/bluetooth/peripheral_lbs`

.. |vsc_sample_board_target_line| replace:: select ``thingy53/nrf5340/cpuapp`` as the board target

.. include:: ../../includes/vsc_build_and_run.txt

3. Program the sample or application:

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

      west build -b *board_target* -d *destination_directory_name*

   The board target should be ``thingy53/nrf5340/cpuapp`` or ``thingy53/nrf5340/cpuapp/ns`` when building samples for the application core.
   The proper child image for ``thingy53/nrf5340/cpunet`` will be built automatically.
   See :ref:`thingy53_build_pgm_targets` for details.

#. Program the sample or application:

   a. Connect the Nordic Thingy:53 to the debug out port on a 10-pin external debug probe, for example nRF5340 DK, using a 10-pin JTAG cable.
   #. Connect the external debug probe to the PC using a USB cable.
   #. Make sure that the Nordic Thingy:53 and the external debug probe are powered on.
   #. Use the following command to program the sample or application to the device::

         west flash

   The device resets and runs the programmed sample or application.
