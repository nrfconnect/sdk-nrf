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

You can build and program separate images or combined images using |nRFVSC|.

.. tabs::

   .. group-tab:: Separate images

      To build and program the application core, follow the instructions in `How to build an application`_ and use ``nrf5340dk/nrf5340/cpuapp`` or ``nrf5340dk/nrf5340/cpuapp/ns`` as the board target.

      To build and program the network core, follow the instructions in `How to build an application`_ and use ``nrf5340dk/nrf5340/cpunet`` as the board target.

   .. group-tab:: Multi-image build

      If you are working with Bluetooth® LE, Thread, Zigbee, or Matter samples, the network core sample is built as a separate image when you build the application core image.

      Complete the following steps to build and program a multi-image build to the nRF5340 application core and network core:

	   .. include:: /includes/vsc_build_and_run.txt

Program the sample or application
---------------------------------

Complete the following steps to program the sample or application onto nRF5340 DK:

#. Connect the nRF5340 development kit to your PC using a USB cable.
#. Make sure that the nRF5340 DK and the external debug probe are powered on.
#. Click :guilabel:`Build` in the `Actions View`_ to start the build process.
#. Click :guilabel:`Flash` in the :guilabel:`Actions View` to program the resulting image to your device.

Using the command line
======================

To build nRF5340 samples from the command line, use :ref:`west <zephyr:west>`.
To program the nRF5340 DK from the command line, use either west or :ref:`nRF Util <requirements_nrf_util>` (which is also used by west as the :ref:`default runner <programming_selecting_runner>`).

.. tabs::

   .. group-tab:: Separate images

      To build and program the application sample and the network sample as separate images, follow the instructions in :ref:`programming_cmd` for each of the samples.

      See the following instructions for programming the images separately:

      .. tabs::

         .. group-tab:: west

            1. |open_terminal_window_with_environment|
            #. Run the following command to erase the flash memory of the network core and program the network sample:

               .. code-block:: console

                  west flash --erase

            #. Navigate to the build folder of the application sample and run the same command to erase the flash memory of the application core and program the application sample:

               .. code-block:: console

                  west flash --erase

         .. group-tab:: nRF Util

            1. |open_terminal_window_with_environment|
            #. Run the following command to erase the flash memory of the network core and program the network sample:

               .. code-block:: console

                  nrfutil device program --firmware zephyr.hex --options chip_erase_mode=ERASE_ALL --core Network

               .. note::
                    If you cannot locate the build folder of the network sample, look for a folder with one of these names inside the build folder of the application sample:

                    * :file:`rpc_host`
                    * :file:`hci_rpsmg`
                    * :file:`802154_rpmsg`
                    * :file:`ipc_radio`

            #. Navigate to the build folder of the application sample and run the following command to erase the flash memory of the application core and program the application sample:

               .. code-block:: console

                  nrfutil device program --firmware zephyr.hex  --options chip_erase_mode=ERASE_ALL

               .. note::
                    The application build folder will be in a sub-directory which is the name of the folder of the application

            #. Reset the development kit:

               .. code-block:: console

                  nrfutil device reset --reset-kind=RESET_PIN

      See :ref:`readback_protection_error` if you encounter an error.

   .. group-tab:: Sysbuild

      To build and program a sysbuild HEX file, follow the instructions in :ref:`programming_cmd` for the application core sample.

      To program the multi-image HEX file, you can use west or nRF Util.

      .. tabs::

         .. group-tab:: west

            Enter the following command to program multi-image builds for different cores:

            .. code-block:: console

               west flash

         .. group-tab:: nRF Util

            Enter the following commands to program multiple image builds for different cores:

            .. code-block:: console

               nrfutil device program --firmware merged_CPUNET.hex --options verify=VERIFY_READ,chip_erase_mode=ERASE_CTRL_AP
               nrfutil device program --firmware merged.hex --options verify=VERIFY_READ,chip_erase_mode=ERASE_CTRL_AP

            .. note::
                 The ``--verify`` command confirms that the writing operation has succeeded.
