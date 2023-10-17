.. _wifi_wfa_qt_app_sample:

Wi-Fi: WFA QuickTrack control application
#########################################

.. contents::
   :local:
   :depth: 2

The QuickTrack sample demonstrates how to use the WFA QuickTrack (WFA QT) library needed for Wi-Fi Alliance QuickTrack certification.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The QuickTrack sample and library offer support for QuickTrack certification testing through two distinct interfaces: a netUSB interface and a Serial Line Internet Protocol (SLIP) interface.

You can choose either of these options for running QuickTrack certification.

See `Wi-Fi Alliance Certification for nRF70 Series`_ for more information.

Test setup
==========

The following figure shows a reference test setup.

.. figure:: /images/wifi_quicktrack_test_setup.png
     :alt: Wi-Fi QuickTrack test setup

     Wi-Fi QuickTrack reference test setup


Build configuration
*******************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/wfa_qt_app/Kconfig`) :

.. options-from-kconfig::
   :show-type:

To specify IP addresses, you can edit the following Kconfig options:

* Use the :kconfig:option:`CONFIG_NET_CONFIG_USB_IPV4_ADDR` Kconfig option in the :file:`overlay-netusb.conf` file to set the IPv4 address for USB communication.
* Use the :kconfig:option:`CONFIG_NET_CONFIG_SLIP_IPV4_ADDR` Kconfig option in the :file:`overlay-slip.conf` file to set the IPv4 address for UART communication.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/wfa_qt_app`

.. include:: /includes/build_and_run_ns.txt

Currently, the following configurations are supported:

* nRF7002 DK + QSPI
* nRF7002 EK + SPI

To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp

To build for the nRF7002 EK with the nRF5340 DK, use the ``nrf5340dk_nrf5340_cpuapp`` build target with the ``SHIELD`` CMake option set to ``nrf7002ek``.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf5340dk_nrf5340_cpuapp -- -DSHIELD=nrf7002ek

To build for the nRF7002 DK with the netusb support, use the ``nrf7002dk_nrf5340_cpuapp`` build target with the configuration overlay :file:`overlay-netusb.conf`.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp -- -DOVERLAY_CONFIG=overlay-netusb.conf

To build for the nRF7002 DK with the Serial Line Internet Protocol (SLIP) support, use the ``nrf7002dk_nrf5340_cpuapp`` build target with the configuration overlay :file:`overlay-slip.conf` and DTC overlay :file:`nrf7002_uart_pipe.overlay`.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp -- -DOVERLAY_CONFIG=overlay-slip.conf -DDTC_OVERLAY_FILE=nrf7002_uart_pipe.overlay

See also :ref:`cmake_options` for instructions on how to provide CMake options.

Testing
=======

After programming the sample to your development kit, complete the following steps to test it with the SLIP configuration:

1. Install :file:`tunslip6` by installing the ``net-tools`` package on the PC where the QuickTrack (QT) tool is running.
   Run the following command to clone the ``net-tools`` repository:

   .. code-block:: console

      git clone https://github.com/zephyrproject-rtos/net-tools.git

#. Navigate to the :file:`net-tools` directory.

   .. code-block:: console

      cd net-tools/

#. Run the following command to compile the ``net-tools`` package:

   .. code-block:: console

      make

   .. note::
      Install all dependent packages.

#. Run the following command to create a SLIP interface.

   .. code-block:: console

      ./tunslip6 -s <serial_port> -T <IPv6_prefix>

   * ``tunslip6``: Creates a SLIP interface on the host PC, which can be used for serial communication.
   * ``serial_port``: Can be replaced with the device path on which the DUT is connected to.
   * ``IPv6_prefix``: Can be replaced with the desired IPv6 address and subnet prefix length for your device.

   The following is an example of the CLI command:

   .. code-block:: console

      ./tunslip6 -s /dev/ttyACM4 -T 2001:db8::1/64

Sample output
=============

After programming, the sample shows the following output:

.. code-block:: console

   *** Booting nRF Connect SDK 48f33c9870f1 ***
   Starting nrf7002dk_nrf5340_cpuapp with CPU frequency: 128 MHz
   [00:00:00.330,932] <inf> net_config: Initializing network
   [00:00:00.330,932] <inf> net_config: Waiting interface 2 (0x20007a58) to be up...
   [00:00:00.331,085] <inf> wpa_supp: Successfully initialized wpa_supplicant
   [00:00:00.331,237] <inf> wfa_qt: Welcome to use QuickTrack Control App DUT version v2.1

   [00:00:00.331,268] <inf> wfa_qt: QuickTrack control app running at: 9004
   [00:00:00.331,268] <inf> wfa_qt: Wireless Interface: wlan0
   [00:00:00.757,965] <inf> net_config: Interface 1 (0x20007940) coming up
   [00:00:00.758,087] <inf> net_config: Running dhcpv4 client...
   [00:00:00.763,732] <inf> usb_ecm: Set Interface 0 Packet Filter 0x000c not supported
   [00:00:00.830,627] <inf> usb_ecm: Set Interface 0 Packet Filter 0x000e not supported
   [00:00:00.832,214] <inf> usb_ecm: Set Interface 0 Packet Filter 0x000e not supported
   [00:00:00.832,519] <inf> usb_ecm: Set Interface 0 Packet Filter 0x000e not supported
   [00:00:00.834,594] <inf> usb_ecm: Set Interface 0 Packet Filter 0x000e not supported
   [00:00:00.835,021] <inf> usb_ecm: Set Interface 0 Packet Filter 0x000e not supported
   [00:00:00.835,327] <inf> usb_ecm: Set Interface 0 Packet Filter 0x000e not supported
   [00:00:00.835,937] <inf> usb_ecm: Set Interface 0 Packet Filter 0x000e not supported

QuickTrack certification testing
********************************

QuickTrack certification tests can be done by individual companies with Wi-Fi Alliance membership or by authorized test houses.
See `QuickTrack Test Tool User Manual`_ for how to install the QuickTrack test tool.
See `QuickTrack Test Tool Platform Integration Guide`_ for the installation process of relevant access points.

.. note::
   OpenWRT access points have known limitations when running Station Under Test (STAUT) Security Vulnerability Detection test cases.

   Wi-Fi Alliance recommends using Intel AX210 station as the SoftAP to perform these tests.

See `Platform Intel Ax210 Integration Guide`_ for more details.

Dependencies
************

This sample uses a module that can be found in the following location in the |NCS| folder structure:

* :file:`modules/lib/wfa-qt-control-app`
