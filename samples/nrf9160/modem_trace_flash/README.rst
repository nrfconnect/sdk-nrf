.. _modem_trace_flash:

nRF9160: Modem trace external flash backend
###########################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to add a modem trace backend that stores the trace data to external flash.

Requirements
************

The sample supports the following development kit, version 0.14.0 or higher:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

.. include:: /includes/external_flash_nrf91.txt

Overview
********

You can use this sample to implement storing and reading of modem traces on an external flash device.
The sample contains an implementation of a custom trace backend that writes modem traces on the external flash chip of the nRF9160 DK.
In addition, it reads out the traces from the external flash and writes them out to UART1 on a button press.

You can use the sample for creating a trace backend for your own flash device.
You can store a reduced set of modem traces using the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_LEVEL_CHOICE` option.
The sample starts storing modem traces when the backend is initialized by the :ref:`nrf_modem_lib_readme` library.
However, you can also start storing the modem traces during runtime.
Use the :c:func:`nrf_modem_lib_trace_level_set()` function for enabling or disabling modem traces at runtime.

Write performance
=================

A modem trace backend must be able to handle the trace data as fast as they are produced to avoid dropping traces.
It is recommended to handle the trace data at approximately 1 Mbps.
The Macronix MX25R6435 Ultra Low Power Flash on the nRF9160DK is optimized for low power consumption rather than write speed, but it features a high performance mode.
The high-performance mode consumes more power but is able to erase and write at roughly double the speed.
Use the device tree property ``mxicy,mx25r-power-mode`` to configure MX25R6435 in either high-performance or low-power mode.
In this sample, the MX25R6435 is configured in high-performance mode.

The use of :kconfig:option:`CONFIG_SPI_NOR_IDLE_IN_DPD` option to enable deep-power-down mode significantly reduces the stand-by power consumption.
However, it adds overhead for each SPI instruction, which causes an approximately 20% reduction in write performance.

The sample uses the :ref:`nrfxlib:nrf_modem` to turn on and off the modem traces and also enable and disable the modem.
In addition, it uses the :ref:`zephyr:uart_api` to send modem traces over UART1.

Flash space
===========

The sample uses 1 MB (out of 8 MB) of the external flash.
This is sufficient for the modem traces that are handled by this sample.
Since the external flash area is erased during startup, the size of the flash used affects the startup time.
Erasing 1 MB takes approximately 8 seconds in high performance mode.
Erasing the whole flash (8 MB) takes approximately 1 minute in high performance mode.

User interface
***************

Button 1:
   Reads out the modem traces from external flash and sends it on UART 1.


Building and running
********************

.. |sample path| replace:: :file:`samples/modem_trace_flash`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

After programming the sample and board controller firmware (as mentioned in Requirements section) to your development kit, test it by performing the following steps:

#. |connect_kit|
#. |connect_terminal|
#. Open the `Trace Collector`_ desktop application and connect it the DK.
#. When the console output  ``Flushed modem traces to flash`` is received, press Button 1 on the development kit.
#. Observe modem traces received on the Trace Collector desktop application.

.. note::
   Since the external flash is erased at startup, there will be a few seconds of delay before the first console output is received from the sample.


Dependencies
************

It uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr libraries:

* :ref:`zephyr:uart_api`


In addition, it uses the following secure firmware components:

* :ref:`Trusted Firmware-M <ug_tfm>`
