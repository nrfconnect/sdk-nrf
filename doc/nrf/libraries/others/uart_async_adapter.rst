.. _lib_uart_async_adapter:

UART async adapter
##################

.. contents::
   :local:
   :depth: 2

UART async adapter library works as an interface between interrupt-driven API of a UART device and software that is using the asynchronous API interface.

.. note::
   The current implementation is :ref:`experimental <software_maturity>`.

Overview
********

Zephyr provides the following three different ways to access a UART peripheral:

* Pooling API
* Interrupt-driven API
* Asynchronous API

While most of the UART drivers provide all three APIs, some drivers still do not provide the asynchronous UART API.
An example of such a driver is USB CDC ACM.
The UART async adapter library is initialized with a UART device and communicates with it using the interrupt-driven API, providing an asynchronous API for the application.

This removes the need to implement two ways of interfacing with UART devices in software.
When using the UART async adapter library, the software only needs to implement interaction with a UART device using the asynchronous API and just initialize the adapter for the ones that do not support it.

Requirements
************

To use the library, you need to enable the :kconfig:option:`CONFIG_UART_ASYNC_API` and :kconfig:option:`CONFIG_UART_INTERRUPT_DRIVEN` Kconfig options.

Configuration
*************

Use the :kconfig:option:`CONFIG_UART_ASYNC_ADAPTER` Kconfig option to enable the library in the build system.

Usage
*****

See the following code snippet to learn how to use this library:

.. code-block:: c

    #include <uart_async_adapter.h>

    ...
    /* Get the UART device we want to use */
    static const struct device *uart = DEVICE_DT_GET([UART node identifier]);

    /* Create UART async adapter instance */
    UART_ASYNC_ADAPTER_INST_DEFINE(async_adapter);

    /* UART initialization (called before any UART usage) */
    static int uart_init(void)
    {
        if (!uart_test_async_api(uart)) {
		/* Implement API adapter */
		uart_async_adapter_init(async_adapter, uart);
		uart = async_adapter;
	}
	/* Continue initialization using asynchronous API on uart device */
	(...)
    }

.. note::
   When using the library in the way shown in the code snippet, it is totally transparent to the application.

If the selected UART device provides the asynchronous API, the adapter is not initialized.
If the selected UART device does not provide the asynchronous API, the adapter is initialized to use such a device and a direct pointer to the device is replaced by the adapter.

Samples using the library
*************************

The following |NCS| sample use this library:

* :ref:`peripheral_uart`

Limitations
***********

Using this library adds code and performance overhead over direct usage of UART asynchronous API provided by the driver.
For UART drivers that only provide the interrupt-driven API, consider using it directly in the application.
The library allows using UART devices with different APIs with only one API in the application, speeding up the development process.

Dependencies
************

This module may use :ref:`zephyr:logging_api`.

API documentation
*****************

| Header file: :file:`include/uart_async_adapter.h`
| Source files: :file:`subsys/uart_async_adapter/`

.. doxygengroup:: uart_async_adapter
   :project: nrf
   :members:
