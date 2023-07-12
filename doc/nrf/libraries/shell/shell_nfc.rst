.. _shell_nfc_readme:

NFC shell transport
###################

.. contents::
   :local:
   :depth: 2

The NFC shell transport functionality allows you to receive shell commands remotely over the ISO-DEP (ISO14443-4A) protocol with any NFC polling device.
It uses the :ref:`type_4_tag` (T4T) library.

The NFC shell transport is used by default in the :ref:`nfc_shell` sample.

Overview
********

The NFC shell transport uses the NFC T4T library to emulate an NFC tag.
The NFC tag is a passive device that sends a response to the NFC polling device.
Data is exchanged over the raw ISO-DEP (ISO14443-4A) that is a block, half-duplex transmission protocol.

NFC data exchange
=================

The first payload is always sent by the polling device.
The NFC tag sends a response after receiving the payload from the polling device.
The shell interface needs to exchange data in both directions and can be used also as the :ref:`zephyr:logging_api` subsystem backend.
Therefore, the polling device needs to poll for data constantly.

If at least one of the communication sides does not have any data, it must send the payload containing one dummy byte.
You can set this byte to any value on your polling device.
The NFC shell answers with the payload that was started with the same byte as it was received from the polling device.

The NFC tag always responds to the polling device's frame.
A record payload always starts with a dummy one-byte header.
Its value is not relevant and it is not checked by this library.

The following diagram illustrates various data exchange cases:

.. msc::
    hscale = "1.3";
    a [label="Polling device"], b [label="Tag"];
    |||;
    a rbox b [label = "Empty packet exchange (poll for data)"];
    a=>b [label = "data: ['S'] - one dummy byte, 'S' character here"];
    a<=b [label = "data: ['S'] - one dummy byte"];
    |||;
    a rbox b  [label = "Only the polling device has data"];
    a=>b [label = "data: ['S', 1, ..., N bytes]"];
    a<=b [label = "data: ['S'] - one dummy byte"];
    |||;
    a rbox b [label = "Only the polling device has data"];
    a=>b     [label = "data: ['S'] - one dummy byte"];
    a<=b     [label = "data: ['S', 1, ..., N bytes]"];
    |||;
    a rbox b [label = "Both devices have data"];
    a=>b [label = "data: ['S', 1, .., N bytes]"];
    a<=b [label = "data: ['S', 1, ..., N bytes]"];

Requirements
************

This library has the following requirements:

  * The NFC callback function must be called from a thread context.
    To do so, ensure that the :kconfig:option:`CONFIG_NFC_THREAD_CALLBACK` Kconfig option is enabled.
    For a default configuration, the function is called from a system workqueue context.
    In the callback function, this library uses operating system primitives which cannot be used in an interrupt context.
  * If library logs and a logger are enabled, it is not recommended to use the logger in the synchronous mode.
    Synchronous logs processing might result in high latency in NFC events processing that can cause instability of the NFC shell backend.
    If you need logs from this library, configure the logger to be in the deferred mode by setting the :kconfig:option:`CONFIG_LOG_MODE_DEFERRED` Kconfig option.

Configuration
*************

Set the :kconfig:option:`CONFIG_SHELL_NFC` Kconfig option to enable the NFC shell backend.

The following configuration options are available for this module:

   * :kconfig:option:`CONFIG_SHELL_NFC_INIT_PRIORITY` sets the initialization priority for the NFC shell backend.
   * :kconfig:option:`CONFIG_SHELL_NFC_BACKEND_TX_BUFFER` sets a size of the NFC T4T ISO-DEP response buffer.
     The size might depend on the maximum supported frame size for the polling device.
   * :kconfig:option:`CONFIG_SHELL_NFC_BACKEND_TX_RING_BUFFER_SIZE` sets a size of the shell Tx ring buffer.
     It is used to buffer data from the shell interface through the NFC T4T transport that can start sending data only on the data reception from the polling device.
   * :kconfig:option:`CONFIG_SHELL_NFC_BACKEND_RX_RING_BUFFER_SIZE`- sets a size of the shell Rx ring buffer.
     It handles shell incoming data from the NFC transport.

See the Kconfig help for details.

Usage
*****

You can set the :kconfig:option:`CONFIG_SHELL_NFC` Kconfig option to enable the NFC shell.
The shell is initialized automatically during the system initialization.

You can approach your polling device that uses the raw ISO-DEP (ISO14443-4A) protocol to start the shell interface.
Next, forward the received data to the terminal and send back data received from terminal through your polling device.
For example, the terminal data can be the keyboard data or the bulk data.

.. note::
   The NFC transport can be physically disconnected at any time.
   After disconnection, all data sent by the shell will be discarded.
   All uncompleted commands sent by the polling device before the disconnection are discarded as well.
   Every transport connection or re-connection starts a new shell session.

Samples using the library
*************************

The :ref:`nfc_shell` |NCS| sample uses this library.

Limitations
***********

The NFC T4T ISO-DEP transport is relatively slow - the maximum bit rate for NFC-A technology is 106 kbps.
The polling device needs to send data first or to send dummy data to poll data from the tag.
As a result, latency visible to the unaided eye occurs during printing characters in the terminal.
Moreover, it is not recommended to set the NFC shell as a logger backend due to the transport speed.
You can enable the :kconfig:option:`CONFIG_SHELL_NFC_INIT_LOG_LEVEL_NONE` Kconfig option so that set the initial log level to ``none`` for the NFC shell.

API documentation
*****************

| Header file: :file:`include/shell/shell_nfc.h`
| Source file: :file:`subsys/shell/shell_nfc.c`

.. doxygengroup:: shell_nfc
   :project: nrf
   :members:
