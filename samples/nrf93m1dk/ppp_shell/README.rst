.. _nrf93m1dk_ppp_shell:

nRF93M1 DK: PPP shell
#####################

.. contents::
   :local:
   :depth: 2

This sample establishes a PPP connection between the nRF54L15 host core and the nRF93M1 modem over a CMUX-multiplexed UART link, giving shell-driven access to IP networking and performance testing tools.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample uses the Zephyr modem cellular driver with CMUX to multiplex the single physical UART between the PPP data channel and an AT command channel.
The AT command channel is made available through the AT command shell (:kconfig:option:`CONFIG_MODEM_AT_SHELL`) backend so that AT commands can be issued from the shell.

IPv4, IPv6, DNS, TCP, UDP, and POSIX sockets are all enabled.
The :ref:`zperf <zephyr:zperf>` networking performance tool is included for throughput testing.

The nRF93M1 side requires no additional firmware preparation, as the nRF54L15 host completely controls the modem.

The UART baud rate is 115200.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf93m1dk/ppp_shell`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to the development kit, complete the following steps to establish the PPP connection:

1. Connect the kit to the host PC using a USB cable and open a serial terminal at 115200 baud.
#. Initialize the PPP interface:

   .. code-block:: console

      uart:~$ net iface up 1

   The shell prints network management events as the connection is established.
   When connectivity is available, you will see output similar to::

      EVENT: L4 [1] IPv4 connectivity available

#. Verify the IP configuration:

   .. code-block:: console

      uart:~$ net iface

#. Close the PPP connection when you are done:

   .. code-block:: console

      uart:~$ net iface down 1
