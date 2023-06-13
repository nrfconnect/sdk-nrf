.. _udp:

Cellular: UDP
#############

.. contents::
   :local:
   :depth: 2

The UDP sample demonstrates the sequential transmission of UDP packets to a predetermined server identified by an IP address and a port.
The sample uses the :ref:`nrfxlib:nrf_modem` and :ref:`lte_lc_readme` library.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Additionally, it supports :ref:`qemu_x86`.

Overview
********

The sample acts directly on socket level abstraction.
It configures a UDP socket and continuously transmits data over the socket to the modem's TCP/IP stack, where the data eventually gets transmitted to a server specified by an IP address and a port number.
To control the LTE link, it uses the :ref:`lte_lc_readme` library and requests Power Saving Mode (PSM), :term:`extended Discontinuous Reception (eDRX)` mode and `Release Assistance Indication (RAI)`_ parameters.
These parameters can be set through the sample configuration file :file:`prj.conf`.

You can configure the frequency with which the packets are transmitted and the size of the UDP payload through the Kconfig system.
In addition to setting of the above options, you can also set the various LTE parameters that are related to current consumption for adding low power behavior to the device.

The UDP sample can be used to characterize the current consumption of the nRF9160 SiP.
This is due to the simple UDP/IP behavior demonstrated by the sample, which makes it suitable for current measurement.

.. note::
   Logging output is disabled by default in this sample to produce the lowest possible amount of current consumption.

Measuring current
=================

For measuring current on an nRF9160 DK, it must first be prepared as described in `Measuring Current on nRF9160 DK`_.
If you are measuring current on a Thingy:91, see `Measuring Current on Thingy:91`_.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. _CONFIG_UDP_DATA_UPLOAD_SIZE_BYTES:

CONFIG_UDP_DATA_UPLOAD_SIZE_BYTES - UDP data upload size configuration
   This configuration option sets the number of bytes to be transmitted to the server.

.. _CONFIG_UDP_DATA_UPLOAD_FREQUENCY_SECONDS:

CONFIG_UDP_DATA_UPLOAD_FREQUENCY_SECONDS - UDP data upload frequency configuration
   This configuration option sets the frequency with which the sample transmits data to the server.

.. _CONFIG_UDP_SERVER_ADDRESS_STATIC:

CONFIG_UDP_SERVER_ADDRESS_STATIC - UDP Server IP Address configuration
   This configuration option sets the static IP address of the server.

.. _CONFIG_UDP_SERVER_PORT:

CONFIG_UDP_SERVER_PORT - UDP server port configuration
   This configuration option sets the server address port number.

.. _CONFIG_UDP_PSM_ENABLE:

CONFIG_UDP_PSM_ENABLE - PSM mode configuration
   This configuration option, if set, allows the sample to request PSM from the modem or cellular network.

.. _CONFIG_UDP_EDRX_ENABLE:

CONFIG_UDP_EDRX_ENABLE - eDRX mode configuration
   This configuration option, if set, allows the sample to request eDRX from the modem or cellular network.

.. _CONFIG_UDP_RAI_ENABLE:

CONFIG_UDP_RAI_ENABLE - RAI configuration
   This configuration option, if set, allows the sample to request RAI for transmitted messages.

.. note::
   PSM, eDRX and RAI value or timers are set using the configurable options for the :ref:`lte_lc_readme` library.


Additional configuration
========================

The following configurations are recommended for low power behavior:

* :kconfig:option:`CONFIG_LTE_PSM_REQ_RPTAU` option set to a value greater than the value of :ref:`CONFIG_UDP_DATA_UPLOAD_FREQUENCY_SECONDS <CONFIG_UDP_DATA_UPLOAD_FREQUENCY_SECONDS>`.
* :kconfig:option:`CONFIG_LTE_PSM_REQ_RAT` set to 0.
* :kconfig:option:`CONFIG_SERIAL` disabled in :file:`prj.conf`.
* :ref:`CONFIG_UDP_EDRX_ENABLE <CONFIG_UDP_EDRX_ENABLE>` set to false.
* :ref:`CONFIG_UDP_RAI_ENABLE <CONFIG_UDP_RAI_ENABLE>` set to true for NB-IoT. It is not supported for LTE-M.

.. note::
   In applications where downlink messaging from the cloud to the device is expected, we recommend setting
   the :kconfig:option:`CONFIG_LTE_PSM_REQ_RAT` option to a higher value than ``0`` to ensure data is received
   before the device enters PSM.

PSM and eDRX timers are set with binary strings that signify a time duration in seconds.
See `Power saving mode setting section in AT commands reference document`_ for a conversion chart of these timer values.

.. note::
   The availability of power saving features or timers is entirely dependent on the cellular network.
   The above recommendations may not be the most current efficient if the network does not support the respective feature.

Configuration files
===================

The sample provides predefined configuration files for the following development kits:

* :file:`prj.conf` - For nRF9160 DK and Thingy:91
* :file:`prj_qemu_x86.conf` - For x86 Emulation (QEMU)

They are located in :file:`samples/cellular/udp` folder.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/udp`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your device, test it by performing the following steps:

1. |connect_kit|
#. |connect_terminal|
#. Enable logging by setting the :kconfig:option:`CONFIG_SERIAL` option to ``y`` in the :file:`prj.conf` configuration file.
#. Observe that the sample shows output similar to the following in the terminal emulator:

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

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
