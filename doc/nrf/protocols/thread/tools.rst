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

.. sniffer_shortdesc_start

The nRF Sniffer for 802.15.4 is a tool for learning about and debugging applications that are using protocols based on IEEE 802.15.4, like Thread or Zigbee.
It provides a near real-time display of 802.15.4 packets that are sent back and forth between devices, even when the link is encrypted.

See `nRF Sniffer for 802.15.4`_ for documentation.

.. sniffer_shortdesc_end

.. _ug_thread_tools_ttm:

nRF Thread Topology Monitor
***************************

.. ttm_shortdesc_start

nRF Thread Topology Monitor is a desktop application that connects to a Thread network through a serial connection to visualize the topology of Thread devices.
It allows you to scan for new devices in real time, check their parameters, and inspect network processes through the log.

See `nRF Thread Topology Monitor`_ for documentation.

.. ttm_shortdesc_end

.. _ug_thread_tools_tbr:

Thread Border Router
********************

.. tbr_shortdesc_start

The Thread Border Router is a specific type of Border Router device that provides connectivity from the IEEE 802.15.4 network to adjacent networks on other physical layers (such as Wi-Fi or Ethernet).
Border Routers provide services for devices within the IEEE 802.15.4 network, including routing services for off-network operations.

.. tbr_shortdesc_end

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

#. Build the :ref:`ot_coprocessor_sample` sample for the hardware platform and the transport of your choice:

   .. tabs::

      .. tab:: nRF52840 Dongle (USB transport)

         .. code-block:: console

            west build -p always -b nrf52840dongle_nrf52840 nrf/samples/openthread/coprocessor/

      .. tab:: nRF52840 Development Kit (UART transport)

         .. code-block:: console

            west build -p always -b nrf52840dk_nrf52840 nrf/samples/openthread/coprocessor/

#. Depending on the hardware platform, complete the following steps:

   .. tabs::

      .. tab:: nRF52840 Dongle (USB transport)

         a. Install nRF Util as described in `Installing nRF Util for nRF5 SDK`_.
         #. Generate the RCP firmware package:

            .. code-block:: console

               nrfutil pkg generate --hw-version 52 --sd-req=0x00 \
                --application build/zephyr/zephyr.hex --application-version 1 build/zephyr/zephyr.zip

         #. Connect the nRF52840 Dongle to the USB port.
         #. Press the **RESET** button on the dongle to put it into the DFU mode.
            The LED on the dongle starts blinking red.
         #. Install the RCP firmware package onto the dongle by running the following command, with ``/dev/ttyACM0`` replaced with the device node name of your nRF52840 Dongle:

            .. code-block:: console

               nrfutil dfu usb-serial -pkg build/zephyr/zephyr.zip -p /dev/ttyACM0

      .. tab:: nRF52840 Development Kit (UART transport)

         a. Program the image using the :ref:`regular command <programming>`.
         #. Disable the Mass Storage feature on the device, so that it does not interfere with the core RCP functionalities.
            Also, force Hardware Flow Control to avoid potential race conditions related to the auto-detection:

            .. parsed-literal::
               :class: highlight

               JLinkExe -device NRF52840_XXAA -if SWD -speed 4000 -autoconnect 1 -SelectEmuBySN *SEGGER_ID*
               J-Link>MSDDisable
               Probe configured successfully.
               J-Link>SetHWFC Force
               New configuration applies immediately.
               J-Link>exit

            Replace *SEGGER_ID* with the SEGGER ID of your nRF52840 Development Kit.
            This setting remains valid even if you program another firmware onto the device.
         #. Power-cycle the device to apply the changes.

Installing OTBR manually (Raspberry Pi)
=======================================

The recommended option is to build and configure the OpenThread Border Router on a Raspberry Pi 3 Model B or newer.
This option provides most of the functionalities available in the OpenThread Border Router, such as border routing capabilities needed for establishing Thread communication with a mobile phone on a Wi-Fi network.
However, this approach requires you to download the OpenThread Border Router repository and install the Border Router manually on the Raspberry Pi.

To set up and configure the OpenThread Border Router, follow the official `OpenThread Border Router Codelab tutorial`_ on the OpenThread documentation portal with the below modifications:

* After cloning the repository in the *Get OTBR code* section, make sure to check out the compatible commit id:

   .. code-block:: console

      cd ot-br-posix
      git pull --unshallow
      git checkout 9185bda

* After the *Build and install OTBR* section, configure RCP device's UART baud rate in *otbr-agent*.
  Modify the :file:`/etc/default/otbr-agent` configuration file with default RCP baud rate:

  .. code-block:: console

     spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=1000000

* Omit the *Build and flash RCP firmware* section, because that section duplicates the steps already performed in the `Configuring a radio co-processor`_ section of this guide.

Running OTBR using Docker
=========================

For development purposes, you can run the OpenThread Border Router on any Linux-based system using a Docker container that already has the Border Router installed.
This solution can be used when you are only interested in direct communication between your Border Router and the Thread network.
For example, you can use the Docker container when you want to establish IP communication between an application running on Linux (such as the :ref:`CHIP Tool Matter controller <ug_matter_configuring>`) and an application running on a Thread node.

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

#. Download the compatible version of the OpenThread Border Router docker image by running the following command:

   .. code-block:: console

      docker pull nrfconnect/otbr:84c6aff

#. Connect the radio co-processor that you configured in :ref:`ug_thread_tools_tbr_rcp` to the Border Router device.
#. Start the OpenThread Border Router container using the following commands:

   .. code-block:: console

      sudo modprobe ip6table_filter
      sudo docker run -it --rm --privileged --name otbr --network otbr -p 8080:80 \
      --sysctl "net.ipv6.conf.all.disable_ipv6=0 net.ipv4.conf.all.forwarding=1 net.ipv6.conf.all.forwarding=1" \
      --volume /dev/ttyACM0:/dev/radio nrfconnect/otbr:84c6aff --radio-url spinel+hdlc+uart:///dev/radio?uart-baudrate=1000000

   Replace ``/dev/ttyACM0`` with the device node name of the OpenThread radio co-processor.

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

.. note::
   OTBR on the Docker has got disabled DNS64 service by default.

.. _ug_thread_tools_ot_apps:

OpenThread POSIX applications
*****************************

OpenThread POSIX applications allow to communicate with a radio co-processor (RCP) in a comfortable way.

OpenThread provides the following applications:

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


#. Enter the :file:`openthread` directory:

   .. code-block:: console

      cd modules/lib/openthread

#. Install the OpenThread dependencies:

   .. code-block:: console

      ./script/bootstrap

#. Build the applications with the required options.
   For example, to build the ``ot-cli`` application with support for Thread v1.1, run the following command::

      ./script/cmake-build posix -DOT_THREAD_VERSION=1.1

   Alternatively, to build the ``ot-daemon`` and ``ot-ctl`` applications with support for Thread v1.2, run the following command::

      ./script/cmake-build posix -DOT_THREAD_VERSION=1.2 -DOT_DAEMON=ON

You can find the generated applications in :file:`./build/posix/src/posix/`.

Running the OpenThread POSIX applications
=========================================

Use the following radio URL parameter to connect to an RCP node.

.. code-block:: console

   'spinel+hdlc+uart://\ *ncp_uart_device*\ ?uart-baudrate=\ *baud_rate*'

Replace the following parameters:

   * *ncp_uart_device* - Specifies the location of the device, for example: :file:`/dev/ttyACM0`.
   * *baud_rate* - Specifies the baud rate to use.
     The Thread Co-Processor sample supports baud rate ``1000000``.

For example, to use ``ot-daemon``, enter the following command:

.. code-block:: console

   sudo ./build/posix/src/posix/ot-daemon 'spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=1000000' --verbose

And on a separate terminal window:

.. code-block:: console

   sudo ./build/posix/src/posix/ot-ctl

To use ``ot-cli``, enter the following command instead:

.. code-block:: console

   sudo ./build/posix/src/posix/ot-cli 'spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=1000000' --verbose
