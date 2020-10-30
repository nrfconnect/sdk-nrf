.. _doc_uart_cobs:

UART Consistent Overhead Byte Stuffing (COBS) library
#####################################################

The UART COBS library provides an API for sending and receiving Consistent Overhead Byte Stuffing (COBS) encoded frames over a UART instance.
The library can be used to implement a two-way communication protocol with a low byte overhead.
The implementation uses the asynchronous UART interface to have more infrequent CPU interrupts and better throughput than polling-based methods.

The library also has a mechanism for coordinating access to the UART between multiple users.
This mechanism can be used to arbitrate between multiple protocols that share the same UART link.
It is also useful for supporting different users of the same protocol, such as a client and server instance.

Consistent Overhead Byte Stuffing
*********************************

Consistent Overhead Byte Stuffing (COBS) is a byte encoding that enables packet framing with a small and predictable overhead.
COBS encodes the data in the following way, illustrated using an example.
The below table shows an example payload:

+----------------+
| Data (hex)     |
+----------------+
| 00 22 33 00 44 |
+----------------+

Given the unencoded data above, the following steps are performed to encode it.
An overhead byte *O* with value ``0x00`` is inserted before the packet data.
The delimiter byte *D* with value ``0x00`` is inserted after the packet data:

+----+----------------+----+
| O  | Data (hex)     | D  |
+----+----------------+----+
| 00 | 00 11 22 00 33 | 00 |
+----+----------------+----+

For each byte, including *O* but not including *D* :

* If the byte is equal to ``0x00``, it is instead set to the relative offset to the next ``0x00`` byte in the data.
* Otherwise, the byte is left unchanged.

In this example, *O* points to the first data byte (offset 1), which points to the fourth data byte (offset 3), which finally points to the delimiter (offset 2):

+----+----------------+----+
| O  | Data (hex)     | D  |
+----+----------------+----+
| 01 | 03 11 22 02 33 | 00 |
+----+----------------+----+

This COBS encoding is unambiguous and has a constant 2 byte overhead.
The implementation supports up to 253 data bytes per frame.

COBS decoding works by reversing the above process.
It starts at the first received byte (i.e. the overhead byte *O*) and uses the relative offsets to revert each escaped byte back to ``0x00``.

User concept
************

To use the API one must first define a :cpp:struct:`uart_cobs_user` containing a pointer to an event handler function.
The convenience macro :c:macro:`UART_COBS_USER_DEFINE` can be used to define the structure.
This user structure functions as an API user identifier and must be passed to all the API functions.

Before using the API for sending and receiving one must call :cpp:func:`uart_cobs_user_start` to claim access to the UART.
An event with type :cpp:enumerator:`UART_COBS_EVT_USER_START<uart_cobs::UART_COBS_EVT_USER_START>` is sent to the provided callback function once the API is ready for use.
Once a user has received this event, it will have exclusive access to the API and will not be preempted.
To release the UART and make it available to other users, one must call :cpp:func:`uart_cobs_user_end` with a status code.
An event :cpp:enumerator:`UART_COBS_EVT_USER_END<uart_cobs::UART_COBS_EVT_USER_END>` will then be sent to the user callback, with the status code included in the event.

Whenever a user claims or releases the UART, the library may enter a "switch" state in which asynchronous sends and receives are aborted.
The return values of :cpp:func:`uart_cobs_user_start` and :cpp:func:`uart_cobs_user_end` indicate whether this state was entered.
The events described above will always be sent *after* all asynchronous actions have been stopped.

In addition to exclusive users, the library also supports an "idle user" which is set using :cpp:func:`uart_cobs_idle_user_set`.
This idle user will be active whenever no other users are active, and may be preempted whenever :cpp:func:`uart_cobs_user_start` is called.
This makes the idle user suitable for passive listening.

Receiving data
**************

To start receiving data, call :cpp:func:`uart_cobs_rx_start` with the number of bytes to receive.
The UART COBS library will then start receiving data in an internal buffer.
Note that the library assumes that each reception contains up to one frame.
Once a complete COBS frame has been decoded, an event :cpp:enumerator:`UART_COBS_EVT_RX<uart_cobs::UART_COBS_EVT_RX>` will be sent containing a pointer to the received (and decoded) data and the length of the data.

The reception can be stopped before a frame has been decoded, either because of a user abort, timeout or UART break error.
When this occurs the UART COBS library will generate an event :cpp:enumerator:`UART_COBS_EVT_RX_ERR<uart_cobs::UART_COBS_EVT_RX_ERR>` with the reason why it stopped.
See :cpp:enumerator:`uart_cobs_err` for more information.

Other UART errors than the break error will cause an automatic restart of the reception and will not generate an event.
There is currently no way to access partially decoded frames.

The reception by default does not have a timeout.
Receive timeout can be optionally started with :cpp:func:`uart_cobs_rx_timeout_start`.
The timeout must be stopped with :cpp:func:`uart_cobs_rx_timeout_stop`.
Once the receive timeout occurs the reception will be automatically stopped and an event will be generated as described above.

Sending data
************

Sending data is performed in a two-step process.
First, the data to send is written to an internal buffer using :cpp:func:`uart_cobs_tx_buf_write`.
This function may be called multiple times and will store the length of previously written data.
This means that consecutive calls will result in writing contiguous positions in the buffer.
The buffer may be reset with :cpp:func:`uart_cobs_tx_buf_clear` if necessary.

After the send buffer is written, it is sent by calling :cpp:func:`uart_cobs_tx_start`, specifying a send timeout.
The timeout parameter can be set to ``SYS_FOREVER_MS`` to disable timeout.
The call to :cpp:func:`uart_cobs_tx_start` will COBS-encode the send buffer before transmission.
Once the transmission is complete, an event :cpp:enumerator:`UART_COBS_EVT_TX<uart_cobs::UART_COBS_EVT_TX>` is sent to the event callback.

The transmission can be stopped prematurely either due to a user abort or a timeout.
When this occurs an event :cpp:enumerator:`UART_COBS_EVT_TX_ERR` is generated containing the reason for the stop.

Devicetree configuration
************************

The UART instance to use with the library is selected by setting the DTS chosen node property ``nordic,cobs-uart-controller`` to the DTS node of the instance.
For example, to use ``uart1`` with the library:

.. code-block:: DTS

   / {
        chosen {
                nordic,cobs-uart-controller=&uart1;
        };
   };

The selected UART instance (e.g. ``uart1``) is required to have ``compatible = "nordic,nrf-uarte"`` and ``hw-flow-control`` set.
This is because the UART COBS library takes advantage of EasyDMA and flow control.
An example configuration for ``uart1`` containing these settings is shown below:

.. code-block:: DTS

   &uart1 {
          compatible = "nordic,nrf-uarte";
          status = "okay";
	  hw-flow-control;
	  /* ... */
   };

Kconfig configuration
*********************

:option:`CONFIG_UART_COBS` enables the library.

The UART COBS library uses a workqueue thread for handling some UART events and for decoding received data.
Event callbacks are also done in the workqueue thread context.
:option:`CONFIG_UART_COBS_THREAD_PRIO` sets the priority of the workqueue thread.
:option:`CONFIG_UART_COBS_THREAD_STACK_SIZE` sets the size of the stack used by the workqueue thread.

Limitations
***********
* Payload sizes up to 253 bytes are supported.

API documentation
*****************

| Header file: :file:`include/uart_cobs.h`
| Source files: :file:`lib/uart_cobs`

.. doxygengroup:: uart_cobs
   :project: nrf
   :members:
