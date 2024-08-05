.. _modem_shell_application:

Cellular: Modem Shell
#####################

.. contents::
   :local:
   :depth: 2

The Modem Shell (MoSh) sample application enables you to test various device connectivity features, including data throughput.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

.. include:: /includes/external_flash_nrf91.txt

Overview
********

MoSh enables testing of different connectivity features such as LTE link handling, TCP/IP connections, data throughput (iperf3 and curl), SMS, GNSS, FOTA updates and PPP.
Hence, this sample is not only code sample, which it also is in many aspects, but also a test application for aforementioned features.
MoSh uses the LTE link control driver to establish an LTE connection and initializes the Zephyr shell to provide a shell command-line interface for users.

The subsections list the MoSh features and show shell command examples for their usage.

.. note::
   To learn more about using a MoSh command, run the command without any parameters.

LTE link control
================

MoSh command: ``link``

LTE link control changes and queries the state of the LTE connection.
Many of the changes are applied when going to online mode for the next time.
You can store some link subcommand parameters into settings, which are persistent between sessions.

For nRF9160, 3GPP Release 14 features are enabled by default, which means they are set when going into normal mode and when booting up.
To disable these features in normal mode,  use the ``link funmode --normal_no_rel14`` command.
During autoconnect in bootup, use the ``link nmodeauto --enable_no_rel14`` command.
For the list of supported features, refer to the `3GPP Release 14 features AT command`_ section in the nRF9160 AT Commands Reference Guide.
For nRF91x1, 3GPP Release 14 features are always enabled and cannot be disabled.

Examples
--------

* Change the system mode to NB-IoT and query the connection status:

  .. code-block:: console

     link funmode -4
     link sysmode -n
     link funmode -1
     link status

* Open another PDP context and close it:

  .. code-block:: console

     link connect -a internet.operator.com
     link status
     link disconnect -I 1

* Enable and configure eDRX:

  .. code-block:: console

     link edrx -e --ltem_edrx 0010 --nbiot_edrx 0010

* Subscribe for modem TAU and sleep notifications, enable and configure PSM:

  .. code-block:: console

     link tau --subscribe
     link msleep --subscribe
     link psm -e --rptau 01100100 --rat 00100010

* Disable autoconnect when the DK starts (modem will remain in pwroff functional mode and the link is not connected):

  .. code-block:: console

     link nmodeauto --disable

* Set the custom AT commands that are run when switching to normal mode (locking device to band #3):

  .. code-block:: console

     link funmode --poweroff
     link nmodeat --mem1 "at%xbandlock=2,\"100\""
     link funmode --normal

* Write periodic search parameters with two patterns, read the parameters and start modem network search operation:

  .. code-block:: console

     link search --write --search_cfg="0,1,1" --search_pattern_table="10,10,30" --search_pattern_range="50,300,10,20"
     link search --read
     link search --start

----

AT commands
===========

MoSh command: ``at``

You can use the AT command module to send AT commands to the modem, individually or
in a separate plain AT command mode where also pipelining of AT commands is supported.

.. note::
   Using AT commands that read information from the modem is safe together with MoSh commands.
   However, it is not recommended to write any values with AT commands and use MoSh commands among them.
   If you mix AT commands and MoSh commands, the internal state of MoSh might get out of synchronization and result in unexpected behavior.

.. note::
   When using ``at`` command, any quotation marks (``"``), apostrophes (``'``) and backslashes (``\``) within the AT command syntax must be escaped with a backslash (``\``).
   The percentage sign (``%``) is often needed and can be written as is.

Examples
--------

* Send AT command to query network status:

  .. code-block:: console

     at at+cereg?

* Send AT command to query neighbor cells:

  .. code-block:: console

     at at%NBRGRSRP

* Escape quotation marks in a command targeting the network search to a specific operator:

  .. code-block:: console

     at AT+COPS=1,2,\"24407\"

* Enable AT command events:

  .. code-block:: console

     at events_enable

* Disable AT command events:

  .. code-block:: console

     at events_disable

* Enable autostarting of AT command mode in next bootup:

  .. code-block:: console

     at at_cmd_mode enable_autostart

* Start AT command mode:

  .. code-block:: console

     at at_cmd_mode start
     MoSh AT command mode started, press ctrl-x ctrl-q to escape
     MoSh specific AT commands:
       ICMP Ping: AT+NPING=<addr>[,<payload_length>,<timeout_msecs>,<count>[,<interval_msecs>[,<cid>]]]
     Other custom functionalities:
       AT command pipelining, for example:
         at+cgmr|at+cfun?|at+nping="example.com"
     ===========================================================
     at
     OK

----

Ping
====

MoSh command: ``ping``

Ping is a tool for testing the reachability of a host on an IP network.

Examples
--------

* Ping a URL:

  .. code-block:: console

     ping -d ping.server.url

* Ping an IPv6 address with length of 500 bytes and 1 s intervals (the used PDN needs to support IPv6):

  .. code-block:: console

     ping -d 1a2b:1a2b:1a2b:1a2b::1 -6 -l 500 -i 1000

----

Iperf3
======

MoSh command: ``iperf``

Iperf3 is a tool for measuring data transfer performance both in uplink and downlink direction.

.. note::
   Some features, for example file operations and TCP option tuning, are not supported.

Examples
--------

* Download data over TCP for 30 seconds with a buffer size of 3540 bytes and use detailed output:

  .. code-block:: console

     iperf3 --client 111.222.111.222 --port 10000 -l 3540 --time 30 -V -R

* Upload data over TCP for 30 seconds with the payload size of 708 bytes using a PDN with ID #2 (use the ``link status`` command to see the active PDNs and their IDs):

  .. code-block:: console

     iperf3 -c 111.222.111.222 -p 10000 -l 708 -t 30 --pdn_id 2

* Upload data over UDP for 60 seconds with the payload size of 1240 bytes and use the detailed output as well as debug output:

  .. code-block:: console

     iperf3 --client 111.222.111.222 --port 10000 -l 1240 --time 60 -u -V -d

* Download data over TCP/IPv6 for 10 seconds (the used PDN needs to support IPv6, use the ``link status`` command to see PDP type support for active contexts):

  .. code-block:: console

     iperf3 --client 1a2b:1a2b:1a2b:1a2b::1 --port 20000 --time 10 -R -6

----

Curl
====

MoSh command: ``curl``

Curl is a command-line tool for transferring data specified with URL syntax.
It is a part of MoSh and enables you to test the data download with a "standard" tool.

.. note::
   File operations are not supported.

Examples
--------

* HTTP download:

  .. code-block:: console

     curl http://curl.server.url/small.txt
     curl http://curl.server.url/bigger_file.zip --output /dev/null

* HTTP upload for given data:

  .. code-block:: console

     curl http://curl.server.url/data -d "foo=bar"

* HTTP upload for given number of bytes sent in a POST body:

  .. code-block:: console

     curl http://curl.server.url/data -d #500000

* HTTP upload for given number of bytes sent in a POST body using a PDN with ID #1 (use the ``link status`` command to see active PDNs and their IDs):

  .. code-block:: console

     curl http://curl.server.url/data -d #500000 --pdn_id 1

----

Socket tool
===========

MoSh command: ``sock``

You can use the socket tool to:

* Create and manage socket connections.
* Send and receive data.

Examples
--------

* Open and connect to an IP address and port (IPv4 TCP socket):

  .. code-block:: console

     sock connect -a 111.222.111.222 -p 20000

* Open and connect to hostname and port (IPv4 TCP socket):

  .. code-block:: console

     sock connect -a google.com -p 20000

* Open and connect an IPv6 TCP socket and bind to a port:

  .. code-block:: console

     sock connect -a 1a2b:1a2b:1a2b:1a2b::1 -p 20000 -f inet6 -t stream -b 40000

* Open an IPv6 UDP socket:

  .. code-block:: console

     sock connect -a 1a2b:1a2b:1a2b:1a2b::1 -p 20000 -f inet6 -t dgram

* Open an IPv6 DTLS socket:

  .. code-block:: console

     sock connect -a 1a2b:1a2b:1a2b:1a2b::1 -p 20000 -f inet6 -t dgram -S -T 123

  .. note::
     The certificate must have been written beforehand to security tag ``123``.
     See the `Credential storage management %CMNG`_ section in the nRF9160 AT Commands Reference Guide or the `same section <nRF91x1 credential storage management %CMNG_>`_ in the nRF91x1 AT Commands Reference Guide, depending on the SiP you are using.

* Open a raw socket:

  .. code-block:: console

     sock connect -f packet -t raw

* Open a socket to a non-default PDP context:

  .. code-block:: console

     link connect -a nondefault.context.com
     sock connect -f packet -t raw -I 1

* Send a string through the socket:

  .. code-block:: console

     sock send -i 0 -d testing

* Send 100 kB of data and show throughput statistics:

  .. code-block:: console

     sock send -i 0 -l 100000

* Send data periodically with 10 s interval:

  .. code-block:: console

     sock send -i 0 -e 10 -d test_periodic

* Calculate the receive throughput:

  .. code-block:: console

     <do whatever is needed to make device receive data after some time>
     sock recv -i 0 -r -l 1000000
     sock recv -i 0
     sock recv -i 0

* Close a socket:

  .. code-block:: console

     sock close -i 0

* Use RAI settings:

  .. code-block:: console

     link funmode -4
     link rai -e
     link funmode -1
     sock connect -a 111.222.111.222 -p 20000
     sock rai -i 0 --rai_last
     sock send -i 0 -d testing

  When both 3GPP Release 13 Control Plane (CP) Release Assistance Indication (RAI) and 3GPP Release 14 Access Stratum (AS) RAI are enabled,
  which can be the case for NB-IoT, both are signalled.
  Which RAI takes effect depends on the network configuration and prioritization.

* List open sockets:

  .. code-block:: console

     sock list

----

SMS tool
========

MoSh command: ``sms``

You can use the SMS tool for sending and receiving SMS messages.

Examples
--------

* Register the SMS service so that messages can be received if SIM subscription supports it:

  .. code-block:: console

     sms reg

* Send an SMS message (registration is done automatically if not already done):

  .. code-block:: console

     sms send -n +987654321 -m testing

----

Location tool
=============

MoSh command: ``location``

You can use the Location tool for retrieving device's location with different methods.
See :ref:`lib_location` library for information on the configuration of different location methods and services.
Some default configurations are available to facilitate trials.

This sample is using cloud service for positioning through the :ref:`lib_location` library by default.
However, an application can also handle the cloud communication for the location services by itself.
To enable cloud communication, use the :kconfig:option:`CONFIG_LOCATION_SERVICE_EXTERNAL` Kconfig option and
a separate configuration (:file:`overlay-cloud_mqtt.conf`) to enable nRF Cloud service over MQTT for retrieving location data.
Use the ``cloud`` command to establish the MQTT connection before ``location`` commands.

Examples
--------

* Retrieve location with default configuration:

  .. code-block:: console

     location get

* Retrieve location with Wi-Fi positioning.
  You need to have a Wi-Fi-enabled device and build the sample with Wi-Fi support.
  If the location is not found, use cellular positioning:

  .. code-block:: console

     location get --method wifi --wifi_timeout 60000 --method cellular --cellular_service nrf

* Retrieve location periodically every hour with GNSS and if not found, use cellular positioning:

  .. code-block:: console

     location get --interval 3600 --method gnss --gnss_timeout 300000 --method cellular

* Cancel ongoing location request or periodic location request:

  .. code-block:: console

     location cancel


----

Modem traces
============

MoSh command: ``modem_trace``

Enable the ``modem_trace`` command using the :kconfig:option:`CONFIG_NRF_MODEM_LIB_SHELL_TRACE` and :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE` Kconfig options.

You can use the modem trace commands to control the trace functionality in the modem.
See :ref:`modem_trace_module` for more information on how to configure modem tracing and the built-in trace backends available.
See :ref:`modem_trace_shell_command` for details about the shell command.

To enable modem traces with the flash backend, build with the ``nrf91-modem-trace-ext-flash`` snippet for an nRF91 Series DK that has external flash.
For more information on snippets, see :ref:`zephyr:using-snippets`.

GNSS
====

MoSh command: ``gnss``

GNSS provides commands for searching the location of the device.

Examples
--------

* Start GNSS tracking and stop it:

  .. code-block:: console

     gnss start
     gnss stop

* Disable LTE, enable all GNSS output and start continuous tracking with power saving enabled:

  .. code-block:: console

     link funmode --lteoff
     gnss output 2 1 1
     gnss mode cont
     gnss config powersave perf
     gnss start

* Enable LTE PSM, only NMEA output, automatic A-GNSS data fetching and start periodic fixes with 5 minute interval and 120 second timeout:

  .. code-block:: console

     link psm -e
     gnss output 0 1 0
     gnss agnss automatic enable
     gnss mode periodic 300 120
     gnss start

----

FOTA
====

MoSh command: ``fota``

You can use FOTA to perform software updates over-the-air for both modem and application side.
However, to use this feature, you need to know which updates are available in the servers.
This feature is intended to be used only by selected users and customers to whom available image names are communicated separately.

Examples
--------

* Perform a FOTA update:

  .. code-block:: console

     fota download eu fw_update_package_filename.hex

----

PPP
===

MoSh command: ``ppp``

You can use the PPP (Point-to-Point Protocol) to enable dial-up access to the Internet.
The MoSh command is simple but you need to have a normal dial-up setup in your PC to be able to use the development kit's PPP interface.

.. note::

   On Windows, dial-up connection is not functional when using SEGGER virtual UART ports.
   PPP has been used successfully with FTDI UART port though.
   Refer to `nRF9160 Hardware Verification Guidelines - UART interface`_.

   PPP has been successfully used running Ubuntu Linux in a virtualization environment hosted by Windows.
   In the hosted virtual Linux environment, using PPP is possible also with plain SEGGER UART ports.

Examples
--------

* PPP network interface is brought up/down automatically when LTE connection is up/down. Set the PPP network interface up manually:

  .. code-block:: console

     ppp up

* Set the PPP network interface down manually:

  .. code-block:: console

     ppp down

* Set the custom baudrate configuration for PPP UART (default: 115200):

  .. code-block:: console

     ppp uartconf --baudrate 921600

* Set the ``rts_cts`` hardware flow control configuration for PPP UART (default: none):

  .. code-block:: console

     ppp uartconf --flowctrl rts_cts

----

REST client
===========

MoSh command: ``rest``

You can use the REST client for sending simple REST requests and receiving responses to them.

Examples
--------

* Sending a HEAD request with custom dummy headers:

  .. code-block:: console

     rest -d example.com -l 1024 -m head -H "X-foo1: bar1\x0D\x0A" -H "X-foo2: bar2\x0D\x0A"

----

Running stored commands after startup
=====================================

MoSh command: ``startup_cmd``

You can store up to three MoSh commands to run on start/bootup.
By default, commands are run after the default PDN context is activated,
but can be set to run N seconds after bootup.

Examples
--------

* Starting periodic location acquiring after LTE has been connected with both cellular and GNSS, including sending the location to nRF Cloud:

  .. code-block:: console

     startup_cmd --mem1 "location get --mode all --method cellular --method gnss --gnss_cloud_pvt --interval 15"

----

.. _pipelining_commands:

Running commands in different threads and pipelining commands
=============================================================

MoSh command: ``th``

You can run ``iperf3`` and ``ping`` in separate threads either in the background or in the foreground.
Subcommand ``pipeline`` allows running any commands sequentially in the foreground.

Examples
--------

* Start iperf test in the background.
  Meanwhile, start ping in the foreground.
  Print the output buffer of iperf thread once done:

  .. code-block:: console

     th startbg iperf3 --client 111.222.111.222 --port 10000 -l 3540 --time 30 -V -R
     th startfg ping -d 8.8.8.8
     th results 1

* Establish MQTT connection to nRF Cloud, wait 10 seconds for the connection establishment, and request current location:

  .. code-block:: console

     th pipeline "cloud connect" "sleep 10" "location get"

----

Sleep
=====

MoSh command: ``sleep``

When pipelining commands using ``th pipeline``, you can use the ``sleep`` command to pause the execution for a given period to allow previous command to return before executing next one.
See :ref:`pipelining_commands` for usage.


Cloud
=====

MoSh command: ``cloud``

nRF Cloud is a platform for providing, among other things, various location services.
Modem Shell enables you to establish an MQTT connection to nRF Cloud using the :ref:`lib_nrf_cloud` library.
Currently, the ``cloud`` command is useful mostly when using the location services and MQTT is the desired transport protocol.
However, you can use any nRF Cloud services once the MQTT connection is established.

Examples
--------

* Establish the connection to nRF Cloud, request the cell-based location of the device, and disconnect when ready:

  .. code-block:: console

     cloud connect
     location get --method cellular
     cloud disconnect

----

Remote control using nRF Cloud
==============================

Once you have established an MQTT connection to nRF Cloud using the ``cloud`` command, you can use the **Terminal** window in the nRF Cloud portal to execute MoSh commands to the device.
This feature enables full remote control of the MoSh application running on a device that is connected to cloud.
MoSh output, such as responses to commands and other notifications can be echoed to the ``messages`` endpoint and the **Terminal** window of the nRF Cloud portal.
Use the ``print cloud`` command to enable this behavior.
The data format of the input data in the **Terminal** window must be JSON.

Examples
--------

* Establish the connection to nRF Cloud:

  .. code-block:: console

     cloud connect

* To request the device location, enter the following command in the **Terminal** window of the nRF Cloud portal:

   .. code-block:: console

     {"appId":"MODEM_SHELL", "data":"location get --method cellular"}

  The device location appears in the **Location** window.

* An AT command is sent to the modem:

   .. code-block:: console

     {"appId":"MODEM_SHELL", "data":"at AT+COPS=1,2,\\\"24412\\\""}

  Note the syntax for escaping the quotation marks.

----

.. _uart_command:

UART
====

MoSh command: ``uart``

Disable UARTs for power measurement purposes or change shell UART baudrate.

* Disable UARTs for 30 seconds:

  .. code-block:: console

     uart disable 30

* Disable UARTs whenever modem is in sleep state:

  .. code-block:: console

     uart during_sleep disable

* Change shell UART baudrate to 921600:

  .. code-block:: console

     uart baudrate 921600

----

Heap usage statistics
=====================

MoSh command: ``heap``

You can use the command to print kernel and system heap usage statistics.

  .. code-block:: console

     mosh:~$ heap
     kernel heap statistics:
     free:             7804
     allocated:         272
     max. allocated:   1056

     system heap statistics:
     max. size:       81400
     size:              248
     free:              160
     allocated:          88

----

GPIO pin pulse counter
======================

MoSh command: ``gpio_count``

You can use the command to count pulses on a given GPIO pin.
A rising edge of the signal is counted as a pulse.
Pulse counting can be enabled only for a single pin at a time.
When pulse counting is enabled, **LED 2** on the nRF91 Series DKs shows the state of the pin input.

.. note::

   The ``gpio_count enable`` command configures the GPIO pin as input and enables pull down.

.. code-block:: console

   mosh:~$ gpio_count get
   Number of pulses: 0
   mosh:~$ gpio_count enable 10
   mosh:~$ gpio_count get
   Number of pulses: 42
   mosh:~$ gpio_count disable
   mosh:~$ gpio_count get
   Number of pulses: 42

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. _CONFIG_MOSH_LINK:

CONFIG_MOSH_LINK
   Enable LTE link control feature in modem shell.

.. _CONFIG_MOSH_PING:

CONFIG_MOSH_PING
   Enable ping feature in modem shell.

.. _CONFIG_MOSH_IPERF3:

CONFIG_MOSH_IPERF3
   Enable iperf3 feature in modem shell.

.. _CONFIG_MOSH_CURL:

CONFIG_MOSH_CURL
   Enable curl feature in modem shell.

.. _CONFIG_MOSH_SOCK:

CONFIG_MOSH_SOCK
    Enable socket tool feature in modem shell.

.. _CONFIG_MOSH_SMS:

CONFIG_MOSH_SMS
    Enable SMS feature in modem shell

.. _CONFIG_MOSH_LOCATION:

CONFIG_MOSH_LOCATION
   Enable Location tool in modem shell.

.. _CONFIG_MOSH_GNSS:

CONFIG_MOSH_GNSS
   Enable GNSS feature in modem shell

.. _CONFIG_MOSH_FOTA:

CONFIG_MOSH_FOTA
   Enable FOTA feature in modem shell

.. _CONFIG_MOSH_PPP:

CONFIG_MOSH_PPP
   Enable PPP feature in modem shell

.. _CONFIG_MOSH_REST:

CONFIG_MOSH_REST
   Enable REST client feature in modem shell.

.. _CONFIG_MOSH_CLOUD_REST:

CONFIG_MOSH_CLOUD_REST
   Enable nRF Cloud REST feature in modem shell.

.. _CONFIG_MOSH_CLOUD_MQTT:

CONFIG_MOSH_CLOUD_MQTT
   Enable nRF Cloud MQTT connection feature in modem shell.

.. _CONFIG_MOSH_AT_CMD_MODE:

CONFIG_MOSH_AT_CMD_MODE
   Enable AT command mode feature in modem shell.

.. _CONFIG_MOSH_GPIO_COUNT:

CONFIG_MOSH_GPIO_COUNT
   Enable GPIO pin pulse counter feature in modem shell.

.. note::
   You may not be able to use all features at the same time due to memory restrictions.
   To see which features are enabled simultaneously, check the configuration files and overlays.

Additional configuration
========================

Check and configure the following library option that is used by the sample:

* :kconfig:option:`CONFIG_MODEM_ANTENNA_GNSS_EXTERNAL` - Selects an external GNSS antenna.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/modem_shell`

.. include:: /includes/build_and_run_ns.txt

See :ref:`cmake_options` for instructions on how to provide CMake options, for example to use a configuration overlay.

.. _dk_buttons:

DK buttons
==========

The buttons have the following functions:

Button 1:
   Raises a kill or abort signal. A long press of the button will kill or abort all supported running commands. You can abort commands ``iperf3`` (also with ``th``), ``curl``, ``ping`` and ``sock send``.

Button 2:
   Enables or disables the UARTs for power consumption measurements. Toggles between UARTs enabled and disabled.

LED indications
===============

The LEDs have the following functions:

nRF91 Series DKs:

* **LED 2** indicates the state of the GPIO pin when pulse counting has been enabled using the ``gpio_count enable`` command.
* **LED 3** indicates the LTE registration status.
* **LED 4** is lit for five seconds when the current location has been successfully retrieved by using the ``location get`` command.

Thingy:91 and Thingy:91 X RGB LED:

* LTE connected:

  * Default state constant blue.
  * Lit purple for five seconds when the current location has been successfully retrieved by using the ``location get`` command.

* LTE disconnected:

  * Default state OFF.
  * Lit red for five seconds when the current location has been successfully retrieved by using the ``location get`` command.

Power measurements
==================

You can perform power measurements using the `Power Profiler Kit II (PPK2)`_.
See the documentation for instructions on how to setup the DK for power measurements.
The documentation shows, for example, how to connect the wires for both source meter and ampere meter modes.
The same instructions are valid also when using a different meter.

To achieve satisfactory power measurement results, it is often desirable to disable UART interfaces unless their contribution to overall power consumption is of interest.
In MoSh, perform one of the following actions:

  * Use MoSh command ``uart`` to disable UARTs as in :ref:`uart_command`
  * Press **Button 2** in DK to enable or disable UARTs as instructed in :ref:`dk_buttons`

For more information about application power optimizations, refer to :ref:`app_power_opt`.

Testing
=======

After programming the application and all prerequisites to your development kit, test it by performing the following steps:

1. |connect_kit|
#. |connect_terminal_ANSI|
#. Reset the development kit.
#. Observe in the terminal window that the application starts.
   This is indicated by output similar to the following (there is also a lot of additional information about the LTE connection):

   .. code-block:: console

      *** Booting Zephyr OS build v2.4.99-ncs1-3525-g4d068de3f50f  ***

      MOSH version:       v1.5.0-649-g7e657c2fab02
      MOSH build id:      152
      MOSH build variant: normal

      Initializing modemlib...

      Initialized modemlib
      Network registration status: searching
      Network registration status: Connected - home network
      mosh:~$

#. Type any of the commands listed in the Overview section to the terminal. When you type only the command, the terminal shows the usage, for example ``sock``.

Getting nRF91 Series DK out-of-the-box and to nRF Cloud
=======================================================

To program the certificates and connect to nRF Cloud, complete the following steps:

1. `Download nRF Connect for Desktop`_.
#. Update the modem firmware on the on-board modem of the nRF91 Series DK to the latest version as instructed in :ref:`nrf9160_updating_fw_modem`.
#. Build and program the MoSh to the nRF91 Series DK using the default MoSh configuration (with REST as the transport):

.. parsed-literal::
   :class: highlight

   west build -p -b *board_target*
   west flash

|board_target|

#. Get certificates from nRF Cloud as explained in the documentation for the :ref:`nRF91x1 DK <downloading_cloud_certificate_nRF91x1>` or the :ref:`nRF9160 DK <downloading_cloud_certificate_nRF9160>`, depending on the DK you are using.
#. In the MoSH terminal, power off the modem and start the AT command mode:

   .. code-block:: console

      mosh:~$ link funmode -0
      mosh:~$ at at_cmd_mode start

#. Disconnect the MoSh terminal.
#. Connect and use `Cellular Monitor`_  to store the certificates to the modem (default nRF Cloud security tag).

   See `Managing credentials`_ in the Cellular Monitor user guide for instructions.
#. Reconnect the MoSh terminal and press ``ctrl-x`` and ``ctrl-q`` to exit the AT command mode.
#. Set the modem to normal mode to activate LTE:

   .. code-block:: console

      mosh:~$ link funmode -1

   Observe that LTE is getting connected.
#. Perform just-in-time provisioning (JITP) with nRF Cloud through REST:

   .. code-block:: console

      mosh:~$ cloud_rest jitp

   You only need to do this once for each device.

#. Follow the instructions for JITP printed in the MoSh terminal.
#. Complete the user association:

   1. Open the `nRF Cloud`_ portal.
   #. Click the large plus sign in the upper left corner.
   #. Enter the device ID from MoSh in the **Device ID** field.

   When the device has been added, the message **Device added to account. Waiting for it to connect...** appears.
   When the message disappears, click :guilabel:`Devices` on the left side menu.
   Your MoSh device is now visible in the list.
#. Send MoSh device information to nRF Cloud:

   .. code-block:: console

      mosh:~$ cloud_rest shadow_update

   It might take a while for the data to appear in the nRF Cloud UI.
#. Use the ``location`` command to verify that the REST transport to nRF Cloud is working.

   .. code-block:: console

      mosh:~$ location get --method cellular

#. As a success response, the location is printed in the MoSh terminal.
#. Open the entry for your device in the **Devices** view.
#. Observe that location and device information are shown in the device page.

nRF91 Series DK with nRF7002 EK Wi-Fi support
=============================================

To build the MoSh sample for an nRF91 Series DK with nRF7002 EK Wi-Fi support, use the ``-DSHIELD=nrf7002ek``, ``-DEXTRA_CONF_FILE=overlay-nrf700x-wifi-scan-only.conf``, ``-DSB_CONFIG_WIFI_NRF70=y`` and ``-DSB_CONFIG_WIFI_NRF70_SCAN_ONLY=y`` options.
For example:

.. parsed-literal::
   :class: highlight

   west build -p -b *board_target* -- -DSHIELD=nrf7002ek -DEXTRA_CONF_FILE=overlay-nrf700x-wifi-scan-only.conf -DSB_CONFIG_WIFI_NRF70=y -DSB_CONFIG_WIFI_NRF70_SCAN_ONLY=y

|board_target|

See :ref:`cmake_options` for more instructions on how to add these options.

Thingy:91 X Wi-Fi support
=========================

To build the MoSh sample with Thingy:91 X Wi-Fi support, use the ``-DDTC_OVERLAY_FILE=thingy91x_wifi.overlay``, ``-DEXTRA_CONF_FILE=overlay-nrf700x-wifi-scan-only.conf``, ``-DSB_CONFIG_WIFI_NRF70=y``, and ``-DSB_CONFIG_WIFI_NRF70_SCAN_ONLY=y`` options.
For example:

.. code-block:: console

   west build -p -b thingy91x/nrf9151/ns -- -DDTC_OVERLAY_FILE=thingy91x_wifi.overlay -DEXTRA_CONF_FILE=overlay-nrf700x-wifi-scan-only.conf -DSB_CONFIG_WIFI_NRF70=y -DSB_CONFIG_WIFI_NRF70_SCAN_ONLY=y

See :ref:`cmake_options` for more instructions on how to add these options.

PPP support
===========

To build the MoSh sample with PPP/dial up support, use the ``-DDTC_OVERLAY_FILE=ppp.overlay`` and ``-DEXTRA_CONF_FILE=overlay-ppp.conf`` options.
For example:

.. parsed-literal::
   :class: highlight

   west build -p -b *board_target* -- -DDTC_OVERLAY_FILE=ppp.overlay -DEXTRA_CONF_FILE=overlay-ppp.conf

|board_target|

After programming the development kit, test it in the Linux environment by performing the following steps:

1. Connect the development kit to the computer using a USB cable.
   The development kit is assigned a ttyACM device (Linux).

#. Open a serial connection to the development kit (/dev/ttyACM2) with a terminal |ANSI| (for example, nRF Connect Serial Terminal).
   See :ref:`test_and_optimize` for the required settings and steps.

#. Reset the development kit.

#. Observe in the terminal window that the MoSh starts with the PPP support.
   This is indicated by output similar to the following (there is also a lot of additional information about the LTE connection):

   .. code-block:: console

      Network registration status: searching
      Network registration status: Connected - home network
      Default PDN is active: starting PPP automatically
      PPP: started
      mosh:~$

   Higher baudrates than the default 115200 result in better performance with the usual use cases for PPP/dial up.
   Set the nRF91 Series DK side UART for PPP with a MoSh command, for example ``ppp uartconf -b 921600``.
   You also need to set the corresponding UART accordingly from PC side (in this example, within the ``pppd`` command).

#. Enter command ``ppp uartconf`` that results in the following UART configuration:

   .. code-block:: console

      mosh:~$ ppp uartconf -r
      PPP uart configuration:
        baudrate:     921600
        flow control: RTS_CTS
        parity:       none
        data bits:    bits8
        stop bits:    bits1
      mosh:~$

#. In a Linux terminal, enter the following command to start the PPP connection:

   .. code-block:: console

      $ sudo pppd -detach /dev/ttyACM0 921600 noauth crtscts noccp novj nodeflate nobsdcomp local debug +ipv6 ipv6cp-use-ipaddr usepeerdns noipdefault defaultroute ipv6cp-restart 5 ipcp-restart 5 lcp-echo-interval 0

#. In a MoSh terminal, observe that the PPP connection is created:

   .. code-block:: console

      Dial up (IPv6) connection up
      Dial up (IPv4) connection up
      mosh:~$

#. Now, you are ready to browse Internet in Linux by using the MoSh PPP dial-up over LTE connection.

Application FOTA support
========================

To build the MoSh sample with application FOTA support, use the ``-DEXTRA_CONF_FILE=overlay-app_fota.conf`` and ``-DSB_CONFIG_BOOTLOADER_MCUBOOT=y`` options.
For example:

.. parsed-literal::
   :class: highlight

   west build -p -b *board_target* -- -DEXTRA_CONF_FILE=overlay-app_fota.conf -DSB_CONFIG_BOOTLOADER_MCUBOOT=y

|board_target|

nRF91 Series DK with full modem FOTA support
============================================

To build the MoSh sample for an nRF91 Series DK with full modem FOTA support, use the devicetree overlay for external flash corresponding to your device and the ``-DEXTRA_CONF_FILE=overlay-modem_fota_full.conf`` option.
The following is an example for the nRF9161 DK:

.. code-block:: console

   west build -p -b nrf9161dk/nrf9161/ns -- -DEXTRA_CONF_FILE=overlay-modem_fota_full.conf -DDTC_OVERLAY_FILE=nrf9161dk_ext_flash.overlay

LwM2M carrier library support
=============================

To build the MoSh sample with LwM2M carrier library support, use the ``-DEXTRA_CONF_FILE=overlay-carrier.conf`` option.
For example:

.. parsed-literal::
   :class: highlight

   west build -p -b *board_target* -- -DEXTRA_CONF_FILE=overlay-carrier.conf

|board_target|

P-GPS support
=============

To build the MoSh sample with P-GPS support, use the ``-DEXTRA_CONF_FILE=overlay-pgps.conf`` option.
For example:

.. parsed-literal::
   :class: highlight

   west build -p -b *board_target* -- -DEXTRA_CONF_FILE=overlay-pgps.conf

|board_target|

.. _cloud_build:

Cloud over MQTT
===============

To build the MoSh sample with cloud connectivity over MQTT, use the ``-DEXTRA_CONF_FILE=overlay-cloud_mqtt.conf`` option.
For example:

.. parsed-literal::
   :class: highlight

   west build -p -b *board_target* -- -DEXTRA_CONF_FILE=overlay-cloud_mqtt.conf

|board_target|

Cloud over CoAP
===============

To build the MoSh sample with cloud connectivity over CoAP, use the ``-DEXTRA_CONF_FILE=overlay-cloud_coap.conf`` option.
For example:

.. parsed-literal::
   :class: highlight

   west build -p -b *board_target* -- -DEXTRA_CONF_FILE=overlay-cloud_coap.conf

|board_target|

Location service handled in application
=======================================

This sample is using cloud service for positioning through the :ref:`lib_location` library by default.
To build the sample with location cloud services handled in the MoSh,
use the ``-DEXTRA_CONF_FILE="overlay-cloud_mqtt.conf"`` and ``-DCONFIG_LOCATION_SERVICE_EXTERNAL=y`` options.
For example:

.. parsed-literal::
   :class: highlight

   west build -p -b *board_target* -- -DEXTRA_CONF_FILE=overlay-cloud_mqtt.conf -DCONFIG_LOCATION_SERVICE_EXTERNAL=y

|board_target|

To add P-GPS on top of that, use the ``-DEXTRA_CONF_FILE="overlay-cloud_mqtt.conf;overlay-pgps.conf"``, ``-DCONFIG_LOCATION_SERVICE_EXTERNAL=y`` and ``-DCONFIG_NRF_CLOUD_PGPS_TRANSPORT_NONE=y`` options.
For example:

.. parsed-literal::
   :class: highlight

   west build -p -b *board_target* -- -DEXTRA_CONF_FILE="overlay-cloud_mqtt.conf;overlay-pgps.conf" -DCONFIG_LOCATION_SERVICE_EXTERNAL=y -DCONFIG_NRF_CLOUD_PGPS_TRANSPORT_NONE=y

|board_target|

Remote control using nRF Cloud over MQTT
========================================

To enable the remote control feature, you need to build the sample with cloud connectivity, see :ref:`cloud_build`.

nRF91 Series DK with Zephyr native TCP/IP stack
===============================================

To build the MoSh sample for an nRF91 Series DK with the nRF91 device driver that does not offload the TCP/IP stack to modem, use the ``-DEXTRA_CONF_FILE=overlay-non-offloading.conf`` option.
With this configuration, the configured MoSh commands, for example ``iperf3``, use the Zephyr native TCP/IP stack over the default LTE PDN context.
For example:

.. parsed-literal::
   :class: highlight

   west build -p -b *board_target* -- -DEXTRA_CONF_FILE=overlay-non-offloading.conf

|board_target|

BT shell support
================

To build the MoSh sample with Zephyr BT shell command support, use the :file:`-DDTC_OVERLAY_FILE=bt.overlay` and :file:`-DEXTRA_CONF_FILE=overlay-bt.conf` options.
When running this configuration, you can perform BT scanning and advertising using the ``bt`` command.

Compile as follows:

.. code-block:: console

   west build -p -b nrf9160dk/nrf9160/ns -- -DDTC_OVERLAY_FILE="bt.overlay" -DEXTRA_CONF_FILE="overlay-bt.conf"

Additionally, you need to program the nRF52840 side of the nRF9160 DK as instructed in :ref:`lte_sensor_gateway`.

Compile the :ref:`bluetooth-hci-lpuart-sample` sample as follows:

.. code-block:: console

   west build -p -b nrf9160dk/nrf52840

The following example demonstrates how to use MoSh with two development kits, where one acts as a broadcaster and the other one as an observer.

DK #1, where MoSh is used in broadcaster (advertising) role:

   .. code-block:: console

      mosh:~$ bt init
      Bluetooth initialized
      Settings Loaded
      mosh:~$ bt name mosh-adv
      mosh:~$ bt name
      Bluetooth Local Name: mosh-adv
      mosh:~$ bt advertise scan
      Advertising started

      /* And when done: */
      mosh:~$ bt advertise off
      Advertising stopped
      mosh:~$

DK #2, where MoSh is used in observer (scanning) role:

   .. code-block:: console

      mosh:~$ bt init
      Bluetooth initialized
      Settings Loaded
      mosh:~$ bt name mosh-scanner
      mosh:~$ bt name
      Bluetooth Local Name: mosh-scanner
      mosh:~$ bt scan-filter-set name mosh-adv
      mosh:~$ bt scan on
      Bluetooth active scan enabled
      [DEVICE]: 11:22:33:44:55:66(random), AD evt type 4, RSSI -42 mosh-adv C:0 S:1 D:0 SR:1 E:0 Prim: LE 1M, Secn: No packets, Interval: 0x0000 (0 ms), SID: 0xff
      ...

      /* And when done: */
      mosh:~$ bt scan off
      Scan successfully stopped
      mosh:~$

.. note::
   The MoSh sample with Zephyr BT shell command is not supported by the nRF91x1 DK.

SEGGER RTT support
==================

To build the MoSh sample with SEGGER's Real Time Transfer (RTT) support, use the ``-DEXTRA_CONF_FILE=overlay-rtt.conf`` option.
When running this configuration, RTT is used as the shell backend instead of UART.
For example:

.. parsed-literal::
   :class: highlight

   west build -p -b *board_target* -- -DEXTRA_CONF_FILE=overlay-rtt.conf

|board_target|

LwM2M support
=============

Before building and running the sample, select the LwM2M Server for testing.
Follow the instructions in :ref:`server_setup_lwm2m_client` to set up the server and register your device to the server.
With the default LwM2M configuration, the device connects directly to the device management server without bootstrap support.
You can change the LwM2M Server address by setting the :kconfig:option:`CONFIG_LWM2M_CLIENT_UTILS_SERVER` Kconfig option.

Location assistance uses a proprietary mechanism to fetch location assistance data from nRF Cloud by proxying it through the LwM2M Server.
As of now, you can only use AVSystem's Coiote LwM2M Server for the location assistance data from nRF Cloud.
To know more about the AVSystem integration with |NCS|, see :ref:`ug_avsystem`.

You can build the MoSh sample with different LwM2M configurations:

  * To build the MoSh sample with the default LwM2M configuration, use the ``-DEXTRA_CONF_FILE=overlay-lwm2m.conf`` option and set the used Pre-Shared-Key (PSK) using :kconfig:option:`CONFIG_MOSH_LWM2M_PSK` Kconfig option.
  * To enable bootstrapping, use the optional overlay file :file:`overlay-lwm2m_bootstrap.conf`.
  * To enable P-GPS support, use the optional overlay files :file:`overlay-lwm2m_pgps.conf` and :file:`overlay-pgps.conf`.

To build the sample with LwM2M support, use the following command:

.. parsed-literal::
   :class: highlight

   west build -p -b *board_target* -- -DEXTRA_CONF_FILE=overlay-lwm2m.conf -DCONFIG_MOSH_LWM2M_PSK=\"000102030405060708090a0b0c0d0e0f\"


To also enable P-GPS, use the following command:

.. parsed-literal::
   :class: highlight

   west build -p -b *board_target* -- -DEXTRA_CONF_FILE="overlay-lwm2m.conf;overlay-lwm2m_pgps.conf;overlay-pgps.conf" -DCONFIG_MOSH_LWM2M_PSK=\"000102030405060708090a0b0c0d0e0f\"

|board_target|

Use the following command to establish connection to the LwM2M Server:

.. code-block:: console

   mosh:~$ cloud_lwm2m connect
   LwM2M: Starting LwM2M client
   LwM2M: Registration complete

Use the following command to disconnect from the LwM2M Server:

.. code-block:: console

   mosh:~$ cloud_lwm2m disconnect
   LwM2M: Stopping LwM2M client
   LwM2M: Disconnected

When connected, the ``location`` and ``gnss`` commands use the LwM2M cloud connection for fetching GNSS assistance data and for cellular positioning.

.. _modem_shell_trace_flash_support:

nRF91 Series DK with modem trace flash backend support
======================================================

To build the MoSh sample for an nRF91 Series DK with modem trace flash backend support, use the snippet ``nrf91-modem-trace-ext-flash``.
For example:

.. parsed-literal::
   :class: highlight

   west build -p -b *board_target* -- -Dmodem_shell_SNIPPET="nrf91-modem-trace-ext-flash"

|board_target|

References
**********

* `nRF9160 Hardware Verification Guidelines - UART interface`_

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`at_parser_readme`
* :ref:`lib_date_time`
* :ref:`lib_location`
* :ref:`lte_lc_readme`
* :ref:`lib_lwm2m_client_utils`
* :ref:`lib_lwm2m_location_assistance`
* :ref:`modem_info_readme`
* :ref:`lib_modem_jwt`
* :ref:`lib_nrf_cloud`
* :ref:`lib_rest_client`
* :ref:`sms_readme`

This sample uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
