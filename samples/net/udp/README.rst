.. _udp_sample:

UDP
###

.. contents::
	:local:
	:depth: 2

The UDP sample demonstrates how to perform sequential transmissions of UDP packets to a server using an IP-connected device.
The sample connects to an LTE network using an nRF91 Series DK or Thingy:91, or to Wi-Fi using the nRF7002 DK.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Additionally, the sample supports emulation using :ref:`QEMU x86 <zephyr:qemu_x86>`.

Overview
********

The sample connects through either an LTE network or Wi-Fi, depending on the selected target board.
Once connected, it sets up a UDP socket and continuously transmits data over the socket to a configurable IP address and port number.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options:

.. _CONFIG_UDP_SAMPLE_DATA_UPLOAD_SIZE_BYTES:

CONFIG_UDP_SAMPLE_DATA_UPLOAD_SIZE_BYTES - UDP data upload size
   This configuration option sets the number of bytes that are transmitted to the server.

.. _CONFIG_UDP_SAMPLE_DATA_UPLOAD_FREQUENCY_SECONDS:

CONFIG_UDP_SAMPLE_DATA_UPLOAD_FREQUENCY_SECONDS - UDP data upload frequency
   This configuration option sets how often the sample transmits data to the server.

.. _CONFIG_UDP_SAMPLE_SERVER_ADDRESS_STATIC:

CONFIG_UDP_SAMPLE_SERVER_ADDRESS_STATIC - UDP Server IP Address
   This configuration option sets the static IP address of the server.

.. _CONFIG_UDP_SAMPLE_SERVER_PORT:

CONFIG_UDP_SAMPLE_SERVER_PORT - UDP server port configuration
   This configuration option sets the server address port number.

.. include:: /includes/wifi_credentials_shell.txt

.. include:: /includes/wifi_credentials_static.txt

Configuration files
===================

The sample provides predefined configuration files for the following development kits:

* :file:`prj.conf` - General configuration file for all devices.
* :file:`boards/nrf9160dk_nrf9160_ns.conf` - Configuration file for the nRF9160 DK.
* :file:`boards/nrf9161dk_nrf9161_ns.conf` - Configuration file for the nRF9161 DK.
* :file:`boards/thingy91_nrf9160_ns.conf` - Configuration file for the Thingy:91.
* :file:`boards/nrf7002dk_nrf5340_cpuapp.conf` - Configuration file for the nRF7002 DK.
* :file:`boards/qemu_x86.conf` - Configuration file for QEMU x86 emulation.

Building and running
********************

.. |sample path| replace:: :file:`samples/net/udp`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your device, test it by performing the following steps:

1. |connect_kit|
#. |connect_terminal|
#. Observe that the sample shows output similar to the following in the terminal emulator:

   .. code-block:: console

      *** Booting nRF Connect SDK v2.4.99-dev2-114-g305275323644 ***
      [00:00:00.268,920] <inf> udp_sample: UDP sample has started
      [00:00:00.268,951] <inf> udp_sample: Bringing network interface up and connecting to the network
      [00:00:05.557,800] <inf> udp_sample: Network connectivity established
      [00:00:05.558,715] <inf> udp_sample: Transmitting UDP/IP payload of 38 bytes to the IP address 8.8.8.8, port number 2469

Troubleshooting
===============

If you have issues with connectivity on nRF91 Series devices, see the `Cellular Monitor`_ documentation to learn how to capture modem traces in order to debug network traffic in Wireshark.
This sample enables modem traces by default.

Dependencies
************

This sample uses the following |NCS| and Zephyr libraries:

* :ref:`net_if_interface`
* :ref:`net_mgmt_interface`
* :ref:`bsd_sockets_interface`
