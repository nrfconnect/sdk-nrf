.. _ug_thread_tools:

Thread tools
************

When working with Thread in |NCS|, you can use the following tools during Thread application development:

* `nRF Thread Topology Monitor`_ - This desktop application helps to visualize the current network topology.
* `nRF Sniffer for 802.15.4 based on nRF52840 with Wireshark`_ - Tool for analyzing network traffic during development.
* :ref:`ug_thread_tools_wpantund`
* `PySpinel`_ - Tool for controlling OpenThread Network Co-Processor instances through command line interface.

Using Thread tools is optional.

.. _ug_thread_tools_wpantund:

wpantund
========

`wpantund`_ is a utility for providing a native IPv6 interface to a Network Co-Processor.
When working with Thread, it is used for the interaction with the application by the following samples:

* :ref:`ot_ncp_sample`

The interaction is possible using commands proper to wpanctl, a module installed with wpantund.

.. note::
    The tool is available for Linux and macOS and is not supported on Windows.

Installing wpantund
-------------------

For installation and initial configuration, see `wpantund Installation Guide`_.

.. _ug_thread_tools_wpantund_configuring:

Configuring wpantund
--------------------

When working with samples that support wpantund, complete the following steps to start the wpantund processes:

1. Open a shell and run the wpantund process by using the following command:

   .. code-block:: console

      wpantund -I <network_interface_name> -s <serial_port_name> -b <baudrate>

   For ``baudrate``, use value ``1000000``.
   For ``serial_port_name``, use the value that is valid for the sample.
   For ``network_interface_name``, use a name of your choice.
   For example, `leader_interface`.
#. Open another shell and run the wpanctl process by using the following command:

   .. code-block:: console

      wpanctl -I leader_interface

   This process can be used to control the connected NCP board.

Once wpantund and wpanctl are started, you can start running wpanctl commands to interact with the board.

Using wpanctl commands
----------------------

To issue a wpanctl command, run it in the wpanctl shell.
For example, the following command checks the the NCP board state:

.. code-block:: console

   wpanctl:leader_interface> status

The output will be different depending on the board and the sample.

The most common wpanctl commands are the following:

* ``status`` - Checks the board state.
* ``form "My_OpenThread_network"`` - Sets up a Thread network with the name ``My_OpenThread_network``.
* ``get`` - Gets the values of all properties.
* ``get <property>`` - Gets the value of the requested property.
  For example, ``get NCP:SleepyPollInterval`` will list the value of the ``NCP:SleepyPollInterval`` property.
* ``set <property> <value>`` - Sets the value of the requested property to the required value.
  For example, ``set NCP:SleepyPollInterval 1000`` will set the value of the ``NCP:SleepyPollInterval`` property to ``1000``.

For the full list of commands, run the ``help`` command in wpanctl.
