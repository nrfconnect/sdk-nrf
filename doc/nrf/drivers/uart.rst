.. _uart_driver:

UART driver
###########

.. contents::
   :local:
   :depth: 2

UART is one of the most common peripheral that is used. It can be used in various ways
depending on the use case and requirements. UART driver API consists of configuration and
control API as well as functions for transmission.

API for transmission
********************

Zephyr UART driver has 3 types of API for transmission. Driver can support all three simultaneously
but a unique instance can support only a single type.

.. _uart_polling:

Polling API
===========

It is the simplest API which is blocking and does not use interrupts. It is recommended for testing
and prototyping. It has minimal memory footprint. By default, it is not low power because receiver
is always enabled. If only UART TX part is used (for example, for logging) then it can be configured
to not use the receiver and UART will be off when not transmitting.

Configuration
-------------

This API is enabled by default. Low power can be achieved by adding ``disable-rx`` property to
the UART node in the Devicetree. In that case, receiver is not enabled.
:kconfig:option:`CONFIG_UART_NRFX_UARTE_NO_IRQ` can be used to not use interrupt handler at all.
It can be used in memory constraint scenarios.

Usage
-----

:c:func:`uart_poll_out` can be used to transmit a single byte. Function can be safely called from
any context and driver ensures that all bytes will be transmitted. If HWFC (Hardware flow control)
is used this function may block forever.

:c:func:`uart_poll_in` can be used to get a received byte. Error is returned if there is no
byte pending.

Interrupt driven API
====================

API is using interrupts but requires receiver to be always enabled. It allows higher bandwidth
than polling since DMA can be utilized on the transmitter side. Receiver implementation
is using a single byte transfers (interrupt on every byte). It is not recommended to be used.

Configuration
-------------

API is enabled by :kconfig:option:`CONFIG_UART_INTERRUPT_DRIVEN`. There are Kconfig option for
each instance (for example, :kconfig:option:`CONFIG_UART_0_INTERRUPT_DRIVEN`) which enables this
API for that instance.

Usage
-----

User provides a callback that is called in the UART interrupt context and from there data
transmission can be initiated (:c:func:`uart_fifo_fill`) and received data can be fetched
(:c:func:`uart_fifo_read`). API is not low power because receiver is always enabled.
API can be used simultaneously with :ref:`uart_polling`.

.. _uart_async:

Asynchronous API
================

Historically, it is the newest API extension. It was added to allow better performance and
control over UART activity. This API is recommended due to the best bandwidth and performance.
It can be low power because receiver is opened only when request by the user. API can be called
from any context (see :ref:`uart_limitations` for exceptions).

Configuration
-------------

API is enabled by :kconfig:option:`CONFIG_UART_ASYNC_API`. There are Kconfig option for
each instance (for example, :kconfig:option:`CONFIG_UART_0_ASYNC`) which enables this API for
that instance.

:kconfig:option:`CONFIG_UART_ASYNC_TX_CACHE_SIZE` sets the TX buffer size (same size for all
instances). This buffer is used as a bounce buffer if input TX buffer is located in the memory
that cannot be used by the EasyDMA.

Usage
-----

User provides a callback that is called in the UART interrupt context. Callback is called with
a specific UART event. Event contains type and data associated with that event.

Receiver side is started by :c:func:`uart_rx_enable` call with a buffer and timeout
(in microseconds). Immediately after RX is enabled, event ``UART_RX_BUF_REQUEST`` is generated
and user can provide next buffer with :c:func:`uart_rx_buf_rsp`. Receiver attempts to always
have spare buffer to allow continuous reception. Event ``UART_RX_RDY`` with received data will
be generated if buffer is filled or if there is an idle period (no new bytes) for the specified
time. Depending on the implementation when timeout occurs receiver will switch to the next
buffer or will continue receiving the same buffer until it is filled. Whenever receiver is
done with the buffer it will generate ``UART_RX_BUF_RELEASED`` event to indicate that buffer
is no longer used by the driver. If user is not responding with :c:func:`uart_rx_buf_rsp` and
receiver has no more buffers to receive it will stop the receiver and ``UART_RX_DISABLED``
event is generated when the receiver is stopped. At any point in time receiver can be
disabled explicitly with :c:func:`uart_rx_disable`. This operation is asynchronous and
``UART_RX_DISABLED`` event is generated when disabling is completed.

Transmitter is started by :c:func:`uart_tx`. When transfer is completed ``UART_TX_DONE`` event
is generated. Ongoing transfer can be aborted with :c:func:`uart_tx_abort`. When aborting is
completed ``UART_TX_ABORTED`` event is generated. :c:func:`uart_poll_out` can be used from
any context at the same time when :c:func:`uart_tx` is used and driver ensures that all data
will be transmitted.

When transmitter and receiver are disabled then UARTE peripheral is off.

Reliable reception
------------------

By default receiver is using a mode which triggers a buffer switch when idle timeout expires.
Buffer switching in that case requires receiver restarting. This mode has a weakness that if
there is an active transfer on the line when receiver is restarted then the byte which was on
the line at that exact moment will be lost or corrupted. If HWFC is used then receiver will
always be restarted when line is idle. Same will happen in communication protocols where
transmitter waits for response but there are cases where such risk exists. In that case, special
driver mode need to be used that uses more resources (1 instance of TIMER peripheral,
1 (D)PPI channel and additional RAM buffer).

Reliable reception on nRF52, nRF53 and nRF91 series
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Reliable reception without HWFC is enabled in Kconfig by providing a TIMER instance, for example
enabling :kconfig:option:`CONFIG_UART_1_NRF_HW_ASYNC` and setting
:kconfig:option:`CONFIG_UART_1_NRF_HW_ASYNC_TIMER` to 2 will enable reliable reception for UARTE1
instance using TIMER2 peripheral. Mode is using TIMER and PPI to count number of received bytes
without stopping the DMA transfer so that at any point when idle gap is detected by the receiver,
driver can reported data that is already received.

Reliable reception on nRF54L and nRF54H Series
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Reliable reception without HWFC is enabled by adding the ``timer`` property to the uart node in
the Devicetree. Newer SoC requires different approach than legacy platforms because of EasyDMA mode
of operation which buffers bytes and performs 32 bit word transfers if possible. It means that when
UARTE generates event notifying about a byte reception, this byte may not be in RAM but may still
be buffered in the EasyDMA internal buffer. Internal bounce buffers are required in the driver to
handle that case.
There are 2 Kconfig options that can be used to tune that mode:
- :kconfig:option:`CONFIG_UART_NRFX_UARTE_BOUNCE_BUF_LEN` configures bounce buffer size for all instances. It mainly impacts how many additional interrupts will be triggered during long packets.
- :kconfig:option:`CONFIG_UART_NRFX_UARTE_BOUNCE_BUF_SWAP_LATENCY` configures maximum interrupt latency. May need to be increased for high baud rates and high interrupt latency.

Default values should work for most of the cases but if high baud rate is used then driver may
assert when it detects that buffer is not switched on time. In that case
:kconfig:option:`CONFIG_UART_NRFX_UARTE_BOUNCE_BUF_SWAP_LATENCY` need to be increased.

Detailed explanation
""""""""""""""""""""

:kconfig:option:`CONFIG_UART_NRFX_UARTE_BOUNCE_BUF_LEN` space is divided into 2 buffers. Driver is
internally detecting when buffers need to be switched. Switching is done by software and if it is
not done on time then UARTE EasyDMA will overflow the bounce buffer and corrupt the data following
that buffer. It is detected and application will assert (if asserts are enabled). It is crucial to
switch buffers on time and it is possible to switch buffer before they are fully filled. Buffer
switching is scheduled to be performed when buffer is partially filled and
:kconfig:option:`CONFIG_UART_NRFX_UARTE_BOUNCE_BUF_SWAP_LATENCY` is used to calculate the moment when
it is scheduled early enough to cover for potential latency.

By default, :kconfig:option:`CONFIG_UART_NRFX_UARTE_BOUNCE_BUF_LEN` is set to 256 which means that
there are two 128 byte buffers. :kconfig:option:`CONFIG_UART_NRFX_UARTE_BOUNCE_BUF_SWAP_LATENCY` is
set to 300 us. For 1000000 baud rate approximately 30 bytes can be received during 300 us. Buffer
switching procedure is scheduled when 98 (128 - 30) bytes are received. Newly received bytes are
still written to the current bounce buffer until switching procedure is completed and EasyDMA is
reconfigured to the second bounce buffer.

Configuration
*************

Each instance is configured in the Devicetree and Kconfig.

Devicetree configuration includes:

- Pins
- Baud rate
- Parity
- Reliable reception using timer
- Disabling RX
- Specifying memory section (nRF54H20)

It is also possible to reconfigure the device in runtime (:c:func:`uart_configure`) after enabling
:kconfig:option:`CONFIG_UART_USE_RUNTIME_CONFIGURE`. It is by default disabled to reduce the code size.

.. _uart_limitations:

Limitations
***********

nRF54H20
========

Fast instance (UARTE120) requires IPC communication to the System Controller (NRFS) before and after
it is being used. It means that :c:func:`uart_tx` and :c:func:`uart_rx_enable` cannot be used from
an interrupt context because IPC communication can be done in the thread context. Those operations
are performed by the Device Power Management and :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` need to
be enabled. IPC communication is time consuming so resuming and suspending UARTE120 device takes
much more time then other instances. It is recommended to explicitly call :c:func:`pm_device_runtime_get` and :c:func:`pm_device_runtime_put` when that device is in use. When it is done,
:c:func:`uart_tx` and :c:func:`uart_rx_enable` can be called from an interrupt context.
