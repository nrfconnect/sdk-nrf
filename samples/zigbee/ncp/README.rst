.. _zigbee_ncp_sample:

Zigbee: NCP
###########

.. contents::
   :local:
   :depth: 2

The :ref:`Zigbee <ug_zigbee>` NCP sample demonstrates the usage of Zigbee's :ref:`ug_zigbee_platform_design_ncp_details` architecture.

Together with the source code from :ref:`ug_zigbee_tools_ncp_host`, you can use this sample to create a complete and functional Zigbee device.
For example, as shown in the `Testing`_ scenario, you can program a development kit with the NCP sample and bundle it with the simple gateway application on the NCP host processor.

You can then use this sample together with the :ref:`Zigbee Light bulb <zigbee_light_bulb_sample>` to set up a basic Zigbee network.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The nRF5340 DK (``nrf5340dk/nrf5340/cpuapp``) is supported only for development.

You can use any of the development kits listed above.

.. note::
    The nRF52840 Dongle uses a :ref:`different bootloader <zigbee_ncp_bootloader>` than other development kits.

To test this sample, you also need the following:

* :ref:`ug_zigbee_tools_ncp_host` tool, which is based on the ZBOSS stack and requires a PC with an operating system compatible with the 64-bit Ubuntu 18.04 Linux.
  The tool is available for download as a standalone :file:`zip` package using the following link:

  * `ZBOSS NCP Host`_ (|zigbee_ncp_package_version|)

  For more information, see also the `NCP Host documentation`_.
* The :ref:`zigbee_light_bulb_sample` sample programmed on one separate device.

This means that you need at least two development kits for testing this sample.

.. figure:: /images/zigbee_ncp_sample_overview.svg
   :alt: Zigbee NCP sample setup overview

   Zigbee NCP sample setup overview

Requirements for DFU through MCUboot
====================================

If you opt for `MCUboot`_ when selecting the `Bootloader support`_, you need the `dfu-util tool`_ to upload the new firmware image.

Overview
********

The sample demonstrates using a Nordic Semiconductor's Development Kit as a Zigbee Network Co-Processor.

The sample uses the :kconfig:option:`CONFIG_ZIGBEE_LIBRARY_NCP_DEV` Kconfig option, which is available as part of the :ref:`nrfxlib:zboss_configuration`.
The NCP Kconfig option extends the compilation process with an implementation of the ZBOSS API serialization through NCP commands.
It also implements the ZBOSS default signal handler function that controls the ZBOSS and commissioning logic.

The sample is built to work with bootloader.
See :ref:`zigbee_ncp_bootloader` for more information.

The NCP application creates and starts a ZBOSS thread as well as the communication channel for NCP commands that are exchanged between the connectivity device and the host processor.

Configuration
*************

|config|

See :ref:`ug_zigbee_configuring_eui64` for information about how to configure the IEEE address for this sample.

Serial communication setup
==========================

The communication channel uses Zephyr's :ref:`zephyr:uart_api` API. The serial device is selected in devicetree like this:

.. code-block:: devicetree

   chosen {
       ncs,zigbee-uart = &uart0;
   };

By default, Zephyr's logger uses ``uart0`` and the NCP sample communicates through the UART serialization using ``uart1``.
The DTS overlay file configures ``uart1`` to be connected to the on-board J-Link instead of ``uart0``.
As the result, Zephyr's logger ``uart0`` is available only through GPIO pins (**P1.00** and **P1.01**).

The ``uart0`` pins are configured by devicetree overlay files for each supported development kit in the :file:`boards` directory.

Communication through USB
-------------------------

To change the communication channel from the default UART to nRF USB CDC ACM ``cdc_acm_uart0``, use the :file:`prj_usb.conf` configuration file and add the ``-DFILE_SUFFIX=usb`` flag to the build command.
See :ref:`cmake_options` for instructions on how to add this flag to your build.
For example, when building from the command line, use the following command:

.. code-block:: console

   west build samples/zigbee/ncp -b nrf52840dk/nrf52840 -- -DFILE_SUFFIX=usb

The USB device VID and PID are configured by the sample's Kconfig file.

.. note::
   USB is used as the default NCP communication channel when using the nRF52840 Dongle.

When you change the communication channel to nRF USB with :file:`prj_usb.conf` and select any of the :file:`<board>_usb.overlay` files, :ref:`Zigbee stack logs <zigbee_ug_logging_stack_logs>` are printed by default using ``uart1``.
This is configured in the :file:`prj_usb.conf` file with the following settings:

* :kconfig:option:`CONFIG_ZBOSS_TRACE_BINARY_LOGGING` - To enable binary format.
* :kconfig:option:`CONFIG_ZBOSS_TRACE_UART_LOGGING` - To select the UART serial over the nRF USB serial.
  This option is set by default when the binary format is enabled.

And, in the overlay file like this:

.. code-block:: devicetree

   chosen {
       ncs,zboss-trace-uart = &uart1;
   };

Alternatively, you can configure :ref:`Zigbee stack logs <zigbee_ug_logging_stack_logs>` to be printed in binary format using an independent CDC ACM device of the same nRF USB.
Complete the following steps:

1. Set the following Kconfig options:

   * :kconfig:option:`CONFIG_ZBOSS_TRACE_BINARY_LOGGING` - This option enables the binary format.
   * :kconfig:option:`CONFIG_ZBOSS_TRACE_USB_CDC_LOGGING` - This option selects nRF USB serial over UART serial.

#. Create two instances of USB CDC ACM for the application:

   a. Create two entries in the DTS overlay file :file:`<board>_usb.overlay` for the selected board, one for each USB CDC ACM instance.
      See :ref:`zephyr:usb_device_cdc_acm` for more information.
   #. Extend the ``zephyr_udc0`` node in the DTS overlay file to also configure the second USB CDC ACM instance ``"cdc_acm_uart1"``:

      .. code-block:: devicetree

         &zephyr_udc0 {
            cdc_acm_uart0: cdc_acm_uart0 {
               compatible = "zephyr,cdc-acm-uart";
            };

            cdc_acm_uart1: cdc_acm_uart1 {
               compatible = "zephyr,cdc-acm-uart";
            };
         };

#. Configure the chosen tracing UART device like this:

   .. code-block:: devicetree

      chosen {
          ncs,zboss-trace-uart = &cdc_acm_uart1;
      };

#. Enable the composite USB device driver using the :kconfig:option:`CONFIG_USB_COMPOSITE_DEVICE` Kconfig option.

With this configuration, you have two serial ports created by the NCP sample.
Use the first one for NCP communication.
Use the second serial port for collecting Zigbee stack logs.

You can also check the :file:`prj_usb.conf` file that contains the complete Kconfig configuration of Zigbee stack logs over nRF USB serial commented out.
For more configuration options, see :ref:`Zigbee stack logs <zigbee_ug_logging_stack_logs>`.

.. _zigbee_ncp_bootloader:

Bootloader support
==================

The bootloader support in the NCP sample depends on the development kit, its respective board target, and `Serial communication setup`_:

* For the ``nrf52840dongle/nrf52840`` board target, the `nRF5 SDK Bootloader`_ is used by default because the dongle comes with this bootloader preinstalled.
* For the ``nrf52840dk/nrf52840``, ``nrf52833dk/nrf52833``, and ``nrf21540dk/nrf52840`` board targets, the following scenarios are possible when building for them:

  * If you select `Communication through USB`_, `MCUboot`_ is enabled by default.
  * If you use the default UART serial communication channel, the bootloader support is not enabled, but you can enable MCUboot.

MCUboot
-------

When you select `Communication through USB`_, MCUboot is built with support for a single application slot, and it uses the USB DFU class driver to allow uploading of the image over USB.

If you want to use the default UART serial communication channel, set the :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` Kconfig option to enable MCUboot.
To use the same MCUboot configuration as in `Communication through USB`_, you need to provide MCUboot with the Kconfig options included in the :file:`child_image/mcuboot_usb.conf` file.
See :ref:`ug_multi_image_variables` to learn how to set the required options.

MCUboot with the USB DFU requires a larger partition.
To increase the partition, define the ``PM_STATIC_YML_FILE`` variable that provides the path to the :file:`pm_static.yml` static configuration file in the :file:`configuration` directory for the board target of your choice.

For instructions on how to set these additional options and configuration at build time, see :ref:`cmake_options` and :ref:`configure_application`.

See the following command-line instruction for an example:

.. code-block:: console

   west build samples/zigbee/ncp -b nrf52840dk/nrf52840 -- -DCONFIG_BOOTLOADER_MCUBOOT=y  -Dmcuboot_CONFIG_MULTITHREADING=y -Dmcuboot_CONFIG_BOOT_USB_DFU_WAIT=y -Dmcuboot_CONFIG_SINGLE_APPLICATION_SLOT=y -DPM_STATIC_YML_FILE=samples/zigbee/ncp/configuration/nrf52840dk_nrf52840/pm_static.yml

When building the sample, the build system also generates the signed :file:`app_update.bin` image file in the :file:`build` directory.
You can use this file to upgrade a device.
See :ref:`mcuboot_ncs` for more information about this and other automatically generated files.

After every reset, the sample first boots to MCUboot and then, after a couple of seconds, the NCP sample is booted.
When booted to MCUboot, you can upload the new image with the `dfu-util tool`_.
See the Testing section of the :zephyr:code-sample:`usb-dfu` for the list of required dfu-util commands.

To learn more about configuring bootloader for an application in |NCS|, see the :ref:`ug_bootloader` page.

nRF5 SDK Bootloader
-------------------

When the sample is built for ``nrf52840dongle/nrf52840``, the build system does not produce an upgrade image.
To upgrade the dongle, you can use one of the following options:

* nRF Connect Programmer application (part of `nRF Connect for Desktop`_).

  For more details, see `Programming the nRF52840 Dongle`_ in the nRF Connect Programmer user guide.

* `nRF Util`_ tool, if you do not want to use the nRF Connect Programmer application.

  To generate a DFU package, see `Generating DFU packages`_ in the nRF Util user guide.
  Upgrading the dongle using this method requires putting the dongle into the DFU mode.
  When in the DFU mode, you can use `nRF Util`_ for sending the upgrade image.
  See `DFU over a serial USB connection`_ in the nRF Util user guide.

  .. note::
      By default, you can enter the DFU mode on the dongle using the pin reset.
      Alternatively, you can call :c:func:`ncp_host_ota_run_bootloader` to trigger the bootloader on the dongle from the NCP Host application.

FEM support
===========

.. include:: /includes/sample_fem_support.txt

.. _zigbee_ncp_vendor_specific_commands:

Vendor-specific commands
========================

You can extend the NCP sample with support for vendor-specific commands to implement new features, such as controlling LEDs.
These commands let you request custom actions over the ZBOSS NCP protocol.

Vendor-specific commands are sent over the NCP protocol using the following types of packets:

* Request

  These can be sent only by the host.
  The nRF SoC must respond to all of them.

* Response

  These can be sent only by the nRF SoC.
  The nRF SoC can send them only when requested by the host.
  This limitation can be bypassed by indications.

* Indication

  These can be sent only by the nRF SoC.
  The indications are sent on demand from the nRF SoC to the host, for example to inform the host about polling avoidance or sudden or rare events, such as pressing the button.
  Indications are neither responded to nor acknowledged.

By default, the NCP sample already supports the vendor-specific commands for controlling a LED.

Implementing vendor-specific commands
-------------------------------------

.. note::
      The steps in this section describe the process of implementing the vendor-specific commands in the NCP sample.
      Documentation for the implementation steps for the host side is not yet available.

To implement custom vendor-specific commands, you need to modify the NCP sample as follows:

1. Implement a callback for handling custom commands requests.

   The function must follow the :c:func:`zb_ncp_custom_request_cb_t` declaration.
   For an example, see :c:func:`ncp_vendor_specific_req_handler` in the sample.
   This function parses the command payload, validates it, performs required action on **LED 2**, and sends a response.

#. Register the handler function using :c:func:`zb_ncp_custom_register_request_cb`, as implemented in the sample.

   Once the callback is registered, it is called for every custom command request received from the host.

#. At the end of the custom request callback, implement the response to the request using one of the following options:

   * Implement an immediate response that is sent just after the request is received and parsed.
     For this option, make sure that the length of the response is returned from the custom command callback (implemented in step 1) and :c:func:`zb_ncp_custom_response` is called with a buffer that contains both the response payload and the parameters encapsulated in :c:struct:`ncp_hl_custom_resp_t`.
   * Implement a delayed response that can be useful for example to implement commands that require more time to complete.
     For this option, make sure that :c:macro:`NCP_RET_LATER` is returned from the custom command callback (implemented in step 1) and :c:func:`zb_ncp_custom_response` is called after this callback.

   If the function fails, it must return an error code, while a response is automatically generated by the stack.
   See :c:func:`ncp_vendor_specific_req_handler` for an example of the immediate response.
#. Implement custom indications by using :c:func:`zb_ncp_custom_indication` to send an indication to the host.

   The NCP sample demonstrates the indication usage to inform the host about the index of the LED that is controlled by custom requests.
   The indication is sent three seconds after start-up.
   See :c:func:`custom_indication` in the sample to see how to pass the data to the indication's payload.

Once you complete these steps and configure the vendor-specific commands on the host, you can call these commands from the host to use the extended functionalities in the NCP sample.

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

After building the sample and programming it to your development kit, complete the following steps to test it:

1. Download and extract the `ZBOSS NCP Host`_ package.

   .. note::
      If you are using a Linux distribution different than the 64-bit Ubuntu 18.04, make sure to rebuild the package libraries and applications.
      Follow the instructions in the `Rebuilding the ZBOSS libraries for host`_ section in the `NCP Host documentation`_.

#. If you are using `Communication through USB`_, connect the nRF USB port of the NCP sample's development kit to the PC USB port with a USB cable.
#. Get the kit’s serial port name (for example, ``/dev/ttyACM0``).

   If you are communicating through the nRF USB, get the nRF USB serial port name.

#. Turn on the development kit that runs the Light bulb sample.
#. To start the simple gateway application, run the following command with *serial_port_name* replaced with the serial port name used for communication with the NCP sample:

   .. parsed-literal::
      :class: highlight

      NCP_SLAVE_PTY=*serial_port_name* ./application/simple_gw/simple_gw

The simple gateway device forms the Zigbee network and opens the network for 180 seconds for new devices to join.
When the light bulb joins the network, the **LED 3** on the light bulb device turns on to indicate that it is connected to the simple gateway.
The gateway then starts discovering the On/Off cluster.
When it is found, the simple gateway configures bindings and reporting for the device.
It then starts sending On/Off toggle commands with a 15-second interval that toggle the **LED 4** on the light bulb on and off.

Dependencies
************

This sample uses the following |NCS| libraries:

* Zigbee subsystem:

  * :file:`zb_nrf_platform.h`

It uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:zboss` |zboss_version| (`API documentation`_)

In addition, it uses the following Zephyr libraries:

* :file:`include/device.h`
* :ref:`zephyr:usb_api`
* :ref:`zephyr:logging_api`
* :ref:`zephyr:uart_api`
