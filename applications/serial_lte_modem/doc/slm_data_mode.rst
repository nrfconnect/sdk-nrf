.. _slm_data_mode:

Running in data mode
####################

.. contents::
   :local:
   :depth: 2

The Serial LTE Modem (SLM) application can run in the two operation modes defined by the AT command set: AT-command mode and data mode.

When running in data mode, the application does the following:

* It considers all the data received from the MCU over the UART bus as arbitrary data to be streamed through the LTE network by various service modules.
* It considers all the data streamed from a remote service as arbitrary data to be sent to the MCU over the UART bus.

Overview
********

You can manually switch between AT-command mode and data mode.
However, the SLM data mode is applied automatically while using the following modules:

* Socket ``send()`` and ``sendto()``
* TCP/TLS proxy send
* UDP/DTLS proxy send
* FTP put, uput and mput
* MQTT publish
* HTTP request
* GNSS nRF Cloud send message
* LwM2M carrier library app data send

Entering data mode
==================

The SLM application enters data mode when an AT command to send data out does not carry the payload.
See the following examples:

* ``AT#XSEND`` makes SLM enter data mode to receive arbitrary data to transmit.
* ``AT#XSEND="data"`` makes SLM transmit data in normal AT Command mode.

.. note::
   If the data contains either  ``,`` or ``"`` as characters, it can only be sent in data mode.
   A typical use case is to send JSON messages.

Other examples:

* ``AT#XTCPSEND``
* ``AT#XUDPSEND``
* ``AT#XFTP="put",<file>``
* ``AT#XFTP="uput"``
* ``AT#XFTP="mput",<file>``
* ``AT#XMQTTPUB=<topic>,"",<qos>,<retain>``
* ``AT#XNRFCLOUD=2``
* ``AT#XCARRIER="app_data_set"``

The SLM application sends an *OK* response when it successfully enters data mode.

Sending data in data mode
=========================

Any arbitrary data received from the MCU is sent to LTE network *as-is*.

If the current sending function succeeds and :ref:`CONFIG_SLM_DATAMODE_URC <CONFIG_SLM_DATAMODE_URC>` is defined, the SLM application reports back the total size as ``#XDATAMODE: <size>``.
The ``<size>`` value is a positive integer.
This Unsolicited Result Code (URC) can also be used to impose flow control on uplink sending.

.. note::
  If the sending operation fails due to a network problem while in data mode, the SLM application moves to a state where the data received from UART is dropped until the MCU sends the termination command :ref:`CONFIG_SLM_DATAMODE_TERMINATOR <CONFIG_SLM_DATAMODE_TERMINATOR>`.

Exiting data mode
=================

To exit the data mode, the MCU sends the termination command set by the :ref:`CONFIG_SLM_DATAMODE_TERMINATOR <CONFIG_SLM_DATAMODE_TERMINATOR>` configuration option over UART.

The pattern string could be sent alone or as an affix to the data.
The pattern string must be sent in full.

When exiting the data mode, the SLM application sends the ``#XDATAMODE`` unsolicited notification.

After exiting the data mode, the SLM application returns to the AT command mode.

.. note::
  The SLM application sends the termination string :ref:`CONFIG_SLM_DATAMODE_TERMINATOR <CONFIG_SLM_DATAMODE_TERMINATOR>` and moves to a state where the data received on the UART is dropped in the following scenarios:

  * The TCP server is stopped due to an error.
  * The remote server disconnects the TCP client.
  * The TCP client disconnects from the remote server due to an error.
  * The UDP client disconnects from the remote server due to an error.

  For SLM to stop dropping the data received from UART and move to AT-command mode, the MCU needs to send the termination command :ref:`CONFIG_SLM_DATAMODE_TERMINATOR <CONFIG_SLM_DATAMODE_TERMINATOR>` back to the SLM application.

Triggering the transmission
===========================

The SLM application buffers all the arbitrary data received from the UART bus before initiating the transmission.

The transmission of the buffered data to the LTE network is triggered in the following scenarios:

* Time limit when the defined inactivity timer times out.
* Reception of the termination string.
* Filling of the data mode buffer.

If there is no time limit configured, the minimum required value applies.
For more information, see the `Data mode control #XDATACTRL`_  command.

Flow control in data mode
=========================

When SLM fills its UART receive buffers, it enables the UART hardware flow control, which disables UART reception.
SLM reenables UART reception when the data has been moved to the data mode buffer.
If the data mode buffer fills, the data are transmitted to the LTE network.

.. note::
   There is no unsolicited notification defined for this event.
   UART hardware flow control is responsible for imposing and revoking flow control.

The data mode buffer size is controlled by :ref:`CONFIG_SLM_DATAMODE_BUF_SIZE <CONFIG_SLM_DATAMODE_BUF_SIZE>`.

.. note::
   The whole buffer is sent in a single operation.
   When transmitting UDP packets, only one complete packet must reside in the data mode buffer at any time.

Configuration options
*********************

Check and configure the following configuration options for data mode:

.. _CONFIG_SLM_DATAMODE_TERMINATOR:

CONFIG_SLM_DATAMODE_TERMINATOR - Pattern string to terminate data mode
   This option specifies a pattern string to terminate data mode.
   The default pattern string is ``+++``.

.. _CONFIG_SLM_DATAMODE_URC:

CONFIG_SLM_DATAMODE_URC - Send URC in data mode
   This option reports the result of the previous data-sending operation while the SLM application remains in data mode.
   The MCU could use this URC for application-level uplink flow control.
   It is not selected by default.

.. _CONFIG_SLM_DATAMODE_BUF_SIZE:

CONFIG_SLM_DATAMODE_BUF_SIZE - Buffer size for data mode
   This option defines the buffer size for the data mode.
   The default value is 4096.

Data mode AT commands
*********************

The following command list describes data mode-related AT commands.

Data mode control #XDATACTRL
============================

The ``#XDATACTRL`` command allows you to configure the time limit used to trigger data transmissions.
It can be applied only after entering data mode.

When the time limit is configured, small-size packets will be sent only after the timeout.

Set command
-----------

The set command allows you to configure the time limit for the data mode.

Syntax
~~~~~~

::

   #XDATACTRL=<time_limit>

* The ``<time_limit>`` parameter sets the timeout value in milliseconds.
  The default value is the minimum required value, based on the configured UART baud rate.
  This value must be long enough to allow for a DMA transmission of an UART receive (RX) buffer (:ref:`CONFIG_SLM_UART_RX_BUF_SIZE <CONFIG_SLM_UART_RX_BUF_SIZE>`).

Read command
------------

The read command allows you to check the current time limit configuration and the minimum value required, based on the configured UART baud rate.

Syntax
~~~~~~

::

   #XDATACTRL?

Response syntax
~~~~~~~~~~~~~~~

::

   #XDATACTRL: <current_time_limit>,<minimal_time_limit>

Test command
------------

The test command tests the existence of the command and provides information about the type of its subparameters.

Syntax
~~~~~~

::

   #XDATACTRL=?

Response syntax
~~~~~~~~~~~~~~~

::

   #XDATACTRL=<time_limit>

Exit data mode #XDATAMODE
=========================

When the application receives the termination command :ref:`CONFIG_SLM_DATAMODE_TERMINATOR <CONFIG_SLM_DATAMODE_TERMINATOR>` in data mode, it sends the ``#XDATAMODE`` unsolicited notification.

Unsolicited notification
------------------------

::

   #XDATAMODE: <status>

The ``<status>`` value ``0`` indicates that the data mode operation was successful.
A negative value indicates the error code of the failing operation.

Example
~~~~~~~

::

   AT#XSEND
   OK
   Test TCP datamode
   +++
   #XDATAMODE: 0
