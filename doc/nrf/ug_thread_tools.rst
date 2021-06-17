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

See `nRF Sniffer for 802.15.4`_ for documentation.

.. _ug_thread_tools_ttm:

nRF Thread Topology Monitor
***************************

nRF Thread Topology Monitor is a desktop application that connects to a Thread network through a serial connection to visualize the topology of Thread devices.
It allows you to scan for new devices in real time, check their parameters, and inspect network processes through the log.

See `nRF Thread Topology Monitor`_ for documentation.

.. _ug_thread_tools_tbr:

Thread Border Router
********************

The Thread Border Router is a specific type of Border Router device that provides connectivity from the IEEE 802.15.4 network to adjacent networks on other physical layers (such as Wi-Fi or Ethernet).
Border Routers provide services for devices within the IEEE 802.15.4 network, including routing services for off-network operations.

Typically, a Border Router solution consists of the following parts:

* An application based on the :ref:`thread_architectures_designs_cp_ncp` design or its :ref:`thread_architectures_designs_cp_rcp` variant compatible with the IEEE 802.15.4 standard.
  This application can be implemented, for example, on an nRF52 device.
* A host-side application, usually implemented on a more powerful device with an incorporated Linux-based operating system.

The |NCS| does not provide a complete Thread Border Router solution.
For development purposes, you can use the `OpenThread Border Router`_ (OTBR) released by Google, an open-source Border Router implementation that you can set up either on your PC using Docker or on a Raspberry Pi.

The OpenThread Border Router is compatible with Nordic Semiconductor devices.
It implements a number of features, including:

* Bidirectional IP connectivity between Thread and Wi-Fi or Ethernet networks (or both)
* Network components that allow Thread nodes to connect to IPv4 networks (NAT64, DNS64)
* Bidirectional DNS-based service discovery using mDNS (on Wi-Fi or Ethernet link, or both) and SRP (on Thread network)
* External Thread commissioning (for example, using a mobile phone) to authenticate and join a Thread device to a Thread network

You can either install the OpenThread Border Router on a Raspberry Pi or run it using a Docker container, as described in the following sections.
In both cases, you must first configure a radio co-processor (RCP), which provides the required radio capability to your Linux device.

.. _ug_thread_tools_tbr_rcp:

Configuring a radio co-processor
================================

The OpenThread Border Router must have physical access to the IEEE 802.15.4 network that is used by the Thread protocol.
As neither the Linux-based PC nor the Raspberry Pi have such radio capability, you must connect an external nRF device that serves as radio co-processor.

To program the nRF device with the RCP application, complete the following steps:

#. Clone the OpenThread nRF528xx platform repository into the current directory:

   .. code-block:: console

      git clone --recursive https://github.com/openthread/ot-nrf528xx.git

#. Enter the :file:`ot-nrf528xx` directory:

   .. code-block:: console

      cd ot-nrf528xx

#. Install the OpenThread dependencies:

   .. code-block:: console

      ./script/bootstrap

#. Build the RCP example for the hardware platform and the transport of your choice:

   .. tabs::

      .. tab:: nRF52840 Dongle (USB transport)

         .. code-block:: console

            rm -rf build
            script/build nrf52840 USB_trans -DOT_BOOTLOADER=USB -DOT_THREAD_VERSION=1.2

      .. tab:: nRF52840 Development Kit (UART transport)

         .. code-block:: console

            rm -rf build
            script/build nrf52840 UART_trans -DOT_THREAD_VERSION=1.2

   ..

   This creates an RCP image at :file:`build/bin/ot-rcp`.
#. Convert the RCP image to hexadecimal format:

   .. code-block:: console

      arm-none-eabi-objcopy -O ihex build/bin/ot-rcp build/bin/ot-rcp.hex

#. Depending on the hardware platform, complete the following steps:

   .. tabs::

      .. tab:: nRF52840 Dongle (USB transport)

         a. Install nRF Util:

            .. code-block:: console

               python3 -m pip install -U nrfutil

            .. note::

               If you are using a Raspberry Pi, the nRF Util version distributed officially through PyPI is not supported.
               To install a compatible version on Raspbian OS, execute the following commands:

               .. code-block:: console

                  sudo apt-get -y install libusb-1.0-0-dev sed
                  pip3 install click crcmod ecdsa intelhex libusb1 piccata protobuf pyserial pyyaml tqdm pc_ble_driver_py pyspinel
                  pip3 install -U --no-dependencies nrfutil==6.0.1
                  export PATH="$HOME/.local/bin:$PATH"

         #. Generate the RCP firmware package:

            .. code-block:: console

               nrfutil pkg generate --hw-version 52 --sd-req=0x00 \
                --application build/bin/ot-rcp.hex --application-version 1 build/bin/ot-rcp.zip

         #. Connect the nRF52840 Dongle to the USB port.
         #. Press the **RESET** button on the dongle to put it into the DFU mode.
            The LED on the dongle starts blinking red.
         #. Install the RCP firmware package onto the dongle by running the following command, with ``/dev/ttyACM0`` replaced with the device node name of your nRF52840 Dongle:

            .. code-block:: console

               nrfutil dfu usb-serial -pkg build/bin/ot-rcp.zip -p /dev/ttyACM0

      .. tab:: nRF52840 Development Kit (UART transport)

         a. Program the image using the nrfjprog utility (which is part of the `nRF Command Line Tools`_):

            .. code-block:: console

               nrfjprog -f nrf52 --chiperase --program build/bin/ot-rcp.hex --reset

         #. Disable the Mass Storage feature on the device, so that it does not interfere with the core RCP functionalities:

            .. parsed-literal::
               :class: highlight

               JLinkExe -device NRF52840_XXAA -if SWD -speed 4000 -autoconnect 1 -SelectEmuBySN *SEGGER_ID*
               J-Link>MSDDisable
               Probe configured successfully.
               J-Link>exit

            Replace *SEGGER_ID* with the SEGGER ID of your nRF52840 Development Kit.
            This setting remains valid even if you program another firmware onto the device.
         #. Power-cycle the device to apply the changes.

Installing OTBR manually (Raspberry Pi)
=======================================

The recommended option is to build and configure the OpenThread Border Router on a Raspberry Pi 3 Model B or newer.
This option provides most of the functionalities available in the OpenThread Border Router, such as border routing capabilities needed for establishing Thread communication with a mobile phone on a Wi-Fi network.
However, this approach requires you to download the OpenThread Border Router repository and install the Border Router manually on the Raspberry Pi.

To set up and configure the OpenThread Border Router, follow the official `OpenThread Border Router Codelab tutorial`_ on the OpenThread documentation portal.
Omit the *Build and flash RCP firmware* section, because this section duplicates the steps performed in the previous section.


Running OTBR using Docker
=========================

For development purposes, you can run the OpenThread Border Router on any Linux-based system using a Docker container that already has the Border Router installed.
This solution can be used when you are only interested in direct communication between your Border Router and the Thread network.
For example, you can use the Docker container when you want to establish IP communication between an application running on Linux (such as the Python Controller for Matter) and an application running on a Thread node.

To install and configure the OpenThread Border Router using the Docker container on an Ubuntu operating system, complete the following steps:

#. Install the Docker daemon:

   .. code-block:: console

      sudo apt update && sudo apt install docker.io

#. Start the Docker daemon:

   .. code-block:: console

      sudo systemctl start docker

#. Create an IPv6 network for the OpenThread Border Router container in Docker:

   .. code-block:: console

      sudo docker network create --ipv6 --subnet fd11:db8:1::/64 -o com.docker.network.bridge.name=otbr0 otbr

#. Download the latest version of the OpenThread Border Router Docker image by running the following command:

   .. code-block:: console

      docker pull openthread/otbr

#. Connect the radio co-processor that you configured in :ref:`ug_thread_tools_tbr_rcp` to the Border Router device.
#. Start the OpenThread Border Router container using the following command (in the last line, replace ``/dev/ttyACM0`` with the device node name of the OpenThread radio co-processor):

   .. code-block:: console

      sudo docker run -it --rm --privileged --name otbr --network otbr -p 8080:80 \
      --sysctl "net.ipv6.conf.all.disable_ipv6=0 net.ipv4.conf.all.forwarding=1 net.ipv6.conf.all.forwarding=1" \
      --volume /dev/ttyACM0:/dev/radio openthread/otbr --radio-url spinel+hdlc+uart:///dev/radio

#. Form the Thread network using one of the following options:

   * Follow the instruction in the `OpenThread Border Router Codelab tutorial step 3`_.
   * Open the ``http://localhost:8080/`` address in a web browser and choose :guilabel:`Form` from the menu.

     .. note::
        If you are using a Raspberry Pi without a screen, but you have a different device in the same network, you can start a web browser on that device and use the address of the Raspberry Pi instead of ``localhost``.

#. Note down the selected On-Mesh Prefix value.
   For example, ``fd11:22::/64``.
#. Make sure that packets addressed to devices in the Thread network are routed through the OpenThread Border Router container in Docker.
   To do this, run the following command that uses the On-Mesh Prefix that you configured in the previous step (in this case, ``fd11:22::/64``):

   .. code-block:: console

      sudo ip -6 route add fd11:22::/64 dev otbr0 via fd11:db8:1::2

#. Check the status of the OpenThread Border Router by executing the following command:

   .. code-block:: console

      sudo docker exec -it otbr sh -c "sudo service otbr-agent status"

#. Check the status of the Thread node running inside the Docker:

   .. code-block:: console

      sudo docker exec -it otbr sh -c "sudo ot-ctl state"

.. _ug_thread_tools_wpantund:

wpantund
********

`wpantund`_ is a utility for providing a native IPv6 interface to a network co-processor.
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
