.. _SLM_AT_PPP:

PPP AT commands
****************

.. contents::
   :local:
   :depth: 2

This page describes PPP-related AT commands.

Control PPP #XPPP
=================

Set command
-----------

The set command allows you to start and stop PPP.

Syntax
~~~~~~

::

   #XPPP=<op>

* The ``<op>`` parameter can be the following:

  * ``0`` - Stop PPP.
  * ``1`` - Start PPP.
    If PPP is already started, it is first stopped before being started again.

.. note::

   PPP is automatically started and stopped when the default PDN connection is established and lost, respectively.
   This happens even if PPP has previously been stopped or started with this command.

Read command
------------

The read command allows you to get the status of PPP.

Syntax
~~~~~~

::

   #XPPP?

Response syntax
~~~~~~~~~~~~~~~

::

   #XPPP: <running>,<peer_connected>

* The ``<running>`` parameter is an integer that indicates whether PPP is running.
  It is ``1`` for running or ``0`` for stopped.

* The ``<peer_connected>`` parameter is an integer that indicates whether a peer is connected to PPP.
  It is ``1`` for connected or ``0`` for not connected.

Test command
------------

The test command is not supported.

Example
-------

::

  AT+CFUN=1

  OK
  // Wait for network registration. PPP is automatically started.
  AT#XPPP?

  #XPPP: 1,0

  OK
  // Have the peer connect to SLM's PPP. The read command shows that a peer is connected.
  AT#XPPP?

  #XPPP: 1,1

  OK
  AT+CFUN=4

  OK
  AT#XPPP?

  #XPPP: 0,0

  OK
