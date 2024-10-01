.. _modem_trace_flash:

Cellular: Modem trace external flash backend
############################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to add a modem trace backend that stores the trace data to external flash.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

For the nRF9160 DK, version 0.14.0 or higher is supported by the sample.

.. include:: /includes/tfm.txt

.. include:: /includes/external_flash_nrf91.txt

Overview
********

You can use this sample to implement storing and reading of modem traces on an external flash device.
The sample contains an implementation of a custom trace backend that writes modem traces on the external flash chip of the nRF91 Series DK.
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

The Macronix MX25R6435 Ultra Low Power Flash on the nRF9160 DK is optimized for low power consumption rather than write speed, but it features a high performance mode.
The high-performance mode consumes more power but is able to erase and write at roughly double the speed.
Use the device tree property ``mxicy,mx25r-power-mode`` to configure MX25R6435 in either high-performance or low-power mode.
In this sample, the MX25R6435 is configured in high-performance mode.

The use of :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` option to enable deep-power-down mode significantly reduces the stand-by power consumption.
However, it adds overhead to SPI instructions, which causes a reduction in write performance.

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

.. |sample path| replace:: :file:`samples/cellular/modem_trace_flash`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

After programming the sample and board controller firmware (as mentioned in Requirements section) to your development kit, test it by performing the following steps:

#. |connect_kit|
#. Open the `Cellular Monitor`_ desktop application and connect the DK.
#. Select :guilabel:`Autoselect` from the **Modem trace database** drop-down menu, or a modem firmware version that is programmed on the DK.
#. Select :guilabel:`Reset device on start`.
#. Make sure that either :guilabel:`Open in Wireshark` or :guilabel:`Save trace file to disk` is selected.
#. Click :guilabel:`Open Serial Terminal` and keep the terminal window open (optional).
#. Click :guilabel:`Start` to begin the modem trace.
   The button changes to :guilabel:`Stop` and is greyed out.
#. When the console output  ``Flushed modem traces to flash`` is received in Serial Terminal, press **Button 1** on the development kit.
   If you are not using a serial terminal, you can approximately wait for one minute after clicking the :guilabel:`Start` button, and then press **Button 1**.
#. Observe modem traces received on the Cellular Monitor desktop application.

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
