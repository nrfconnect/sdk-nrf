.. _slm_description:

Application description
#######################

.. contents::
   :local:
   :depth: 3

The Serial LTE Modem (SLM) application demonstrates how to use the nRF9160 as a stand-alone LTE modem that can be controlled by AT commands.

Overview
********

The nRF9160 SiP integrates both a full LTE modem and an application MCU, enabling you to run your LTE application directly on the nRF9160.

However, if you want to run your application on a different chip and use the nRF9160 only as a modem, the serial LTE modem application provides you with an interface for controlling the LTE modem through AT commands.

The application accepts both the modem-specific AT commands documented in the `nRF91 AT Commands Reference Guide <AT Commands Reference Guide_>`_ and the proprietary AT commands documented in the :ref:`SLM_AT_intro` page.

Requirements
************

The application supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Configuration
*************

|config|

.. _slm_config:

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. _CONFIG_SLM_CUSTOMIZED:

CONFIG_SLM_CUSTOMIZED - Flag for customized functionality
   This flag can be used to enable customized functionality.
   To add your own custom logic, enclose the code by ``#if defined(CONFIG_SLM_CUSTOMIZED)`` and enable this flag.

.. _CONFIG_SLM_NATIVE_TLS:

CONFIG_SLM_NATIVE_TLS - Use Zephyr mbedTLS
   This option enables using Zephyr's mbedTLS.
   It requires additional configuration.
   See :ref:`slm_native_tls` for more information.

.. _CONFIG_SLM_EXTERNAL_XTAL:

CONFIG_SLM_EXTERNAL_XTAL - Use external XTAL for UARTE
   This option configures the application to use an external XTAL for UARTE.
   See the `nRF9160 Product Specification`_ (section 6.19 UARTE) for more information.

.. _CONFIG_SLM_START_SLEEP:

CONFIG_SLM_START_SLEEP - Enter sleep on startup
   This option makes nRF9160 enter deep sleep after startup.
   It is not selected by default.

.. _CONFIG_SLM_WAKEUP_PIN:

CONFIG_SLM_WAKEUP_PIN - Interface GPIO to exit from sleep or idle
   This option specifies which interface GPIO to use for exiting sleep or idle mode.
   It is set by default as follows:

   * On the nRF9160 DK:

     * **P0.6** (Button 1 on the nRF9160 DK) is used when UART_0 is used.
     * **P0.31** is used when UART_1 is used.

   * On Thingy:91, **P0.26** (Multi-function button on Thingy:91) is used.

   .. note::
      This pin is used as input GPIO and configured as *Active Low*.
      By default, the application pulls up this GPIO.

.. _CONFIG_SLM_INDICATE_PIN:

CONFIG_SLM_INDICATE_PIN - Interface GPIO to indicate data available or unsolicited event notifications
   This option specifies which interface GPIO to use for indicating data available or unsolicited event notifications from the modem.
   On the nRF9160 DK, it is set by default as follows:

   * **P0.2** (LED 1 on the nRF9160 DK) is used when UART_0 is selected.
   * **P0.30** is used when UART_2 is selected.

   It is not defined when the target is Thingy:91.

   .. note::
      This pin is used as output GPIO and configured as *Active Low*.
      By default, the application sets this GPIO as *Inactive High*.

.. _CONFIG_SLM_INDICATE_TIME:

CONFIG_SLM_INDICATE_TIME - Indicate GPIO active time
   This option specifies the length, in milliseconds, of the time interval during which the indicate GPIO must stay active.
   The default value is 100 milliseconds.

.. _CONFIG_SLM_SOCKET_RX_MAX:

CONFIG_SLM_SOCKET_RX_MAX - Maximum RX buffer size for receiving socket data
   This option specifies the maximum buffer size for receiving data through the socket interface.
   By default, this size is set to :c:enumerator:`NET_IPV4_MTU` (576), which is defined in Zephyr.
   The maximum value is 708, which is the maximum segment size (MSS) defined for the modem.

   This option impacts the total RAM usage.

.. _CONFIG_SLM_CR_TERMINATION:

CONFIG_SLM_CR_TERMINATION - CR termination
   This option configures the application to accept AT commands ending with a carriage return.

   Select this option if you want to connect to the development kit using PuTTY.
   See :ref:`putty` for instructions.

.. _CONFIG_SLM_LF_TERMINATION:

CONFIG_SLM_LF_TERMINATION - LF termination
   This option configures the application to accept AT commands ending with a line feed.

.. _CONFIG_SLM_CR_LF_TERMINATION:

CONFIG_SLM_CR_LF_TERMINATION - CR+LF termination
   This option configures the application to accept AT commands ending with a carriage return followed by a line feed.

.. _CONFIG_SLM_TCP_POLL_TIME:

CONFIG_SLM_TCP_POLL_TIME - Poll timeout in seconds for TCP connection
   This option specifies the poll timeout for the TCP connection, in seconds.

.. _CONFIG_SLM_SMS:

CONFIG_SLM_SMS - SMS support in SLM
   This option enables additional AT commands for using the SMS service.

.. _CONFIG_SLM_GNSS:

CONFIG_SLM_GNSS - GNSS support in SLM
   This option enables additional AT commands for using the GNSS service.

.. _CONFIG_SLM_AGPS:

CONFIG_SLM_AGPS - nRF Cloud A-GPS support in SLM
   This option enables additional AT commands for using the nRF Cloud A-GPS service.
   It is not selected by default.

.. _CONFIG_SLM_PGPS:

CONFIG_SLM_PGPS - nRF Cloud P-GPS support in SLM
   This option enables additional AT commands for using the nRF Cloud P-GPS service.
   It is not selected by default.

.. _CONFIG_SLM_LOCATION:

CONFIG_SLM_LOCATION - nRF Cloud cellular and Wi-Fi location support in SLM
   This option enables additional AT commands for using the nRF Cloud location service.
   It is not selected by default.

.. _CONFIG_SLM_FTPC:

CONFIG_SLM_FTPC - FTP client support in SLM
   This option enables additional AT commands for using the FTP client service.

.. _CONFIG_SLM_TFTPC:

CONFIG_SLM_TFTPC - TFTP client support in SLM
   This option enables additional AT commands for using the TFTP client service.

.. _CONFIG_SLM_MQTTC:

CONFIG_SLM_MQTTC - MQTT client support in SLM
   This option enables additional AT commands for using the MQTT client service.

.. _CONFIG_SLM_MQTTC_MESSAGE_BUFFER_LEN:

CONFIG_SLM_MQTTC_MESSAGE_BUFFER_LEN - Size of the buffer for the MQTT library
   This option specifies the maximum message size which can be transmitted or received through MQTT (excluding PUBLISH payload).
   The default value is 512, meaning 512 bytes for TX and RX, respectively.

.. _CONFIG_SLM_HTTPC:

CONFIG_SLM_HTTPC - HTTP client support in SLM
   This option enables additional AT commands for using the HTTP client service.

.. _CONFIG_SLM_TWI:

CONFIG_SLM_TWI - TWI support in SLM
   This option enables additional AT commands for using the TWI service.

Additional configuration
========================

To save power, both the console and the output logs over ``UART_0`` are disabled in this application.
This information is logged using RTT instead.
See :ref:`testing_rtt_connect` for instructions on how to view this information.

To switch to UART output, change the following options in the :file:`prj.conf` file::

   # Segger RTT
   CONFIG_USE_SEGGER_RTT=n
   CONFIG_RTT_CONSOLE=n
   CONFIG_UART_CONSOLE=y
   CONFIG_LOG_BACKEND_RTT=n
   CONFIG_LOG_BACKEND_UART=y


Configuration files
===================

The sample provides predefined configuration files for both the parent image and the child image.
You can find the configuration files in the :file:`applications/serial_lte_modem` directory.

The following configuration files are provided:

* :file:`prj.conf` - This configuration file contains the standard configuration for the serial LTE modem application.

* :file:`overlay-native_tls.conf` - This configuration file contains additional configuration options that are required to use :ref:`slm_native_tls`.
  You can include it by adding ``-DOVERLAY_CONFIG=overlay-native_tls.conf`` to your build command.
  See :ref:`cmake_options`.

* :file:`overlay-secure_bootloader.conf` - This configuration file contains additional configuration options that are required to use :ref:`ug_bootloader`.
  You can include it by adding ``-DOVERLAY_CONFIG=overlay-secure_bootloader`` to your build command.
  See :ref:`cmake_options`.

* :file:`overlay-carrier.conf` - Configuration file that adds |NCS| :ref:`liblwm2m_carrier_readme` support.
  See :ref:`slm_carrier_library_support` for more information on how to connect to an operator's device management platform.

* :file:`boards/nrf9160dk_nrf9160_ns.conf` - Configuration file specific for the nRF9160 DK.
  This file is automatically merged with the :file:`prj.conf` file when you build for the ``nrf9160dk_nrf9160_ns`` build target.

* :file:`boards/thingy91_nrf9160_ns.conf` - Configuration file specific for Thingy:91.
  This file is automatically merged with the :file:`prj.conf` file when you build for the ``thingy91_nrf9160_ns`` build target.

In general, Kconfig overlays have an ``overlay-`` prefix and a :file:`.conf` extension.
Board-specific configuration files are named :file:`<BOARD>.conf` and are located in the :file:`boards` folder.
DTS overlay files are named as the build target they are meant to be used with, and use the file extension :file:`.overlay`.
They are also placed in the :file:`boards` folder.
When the DTS overlay filename matches the build target, the overlay is automatically chosen and applied by the build system.

See :ref:`app_build_system`: for more information on the |NCS| configuration system.

.. _slm_native_tls:

Native TLS
----------

By default, the secure socket (TLS/DTLS) is offloaded onto the modem.
However, if you need customized TLS/DTLS features that are not supported by the modem firmware, you can use native TLS instead.
Currently, the SLM application can be built to use native TLS for the following services:

* Secure socket
* TLS Proxy server
* HTTPS client

If native TLS is enabled, you must use the ``AT#XCMNG`` command to store the credentials.

.. note::

   The modem needs to be in an offline state when storing the credentials.
   The SLM application supports security tags ranging from ``0`` to ``214748364``.

The configuration options that are required to enable the native TLS socket are defined in the :file:`overlay-native_tls.conf` file.

.. note::

   Native TLS sockets have the following limitations:

   * PSK, PSK identity, and PSK public key are currently not supported.
   * The DTLS server is currently not supported.
   * TLS session resumption is currently not supported.

.. _slm_building:

Building and running
********************

.. |sample path| replace:: :file:`applications/serial_lte_modem`

.. include:: /includes/build_and_run_ns.txt

.. _slm_connecting_9160dk:

Communicating with the modem on the nRF9160 DK
==============================================

In this scenario, the nRF9160 DK running the Serial LTE Modem application serves as the host.
You can use either a PC or an external MCU as a client.

.. _slm_connecting_9160dk_pc:

Connecting with a PC
--------------------

To connect to the nRF9160 DK with a PC

.. slm_connecting_9160dk_pc_instr_start

1. Verify that ``UART_0`` is selected in the application.
   It is defined in the default configuration.

2. Use LTE Link Monitor to connect to the development kit.
   See :ref:`lte_connect` for instructions.
   You can then use this connection to send or receive AT commands over UART, and to see the log output of the development kit.

   Alternatively, you can use a terminal emulator like `Termite`_, `Teraterm`_, or PuTTY to establish a terminal connection to the development kit, using the following settings:

   * Baud rate: 115200
   * 8 data bits
   * 1 stop bit
   * No parity
   * HW flow control: None

   .. note::

      The default AT command terminator is a carriage return followed by a line feed (``\r\n``).
      LTE Link Monitor supports this format.
      If you want to use another terminal emulator, make sure that the configured AT command terminator corresponds to the line terminator of your terminal.

      When using `Termite`_ and `Teraterm`_, configure the AT command terminator as follows:

      .. figure:: images/termite.svg
         :alt: Termite configuration for sending AT commands through UART

      .. figure:: images/teraterm.svg
         :alt: Teraterm configuration for sending AT commands through UART

      When using PuTTY, you must set the :ref:`CONFIG_SLM_CR_TERMINATION <CONFIG_SLM_CR_TERMINATION>` SLM configuration option instead.
      See :ref:`application configuration <slm_config>` for more details.

.. slm_connecting_9160dk_pc_instr_end

.. _slm_connecting_9160dk_mcu:

Connecting with an external MCU
-------------------------------

.. note::

   This section does not apply to Thingy:91 as it does not have UART2.

If you run your user application on an external MCU (for example, an nRF52 Series development kit), you can control the modem on nRF9160 directly from the application.
See the :ref:`slm_shell_sample` for a sample implementation of such an application.

To connect with an external MCU using UART_2, change the configuration files for the default board as follows:

* In the :file:`nrf9160dk_nrf9160_ns.conf` file::

     # Use UART_0 (when working with PC terminal)
     # unmask the following config
     #CONFIG_UART_0_NRF_HW_ASYNC_TIMER=2
     #CONFIG_UART_0_NRF_HW_ASYNC=y
     #CONFIG_SLM_WAKEUP_PIN=6
     #CONFIG_SLM_INDICATE_PIN=2

     # Use UART_2 (when working with external MCU)
     # unmask the following config
     CONFIG_UART_2_NRF_HW_ASYNC_TIMER=2
     CONFIG_UART_2_NRF_HW_ASYNC=y
     CONFIG_SLM_WAKEUP_PIN=31
     CONFIG_SLM_INDICATE_PIN=30


* In the :file:`nrf9160dk_nrf9160_ns.overlay` file::

     / {
         chosen {
                  ncs,slm-uart = &uart2;
                }
       };

     &uart0 {
        status = "disabled";
     };

     &uart2 {
        compatible = "nordic,nrf-uarte";
        current-speed = <115200>;
        status = "okay";
        hw-flow-control;

        pinctrl-0 = <&uart2_default_alt>;
        pinctrl-1 = <&uart2_sleep_alt>;
        pinctrl-names = "default", "sleep";
     };


The following table shows how to connect an nRF52 Series development kit to the nRF9160 DK to be able to communicate through UART:

.. list-table::
   :align: center
   :header-rows: 1

   * - nRF52 Series DK
     - nRF9160 DK
   * - UART TX P0.6
     - UART RX P0.11
   * - UART RX P0.8
     - UART TX P0.10
   * - UART CTS P0.7
     - UART RTS P0.12
   * - UART RTS P0.5
     - UART CTS P0.13
   * - GPIO OUT P0.27
     - GPIO IN P0.31
   * - GPIO IN P0.26
     - GPIO OUT P0.30

Use the following UART devices:

* nRF52840 or nRF52832 - UART0
* nRF9160 - UART2

Use the following UART configuration:

* Hardware flow control: enabled
* Baud rate: 115200
* Parity bit: no
* Operation mode: IRQ

.. note::
   The GPIO output level on the nRF9160 side must be 3 V.
   You can set the VDD voltage with the **VDD IO** switch (**SW9**).
   See the `VDD supply rail section in the nRF9160 DK User Guide`_ for more information.

.. _slm_connecting_thingy91:

Communicating with the modem on Thingy:91
=========================================

In this scenario, Thingy:91 running the Serial LTE Modem application serves as the host.
You can use only a PC as a client.

.. _slm_connecting_thingy91_pc:

Connecting with a PC
--------------------

To connect to Thingy:91 with a PC, you must first program the :ref:`connectivity_bridge` on the nrf52840 of Thingy:91.
It routes ``UART_0`` to ``USB_CDC0`` on Thingy:91.
By enabling the option ``CONFIG_BRIDGE_BLE_ENABLE`` , you can also use SLM over :ref:`nus_service_readme`.

Then follow the instructions below:

.. include:: slm_description.rst
   :start-after: .. slm_connecting_9160dk_pc_instr_start
   :end-before: .. slm_connecting_9160dk_pc_instr_end

You can also test the i2c sensor on Thingy:91 using :ref:`SLM_AT_TWI`.
See :ref:`slm_testing_twi` for more details.

.. _slm_testing_section:

Testing
=======

The following testing instructions focus on testing the application with a PC client.
If you have an nRF52 Series DK running a client application, you can also use this DK for testing the different scenarios.

|test_sample|

1. |connect_kit|
#. :ref:`Connect to the kit with LTE Link Monitor <lte_connect>`.
   If you want to use a different terminal emulator, see `slm_connecting_9160dk_pc`_.
#. Reset the kit.
#. Observe that the development kit sends a ``Ready\r\n`` message on UART.
#. Enter ``AT+CFUN=1`` to turn on the modem and connect to the network.
#. Enter ``AT+CFUN?`` and observe that the connection indicators in the LTE Link Monitor side panel turn green.
   This indicates that the modem is connected to the network.
#. Send AT commands and observe the responses from the development kit.

   See :ref:`slm_testing` for typical test cases.

.. _slm_carrier_library_support:

Using the LwM2M carrier library
===============================

.. |application_sample_name| replace:: Serial LTE Modem application

.. include:: /includes/lwm2m_carrier_library.txt

The certificate provisioning can also be done directly in the Serial LTE Modem application by using the same AT commands as described for the :ref:`at_client_sample` sample.

Enabling the LwM2M carrier library will disable this application's support for GNSS in order to have enough space in flash.

Dependencies
************

This application uses the following |NCS| libraries:

* :ref:`at_cmd_parser_readme`
* :ref:`at_monitor_readme`
* :ref:`nrf_modem_lib_readme`
* :ref:`lib_modem_jwt`
* :ref:`lib_ftp_client`
* :ref:`sms_readme`
* :ref:`lib_fota_download`
* :ref:`lib_download_client`
* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_agps`
* :ref:`lib_nrf_cloud_pgps`
* :ref:`lib_nrf_cloud_cell_pos`

It uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
