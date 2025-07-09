.. _uart_driver:

UART driver
###########

.. contents::
   :local:
   :depth: 2

UART is one of the most common peripherals.
You can use it in various ways depending on the use case and requirements.
The UART driver API consists of configuration and control API as well as functions for transmission.

API for transmission
********************

The Zephyr UART driver has three types of API for transmission.
The driver can support all three simultaneously but a unique instance can support only a single type.

.. _uart_polling:

Polling API
===========

This is a blocking API and does not use interrupts.
It is recommended for testing and prototyping.
It has minimal memory footprint.
By default, it is not low power because receiver is always enabled.
If only UART TX part is used (for example, for logging), you can configure it to not use the receiver and UART will be off when not transmitting.

Configuration
-------------

This API is enabled by default.
To achieve low power, add the ``disable-rx`` property to the UART node in the devicetree.
In that case, the receiver is not enabled.
You can use the :kconfig:option:`CONFIG_UART_NRFX_UARTE_NO_IRQ` Kconfig option to disable the interrupt handler.
This is useful in memory-constraint scenarios.

Usage
-----

Use the :c:func:`uart_poll_out` function to transmit a single byte.
You can call this function safely from any context and the driver ensures that all bytes will be transmitted.
If hardware flow control (HWFC) is used, this function may block forever.

Use the :c:func:`uart_poll_in` function to get a received byte.
An error is returned if there is no byte pending.

Interrupt driven API
====================

This API uses interrupts but requires the receiver to be always enabled.
It allows higher bandwidth than polling because DMA can be used on the transmitter side.
The receiver implementation uses single-byte transfers (interrupt on every byte).
It is not recommended to be used.

Configuration
-------------

To enable this API, use the :kconfig:option:`CONFIG_UART_INTERRUPT_DRIVEN` Kconfig option.
There is a Kconfig option for each instance (for example, :kconfig:option:`CONFIG_UART_0_INTERRUPT_DRIVEN`), which enables this API.

Usage
-----

To use this API, you need to provide a callback that is called in the UART interrupt context.
Initiate the transmission using the :c:func:`uart_fifo_fill` function and fetch the received data using :c:func:`uart_fifo_read`.
The API is not low power because the receiver is always enabled.
You can use this API simultaneously with :ref:`uart_polling`.

.. _uart_async:

Asynchronous API
================

This is the newest API extension.
It was added to allow better performance and control over UART activity.
This API is recommended due to the best bandwidth and performance.
It can be low power because the receiver is opened only when requested by the user.
You can call this API from any context (see :ref:`uart_limitations` for exceptions).

Configuration
-------------

To enable this API, use the :kconfig:option:`CONFIG_UART_ASYNC_API` Kconfig option.
There is a Kconfig option for each instance (for example, :kconfig:option:`CONFIG_UART_0_ASYNC`), which enables this API.

The :kconfig:option:`CONFIG_UART_ASYNC_TX_CACHE_SIZE` Kconfig option sets the TX buffer size (same size for all
instances).
This buffer is used as a bounce buffer if the TX buffer input is located in a memory that cannot be used by the EasyDMA.

Usage
-----

To use this API, you must provide a callback that is called in the UART interrupt context.
This callback is called with a specific UART event.
The event contains the type and data associated with that event.

The receiver side is started by a :c:func:`uart_rx_enable` function call with a buffer and timeout (in microseconds).
Immediately after the RX is enabled, a ``UART_RX_BUF_REQUEST`` event is generated and you can provide the next buffer using the :c:func:`uart_rx_buf_rsp` function.
The receiver always attempts to have spare buffer to allow continuous reception.
A ``UART_RX_RDY`` event with received data will be generated if the buffer is filled or if there is an idle period (no new bytes) for the specified time.
When a timeout occurs, the receiver switches to the next buffer or continue receiving the same buffer until it is filled.
Whenever the receiver is done with the buffer, it generates a ``UART_RX_BUF_RELEASED`` event to indicate that the buffer is no longer used by the driver.
If you do not respond with a :c:func:`uart_rx_buf_rsp` function and the receiver has no more buffers to receive, it stops the receiver and a ``UART_RX_DISABLED`` event is generated when the receiver is stopped.
You can disable the receiver explicitly at any time using the :c:func:`uart_rx_disable` function.
This operation is asynchronous and a ``UART_RX_DISABLED`` event is generated when disabling is completed.

To start the transmitter, use the :c:func:`uart_tx` function.
When the transfer is completed, a ``UART_TX_DONE`` event is generated.
You can abort an ongoing transfer using the :c:func:`uart_tx_abort` function.
When aborting is completed, a ``UART_TX_ABORTED`` event is generated.
You can use the :c:func:`uart_poll_out` function from any context at the same time with :c:func:`uart_tx` and the driver ensures that all data is transmitted.

When both the transmitter and receiver are disabled, the UARTE peripheral is off.

Reliable reception
------------------

By default, the receiver is using a mode that triggers a buffer switch when the idle timeout expires.
In such case, buffer switching requires receiver restart.
If there is an active transfer on the line when the receiver is restarted, the byte that was on the line at that exact moment will be lost or corrupted.
If HWFC is used, the receiver is always restarted when the line is idle.
The same happens in communication protocols where the transmitter waits for response but there are cases with a risk of losing the byte.
In such case, you need to use a special driver mode that uses more resources (one instance of TIMER peripheral, one (D)PPI channel, and additional RAM buffer).

Reliable reception on nRF52, nRF53 and nRF91 Series
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To enable reliable reception without HWFC, you need to provide a TIMER instance.
For example, enabling the :kconfig:option:`CONFIG_UART_1_NRF_HW_ASYNC` Kconfig option and setting :kconfig:option:`CONFIG_UART_1_NRF_HW_ASYNC_TIMER` to ``2`` ensure reliable reception for a UARTE1 instance using the TIMER2 peripheral.
This mode is using TIMER and PPI to count the number of received bytes without stopping the DMA transfer.
When an idle gap is detected by the receiver, the driver can report data that is already received.

Reliable reception on nRF54L and nRF54H Series
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To enable reliable reception without HWFC, add the ``timer`` property to the uart node in the devicetree.
Newer SoCs require different approach than legacy platforms because of the EasyDMA mode of operation, which buffers bytes and performs 32 bit word transfers if possible.
When UARTE generates an event notifying about a byte reception, this byte might not be in RAM but might still be buffered in the EasyDMA internal buffer.
Internal bounce buffers in the driver are required to handle such case.
You can use the following two Kconfig options to tune that mode:

* :kconfig:option:`CONFIG_UART_NRFX_UARTE_BOUNCE_BUF_LEN` configures the bounce buffer size for all instances.
  It mainly defines how many additional interrupts will be triggered during long packets.
* :kconfig:option:`CONFIG_UART_NRFX_UARTE_BOUNCE_BUF_SWAP_LATENCY` configures the maximum interrupt latency.
  You might need to increase this for high baud rates and high interrupt latency.

Default values should work for most of the cases but if high baud rate is used, the driver might assert when it detects that the buffer is not switched on time.
In such case, you need to increase the value of the :kconfig:option:`CONFIG_UART_NRFX_UARTE_BOUNCE_BUF_SWAP_LATENCY` Kconfig option.

The :kconfig:option:`CONFIG_UART_NRFX_UARTE_BOUNCE_BUF_LEN` space is divided into two buffers.
The driver is internally detecting when buffers need to be switched.
Switching is done by the software and if it is not done on time, the UARTE EasyDMA will overflow the bounce buffer and corrupt the data following that buffer.
It is detected and an application will assert (if asserts are enabled).
It is crucial to switch buffers on time before they are fully filled.
Buffer switching is scheduled to be performed when a buffer is partially filled.
The :kconfig:option:`CONFIG_UART_NRFX_UARTE_BOUNCE_BUF_SWAP_LATENCY` Kconfig option is used to calculate the moment when it is scheduled early enough to cover for potential latency.

By default, the value of the :kconfig:option:`CONFIG_UART_NRFX_UARTE_BOUNCE_BUF_LEN` Kconfig option is set to 256, which means that there are two 128-byte buffers.
The value of the :kconfig:option:`CONFIG_UART_NRFX_UARTE_BOUNCE_BUF_SWAP_LATENCY` Kconfig option is set to 300 µs.
For 1000000 baud rate, approximately 30 bytes can be received during 300 µs.
The buffer switching procedure is scheduled when 98 (128 - 30) bytes are received.
The newly received bytes are still written to the current bounce buffer until the switching procedure is completed and EasyDMA is reconfigured to the second bounce buffer.

Configuration
*************

Each instance is configured in the devicetree and Kconfig.

The devicetree configuration includes the following parameters:

- Pins
- Baud rate
- Parity
- Reliable reception using timer
- Disabling RX
- Specifying memory section (nRF54H20)

You can also reconfigure the device in runtime (:c:func:`uart_configure`) after enabling the :kconfig:option:`CONFIG_UART_USE_RUNTIME_CONFIGURE` Kconfig option.
It is disabled by default to reduce the code size.

.. _uart_limitations:

Limitations
***********

Below are the list of known driver limitations.

nRF54H20
========

Fast instance (UARTE120) requires IPC communication to the System Controller (NRFS) before and after it is being used.
This means that you cannot use the :c:func:`uart_tx` and :c:func:`uart_rx_enable` functions from an interrupt context, because IPC communication can be done in the thread context.
Those operations are performed by the Device Power Management and the :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` Kconfig option needs to be enabled.
IPC communication is time consuming, so resuming and suspending a UARTE120 device takes much more time than other instances.
It is recommended to explicitly call the :c:func:`pm_device_runtime_get` and :c:func:`pm_device_runtime_put` functions when using that device.
When it is done, you can call the :c:func:`uart_tx` and :c:func:`uart_rx_enable` functions from an interrupt context.
