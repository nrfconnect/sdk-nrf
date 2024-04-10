.. _SLM_AT_PPP:

PPP AT commands
****************

.. contents::
   :local:
   :depth: 2

This page describes AT commands related to the Point-to-Point Protocol (PPP).

.. note::

   To use the nRF91 Series SiP as a standalone modem in Zephyr, see :ref:`slm_as_zephyr_modem`.

PPP is enabled in SLM by compiling it with the appropriate configuration files, depending on your use case (with or without CMUX).
See the :ref:`slm_config_files` section for more information.

.. note::

   If :ref:`CMUX <CONFIG_SLM_CMUX>` is enabled, PPP is usable only through a CMUX channel.
   In that case, the CMUX link should be set up before PPP is started.

Control PPP #XPPP
=================

Set command
-----------

The set command allows you to start and stop PPP.

.. note::

   PPP is automatically started and stopped by SLM when the default PDN connection is established and lost, respectively.
   This happens even if PPP has previously been stopped or started with this command.

Syntax
~~~~~~

::

   #XPPP=<op>

* The ``<op>`` parameter can be the following:

  * ``0`` - Stop PPP.
  * ``1`` - Start PPP.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

.. slm_ppp_status_notif_start

::

   #XPPP: <running>,<peer_connected>

* The ``<running>`` parameter is an integer that indicates whether PPP is running.
  It is ``1`` for running or ``0`` for stopped.

* The ``<peer_connected>`` parameter is an integer that indicates whether a peer is connected to PPP.
  It is ``1`` for connected or ``0`` for not connected.

.. slm_ppp_status_notif_end

Example
-------

::

  AT+CFUN=1

  OK

  // PPP is automatically started when the modem is registered to the network.
  #XPPP: 1,0

  // Stop PPP.
  AT#XPPP=0

  OK

  #XPPP: 0,0

  // Start PPP.
  AT#XPPP=1

  OK

  #XPPP: 1,0

  // Have the peer connect to SLM's PPP.
  #XPPP: 1,1

  // Peer disconnects.
  #XPPP: 1,0

  // SLM restarts PPP automatically when peer disconnects.
  #XPPP: 0,0

  #XPPP: 1,0

  AT+CFUN=4

  OK

  #XPPP: 0,0

Read command
------------

The read command allows you to get the status of PPP.

Syntax
~~~~~~

::

   AT#XPPP?

Response syntax
~~~~~~~~~~~~~~~

.. include:: PPP_AT_commands.rst
   :start-after: slm_ppp_status_notif_start
   :end-before: slm_ppp_status_notif_end

Testing on Linux
================

You can test SLM's PPP on Linux by using the ``pppd`` command, though SLM must be compiled without CMUX because there is no widely available utility that allows multiplexing a device file on Linux.

.. note::

   If you have a utility that allows multiplexing a device file on Linux, you can use SLM's PPP with the ``pppd`` command through CMUX.
   To do this, you must first set up the CMUX link.
   Then, make sure to replace the device file argument in the ``pppd`` command with that of SLM's PPP channel, which will have been created by the CMUX utility.
   See :ref:`SLM_AT_CMUX` for more information on SLM's CMUX.

For the process described here, SLM's UARTs must be connected to the Linux host.

1. Get PPP running on SLM.
   To do this, start SLM and issue an ``AT+CFUN=1`` command.
#. Wait for ``#XPPP: 1,0``, which is sent when the network registration succeeds and PPP has started successfully.
#. Run the following command on the Linux host:

   .. code-block:: console

      $ sudo pppd -detach <PPP_UART_dev> <baud_rate> noauth crtscts novj nodeflate nobsdcomp debug +ipv6 usepeerdns noipdefault defaultroute defaultroute6 ipv6cp-restart 5 ipcp-restart 5

   Replace ``<PPP_UART_dev>`` by the device file assigned to the PPP UART and ``<baud_rate>`` by the baud rate of the UART that PPP is using (which is set in the :file:`overlay-ppp-without-cmux.overlay` file).
   Typically, when ``uart1`` is assigned to be the PPP UART (in the devicetree overlay), the device file assigned to it is :file:`/dev/ttyACM2` for an nRF9160 DK, and :file:`/dev/ttyACM1` for the other nRF91 Series DKs.

#. After the PPP link negotiation has completed successfully, a new network interface will be available, typically ``ppp0``.
   This network interface will allow sending and receiving IP traffic through the modem of the nRF91 Series SiP running SLM.

.. note::

   You might encounter some issues with DNS resolution.
   Edit the :file:`/etc/resolv.conf` file to work around these issues.
   You can add DNS servers that are reachable with your current network configuration.
   These added servers can even be the DNS servers that SLM's PPP sends as part of the PPP link negotiation, which are the DNS servers of the default PDN connection obtained from the modem.
