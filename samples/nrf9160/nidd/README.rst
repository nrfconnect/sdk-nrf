.. _nidd_sample:

nRF9160: NIDD
#############

.. contents::
   :local:
   :depth: 2

The NIDD sample demonstrates how to use Non-IP Data Delivery (NIDD) on an nRF9160 device.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The NIDD sample creates a Non-IP PDN using the configured APN and uses socket operations to send and receive data.
You can allocate a new PDN context to allow dual stack communication (IP and Non-IP).

.. note::

   This sample requires a SIM subscription with Non-IP service enabled, and LTE network configured to route the Non-IP traffic to a server that is able to respond.
   Before using the sample, check with your operator if Non-IP service is supported.


Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. _CONFIG_NIDD_APN:

CONFIG_NIDD_APN - APN used for NIDD connection
   This option specifies the APN to use for the NIDD connection.

.. _CONFIG_NIDD_ALLOC_NEW_CID:

CONFIG_NIDD_ALLOC_NEW_CID - Allocate new context identifier for NIDD connection
   This option, when enabled, allocates a new PDN context identifier instead of modifying the default.
   This enables the use of NIDD together with regular IP traffic.

.. CONFIG_NIDD_PAYLOAD:

CONFIG_NIDD_PAYLOAD - Payload for NIDD transmission
   This option sets the application payload to be sent as data.

.. include:: /libraries/modem/nrf_modem_lib.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/nidd`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Observe that the sample starts and shows the following output from the device.
   Note that this is an example, and the output need not be identical to your observed output.

   .. code-block:: console

      NIDD sample started
      Configured Non-IP for APN "iot.nidd"
      LTE cell changed: Cell ID: 21657858, Tracking area: 40401
      RRC mode: Connected
      Network registration status: Connected - roaming
      Get PDN ID 0
      Created socket 0
      Sent 13 bytes
      Received 14 bytes: Hello, Device!
      Closed socket 0
      LTE cell changed: Cell ID: -1, Tracking area: -1
      RRC mode: Idle
      NIDD sample done

Dependencies
************

This sample uses the following |NCS| library:

* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
