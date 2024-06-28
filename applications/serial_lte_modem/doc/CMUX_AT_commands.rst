.. _SLM_AT_CMUX:

CMUX AT commands
****************

.. contents::
   :local:
   :depth: 2

This page describes CMUX-related AT commands.

The CMUX protocol enables multiplexing multiple data streams through a single serial link, setting up one channel per data stream.
For example, it can be used to exchange AT data and have a :ref:`Point-to-Point Protocol (PPP) <CONFIG_SLM_PPP>` link up at the same time on a single UART.

.. note::

   To use the nRF91 Series SiP as a standalone modem in Zephyr, see :ref:`slm_as_zephyr_modem`.

CMUX is enabled in SLM by compiling it with the appropriate configuration files, depending on your use case.
See the :ref:`slm_config_files` section for more information.

.. slm_cmux_baud_rate_note_start

.. note::

   The maximum recommended baud rate is 460 800.
   At higher baud rates (921 600, 1 000 000), it is possible for bytes to come in faster than the chip is able to handle, which causes the buffer space to run out if it goes on for too long.
   UART RX is not disabled in that case, which results in data loss and communication failures.

   At a baud rate of 460 800, the maximum throughput is slightly below that of the nRF91 Series modem when using LTE-M.

.. slm_cmux_baud_rate_note_end

.. note::

   SLM does not have an equivalent to the ``AT+CMUX`` command described in 3GPP TS 27.007.
   Here is how SLM's implementation of CMUX relates to the standard command's parameters:

   * Only UIH frames are used.
   * The speed used is the configured baud rate of SLM's UART.
   * The maximum frame size is ``2100`` (defined by :c:macro:`SLM_AT_MAX_RSP_LEN` found in :file:`slm_defines.h`).

CMUX setup #XCMUX
=================

The ``#XCMUX`` command manages the configuration of CMUX over the serial link.

Set command
-----------

The set command allows you to start CMUX and assign the CMUX channels.

Syntax
~~~~~~

::

   AT#XCMUX[=<AT_channel>]

The ``<AT_channel>`` parameter is an integer used to indicate the address of the AT channel.
The AT channel denotes the CMUX channel where AT data (commands, responses, notifications) is exchanged.
If specified, it must be ``1``, unless :ref:`PPP <CONFIG_SLM_PPP>` is enabled.
If PPP is enabled, it can also be ``2`` (to allocate the first CMUX channel to PPP).
If not specified, the previously used address is used.
If no address has been previously specified, the default address is ``1``.

.. note::

   If there is more than one CMUX channel (such as when using :ref:`PPP <CONFIG_SLM_PPP>`), the non-AT channels will automatically get assigned to addresses other than the one used for the AT channel.
   For example, if PPP is enabled and CMUX is started with the ``AT#XCMUX=2`` command, the AT channel will be assigned to address ``2`` and the PPP channel to address ``1``.

An ``OK`` response is sent if the command is accepted, after which CMUX is started.
This means that after successfully running this command, you must set up the CMUX link and open the channels appropriately.
The AT channel will be available at the configured address.
You can send an empty ``AT`` command to make sure that the protocol is set up properly.

.. note::

   This command can be run when CMUX is already running to change the address of the AT channel.
   However, this is not allowed when PPP is running.

Read command
------------

The read command allows you to read the address of the AT channel and the total number of channels.

Syntax
~~~~~~

::

   AT#XCMUX?

Response syntax
~~~~~~~~~~~~~~~

::

   #XCMUX: <AT_channel>,<channel_count>

* The ``<AT_channel>`` parameter indicates the address of the AT channel.
  It is between ``1`` and ``<channel_count>``.
* The ``<channel_count>`` parameter is the total number of CMUX channels.
  It depends on what features are enabled (for example, :ref:`PPP <CONFIG_SLM_PPP>`).

Example
-------

Without PPP:

::

   AT#XCMUX?

   #XCMUX: 1,1

   OK
   AT#XCMUX

   OK
   // Here, CMUX is started and communication can now happen only through it (until a reset).
   // Open the AT channel, which is the only one, to continue exchanging AT data.
   AT

   OK

With PPP:

::

   AT#XCMUX?

   #XCMUX: 1,2

   OK
   AT#XCMUX=2

   OK
   // Start up CMUX and open the channels. The AT channel is now at address 2.
   AT#XCMUX?

   #XCMUX: 2,2

   OK
