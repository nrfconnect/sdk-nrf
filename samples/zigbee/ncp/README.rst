.. _zigbee_ncp_sample:

Zigbee: NCP
###########

.. contents::
   :local:
   :depth: 2

The :ref:`Zigbee <ug_zigbee>` NCP sample demonstrates the usage of Zigbee's :ref:`ug_zigbee_platform_design_ncp_details` architecture.

Together with the source code from `ZBOSS NCP Host`_, you can use this sample to create a complete and functional Zigbee device.
For example, as shown in the `Testing`_ scenario, you can program a development kit with the NCP sample and bundle it with the light control application on the NCP host processor.

You can then use this sample together with the :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` and the :ref:`Zigbee light bulb <zigbee_light_bulb_sample>` to set up a basic Zigbee network.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52840dk_nrf52840, nrf52840dongle_nrf52840, nrf52833dk_nrf52833, nrf21540dk_nrf52840

You can use any of the development kits listed above.

.. note::
    The nRF52840 Dongle uses a :ref:`different bootloader <zigbee_ncp_bootloader>` than other development kits.

For this sample to work, you also need the following:

* `ZBOSS NCP Host`_ tool, which is based on the ZBOSS stack and available for download as a standalone :file:`zip` package as one of :ref:`ug_zigbee_tools`.
  It contains the source code for all parts of the ZBOSS library for the NCP host.
  This allows you to recompile the library for the designated hardware platform.
  Running the ZBOSS NCP Host requires a PC with an operating system compatible with the 64-bit Ubuntu 18.04 Linux.
* The :ref:`Zigbee network coordinator <zigbee_network_coordinator_sample>` sample programmed on one separate device.
* The :ref:`zigbee_light_bulb_sample` sample programmed on one separate device.

This means that you need at least three development kits for testing this sample.

.. figure:: /images/zigbee_ncp_sample_overview.svg
   :alt: Zigbee NCP sample setup overview

   Zigbee NCP sample setup overview

Overview
********

The sample demonstrates using a Nordic Semiconductor's Development Kit as Zigbee Network Co-Processor.

The sample uses the :option:`CONFIG_ZIGBEE_LIBRARY_NCP_DEV` Kconfig option, which is available in the :ref:`development version of the ZBOSS stack libraries <nrfxlib:zboss_configuration>` (managed by the :option:`CONFIG_ZIGBEE_LIBRARY_DEVELOPMENT` Kconfig option, also enabled by default).
The NCP Kconfig option extends the compilation process with an implementation of the ZBOSS API serialization through NCP commands.
It also implements the ZBOSS default signal handler function, which controls the ZBOSS and commissioning logic.

The sample is built to work with bootloader.
See :ref:`zigbee_ncp_bootloader` for more information.

The NCP application creates and starts a ZBOSS thread as well as the communication channel for NCP commands that are exchanged between the connectivity device and the host processor.

Configuration
*************

|config|

See :ref:`ug_zigbee_configuring_eui64` for information about how to configure the IEEE address for this sample.

Serial communication setup
==========================

The communication channel uses Zephyr's :ref:`zephyr:uart_api` API.
This serial device is selected with :option:`CONFIG_ZIGBEE_UART_DEVICE_NAME`.

By default, the NCP sample communicates through the UART serialization (``UART0``).
As a result, Zephyr's logger is configured to use ``UART1``, which is available through GPIO pins (**P1.00** and **P1.01**).

The ``UART0`` pins are configured by Devicetree :file:`overlay` files for each supported development kit in the :file:`boards` directory.

Communication through USB
-------------------------

You can change the communication channel from the default UART to nRF USB by using the :file:`prj_usb.conf` configuration file.
This configuration file is not applied automatically and can be passed into CMake by using the ``--`` separator when building the sample.
For example:

.. code-block:: console

   west build samples/zigbee/ncp -b nrf52840dk_nrf52840 -- -DCONF_FILE='prj_usb.conf'

The USB device VID and PID are configured by the sample's Kconfig file.

.. note::
    The USB is used as the default NCP communication channel when using the nRF52840 Dongle.

.. _zigbee_ncp_bootloader:

Bootloader support
==================

The bootloader support in the NCP sample depends on the development kit, its respective build target, and `Serial communication setup`_:

* For the ``nrf52840dongle_nrf52840`` build target, `nRF5 SDK Bootloader`_ is used by default because the dongle comes with this bootloader preinstalled.
* For the ``nrf52840dk_nrf52840``, ``nrf52833dk_nrf52833``, and ``nrf21540dk_nrf52840`` build targets, the following scenarios are possible when building for these build targets:

  * If the `Communication through USB`_ is selected, `MCUboot`_ is enabled by default.
  * If the default UART serial communication channel is used, the bootloader support is not enabled, but MCUboot can be enabled by the user.

MCUboot
-------

When the `Communication through USB`_ is selected, MCUboot in this sample is built with support for single application slot, and uses the USB DFU class driver to allow uploading image over USB.

If you want to use the default UART serial communication channel, you can enable MCUboot by setting the :option:`CONFIG_BOOTLOADER_MCUBOOT` Kconfig option.
To use the same MCUboot configuration as in `Communication through USB`_, you need to provide MCUboot with the Kconfig options included in the :file:`child_image/mcuboot_usb.conf` file.
See :ref:`ug_multi_image_variables` for how to set the required options.

MCUboot with the USB DFU requires a larger partition.
To increase the partition, define the ``PM_STATIC_YML_FILE`` variable that provides the path to the :file:`pm_static.yml` static configuration file in the :file:`configuration` directory for the build target of your choice.
These additional options and configuration file can be passed into CMake by using the ``--`` separator when building the sample.
For example:

.. code-block:: console

   west build samples/zigbee/ncp -b nrf52840dk_nrf52840 -- -DCONFIG_BOOTLOADER_MCUBOOT=y  -Dmcuboot_CONFIG_MULTITHREADING=y -Dmcuboot_CONFIG_BOOT_USB_DFU_WAIT=y -Dmcuboot_CONFIG_SINGLE_APPLICATION_SLOT=y -DPM_STATIC_YML_FILE=samples/zigbee/ncp/configuration/nrf52840dk_nrf52840/pm_static.yml

When building the sample, the build system also generates the signed :file:`app_update.bin` image file in the :file:`build` directory.
This file can be used to upgrade a device.
See :ref:`mcuboot_ncs` for more information about this and other automatically generated files.

After every reset, the sample first boots to MCUboot and then, after a couple of seconds, the NCP sample is booted.
When booted to MCUboot, the new image can be uploaded with the dfu-util tool.
See :ref:`USB DFU sample documentation <zephyr:usb_dfu>` for a reference.

To learn more about configuring bootloader for an application in |NCS|, see the :ref:`ug_bootloader` page.

nRF5 SDK Bootloader
-------------------

When the sample is built for ``nrf52840dongle_nrf52840``, the build system does not produce an upgrade image.
You can use the nRF Connect Programmer application (part of `nRF Connect for Desktop`_) to generate and upload a new application image to the dongle.
For more details, see `Programming the nRF52840 Dongle`_ in the nRF Connect Programmer user guide.

To boot to the bootloader on the dongle, an additional action is required.
For example, you can trigger the pin reset procedure to reboot the device to the bootloader DFU mode, in which the new application image can be uploaded.
See `nRF5 Bootloader DFU Mode`_ for the list of possible conditions for activating the DFU mode.

FEM support
===========

.. |fem_file_path| replace:: :file:`samples/zigbee/common`

.. include:: /includes/sample_fem_support.txt

.. _zigbee_ncp_user_interface:

User interface
**************

All the NCP sample's interactions with the application are automatically handled using serial or USB communication.

Building and running
********************

.. |sample path| replace:: :file:`samples/zigbee/ncp`

|enable_zigbee_before_testing|

.. include:: /includes/build_and_run.txt

.. _zigbee_ncp_testing:

Testing
=======

After building the sample and programming it to your development kit, test it by performing the following steps:

1. Download and extract the `ZBOSS NCP Host`_ package.

   .. note::
      If you are using an Linux distribution different than the 64-bit Ubuntu 18.04, make sure to rebuild the package libraries and applications by following the instructions described in the :file:`README.rst`  file in the ZBOSS NCP Host package.

#. If you are using `Communication through USB`_, connect the nRF USB port of the NCP sample's development kit to the PC USB port with an USB cable.
#. Get the kit’s serial port name (for example, /dev/ttyACM0).
   If you are communicating through the nRF USB, get the nRF USB serial port name.
#. Turn on the development kit that runs the network coordinator sample.
   When **LED 3** turns on, this development kit has become the Coordinator of the Zigbee network.
#. Turn on the development kit that runs the light bulb sample.
   When **LED 3** turns on, the light bulb has become a Router inside the network.

   .. tip::
        If **LED 3** does not turn on, press **Button 1** on the Coordinator to reopen the network.

#. Start the light control application by running the following command with *serial_port_name* replaced with the serial port name used for communication with the NCP sample:

   .. parsed-literal::
      :class: highlight

      NCP_SLAVE_PTY=*serial_port_name* ./application/light_sample/light_control/light_control

The light control device joins the Zigbee network, searches for a light bulb to control, and starts sending On/Off commands with a 15-sec period that toggle the **LED 4** on the light bulb on and off.

Dependencies
************

This sample uses the following |NCS| libraries:

* Zigbee subsystem:

  * :file:`zb_nrf_platform.h`

This sample uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:zboss` (development version)

In addition, it uses the following Zephyr libraries:

* :file:`include/device.h`
* :ref:`zephyr:usb_api`
* :ref:`zephyr:logging_api`
* :ref:`zephyr:uart_api`
