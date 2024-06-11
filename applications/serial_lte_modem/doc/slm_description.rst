.. _slm_description:

Application description
#######################

.. contents::
   :local:
   :depth: 3

The Serial LTE Modem (SLM) application demonstrates how to use an nRF91 Series device as a stand-alone LTE modem that can be controlled by AT commands.

Overview
********

The nRF91 Series SiP integrates both a full LTE modem and an application MCU, enabling you to run your LTE application directly on the device.

However, if you want to run your application on a different chip and use the nRF91 Series device only as a modem, the serial LTE modem application provides you with an interface for controlling the LTE modem through AT commands.

The application accepts both the modem-specific AT commands and proprietary AT commands.
The AT commands are documented in the following guides:

* Modem-specific AT commands - `nRF91x1 AT Commands Reference Guide`_  and `nRF9160 AT Commands Reference Guide`_
* Proprietary AT commands - :ref:`SLM_AT_commands`

Requirements
************

The application supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Configuration
*************

|config|

.. _slm_config_options:

Configuration options
=====================

.. _CONFIG_SLM_CUSTOMER_VERSION:

CONFIG_SLM_CUSTOMER_VERSION - Customer version string
   Version string defined by the customer after customizing the application.
   When defined, this version is reported with the baseline versions by the ``#XSLMVER`` AT command.

.. _CONFIG_SLM_AT_MAX_PARAM:

CONFIG_SLM_AT_MAX_PARAM - AT command parameter count limit
   This defines the maximum number of parameters allowed in an AT command, including the command name.

.. _CONFIG_SLM_CMUX:

CONFIG_SLM_CMUX - Enable CMUX functionality
   This option is enabled by the CMUX overlay.
   It adds support for CMUX.
   See :ref:`SLM_AT_CMUX` for more information.

.. _CONFIG_SLM_PPP:

CONFIG_SLM_PPP - Enable PPP functionality
   This option is enabled by the PPP overlay.
   It adds support for PPP.
   PPP can be used in conjunction with :ref:`CMUX <CONFIG_SLM_CMUX>` in order to use a single UART for both AT data and PPP.
   When CMUX is also enabled, PPP is usable only through a CMUX channel.
   See :ref:`SLM_AT_PPP` for more information.

.. _CONFIG_SLM_NATIVE_TLS:

CONFIG_SLM_NATIVE_TLS - Use Zephyr's Mbed TLS for TLS connections
   This option is enabled by the native TLS overlay.
   See :ref:`slm_native_tls` for more information.

.. _CONFIG_SLM_NATIVE_TLS_CREDENTIAL_BUFFER_SIZE:

CONFIG_SLM_NATIVE_TLS_CREDENTIAL_BUFFER_SIZE - Buffer space reserved for loading credentials
   Specifies the credential buffer size available for a single ``sec_tag`` when loading credentials for Mbed TLS.
   The default value is ``4096``.

.. _CONFIG_SLM_NATIVE_TLS_CREDENTIAL_BUFFER_COUNT:

CONFIG_SLM_NATIVE_TLS_CREDENTIAL_BUFFER_COUNT - Number of buffers for loading credentials
   The number of buffers available for loading ``sec_tag`` credentials for Mbed TLS.
   TLS client only needs the buffer when connecting, while TLS server needs the buffer as long as it is running.
   Increase the value if you need both TLS client and server running simultaneously with different ``sec_tags``.
   The default value is ``1``.

.. _CONFIG_SLM_EXTERNAL_XTAL:

CONFIG_SLM_EXTERNAL_XTAL - Use external XTAL for UARTE
   This option configures the application to use an external XTAL for UARTE.
   For the nRF9160 DK, see the `nRF9160 Product Specification`_ (section 6.19 UARTE) for more information.
   For the nRF9161 DK, see the `nRF9161 Product Specification`_ (section 6.19 UARTE) for more information.

.. _CONFIG_SLM_START_SLEEP:

CONFIG_SLM_START_SLEEP - Enter sleep on startup
   This option makes an nRF91 Series device enter deep sleep after startup.
   It is not enabled by default.

.. _CONFIG_SLM_POWER_PIN:

CONFIG_SLM_POWER_PIN - Interface GPIO pin to power off the SiP and exit from sleep or idle
   This option specifies which pin to use to power on or off the SiP and make SLM exit idle mode.
   It is set by default as follows:

   * On the nRF9161 DK:

     * **P0.8** (Button 1 on the nRF9161 DK) is used when UART_0 is used.
     * **P0.31** is used when UART_1 is used.

   * On the nRF9160 DK:

     * **P0.6** (Button 1 on the nRF9160 DK) is used when UART_0 is used.
     * **P0.31** is used when UART_1 is used.

   * On Thingy:91 and Thingy:91 X, **P0.26** (Multi-function button) is used.

   .. note::
      This pin is configured with a pull up, so it is active low.
      It must be pulled down for a short time to perform one power off or wake up operation.

.. _CONFIG_SLM_INDICATE_PIN:

CONFIG_SLM_INDICATE_PIN - Interface GPIO pin to indicate data available or unsolicited event notifications
   This option specifies which pin to use for indicating data available or unsolicited event notifications from the modem.
   It is set by default as follows:

   * On the nRF9161 DK:

     * **P0.00** (LED 1 on the nRF9161 DK) is used when UART_0 is selected.
     * **P0.30** is used when UART_2 is selected.

   * On the nRF9160 DK:

     * **P0.2** (LED 1 on the nRF9160 DK) is used when UART_0 is selected.
     * **P0.30** is used when UART_2 is selected.

   * It is not defined when the targets are Thingy:91 and Thingy:91 X.

   .. note::
      This pin is configured to be active low, so it will be high when inactive.

.. _CONFIG_SLM_INDICATE_TIME:

CONFIG_SLM_INDICATE_TIME - Indicate GPIO active time
   This option specifies the length, in milliseconds, of the time interval during which the indicate GPIO must stay active.
   The default value is 100 milliseconds.

.. _CONFIG_SLM_AUTO_CONNECT:

CONFIG_SLM_AUTO_CONNECT - Connect to LTE network at start-up or reset
   This option enables connecting to the LTE network at start-up or reset using a defined PDN configuration.
   This option is enabled by the LwM2M Carrier overlay, but is otherwise disabled by default.

   .. note::
      This option requires network-specific configuration in the ``slm_auto_connect.h`` file.

   Here is a sample configuration for NIDD connection in the :file:`slm_auto_connect.h` file::

      /* Network-specific default system mode configured by %XSYSTEMMODE (refer to AT command manual) */
      0,        /* LTE support */
      1,        /* NB-IoT support */
      0,        /* GNSS support, also define CONFIG_MODEM_ANTENNA if not Nordic DK */
      0,        /* LTE preference */
      /* Network-specific default PDN configured by +CGDCONT and +CGAUTH (refer to AT command manual) */
      true,     /* PDP context definition required or not */
      "Non-IP", /* PDP type: "IP", "IPV6", "IPV4V6", "Non-IP" */
      "",       /* Access point name */
      0,        /* PDP authentication protocol: 0(None), 1(PAP), 2(CHAP) */
      "",       /* PDN connection authentication username */
      ""        /* PDN connection authentication password */

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

.. _CONFIG_SLM_NRF_CLOUD:

CONFIG_SLM_NRF_CLOUD - nRF Cloud support in SLM
   This option enables additional AT commands for using the nRF Cloud service.

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

.. _CONFIG_SLM_UART_RX_BUF_COUNT:

CONFIG_SLM_UART_RX_BUF_COUNT - Receive buffers for UART.
   This option defines the number of buffers for receiving (RX) UART traffic.
   The default value is 3.

.. _CONFIG_SLM_UART_RX_BUF_SIZE:

CONFIG_SLM_UART_RX_BUF_SIZE - Receive buffer size for UART.
   This option defines the size of a single buffer for receiving (RX) UART traffic.
   The default value is 256.

.. _CONFIG_SLM_UART_TX_BUF_SIZE:

CONFIG_SLM_UART_TX_BUF_SIZE - Send buffer size for UART.
   This option defines the size of the buffer for sending (TX) UART traffic.
   The default value is 256.

.. _slm_additional_config:

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

.. _slm_config_files:

Configuration files
===================

You can find the configuration files in the :file:`applications/serial_lte_modem` directory.

In general, they have an ``overlay-`` prefix, and a :file:`.conf` or :file:`.overlay` extension for Kconfig or devicetree overlays, respectively.
Board-specific configuration files are named :file:`<BOARD>` with a :file:`.conf` or :file:`.overlay` extension and are located in the :file:`boards` directory.
When the name of the board-specific configuration file matches the board target, the overlay is automatically included by the build system.

See :ref:`app_build_system`: for more information on the |NCS| configuration system.

.. important::

  When adding Kconfig fragments and devicetree overlays, make sure to use the ``-DEXTRA_CONF_FILE`` and ``-DEXTRA_DTC_OVERLAY_FILE`` CMake parameters, respectively.
  Otherwise, if ``-DCONF_FILE`` or ``-DDTC_OVERLAY_FILE`` is used, all the configuration files that normally get picked up automatically will have to be included explicitly.

The following configuration files are provided:

* :file:`prj.conf` - This configuration file contains the standard configuration for the serial LTE modem application and is included by default by the build system.

* :file:`overlay-native_tls.conf` - This configuration file contains additional configuration options that are required to use :ref:`slm_native_tls`.
  You can include it by adding ``-DEXTRA_CONF_FILE=overlay-native_tls.conf`` to your build command.
  See :ref:`cmake_options`.

* :file:`overlay-carrier.conf` - Configuration file that adds |NCS| :ref:`liblwm2m_carrier_readme` support.
  See :ref:`slm_carrier_library_support` for more information on how to connect to an operator's device management platform.

* :file:`overlay-full_fota.conf` - Configuration file that adds full modem FOTA support.
  See :ref:`SLM_AT_FOTA` for more information on how to use full modem FOTA functionality.

* :file:`overlay-cmux.conf` - Configuration file that adds support for the CMUX protocol.
  See :ref:`SLM_AT_CMUX` for more information.

* :file:`overlay-ppp.conf` - Configuration file that adds support for the Point-to-Point Protocol (PPP).
  This disables most of the IP-based protocols available through AT commands (such as FTP and MQTT) as it is expected that the controlling chip's own IP stack is used instead.
  See :ref:`CONFIG_SLM_PPP <CONFIG_SLM_PPP>` and :ref:`SLM_AT_PPP` for more information.

* :file:`overlay-ppp-without-cmux.conf` - Kconfig fragment that configures the UART to be used by PPP.
  This configuration file should be included when building SLM with PPP and without CMUX.

* :file:`overlay-ppp-without-cmux.overlay` - Devicetree overlay that configures the UART to be used by PPP.
  This configuration file should be included when building SLM with PPP and without CMUX, in addition to :file:`overlay-ppp.conf`.
  It can be customized to fit your configuration (UART, baud rate, and so on).
  By default, it sets the baud rate of the PPP UART to 1 000 000.

* :file:`overlay-zephyr-modem.conf`, :file:`overlay-zephyr-modem-external-mcu.conf`, :file:`overlay-zephyr-modem-nrf9160dk-nrf52840.conf`, :file:`overlay-external-mcu.overlay`,  and :file:`overlay-zephyr-modem-nrf9160dk-nrf52840.overlay` - These configuration files are used when compiling SLM to turn an nRF91 Series SiP into a Zephyr-compatible standalone modem.
  See :ref:`slm_as_zephyr_modem` for more information.

* :file:`boards/nrf9160dk_nrf9160_ns.conf` - Configuration file specific for the nRF9160 DK.
  This file is automatically merged with the :file:`prj.conf` file when you build for the ``nrf9160dk/nrf9160/ns`` board target.

* :file:`boards/nrf9161dk_nrf9161_ns.conf` - Configuration file specific for the nRF9161 DK.
  This file is automatically merged with the :file:`prj.conf` file when you build for the ``nrf9161dk/nrf9161/ns`` board target.

* :file:`boards/thingy91_nrf9160_ns.conf` - Configuration file specific for Thingy:91.
  This file is automatically merged with the :file:`prj.conf` file when you build for the ``thingy91/nrf9160/ns`` board target.

* :file:`boards/thingy91x_nrf9151_ns.conf` - Configuration file specific for Thingy:91 X.
  This file is automatically merged with the :file:`prj.conf` file when you build for the ``thingy91x/nrf9151/ns`` board target.

.. _slm_native_tls:

Native TLS
----------

By default, the secure socket (TLS) is offloaded to the modem.
If you need customized TLS features that are not supported by the modem firmware, you can use native TLS instead.
Native TLS uses the Mbed TLS library in Zephyr to establish secure connectivity.
Currently, the SLM application can be built to use native TLS for the following services:

* Secure socket
* TLS Proxy client
* TLS Proxy server
* HTTPS client

With native TLS, the credentials are stored in the Zephyr settings storage with the ``AT#XCMNG`` command.

The configuration options that are required to enable native TLS are defined in the :file:`overlay-native_tls.conf` file.

.. note::

   Native TLS services have the following limitations:

   * TLS session resumption is currently not supported.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

.. _slm_building:

Building and running
********************

.. |sample path| replace:: :file:`applications/serial_lte_modem`

.. include:: /includes/build_and_run_ns.txt

.. _slm_connecting_91dk:

Communicating with the modem on an nRF91 Series DK
==================================================

In this scenario, an nRF91 Series DK running the Serial LTE Modem application serves as the host.
You can use either a PC or an external MCU as a client.

.. _slm_connecting_91dk_pc:

Connecting with a PC
--------------------

To connect to an nRF91 Series DK with a PC:

.. slm_connecting_91dk_pc_instr_start

1. Verify that ``UART_0`` is selected in the application.
   It is defined in the default configuration.

2. Use `nRF Connect Serial Terminal`_ to connect to the development kit.
   See :ref:`serial_terminal_connect` for instructions.
   You can also use the :guilabel:`Open Serial Terminal` option of the `Cellular Monitor`_ app to open the Serial Terminal.
   Using the Cellular Monitor app in combination with the nRF Connect Serial Terminal shows how the modem responds to the different modem commands.
   You can then use this connection to send or receive AT commands over UART, and to see the log output of the development kit.

   Instead of using nRF Connect Serial Terminal, you can use PuTTY to establish a terminal connection to the development kit, using the :ref:`default serial port connection settings <test_and_optimize>`.

   .. note::

      The default AT command terminator is a carriage return followed by a line feed (``\r\n``).
      nRF Connect Serial Terminal supports this format.
      If you want to use another terminal emulator, make sure that the configured AT command terminator corresponds to the line terminator of your terminal.
      When using PuTTY, you must set the :ref:`CONFIG_SLM_CR_TERMINATION <CONFIG_SLM_CR_TERMINATION>` SLM configuration option.
      See :ref:`slm_config_options` for more details.

.. slm_connecting_91dk_pc_instr_end

.. _slm_connecting_91dk_mcu:

Connecting with an external MCU
-------------------------------

.. note::

   This section does not apply to Thingy:91 or Thingy:91 X.

If you run your user application on an external MCU (for example, an nRF52 Series development kit), you can control the modem on an nRF91 Series device directly from the application.
See the :ref:`slm_shell_sample` for a sample implementation of such an application.

To connect with an external MCU using UART_2, change the configuration files for the default board as follows:

.. tabs::

   .. group-tab:: nRF9161 DK

      * In the :file:`nrf9161dk_nrf9161_ns.conf` file::

          # Use UART_0 (when working with PC terminal)
          # unmask the following config
          #CONFIG_UART_0_NRF_HW_ASYNC_TIMER=2
          #CONFIG_UART_0_NRF_HW_ASYNC=y
          #CONFIG_SLM_POWER_PIN=8
          #CONFIG_SLM_INDICATE_PIN=0

          # Use UART_2 (when working with external MCU)
          # unmask the following config
          CONFIG_UART_2_NRF_HW_ASYNC_TIMER=2
          CONFIG_UART_2_NRF_HW_ASYNC=y
          CONFIG_SLM_POWER_PIN=31
          CONFIG_SLM_INDICATE_PIN=30

      * In the :file:`nrf9161dk_nrf9161_ns.overlay` file::

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


   .. group-tab:: nRF9160 DK

      * In the :file:`nrf9160dk_nrf9160_ns.conf` file::

          # Use UART_0 (when working with PC terminal)
          # unmask the following config
          #CONFIG_UART_0_NRF_HW_ASYNC_TIMER=2
          #CONFIG_UART_0_NRF_HW_ASYNC=y
          #CONFIG_SLM_POWER_PIN=6
          #CONFIG_SLM_INDICATE_PIN=2

          # Use UART_2 (when working with external MCU)
          # unmask the following config
          CONFIG_UART_2_NRF_HW_ASYNC_TIMER=2
          CONFIG_UART_2_NRF_HW_ASYNC=y
          CONFIG_SLM_POWER_PIN=31
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

The following table shows how to connect an nRF52 Series development kit to an nRF91 Series development kit to be able to communicate through UART:

.. list-table::
   :align: center
   :header-rows: 1

   * - nRF52 Series DK
     - nRF91 Series DK
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
* nRF9160 or nRF9161 - UART2

Use the following UART configuration:

* Hardware flow control: enabled
* Baud rate: 115200
* Parity bit: no
* Operation mode: IRQ

.. note::
   The GPIO output level on the nRF91 Series device side must be 3 V.
   You can set the VDD voltage with the **VDD IO** switch (**SW9**).
   See the `VDD supply rail section in the nRF9160 DK User Guide`_ for more information related to nRF9160 DK.

.. _slm_connecting_thingy91:

Communicating with the modem on Thingy:91
=========================================

In this scenario, Thingy:91 running the Serial LTE Modem application serves as the host.
You can use a PC as a client.

.. _slm_connecting_thingy91_pc:

Connecting with a PC
--------------------

The nRF52840 SoC of Thingy:91 is pre-programmed with the :ref:`connectivity_bridge` application.
To update the :ref:`connectivity_bridge` application, see the :ref:`programming_thingy` documentation.
The :ref:`connectivity_bridge` application routes ``UART_0`` to ``USB_CDC0`` on Thingy:91.
By enabling the :kconfig:option:`CONFIG_BRIDGE_BLE_ENABLE` Kconfig option in the :ref:`connectivity_bridge`, you can also use SLM over :ref:`nus_service_readme`.

To connect to a Thingy:91 with a PC:

.. include:: slm_description.rst
   :start-after: .. slm_connecting_91dk_pc_instr_start
   :end-before: .. slm_connecting_91dk_pc_instr_end

You can also test the i2c sensor on Thingy:91 using :ref:`SLM_AT_TWI`.
See :ref:`slm_testing_twi` for more details.

.. _slm_testing_section:

Testing
=======

The following testing instructions focus on testing the application with a PC client.
If you have an nRF52 Series DK running a client application, you can also use this DK for testing the different scenarios.

|test_sample|

1. |connect_kit|
#. :ref:`Connect to the kit with nRF Connect Serial Terminal <serial_terminal_connect>`.
   You can also use the :guilabel:`Open Serial Terminal` option of the `Cellular Monitor`_ app to open the Serial Terminal.
   If you want to use a different terminal emulator, see :ref:`slm_connecting_91dk_pc`.
#. Reset the kit.
#. Observe that the development kit sends a ``Ready\r\n`` message on UART.
#. Send AT commands and observe the responses from the development kit.

   See :ref:`slm_testing` for typical test cases.

.. _slm_carrier_library_support:

Using the LwM2M carrier library
===============================

.. |application_sample_name| replace:: Serial LTE Modem application

.. include:: /includes/lwm2m_carrier_library.txt

The certificate provisioning can also be done directly in the Serial LTE Modem application by using the same AT commands as described for the :ref:`at_client_sample` sample.

When the :ref:`liblwm2m_carrier_readme` library is in use, by default the application will auto-connect to the network on startup.
This behavior can be changed by disabling the :kconfig:option:`CONFIG_SLM_AUTO_CONNECT` option.

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
* :ref:`lib_nrf_cloud_agnss`
* :ref:`lib_nrf_cloud_pgps`
* :ref:`lib_nrf_cloud_cell_pos`

It uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
