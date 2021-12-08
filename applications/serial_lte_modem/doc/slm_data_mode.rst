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

Entering data mode
==================

The SLM application enters data mode when an AT command to send data out does not carry the payload.
See the following examples:

* ``AT#XSEND`` makes SLM enter data mode to receive arbitrary data to transmit.
* ``AT#XSEND="data"`` makes SLM transmit data in normal AT Command mode.

Other examples:

* ``AT#XTCPSEND``
* ``AT#XUDPSEND``
* ``AT#XFTP="put",<file>``
* ``AT#XFTP="uput"``
* ``AT#XFTP="mput",<file>``
* ``AT#XMQTTPUB=<topic>,"",<qos>,<retain>``
* ``AT#XNRFCLOUD=2``

The SLM application does not send an *OK* response when it enters data mode.

Exiting data mode
=================

To exit data mode, the MCU sends the termination command set by the :ref:`CONFIG_SLM_DATAMODE_TERMINATOR <CONFIG_SLM_DATAMODE_TERMINATOR>` configuration option over UART and also applies the silence period set by the :ref:`CONFIG_SLM_DATAMODE_SILENCE <CONFIG_SLM_DATAMODE_SILENCE>` option, both before and after the termination command.

When instructed to exit data mode, the SLM application returns the AT command response ``OK``.

If the current sending function fails, the SLM application exits data mode and returns the AT command response ``ERROR``.

The SLM application also exits data mode automatically in the following scenarios:

* The TCP server is stopped.
* The remote server disconnects the TCP client.
* The TCP client disconnects from the remote server due to an error.
* The UDP client disconnects from the remote server due to an error.
* FTP unique or single put operations are completed.
* An HTTP request is sent.

When exiting data mode automatically, the SLM application sends ``#XDATAMODE: 0`` as an unsolicited notification.

After exiting data mode, the SLM application returns to the AT command mode.

Triggering the transmission
===========================

The SLM application buffers all the arbitrary data received from the UART bus before initiating the transmission.

Two triggers can initiate the transmission of the buffered data to the LTE network:

* The time limit trigger, which triggers the transmission when a defined timer times out.
* The single RX trigger, where there is no timer defined and SLM keeps receiving data.

Flow control in data mode
=========================

When SLM fills its receiving buffer, the MCU must impose flow control to the SLM over the UART interface to avoid any buffer overflow.
Otherwise, if SLM imposes flow control, it disables the UART reception when it runs out of space in the buffer, potentially leading to data loss.

SLM reenables UART receptions after the transmission of the data previously received has freed up buffer space.
The buffer size is set to 3884 bytes by default.

.. note:
   There is no unsolicited notification defined for this event.
   UART hardware flow control is responsible for imposing and revoking flow control.

Configuration options
*********************

Check and configure the following configuration options for data mode:

.. _CONFIG_SLM_DATAMODE_TERMINATOR:

CONFIG_SLM_DATAMODE_TERMINATOR - Pattern string to terminate data mode
   This option specifies a pattern string to terminate data mode.
   The default pattern string is ``+++``.

.. _CONFIG_SLM_DATAMODE_SILENCE:

CONFIG_SLM_DATAMODE_SILENCE - Silence time to exit data mode
   This option specifies the length, in seconds, of the UART silence applied before and after the pattern string that is used to exit data mode is sent.
   The default value is 1 second.

Data mode AT commands
*********************

The following commands list contains data-mode related AT commands.

Data mode control #XDATACTRL
============================

The ``#XDATACTRL`` command allows you to configure the time limit used to trigger data transmissions.
It can be applied only after entering data mode.
If it is not issued, the data mode works in single RX mode.

When the time limit is configured, small-size packets will be sent only after the timeout.

Set command
-----------

The set command allows you to configure the time limit for the data mode.

Syntax
~~~~~~

::

   #XDATACTRL=<time_limit>

* The ``<time_limit>`` parameter sets the timeout value in milliseconds.
  The default value is 0.
  Setting the value to 0 disables the time limit and the timeout trigger.
  When different from 0, this value must be long enough to allow for the transmission of one DMA block size of data (hardcoded to 256 bytes).

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
