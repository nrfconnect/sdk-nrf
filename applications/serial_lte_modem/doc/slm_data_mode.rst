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
* ``AT#XCARRIER="app_data"``

The SLM application sends an *OK* response when it successfully enters data mode.

Sending data in data mode
=========================

Any arbitrary data received from the MCU is sent to LTE network *as-is*.

If the current sending function succeeds and :ref:`CONFIG_SLM_DATAMODE_URC <CONFIG_SLM_DATAMODE_URC>` is defined, the SLM application reports back the total size as ``#XDATAMODE: <size>``.
The ``<size>`` value is a positive integer.
This Unsolicited Result Code (URC) can also be used to impose flow control on uplink sending.

Exiting data mode
=================

To exit data mode, the MCU sends the termination command set by the :ref:`CONFIG_SLM_DATAMODE_TERMINATOR <CONFIG_SLM_DATAMODE_TERMINATOR>` configuration option over UART.

The pattern string could be sent alone or as an affix to the data.
The pattern string must be sent in full.

.. note::
   Some terminal software, like LTE Link Monitor, always appends an AT command terminator (for example ``<CR><LF>``) to uplink data.
   This makes it unsuitable for data mode as exiting data mode can't work.

If the current sending function fails, the SLM application exits data mode and returns the error code as ``#XDATAMODE: <error>``.
The ``<error>`` value is a negative integer.

The SLM application also exits data mode automatically in the following scenarios:

* The TCP server is stopped.
* The remote server disconnects the TCP client.
* The TCP client disconnects from the remote server due to an error.
* The UDP client disconnects from the remote server due to an error.

When exiting data mode, the SLM application sends ``#XDATAMODE: 0`` as an unsolicited notification.

After exiting data mode, the SLM application returns to the AT command mode.

Triggering the transmission
===========================

The SLM application buffers all the arbitrary data received from the UART bus before initiating the transmission.

The transmission of the buffered data to the LTE network is triggered by the time limit when the defined inactivity timer times out.
If there is no time limit configured, the minimum required value applies.
For more information, see the `Data mode control #XDATACTRL`_  command.

Flow control in data mode
=========================

When SLM fills its receiving buffer, the MCU must impose flow control to the SLM over the UART interface to avoid any buffer overflow.
Otherwise, if SLM imposes flow control, it disables the UART reception when it runs out of space in the buffer, potentially leading to data loss.

SLM reenables UART receptions after the transmission of the data previously received has freed up buffer space.
The buffer size is set to 3884 bytes by default.

.. note::
   There is no unsolicited notification defined for this event.
   UART hardware flow control is responsible for imposing and revoking flow control.

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

Data mode AT commands
*********************

The following commands list contains data-mode related AT commands.

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
  This value must be long enough to allow for the transmission of one DMA block size of data (hardcoded to 256 bytes).

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

When the application exits data mode, it sends the ``#XDATAMODE`` unsolicited notification.

Unsolicited notification
------------------------

The application sends the following unsolicited notification when it exits data mode:

::

   #XDATAMODE: <size>
   #XDATAMODE: <error>
   #XDATAMODE: 0

The ``<size>`` value returns a positive integer indicating the total size of the sending operation in data mode.
The ``<error>`` value returns a negative integer indicating the error code of the sending operation in data mode.
The ``0`` value indicates that the SLM application quit data mode and returned to AT command mode.

Example
~~~~~~~

::

   AT#XSEND
   OK
   Test TCP datamode
   #XDATAMODE: 15
   +++
   #XDATAMODE: 0
