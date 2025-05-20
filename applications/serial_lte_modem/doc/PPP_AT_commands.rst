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

The set command allows you to start and stop PPP, and optionally define the PDN connection used for PPP.

.. note::

   PPP is automatically started and stopped by SLM when the PDN connection requested for PPP
   is established and lost, respectively.
   This happens even if PPP has previously been stopped or started with this command.

Syntax
~~~~~~

::

   #XPPP=<op>[,<cid>]

* The ``<op>`` parameter can be the following:

  * ``0`` - Stop PPP.
  * ``1`` - Start PPP.

* The ``<cid>`` parameter is an integer indicating the PDN connection to be used for PPP.
  It represents ``cid`` in the ``+CGDCONT`` command.
  Its default value is ``0``, which represents the default PDN connection.

Unsolicited notification
~~~~~~~~~~~~~~~~~~~~~~~~

.. slm_ppp_status_notif_start

::

   #XPPP: <running>,<peer_connected>,<cid>

* The ``<running>`` parameter is an integer that indicates whether PPP is running.
  It is ``1`` for running or ``0`` for stopped.

* The ``<peer_connected>`` parameter is an integer that indicates whether a peer is connected to PPP.
  It is ``1`` for connected or ``0`` for not connected.

* The ``<cid>`` parameter is an integer that indicates the PDN connection used for PPP.

.. slm_ppp_status_notif_end

Examples
--------

PPP with default PDN connection:

::

  AT+CFUN=1

  OK

  // PPP is automatically started when the default PDN is activated.
  #XPPP: 1,0,0

  // Stop PPP.
  AT#XPPP=0

  OK

  #XPPP: 0,0

  // Start PPP.
  AT#XPPP=1

  OK

  #XPPP: 1,0,0

  // Have the peer connect to SLM's PPP.
  #XPPP: 1,1,0

  // Peer disconnects.
  #XPPP: 1,0,0

  // SLM restarts PPP automatically when peer disconnects.
  #XPPP: 0,0,0

  #XPPP: 1,0,0

  AT+CFUN=4

  OK

  #XPPP: 0,0

PPP with non-default PDN connection:

::

  // Exemplary PDN connection creation.
  // Note: APN depends on operator and additional APNs may not be supported by the operator.
  AT+CGDCONT=1,"IP","internet2"

  OK

  // Start PPP with the created PDN connection. This must be before AT+CFUN=1 command or
  // otherwise PPP will be started for the default PDN connection.
  AT#XPPP=1,1

  OK

  AT+CFUN=1

  OK

  // Activate the created PDN connection.
  AT+CGACT=1,1

  // PPP is automatically started when the PDN connection set for PPP has been activated.
  #XPPP: 1,0,1

  // Have the peer connect to SLM's PPP.
  #XPPP: 1,1,1

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

You can test SLM's PPP on Linux by using the ``pppd`` command.
This section describes a configuration without CMUX.
If you are using CMUX, see :ref:`slm_as_linux_modem` for more information on setting it up.

For the process described here, SLM's UARTs must be connected to the Linux host.

1. Get PPP running on SLM.
   To do this, start SLM and issue an ``AT+CFUN=1`` command.
#. Wait for ``#XPPP: 1,0,0``, which is sent when the network registration succeeds and PPP has started successfully with the default PDN connection.
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
