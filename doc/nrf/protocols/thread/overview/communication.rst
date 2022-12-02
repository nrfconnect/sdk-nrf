.. _ug_thread_communication:

OpenThread co-processor communication
#####################################

.. contents::
   :local:
   :depth: 2

The network co-processor (NCP) and radio co-processor (RCP) transport architectures include a transmit (TX) buffer that stores all the data that are to be received by the host using the `Spinel protocol`_.

See :ref:`thread_architectures_designs_cp` for more information on the architectures.
The NCP platform design is currently discontinued by |NCS|.

.. _ug_thread_communication_priorities:

NCP/RCP prioritization
**********************

The Spinel protocol does not enforce any prioritization for writing data.
Therefore, the OpenThread NCP and RCP architectures introduce a data prioritization of their own:

* High priority -- for data in the TX buffer that must be written, including data that must be written as fast as possible.
* Low priority -- for data in the TX buffer that can be delayed or can be dropped if a high priority message is awaiting to be written.

When the buffer is full, some of the low priority frames are dropped or delayed for later transmission.
This happens for example with the :ref:`ug_thread_update_commands`, where the low priority frames are:

1. Frames that can be delayed ("delayable frames") are delayed for later transmission.
#. Frames that cannot be delayed ("droppable frames") are dropped.

.. _ug_thread_communication_rxtx:

Receiving and transmitting data
*******************************

The Spinel communication is based on commands and responses.
The host sends commands to NCP/RCP, and expects a response to each of them.

The commands and responses are tied together with the Transaction Identifier value (TID value) in the Spinel frame header.
Responses have a non-zero TID value, and OpenThread NCP/RCP always gives them high priority.

The pending responses that do not fit into the TX buffer are queued for later execution.
The queue is a separate buffer located above the TX buffer.
If it is full or contains any pending responses, sending of the delayable frames is postponed and all other low priority data is dropped.

Moreover, the Spinel allows sending unsolicited update commands from NCP to the host, as well as :ref:`sending logs <ug_thread_communication_logs>`.
See :ref:`ug_thread_communication_rxtx_tx` for details.

.. _ug_thread_communication_rxtx_rx:

Receiving data and RX data flows
================================

This section illustrates the RX data flows for UART and SPI for when the commands are received by NCP/RCP:

* Data RX flow for UART

  .. figure:: images/thread_data_flow_rx_uart.svg
     :alt: Data RX flow for UART

     Data RX flow for UART

  In this flow:

  1. UART interface stores up to six bytes in the hardware FIFO.
  #. HDLC-encoded data is stored in the Driver receive buffer.
  #. HDLC data is decoded and stored in the NCP UART Driver receive buffer.
  #. Spinel commands are dispatched and handled by proper routines.

     * If a command requires a response, it will be added to the NCP response queue for later execution.

* Data RX flow for SPI

  .. figure:: images/thread_data_flow_rx_spi.svg
     :alt: Data RX flow for SPI

     Data RX flow for SPI

  In this flow:

  1. SPI interface saves data into the NCP SPI RX buffer.
  #. NCP obtains pointer to the Spinel frame in the buffer and handles it.

     If a command requires a response, it will be added to the NCP response queue for later execution.

.. _ug_thread_communication_rxtx_tx:

Transmitting data
=================

NCP/RCP has the following process for sending responses:

1. After a command is received, the response is placed in the NCP/RCP Response Queue.
#. In the NCP/RCP Response Queue, the command is checked for the data required by the host.
#. NCP/RCP gathers the data and writes the response to the TX buffer by emptying the NCP/RCP Response Queue.

   The process of writing the frames to the buffer is described in the :ref:`Writing to the buffer<ug_thread_writing_buffer>` paragraph.

#. NCP/RCP sends the response from the TX buffer to the host.

.. _ug_thread_update_commands:

Unsolicited update commands
===========================

The Spinel protocol also allows sending unsolicited update commands from NCP to the host in addition to responses.
These are used for example when NCP or a node receives a IPv6 packet that must be forwarded to the host.

The unsolicited update commands have the following characteristics:

* They are written to the TX buffer.
* They are asynchronous.
* All have the TID value equal to zero.
* They have low priority.

The unsolicited update commands include both delayable and droppable frames (see :ref:`ug_thread_communication_priorities`), prioritized in the following order:

1. Delayable frames:

   1. MAC, IPv6 and UDP forwarding stream properties.
   #. Property value notification commands, including last status update.

#. Droppable frames:

   1. Debug stream for application.

      This is a separate log for application that has a property ID field that allows the application to distinguish different debug streams.

   #. Log.

      This is a log that can be used to report errors and debug information in the OpenThread stack and in Zephyr to the host :ref:`using Spinel <ug_thread_communication_logs>`.

.. _ug_thread_writing_buffer:

Writing to the buffer
=====================

The responses and unsolicited update commands are written to the buffer according to the following process:

1. NCP/RCP attempts to empty the NCP/RCP Response Queue.
   If any response remains in the queue, it prevents the lower priority messages from being written to the buffer.

   * Network frames from the Thread stack are added to the queue and a reattempt is made later.
   * Property value notification commands are not sent and a reattempt is made later.
   * Log and debug stream frames are dropped.

#. NCP/RCP attempts to empty the OT Message Queue for pending MAC, IPv6, and UDP messages.
   The data from these pending messages is not directly copied into the NCP TX Buffer, but instead it is stored in the OT stack and associated with the Spinel frame.
   The data is copied just before transmission over UART/USB/SPI.
   This helps save the TX buffer space.
#. NCP/RCP attempts to send all pending property value notification commands.
#. If the buffer space is available and no responses are pending in the NCP/RCP Response Queue, NCP/RCP allows the logs and debug stream to be written to the TX buffer.

.. _ug_thread_communication_rxtx_tx-flows:

TX data flows
=============

This section illustrates TX data flows for UART and SPI when sending responses and writing them to the TX buffer:

* Data TX flow for UART

  .. figure:: images/thread_data_flow_tx_uart.svg
     :alt: Data TX flow for UART

     Data TX flow for UART

* Data TX flow for SPI

  .. figure:: images/thread_data_flow_tx_spi.svg
     :alt: Data TX flow for SPI

     Data TX flow for SPI

.. _ug_thread_communication_logs:

Log messages and raw data through Spinel
========================================

Spinel communication is based on commands and responses.
However, logs from OpenThread and from Zephyr system can also be encoded and transmitted using Spinel.
This allows for using only one interface for frame and log transmission.

When using NCP with Zephyr, there is still a possibility that NCP transmits raw data, without encoding it into Spinel frames.
This happens when some critical errors occur in Zephyr and the system wants to provide as much information about the failure as possible without using interrupts.
This exception applies mainly to log messages and is done by turning off UART interrupts and flushing everything from the TX buffer without encoding it.
