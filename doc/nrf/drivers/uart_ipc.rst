.. _uart_ipc:

IPC UART driver
###############

.. contents::
   :local:
   :depth: 2

The IPC UART driver implements the basic UART :ref:`zephyr:uart_polling_api`.
It uses the IPC service to send data to a remote UART driver running on a different device.
The current implementation does not support :ref:`zephyr:uart_async_api` or :ref:`zephyr:uart_interrupt_api`.

Configuration
*************

The IPC UART driver is configured through the devicetree as a standard UART instance.
It requires the IPC service instance and endpoint name.
The remote device must have an endpoint with the same name to communicate with this driver.
The endpoint name is a human-readable string.

A configuration example:

.. code-block:: devicetree

	&uart0 {
		status = "okay";
		compatible = "nordic,nrf-ipc-uart";
		ipc = <&ipc0>;
		ept-name = "remote shell";
	};

The following configuration options are available for this driver:

   * :kconfig:option:`CONFIG_IPC_UART_BIND_TIMEOUT_MS` - sets the endpoint binding timeout in milliseconds.

      The driver waits this time for the remote endpoint to be ready.
   * :kconfig:option:`CONFIG_IPC_UART_RX_RING_BUFFER_SIZE` - sets the receive ring buffer size in bytes.

      This buffer forwards data received through the IPC endpoint to the UART driver.
   * :kconfig:option:`CONFIG_IPC_UART_TX_RING_BUFFER_SIZE` - sets the transmit ring buffer size in bytes.

      This buffer forwards UART driver data to the remote device though IPC endpoint.
   * :kconfig:option:`CONFIG_IPC_UART_INIT_PRIORITY` - sets the driver initialization priority within POST_KERNEL priority level.

      This driver uses the IPC service, which means it must be initialized after the IPC service backend.

Usage
*****

You can control the IPC UART using the :ref:`zephyr:uart_polling_api`.

Use the :c:func:`uart_poll_in` function to read one character from the UART input.

For more details about using this driver, see :ref:`direct_test_mode`.
