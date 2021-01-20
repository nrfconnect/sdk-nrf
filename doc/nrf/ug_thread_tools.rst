.. _ug_thread_tools:

Thread tools
############

.. contents::
   :local:
   :depth: 2

When working with Thread in |NCS|, you can use the following tools during Thread application development:

* `nRF Sniffer for 802.15.4 based on nRF52840 with Wireshark`_ - Tool for analyzing network traffic during development.
* `nRF Thread Topology Monitor`_ - This desktop application helps to visualize the current network topology.
* `PySpinel`_ - Tool for controlling OpenThread Network Co-Processor instances through command line interface.
* :ref:`ug_thread_tools_tbr`
* :ref:`ug_thread_tools_wpantund`

Using Thread tools is optional.

.. _ug_thread_tools_tbr:

Thread Border Router
********************

Thread Border Router is a specific type of Border Router device that provides connectivity from the IEEE 802.15.4 network to adjacent networks on other physical layers (such as Wi-Fi or Ethernet).
Border Routers provide services for devices within the IEEE 802.15.4 network, including routing services for off-network operations.

Typically, a Border Router solution consists of the following parts:

* Application based on the :ref:`thread_architectures_designs_cp_ncp` design or its :ref:`thread_architectures_designs_cp_rcp` variant compatible with the IEEE 802.15.4 standard.
  This application can be implemented for example on an nRF52 device.
* Host-side application, usually implemented on a more powerful device with incorporated Linux-based operating system.

|NCS| does not provide the complete Thread Border Router solution.
For development purposes, you can use `OpenThread Border Router`_ , an open-source Border Router implementation that you can set up either on your PC using Docker or on Raspberry Pi.
OpenThread Border Router is compatible with Nordic Semiconductor devices.

.. _ug_thread_tools_wpantund:

wpantund
********

`wpantund`_ is a utility for providing a native IPv6 interface to a Network Co-Processor.
When working with Thread, it is used for the interaction with the application by the following samples:

* :ref:`ot_ncp_sample`

The interaction is possible using commands proper to wpanctl, a module installed with wpantund.

.. note::
    The tool is available for Linux and macOS and is not supported on Windows.

Installing wpantund
===================

To ensure that the interaction with the samples works as expected, install the version of wpantund that has been used for testing the |NCS|.

See the `wpantund Installation Guide`_ for general installation instructions.
To install the verified version, replace the ``git checkout full/latest-release`` command with the following command:

.. parsed-literal::

   git checkout 87c90eedce0c75cb68a1cbc34ff36223400862f1

When installing on macOS, follow the instructions for the manual installation and replace the above command to ensure that the correct version is installed.

.. _ug_thread_tools_wpantund_configuring:

Configuring wpantund
====================

When working with samples that support wpantund, complete the following steps to start the wpantund processes:

1. Open a shell and run the wpantund process by using the following command:

   .. parsed-literal::
      :class: highlight

      wpantund -I *network_interface_name* -s *serial_port_name* -b *baudrate*

   For *baudrate*, use value ``1000000``.
   For *serial_port_name*, use the value that is valid for the sample.
   For *network_interface_name*, use a name of your choice.
   For example, ``leader_if``.
#. Open another shell and run the wpanctl process by using the following command:

   .. code-block:: console

      wpanctl -I leader_if

   This process can be used to control the connected NCP kit.

Once wpantund and wpanctl are started, you can start running wpanctl commands to interact with the development kit.

Using wpanctl commands
======================

To issue a wpanctl command, run it in the wpanctl shell.
For example, the following command checks the the NCP kit state:

.. code-block:: console

   wpanctl:leader_if> status

The output will be different depending on the kit and the sample.

The most common wpanctl commands are the following:

* ``status`` - Checks the kit state.
* ``form "*My_OpenThread_network*"`` - Sets up a Thread network with the name ``My_OpenThread_network``.
* ``get`` - Gets the values of all properties.
* ``get *property*`` - Gets the value of the requested property.
  For example, ``get NCP:SleepyPollInterval`` will list the value of the ``NCP:SleepyPollInterval`` property.
* ``set *property* *value*`` - Sets the value of the requested property to the required value.
  For example, ``set NCP:SleepyPollInterval 1000`` will set the value of the ``NCP:SleepyPollInterval`` property to ``1000``.

For the full list of commands, run the ``help`` command in wpanctl.
