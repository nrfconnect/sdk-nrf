.. _udp:

Cellular: UDP
#############

.. contents::
   :local:
   :depth: 2

The Cellular: UDP sample demonstrates the sequential transmission of UDP packets to a predetermined server identified by an IP address and a port.
The sample uses the :ref:`nrfxlib:nrf_modem` and :ref:`lte_lc_readme` library.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample acts directly on socket level abstraction.
It configures a UDP socket and continuously transmits data over the socket to the modem's TCP/IP stack, where the data eventually gets transmitted to a server specified by an IP address and a port number.
To control the LTE link, it uses the :ref:`lte_lc_readme` library and requests Power Saving Mode (PSM), :term:`extended Discontinuous Reception (eDRX)` mode and :term:`Release Assistance Indication (RAI)` parameters.
These parameters can be set through the sample configuration file :file:`prj.conf`.

You can configure the frequency with which the packets are transmitted and the size of the UDP payload through the Kconfig system.
In addition to setting of the above options, you can also set the various LTE parameters that are related to current consumption for adding low power behavior to the device.

You can use this sample to characterize the current consumption of the nRF91 Series SiP.
This is due to the simple UDP/IP behavior demonstrated by the sample, which makes it suitable for current measurement.

.. note::
   Logging output is disabled by default in this sample to produce the lowest possible amount of current consumption.

Measuring current
=================

Refer to the following documents for measuring currents:

* nRF9151 DK- `Measuring Current on nRF9151 DK`_
* nRF9161 DK- `Measuring Current on nRF9161 DK`_
* nRF9160 DK- `Measuring Current on nRF9160 DK`_
* Thingy:91- `Measuring Current on Thingy:91`_

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

.. _CONFIG_UDP_DATA_UPLOAD_ITERATIONS:

CONFIG_UDP_DATA_UPLOAD_ITERATIONS - UDP data upload iterations configuration
   This configuration option sets the number of times the sample transmits data to the server before shutting down.
   Set to ``-1`` to transmit indefinitely.

.. _CONFIG_UDP_SERVER_ADDRESS_STATIC:

CONFIG_UDP_SERVER_ADDRESS_STATIC - UDP Server IP Address configuration
   This configuration option sets the IP address of the server.

.. _CONFIG_UDP_SERVER_PORT:

CONFIG_UDP_SERVER_PORT - UDP server port configuration
   This configuration option sets the server address port number.

.. _CONFIG_UDP_PSM_ENABLE:

CONFIG_UDP_PSM_ENABLE - PSM mode configuration
   This configuration option, if set, allows the sample to request PSM from the modem and cellular network.

.. _CONFIG_UDP_EDRX_ENABLE:

CONFIG_UDP_EDRX_ENABLE - eDRX mode configuration
   This configuration option, if set, allows the sample to request eDRX from the modem and cellular network.

.. _CONFIG_UDP_RAI_ENABLE:

CONFIG_UDP_RAI_ENABLE - RAI configuration
   This configuration option, if set, allows the sample to request RAI for transmitted messages.

.. _CONFIG_UDP_RAI_NO_DATA:

CONFIG_UDP_RAI_NO_DATA - RAI indication configuration
   This configuration option, if set, allows the sample to indicate that there will be no upcoming data transmission anymore after the previous transmission.

.. _CONFIG_UDP_RAI_LAST:

CONFIG_UDP_RAI_LAST - RAI indication configuration
   This configuration option, if set, allows the sample to indicate that the next transmission will be the last one for some duration.

.. _CONFIG_UDP_RAI_ONGOING:

CONFIG_UDP_RAI_ONGOING - RAI indication configuration
   This configuration option, if set, allows the sample to indicate that the client expects to use more socket after the next transmission.

.. note::
   To configure PSM and eDRX timer values, use the options from the :ref:`lte_lc_readme` library.

Additional configuration
========================

The following configurations are recommended for low power behavior:

* :kconfig:option:`CONFIG_SERIAL` disabled.
* :kconfig:option:`CONFIG_LTE_PSM_REQ_RPTAU` option set to a value greater than the value of :ref:`CONFIG_UDP_DATA_UPLOAD_FREQUENCY_SECONDS <CONFIG_UDP_DATA_UPLOAD_FREQUENCY_SECONDS>`.
* :kconfig:option:`CONFIG_LTE_PSM_REQ_RAT` set to 0.
* :ref:`CONFIG_UDP_PSM_ENABLE <CONFIG_UDP_PSM_ENABLE>` enabled.
* :ref:`CONFIG_UDP_EDRX_ENABLE <CONFIG_UDP_EDRX_ENABLE>` disabled.
* :ref:`CONFIG_UDP_RAI_ENABLE <CONFIG_UDP_RAI_ENABLE>` enabled.

.. note::
   In applications where downlink messaging from the cloud to the device is expected, we recommend setting
   the :kconfig:option:`CONFIG_LTE_PSM_REQ_RAT` option to a higher value than ``0`` to ensure data is received
   before the device enters PSM.

PSM and eDRX timers are set with binary strings that signify a time duration in seconds.
For a conversion chart of these timer values, see the `Power saving mode setting`_ section in the nRF9160 AT Commands Reference Guide or the `same section <nRF91x1 Power saving mode setting_>`_ in the nRF91x1 AT Commands Reference Guide, depending on the SiP you are using.

.. note::
   The availability of power saving features or timers is entirely dependent on the cellular network.
   The above recommendations may not be the most current efficient if the network does not support the respective feature.

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

      *** Booting nRF Connect SDK v2.5.0-241-g5ada4583172b ***
      UDP sample has started
      LTE cell changed: Cell ID: 37372427, Tracking area: 4020
      RRC mode: Connected
      Network registration status: Connected - roaming
      PSM parameter update: TAU: 3600 s, Active time: 0 s
      Transmitting UDP/IP payload of 38 bytes to the IP address 8.8.8.8, port number 2469
      RRC mode: Idle

      Transmitting UDP/IP payload of 38 bytes to the IP address 8.8.8.8, port number 2469
      RRC mode: Connected
      RRC mode: Idle

      Transmitting UDP/IP payload of 38 bytes to the IP address 8.8.8.8, port number 2469
      RRC mode: Connected
      RRC mode: Idle

Testing RAI feature
-------------------

Test the RAI feature by performing the following steps:

1. |connect_kit|
#. |connect_terminal|
#. Connect the nRF91 Series DK to the `Power Profiler Kit II (PPK2)`_ and set up for current measurement.
#. `Install the Power Profiler app`_ in the `nRF Connect for Desktop`_.
#. Connect the Power Profiler Kit II (PPK2) to the PC using a micro-USB cable and `connect to it using the App <Using the Power Profiler app_>`_.
#. Enable RAI by setting the :ref:`CONFIG_UDP_RAI_ENABLE <CONFIG_UDP_RAI_ENABLE>` option to ``y`` in the :file:`prj.conf` configuration file.
#. Update the data upload frequency by setting the :ref:`CONFIG_UDP_DATA_UPLOAD_FREQUENCY_SECONDS <CONFIG_UDP_DATA_UPLOAD_FREQUENCY_SECONDS>` option to ``30`` in the :file:`prj.conf` configuration file.
#. Program the sample to the device.
#. Power on or reset your nRF91 Series DK.
#. In the Power Profiler app choose a one minute time window.
#. Observe that after some minutes the average power consumption will settle at around 1.7 mA (may vary depending on network conditions).
#. Disable RAI by setting the :ref:`CONFIG_UDP_RAI_ENABLE <CONFIG_UDP_RAI_ENABLE>` option to ``n`` in the :file:`prj.conf` configuration file.
#. Program the sample to the device.
#. Power on or reset your nRF91 Series DK.
#. Observe that after some minutes the average power consumption will settle at around 2.3 mA (may vary depending on network conditions).
#. Observe that power consumption with RAI enabled is lower than with RAI disabled.

Dependencies
************

This sample uses the following |NCS| library:

* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
