.. _uicc_lwm2m_sample:

Cellular: UICC LwM2M
####################

.. contents::
   :local:
   :depth: 2

The UICC LwM2M sample demonstrates how to use the :ref:`lib_uicc_lwm2m` library on an nRF91 Series device.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The UICC LwM2M sample turns on UICC and tries to read the LwM2M bootstrap data record from SIM.

.. note::

   This sample requires a SIM with LwM2M bootstrap data.

Configuration
*************

|config|

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/uicc_lwm2m`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Observe that the sample starts and shows the following output from the device.
   This is an example, and the output need not be identical to your observed output.

   .. code-block:: console

      UICC LwM2M sample started
      LwM2M bootstrap data found, length: 256

      0000  00 01 00 36 00 00 00 00  31 08 00 2e c8 00 25 63   ...6.... 1.....%c
      0010  6f 61 70 3a 2f 2f 6c 65  73 68 61 6e 2e 65 63 6c   oap://le shan.ecl
      0020  69 70 73 65 70 72 6f 6a  65 63 74 73 2e 69 6f 3a   ipseproj ects.io:
      0030  35 37 38 33 c1 01 01 c1  02 03 ff ff ff ff ff ff   5783.... ........
      0040  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff   ........ ........
      0050  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff   ........ ........
      0060  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff   ........ ........
      0070  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff   ........ ........
      0080  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff   ........ ........
      0090  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff   ........ ........
      00a0  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff   ........ ........
      00b0  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff   ........ ........
      00c0  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff   ........ ........
      00d0  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff   ........ ........
      00e0  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff   ........ ........
      00f0  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff   ........ ........

Dependencies
************

This sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
