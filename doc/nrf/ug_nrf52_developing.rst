.. _ug_nrf52:
.. _ug_nrf52_developing:

Developing with nRF52 Series
############################

.. contents::
   :local:
   :depth: 2

The |NCS| provides support for developing on all nRF52 Series devices and contains board definitions for all development kits and reference design hardware.

See one of the following guides for detailed information about the corresponding nRF52 Series development kit (DK) hardware:

* `nRF52840 DK <nRF52840 DK User Guide_>`_
* `nRF52833 DK <nRF52833 DK User Guide_>`_
* `nRF52 DK <nRF52 DK User Guide_>`_

To get started with your nRF52 Series DK, follow the steps in the :ref:`ug_nrf52_gs` section.
If you are not familiar with the |NCS| and the development environment, see the :ref:`introductory documentation <getting_started>`.

.. fota_upgrades_start

FOTA upgrades
*************

|fota_upgrades_def|
You can also use FOTA upgrades to replace the application.

.. note::
   For the possibility of introducing an upgradable bootloader, refer to :ref:`ug_bootloader_adding`.

FOTA over Bluetooth Low Energy
==============================

To enable support for FOTA upgrades, do the following:

* Use MCUboot as the upgradable bootloader (:kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` must be enabled).
  For more information, go to the :doc:`mcuboot:index-ncs` page.
* Enable the mcumgr module that handles the transport protocol over Bluetooth® Low Energy as follows:

  a. Enable the Kconfig options :kconfig:option:`CONFIG_MCUMGR_CMD_OS_MGMT`, :kconfig:option:`CONFIG_MCUMGR_CMD_IMG_MGMT`, and :kconfig:option:`CONFIG_MCUMGR_SMP_BT`.
  #. Call the functions :c:func:`os_mgmt_register_group()` and :c:func:`img_mgmt_register_group()` in your application.
  #. Call the :c:func:`smp_bt_register()` function in your application to initialize the mcumgr Bluetooth Low Energy transport.

  After completing these steps, your application advertises the SMP Service with ``UUID 8D53DC1D-1DB7-4CD3-868B-8A527460AA84``.

To perform a FOTA upgrade, complete the following steps:

1. Create a binary file that contains the new image.

   |fota_upgrades_building|

#. Download the :file:`app_update.bin` image file to your device.

   .. note::
      When performing FOTA upgrade on a Bluetooth mesh device and the device's composition data is going to change after the upgrade, unprovision the device before downloading the new image.

      nRF Connect for Desktop does not currently support the FOTA process.

   Use `nRF Connect Device Manager`_, `nRF Connect for Mobile`_, or `nRF Toolbox`_ to upgrade your device with the new firmware.

   a. Ensure that you can access the :file:`app_update.bin` image file from your phone or tablet.
   #. Connect to the device with the mobile app.
   #. Initiate the DFU process to transfer the image to the device.

FOTA upgrade sample
===================

The :ref:`zephyr:smp_svr_sample` demonstrates how to set up your project to support FOTA upgrades.

The sample documentation is from the Zephyr project and is incompatible with the :ref:`ug_multi_image`.
When working in the |NCS| environment, ignore the part of the sample documentation that describes the building and programming steps.
In |NCS|, you can build and program the :ref:`zephyr:smp_svr_sample` as any other sample using the following commands:

.. parsed-literal::
   :class: highlight

    west build -b *build_target* -- -DOVERLAY_CONFIG=overlay-bt.conf
    west flash

Make sure to indicate the :file:`overlay-bt.conf` overlay configuration for the Bluetooth transport like in the command example.
This configuration was carefully selected to achieve the maximum possible throughput of the FOTA upgrade transport over Bluetooth with the help of the following features:

* Bluetooth MTU - To increase the packet size of a single Bluetooth packet transmitted over the air (:kconfig:option:`CONFIG_BT_BUF_ACL_RX_SIZE` and others).
* Bluetooth connection parameters - To adaptively change the connection interval and latency on the detection of the SMP service activity (:kconfig:option:`CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL`).
* MCUmgr packet reassembly - To allow exchange of large SMP packets (:kconfig:option:`CONFIG_MCUMGR_SMP_REASSEMBLY_BT`, :kconfig:option:`CONFIG_MCUMGR_BUF_SIZE` and others).

Consider using these features in your project to speed up the FOTA upgrade process.

.. fota_upgrades_end

.. fota_upgrades_matter_start

FOTA in Matter
==============

To perform a FOTA upgrade when working with the Matter protocol, use one of the following methods:

* DFU over Bluetooth LE using either smartphone or PC command-line tool.
  Both options are similar to `FOTA over Bluetooth Low Energy`_.

  .. note::
     This protocol is not part of the Matter specification.

* DFU over Matter using Matter-compliant BDX protocol and Matter OTA Provider device.
  This option requires an OpenThread Border Router (OTBR) set up either in Docker or on a Raspberry Pi.

For more information about both methods, read the :doc:`matter:nrfconnect_examples_software_update` page in the Matter documentation.

.. fota_upgrades_matter_end

.. fota_upgrades_thread_start

FOTA over Thread
================

:ref:`ug_thread` does not offer a proprietary FOTA method.

.. fota_upgrades_thread_end

.. fota_upgrades_zigbee_start

FOTA over Zigbee
================

You can enable support for FOTA over the Zigbee network using the :ref:`lib_zigbee_fota` library.
For detailed information about how to configure the Zigbee FOTA library for your application, see :ref:`ug_zigbee_configuring_components_ota`.

.. fota_upgrades_zigbee_end

.. _nrf52_computer_testing:

Testing the application with a computer
***************************************

If you have an nRF52 Series DK with the :ref:`peripheral_uart` sample and either a dongle or second Nordic Semiconductor development kit that supports Bluetooth Low Energy, you can test the sample on your computer.
Use the Bluetooth Low Energy app in `nRF Connect for Desktop`_ for testing.

To perform the test, :ref:`connect to the nRF52 Series DK <nrf52_gs_connecting>` and complete the following steps:

1. Connect the dongle or second development kit to a USB port of your computer.
#. Open the Bluetooth Low Energy app.
#. Select the serial port that corresponds to the dongle or second development kit.
   Do not select the kit you want to test.

   .. note::
      If the dongle or second development kit has not been used with the Bluetooth Low Energy app before, you may be asked to update the J-Link firmware and connectivity firmware on the nRF SoC to continue.
      When the nRF SoC has been updated with the correct firmware, the nRF Connect Bluetooth Low Energy app finishes connecting to your device over USB.
      When the connection is established, the device appears in the main view.

#. Click :guilabel:`Start scan`.
#. Find the DK you want to test and click the corresponding :guilabel:`Connect` button.

   The default name for the Peripheral UART sample is *Nordic_UART_Service*.

#. Select the **Universal Asynchronous Receiver/Transmitter (UART)** RX characteristic value.
#. Write ``30 31 32 33 34 35 36 37 38 39`` (the hexadecimal value for the string "0123456789") and click :guilabel:`Write`.

   The data is transmitted over Bluetooth LE from the app to the DK that runs the Peripheral UART sample.
   The terminal emulator connected to the DK then displays ``"0123456789"``.

#. In the terminal emulator, enter any text, for example ``Hello``.

   The data is transmitted to the DK that runs the Peripheral UART sample.
   The **UART TX** characteristic displayed in the Bluetooth Low Energy app changes to the corresponding ASCII value.
   For example, the value for ``Hello`` is ``48 65 6C 6C 6F``.

.. _build_pgm_nrf52:

Building and programming
************************

You can program applications and samples on the nRF9160 DK after obtaining the corresponding firmware images.

To program applications using the Programmer app from `nRF Connect for Desktop`_, follow the instructions in :ref:`nrf52_gs_installing_application`.
In Step 5, choose the :file:`.hex` file for the application you are programming.

.. _build_pgm_nrf52_vsc:

Building and programming using |VSC|
====================================

|vsc_extension_instructions|

Complete the following steps after installing the |nRFVSC|:

.. |sample_path_vsc| replace:: :file:`nrf/applications/nrf_desktop`

.. |vsc_sample_board_target_line| replace:: see the :ref:`list of supported boards <nrf52_supported_boards>` for the build target corresponding to the nRF52 Series DK you are using

.. include:: /includes/vsc_build_and_run.txt

#. Connect the nRF52 Series DK to the computer with a micro-USB cable, and then turn on the DK.

   **LED1** starts blinking.

#. In |VSC|, click the :guilabel:`Flash` option in the :guilabel:`Actions View`.

   If you have multiple boards connected, you are prompted to pick a device at the top of the screen.

   A small notification banner appears in the bottom right corner of |VSC| to display the progress and confirm when the flash is complete.

.. _build_pgm_nrf52_cmdline:

Building and programming on the command line
============================================

Complete the :ref:`command-line build setup <build_environment_cli>` before you start building |NCS| projects on the command line.

To build and program the source code from the command line, complete the following steps:

1. Open a terminal window.
#. Go to the specific sample or application directory.

   For example, the folder path is ``ncs/nrf/applications/nrf_desktop`` when building the source code for the :ref:`nrf_desktop` application.

#. Make sure that you have the required version of the |NCS| repository by pulling the |NCS| repository `sdk-nrf`_ on GitHub.

   Follow the procedures described in :ref:`dm-wf-get-ncs` and :ref:`dm-wf-update-ncs`.

#. To get the rest of the dependencies, run the ``west update`` command as follows:

   .. code-block:: console

      west update

#. To build the sample or application code, run the ``west build`` command as follows:

   .. parsed-literal::
      :class: highlight

      west build -b *build_target* -d *destination_directory_name*

   For the *build_target* parameter, see the :ref:`list of supported boards <nrf52_supported_boards>` for the build target corresponding to the nRF52 Series DK you are using.

   .. note::

	    You can use the optional *destination_directory_name* parameter to specify the destination directory in the west command.
	    By default, the build files are generated in ``build/zephyr/`` if you have not specified a *destination_directory_name*.

#. Connect the nRF52 Series DK to the computer with a micro-USB cable, and turn on the DK.

   **LED1** starts blinking.

#. Program the sample or application to the device using the following command:

   .. code-block:: console

      west flash

   The device resets and runs the programmed sample or application.

nRF Desktop
===========

The nRF Desktop application is a complete project that integrates Bluetooth LE, see the :ref:`nrf_desktop` application.
You can build it for the nRF Desktop reference hardware or an nRF52840 DK.

The nRF Desktop is a reference design of a HID device that is connected to a host through Bluetooth LE or USB, or both.
This application supports configurations for simple mouse, gaming mouse, keyboard, and USB dongle.
