.. _udp:

nRF9160: UDP
############

.. contents::
   :local:
   :depth: 2

The UDP sample demonstrates the sequential transmission of UDP packets to a predetermined server identified by an IP address and a port.
The sample uses the :ref:`nrfxlib:nrf_modem` and :ref:`lte_lc_readme` library.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy91_nrf9160ns, nrf9160dk_nrf9160ns

Additionally, it supports :ref:`qemu_x86`.

Overview
********

The sample acts directly on socket level abstraction.
It configures a UDP socket and continuously transmits data over the socket to the modem's TCP/IP stack, where the data eventually gets transmitted to a server specified by an IP address and a port number.
To control the LTE link, it uses the :ref:`lte_lc_readme` library and requests Power Saving Mode (PSM), extended Discontinuous Reception (eDRX) mode and `Release Assistance Indication (RAI)`_ parameters.
These parameters can be set through the sample configuration file ``prj.conf``.

You can configure the frequency with which the packets are transmitted and the size of the UDP payload through the Kconfig system.
In addition to setting of the above options, you can also set the various LTE parameters that are related to current consumption for adding low power behavior to the device.

The UDP sample can be used to characterize the current consumption of the nRF9160 SiP.
This is due to the *simple UDP/IP behavior* demonstrated by the sample, which makes it suitable for current measurement.

Functionality and Supported Technologies
========================================

 * UDP/IP

Measuring Current
=================

For measuring current on an nRF9160 DK, it must first be prepared as described in `Measuring Current on nRF9160 DK`_.
If you are measuring current on a Thingy:91, see `Measuring Current on Thingy:91`_.

Configuration
*************

|config|

You can configure the following options:

* :option:`CONFIG_UDP_DATA_UPLOAD_SIZE_BYTES`
* :option:`CONFIG_UDP_DATA_UPLOAD_FREQUENCY_SECONDS`
* :option:`CONFIG_UDP_SERVER_ADDRESS_STATIC`
* :option:`CONFIG_UDP_SERVER_PORT`
* :option:`CONFIG_UDP_PSM_ENABLE`
* :option:`CONFIG_UDP_EDRX_ENABLE`
* :option:`CONFIG_UDP_RAI_ENABLE`


Configuration options
=====================

.. option:: CONFIG_UDP_DATA_UPLOAD_SIZE_BYTES - UDP data upload size configuration

This configuration option sets the number of bytes to be transmitted to the server.

.. option:: CONFIG_UDP_DATA_UPLOAD_FREQUENCY_SECONDS - UDP data upload frequency configuration

This configuration option sets the frequency with which the sample transmits data to the server.

.. option:: CONFIG_UDP_SERVER_ADDRESS_STATIC - UDP Server IP Address configuration

This configuration option sets the static IP address of the server.

.. option:: CONFIG_UDP_SERVER_PORT - UDP server port configuration

This configuration option sets the server address port number.

.. option:: CONFIG_UDP_PSM_ENABLE - PSM mode configuration

This configuration option, if set, allows the sample to request PSM from the modem or cellular network.

.. option:: CONFIG_UDP_EDRX_ENABLE - eDRX mode configuration

This configuration option, if set, allows the sample to request eDRX from the modem or cellular network.

.. option:: CONFIG_UDP_RAI_ENABLE - RAI configuration

This configuration option, if set, allows the sample to request RAI for transmitted messages.

.. note::
   PSM, eDRX and RAI value or timers are set via the configurable options for the :ref:`lte_lc_readme` library.


Additional configuration
========================

Below configurations are recommended for low power behavior:

 * :option:`CONFIG_LTE_PSM_REQ_RPTAU` option set to a value greater than the value of :option:`CONFIG_UDP_DATA_UPLOAD_FREQUENCY_SECONDS`.
 * :option:`CONFIG_LTE_PSM_REQ_RAT` set to 0.
 * :option:`CONFIG_SERIAL` disabled in ``prj.conf`` and ``spm.conf``.
 * :option:`CONFIG_UDP_EDRX_ENABLE` set to false.
 * :option:`CONFIG_UDP_RAI_ENABLE` set to true for NB-IoT. It is not supported for LTE-M.

PSM and eDRX timers are set with binary strings that signify a time duration in seconds.
See `Power saving mode setting section in AT commands reference document`_ for a conversion chart of these timer values.

.. note::
   The availability of power saving features or timers is entirely dependent on the cellular network.
   The above recommendations may not be the most current efficient if the network does not support the respective feature.

Configuration files
===================

The sample provides predefined configuration files for the following development kits:

* ``prj.conf`` : For nRF9160 DK and Thingy:91
* ``prj_qemu_x86``: For x86 Emulation (QEMU)

They are located in ``samples/nrf9160/udp`` folder.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/udp`

.. include:: /includes/build_and_run.txt

.. include:: /includes/spm.txt

Testing
=======

After programming the sample to your device, test it by performing the following steps:

1. |connect_kit|
#. |connect_terminal|
#. Observe that the sample shows the :ref:`UART output <uart_output>` from the device.
   Note that this is an example and the output need not be identical to your observed output.

.. note::
   Logging output is disabled by default in this sample in order to produce the lowest possible amount of current consumption.
   To enable logging, set the :option:`CONFIG_SERIAL` option in the ``prj.conf`` and ``spm.conf`` configuration files.

.. _uart_output:

Sample output
=============

The following serial UART output is displayed in the terminal emulator:

.. code-block:: console

      *** Booting Zephyr OS build v2.3.0-rc1-ncs1-1451-gb160c8c5caa5  ***
      UDP sample has started
      LTE cell changed: Cell ID: 33703711, Tracking area: 2305
      PSM parameter update: TAU: 110287, Active time: 61024
      RRC mode: Connected
      Network registration status: Connected - roaming

      PSM parameter update: TAU: -1, Active time: -1
      Transmitting UDP/IP payload of 38 bytes to the IP address 8.8.8.8, port number 2469
      PSM parameter update: TAU: 3600, Active time: 0
      RRC mode: Idle

      Transmitting UDP/IP payload of 38 bytes to the IP address 8.8.8.8, port number 2469
      RRC mode: Connected
      RRC mode: Idle

      Transmitting UDP/IP payload of 38 bytes to the IP address 8.8.8.8, port number 2469
      RRC mode: Connected
      RRC mode: Idle


Dependencies
************

This sample uses the following |NCS| library:

* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`
