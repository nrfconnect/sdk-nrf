.. _ug_thread_tools:

Thread tools
############

.. contents::
   :local:
   :depth: 2

The tools listed on this page can be helpful when developing your Thread application with the |NCS|.

.. _ug_thread_tools_sniffer:

nRF Sniffer for 802.15.4
************************

The nRF Sniffer for 802.15.4 is a tool for learning about and debugging applications that are using protocols based on IEEE 802.15.4, like Thread or Zigbee.
It provides a near real-time display of 802.15.4 packets that are sent back and forth between devices, even when the link is encrypted.

See `nRF Sniffer for 802.15.4 based on nRF52840 with Wireshark`_ for documentation.

.. _ug_thread_tools_ttm:

nRF Thread Topology Monitor
***************************

nRF Thread Topology Monitor is a desktop application that connects to a Thread network through a serial connection to visualize the topology of Thread devices.
It allows you to scan for new devices in real time, check their parameters, and inspect network processes through the log.

See `nRF Thread Topology Monitor`_ for documentation.

.. _ug_thread_tools_tbr:

Thread Border Router
********************

Thread Border Router is a specific type of Border Router device that provides connectivity from the IEEE 802.15.4 network to adjacent networks on other physical layers (such as Wi-Fi or Ethernet).
Border Routers provide services for devices within the IEEE 802.15.4 network, including routing services for off-network operations.

Typically, a Border Router solution consists of the following parts:

* Application based on the :ref:`thread_architectures_designs_cp_ncp` design or its :ref:`thread_architectures_designs_cp_rcp` variant compatible with the IEEE 802.15.4 standard.
  This application can be implemented, for example, on an nRF52 device.
* Host-side application, usually implemented on a more powerful device with incorporated Linux-based operating system.

|NCS| does not provide a complete Thread Border Router solution.
For development purposes, you can use `OpenThread Border Router`_, an open-source Border Router implementation that you can set up either on your PC using Docker or on Raspberry Pi.
OpenThread Border Router is compatible with Nordic Semiconductor devices.

.. _ug_thread_tools_wpantund:

wpantund
********

`wpantund`_ is a utility for providing a native IPv6 interface to a Network Co-Processor.
When working with Thread, it is used for interacting with the application by the following samples:

* :ref:`ot_coprocessor_sample`

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

1. Open a shell and run the wpantund process.
   The required command depends on whether you want to connect to a network co-processor (NCP) node or a radio co-processor (RCP) node.

   Replace the following parameters:

   * *network_interface_name* - Specifies the name of the network interface, for example, ``leader_if``.
   * *ncp_uart_device* - Specifies the location of the device, for example, :file:`/dev/ttyACM0`.
   * *baud_rate* - Specifies the baud rate to use.
     The Thread samples support baud rate ``1000000``.

   Network co-processor (NCP)
     When connecting to an NCP node, use the following command:

     .. parsed-literal::
        :class: highlight

        wpantund -I *network_interface_name* -s *ncp_uart_device* -b *baud_rate*

     For example::

        sudo wpantund -I leader_if -s /dev/ttyACM0 -b 1000000

   Radio co-processor (RCP)
     When connecting to an RCP node, you must use the ``ot-ncp`` tool to establish the connection.
     See :ref:`ug_thread_tools_ot_apps` for more information.
     Use the following command:

     .. parsed-literal::
        :class: highlight

        wpantund -I *network_interface_name* -s 'system:./output/posix/bin/ot-ncp spinel+hdlc+uart://\ *ncp_uart_device*\ ?uart-baudrate=\ *baud_rate*

     For example::

        sudo wpantund -I leader_if -s 'system:./output/posix/bin/ot-ncp spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=1000000'

#. Open another shell and run the wpanctl process by using the following command:

   .. parsed-literal::
      :class: highlight

      wpanctl -I *network_interface_name*

   This process can be used to control the connected NCP kit.

Once wpantund and wpanctl are started, you can start running wpanctl commands to interact with the development kit.

Using wpanctl commands
======================

To issue a wpanctl command, run it in the wpanctl shell.
For example, the following command checks the kit state:

.. code-block:: console

   wpanctl:leader_if> status

The output will be different depending on the kit and the sample.

The most common wpanctl commands are the following:

* ``status`` - Checks the kit state.
* ``form "*My_OpenThread_network*"`` - Sets up a Thread network with the name ``My_OpenThread_network``.
* ``get`` - Gets the values of all properties.
* ``get *property*`` - Gets the value of the requested property.
  For example, ``get NCP:SleepyPollInterval`` lists the value of the ``NCP:SleepyPollInterval`` property.
* ``set *property* *value*`` - Sets the value of the requested property to the required value.
  For example, ``set NCP:SleepyPollInterval 1000`` sets the value of the ``NCP:SleepyPollInterval`` property to ``1000``.

For the full list of commands, run the ``help`` command in wpanctl.

.. _ug_thread_tools_pyspinel:

Pyspinel
********

`Pyspinel`_ is a tool for controlling OpenThread co-processor instances through a command-line interface.

.. note::
    The tool is available for Linux and macOS and is not supported on Windows.

Installing Pyspinel
===================

See the `Pyspinel`_ documentation for general installation instructions.

Configuring Pyspinel
====================

When working with samples that support Pyspinel, complete the following steps to communicate with the device:

1. Open a shell in a Pyspinel root directory.
#. Run Pyspinel to connect to the node.
   The required command depends on whether you want to connect to a network co-processor (NCP) node or a radio co-processor (RCP) node.

   Replace the following parameters:

   * *debug_level* - Specifies the debug level, range: ``0-5``.
   * *ncp_uart_device* - Specifies the location of the device, for example, :file:`/dev/ttyACM0`.
   * *baud_rate* - Specifies the baud rate to use.
     The Thread samples support baud rate ``1000000``.

   Network co-processor (NCP)
     When connecting to an NCP node, use the following command:

     .. parsed-literal::
        :class: highlight

        sudo python3 spinel-cli.py -d *debug_level* -u *ncp_uart_device* -b *baud_rate*

     For example::

        sudo python3 spinel-cli.py -d 4 -u /dev/ttyACM0 -b 1000000

   Radio co-processor (RCP)
     When connecting to an RCP node, you must use the ``ot-ncp`` tool to establish the connection.
     See :ref:`ug_thread_tools_ot_apps` for more information.
     To enable logs from the RCP Spinel backend, you must build the ``ot-ncp`` tool with option ``LOG_OUTPUT=APP``.
     See :ref:`ug_thread_tools_building_ot_apps` for information on how to build the tool.

     Use the following command to connect to an RCP node:

     .. parsed-literal::
        :class: highlight

        sudo python3 spinel-cli.py -d *debug_level* -p './output/posix/bin/ot-ncp spinel+hdlc+uart://\ *ncp_uart_device*\ ?uart-baudrate=\ *baud_rate*

     For example::

        sudo python3 spinel-cli.py -d 4 -p './output/posix/bin/ot-ncp spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=1000000'

.. _ug_thread_tools_ot_apps:

OpenThread POSIX applications
*****************************

OpenThread POSIX applications allow to communicate with a radio co-processor in a comfortable way.

OpenThread provides the following applications:

* ``ot-ncp`` - Supports :ref:`ug_thread_tools_wpantund` for the RCP architecture.
* ``ot-cli`` - Works like the :ref:`ot_cli_sample` sample for the RCP architecture.
* ``ot-daemon`` and ``ot-ctl`` - Provides the same functionality as ``ot-cli``, but keeps the daemon running in the background all the time.
  See `OpenThread Daemon`_ for more information.

When working with Thread, you can use these tools to interact with the following sample:

* :ref:`ot_coprocessor_sample`

See `OpenThread POSIX app`_ for more information.

.. _ug_thread_tools_building_ot_apps:

Building the OpenThread POSIX applications
==========================================

Build the OpenThread POSIX applications by performing the following steps:

#. Open a shell in the OpenThread source code directory :file:`ncs/modules/lib/openthread`.
#. Initialize the build and clean previous artifacts by running the following commands:

     .. code-block:: console

        # initialize GNU Autotools
        ./bootstrap

        # clean previous artifacts
        make -f src/posix/Makefile-posix clean

#. Build the applications with the required options.
   For example, to build POSIX applications like ``ot-cli`` or ``ot-ncp`` with log output being redirected to the application, run the following command:

     .. code-block:: console

        # build core for POSIX (ot-cli, ot-ncp)
        make -f src/posix/Makefile-posix LOG_OUTPUT=APP

   Alternatively, to build POSIX applications like ``ot-daemon`` or ``ot-ctl``, run the following command:

     .. code-block:: console

        # build daemon mode core stack for POSIX (ot-daemon, ot-ctl)
        make -f src/posix/Makefile-posix DAEMON=1

You can find the generated applications in :file:`./output/posix/bin/`.
