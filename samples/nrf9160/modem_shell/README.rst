.. _modem_shell_application:
.. highlight:: none

nRF9160: Modem Shell
####################

The Modem Shell (MoSh) sample application enables you to test various device connectivity features, including data throughput.

Overview
********

MoSh enables testing of different connectivity features such as
LTE link handling, TCP/IP connections, data throughput (iperf3 and curl), SMS, GNSS, FOTA updates and PPP.
Hence, this sample is not only code sample, which it also is in many aspects, but also a test application for aforementioned features.
MoSh uses the LTE link control driver to establish an LTE connection and
initializes the Zephyr shell to provide a shell command-line interface for users.

The subsections list the MoSh features and show shell command examples on their usage.

LTE link control
================

MoSh command: "link"

LTE link control changes and queries the state of the LTE connection.
Many of the changes are applied when going to online mode for the next time.
You can store some link subcommand parameters into settings, which are persistent between sessions.

Examples
--------

Change the system mode to NB-IoT and query the connection status::

   link funmode -4
   link sysmode -n
   link funmode -1
   link status

Open another PDP context and close it::

   link connect -a internet.operator.com
   link status
   link disconnect -I 1

Enable and configure eDRX::

   link edrx -e -m --edrx_value 0010 --ptw 0010

Subscribe for modem TAU and sleep notifications, enable and configure PSM::

   link tau --subscribe
   link msleep --subscribe
   link psm -e --rptau 01100100 --rat 00100010

Disable autoconnect when the DK starts
(modem will remain in pwroff functional mode and the link is not connected)::

   link nmodeauto --disable

Set the custom AT commands that are run when switching to normal mode
(locking device to band #3)::

   link funmode --poweroff
   link nmodeat --mem1 "at%xbandlock=2,\"100\""
   link funmode --normal

AT commands
===========

MoSh command: "at"

You can use the AT command module to send AT commands to the modem.

Examples
--------

Send AT command to query network status with::

   at at+cereg?

Send AT command to query neightbour cells::

   at at%NBRGRSRP

Enable AT command events::

   at events_enable

Disable AT command events::

   at events_disable

Ping
====

MoSh command: "ping"

Ping is a tool for testing the reachability of a host on an IP network.

Examples
--------

Ping a URL::

   ping -d ping.server.url

Ping an IPv6 address with length of 500 bytes and 1 s intervals (note: used PDN needs to support IPv6)::

   ping -d 1a2b:1a2b:1a2b:1a2b::1 -6 -l 500 -i 1000

Iperf3
======

MoSh command: "iperf3"

Iperf3 is a tool for measuring data transfer performance both in uplink and downlink direction.

.. note::
   Some features, for example file operations and TCP option tuning, are not supported.

Examples
--------

Download data over TCP for 30 seconds with a buffer size of 3540 bytes and use detailed output::

   iperf3 --client 111.222.111.222 --port 10000 -l 3540 --time 30 -V -R

Upload data over TCP for 30 seconds with the payload size of 708 bytes using a PDN with ID #2
(use the ``link status`` command to see the active PDNs and their IDs)::

   iperf3 -c 111.222.111.222 -p 10000 -l 708 -t 30 --pdn_id 2

Upload data over UDP for 60 seconds with the payload size of 1240 bytes and use the detailed output as well as debug output::

   iperf3 --client 111.222.111.222 --port 10000 -l 1240 --time 60 -u -V -d

Download data over TCP/IPv6 for 10 seconds
(note: used PDN need to support IPv6, se ``link status`` command to see PDP type support for active contexts)::

   iperf3 --client 1a2b:1a2b:1a2b:1a2b::1 --port 20000 --time 10 -R -6

Curl
====

MoSh command: "curl"

Curl is a command-line tool for transferring data specified with URL syntax.
It is a part of MoSh and enables you to test the data download with a "standard" tool.

.. note::
   File operations are not supported.

Examples
--------

HTTP download::

   curl http://curl.server.url/small.txt
   curl http://curl.server.url/bigger_file.zip --output /dev/null

HTTP upload for given data::

   curl http://curl.server.url/data -d "foo=bar"

HTTP upload for given number of bytes sent in a POST body::

   curl http://curl.server.url/data -d #500000

HTTP upload for given number of bytes sent in a POST body using a PDN with ID #1
(use the ``link status`` command to see active PDNs and their IDs)::

   curl http://curl.server.url/data -d #500000 --pdn_id 1

Socket tool
===========

MoSh command: "sock"

You can use the socket tool to:

* Create and manage socket connections.
* Send and receive data.

Examples
--------

Open and connect to an IP address and port (IPv4 TCP socket)::

   sock connect -a 111.222.111.222 -p 20000

Open and connect to hostname and port (IPv4 TCP socket)::

   sock connect -a google.com -p 20000

Open and connect an IPv6 TCP socket and bind to a port::

   sock connect -a 1a2b:1a2b:1a2b:1a2b::1 -p 20000 -f inet6 -t stream -b 40000

Open an IPv6 UDP socket::

   sock connect -a 1a2b:1a2b:1a2b:1a2b::1 -p 20000 -f inet6 -t dgram

Open a raw socket::

   sock connect -f packet -t raw

Open a socket to a non-default PDP context::

   link connect -a nondefault.context.com
   sock connect -f packet -t raw -I 1

Send a string through the socket::

   sock send -i 0 -d testing

Send 100 kB of data and show throughput statistics::

   sock send -i 0 -l 100000

Send data periodically with 10 s interval::

   sock send -i 0 -e 10 -d test_periodic

Calculate the receive throughput::

   <do whatever is needed to make device receive data after some time>
   sock recv -i 0 -r -l 1000000
   sock recv -i 0
   sock recv -i 0

Close a socket::

   sock close -i 0

Use RAI settings::

   link funmode -4
   sock rai --rai_enable
   link funmode -1
   sock connect -a 111.222.111.222 -p 20000
   sock rai -i 0 --rai_last
   sock send -i 0 -d testing

List open sockets::

   sock list

SMS tool
========

MoSh command: "sms"

You can use the SMS tool for sending and receiving SMS messages.

Examples
--------

Register the SMS service so that messages can be received if SIM subscription supports it::

   sms reg

Send an SMS message (registration is done automatically if not already done)::

   sms send -n +987654321 -m testing

GNSS
====

MoSh command: "gnss"

GNSS provides commands for searching the location of the device.

Examples
--------

Start GNSS tracking and stop it::

   gnss start
   gnss stop

Disable LTE, enable all GNSS output and start continuous tracking with power saving enabled::

   link funmode --lteoff
   gnss output 2 1 1
   gnss mode cont
   gnss config powersave perf
   gnss start

Enable LTE PSM, only NMEA output, automatic A-GPS data fetching and start periodic fixes with 5 minute interval and 120 second timeout::

   link psm -e
   gnss output 0 1 0
   gnss agps automatic enable
   gnss mode periodic 300 120
   gnss start

Enable Low Noise Amplifier (LNA) for use with the on-board GPS antenna::

   gnss lna enable

FOTA
====

MoSh command: "fota"

You can use FOTA to perform software updates over-the-air for both modem and application side.
However, to use this feature, you need to know which updates are available in the servers.
This feature is intended to be used only by selected users and customers to whom available image names are communicated separately.

Examples
--------

Perform a FOTA update::

   fota download eu fw_update_package_filename.hex

PPP
===

MoSh command: "ppp"

You can use the PPP (Point-to-Point Protocol) to enable dial-up access to the Internet.
The MoSh command is simple but you need to have a normal dial-up setup in your PC to be able to use the development kit's PPP interface.

.. note::

   Dial-up connection is non functional when using Segger virtual UART ports.
   PPP has been used successfully with FTDI UART port though.
   Refer to `nRF9160 Hardware Verification Guidelines - UART interface`_.

Examples
--------

Set the PPP network interface up::

   ppp up

Set the PPP network interface down::

   ppp down

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. option:: CONFIG_MOSH_LINK - Enable LTE link control feature in modem shell.
.. option:: CONFIG_MOSH_PING - Enable ping feature in modem shell.
.. option:: CONFIG_MOSH_IPERF3 - Enable iperf3 feature in modem shell.
.. option:: CONFIG_MOSH_CURL - Enable curl feature in modem shell.
.. option:: CONFIG_MOSH_SOCK - Enable socket tool feature in modem shell.
.. option:: CONFIG_MOSH_SMS - Enable SMS feature in modem shell.
.. option:: CONFIG_MOSH_GNSS - Enable GNSS feature in modem shell.
.. option:: CONFIG_MOSH_FOTA - Enable FOTA feature in modem shell.
.. option:: CONFIG_MOSH_PPP - Enable PPP feature in modem shell.

.. note:
   You may not be able to use all features at the same time due to memory restrictions.
   To see which features are enabled simultaneously, check the configuration files and overlays.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/modem_shell`

.. include:: /includes/build_and_run_nrf9160.txt

Testing
=======

To test MoSh after programming the application and all prerequisites to your development kit, complete the following steps:

1. Connect the development kit to the computer using a USB cable.
   The development kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.

#. Create a serial connection to the development kit (J-Link COM port) with a terminal using the following settings:

   * Hardware flow control: disabled
   * Baud rate: 115200
   * Parity bit: no

#. Reset the development kit.

#. Observe in the terminal window that the application starts.
   This is indicated by output similar to the following (there is also a lot of additional information about the LTE connection)::

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

PPP support
============

To build the MoSh sample with PPP/dial up support for Windows, use the ``-DOVERLAY_CONFIG=overlay-ppp.conf`` option.
For example:

``west build -p -b nrf9160dk_nrf9160ns -d build -- -DOVERLAY_CONFIG=overlay-ppp.conf``

See :ref:`cmake_options` for more instructions on how to add this option.

Application FOTA support
========================

To build the MoSh sample with application FOTA support, use the ``-DOVERLAY_CONFIG=overlay-app_fota.conf`` option.
For example:

``west build -p -b nrf9160dk_nrf9160ns -d build -- -DOVERLAY_CONFIG=overlay-app_fota.conf``

See :ref:`cmake_options` for more instructions on how to add this option.

LwM2M carrier library support
=============================

To build the MoSh sample with LwM2M carrier library support, use the ``-DOVERLAY_CONFIG=overlay-lwm2m_carrier.conf`` option.
For example:

``west build -p -b nrf9160dk_nrf9160ns -d build -- -DOVERLAY_CONFIG=overlay-lwm2m_carrier.conf``

See :ref:`cmake_options` for more instructions on how to add this option.

Dependencies
************

This sample uses the following libraries:

From |NCS|
  * :ref:`lte_lc_readme`
  * :ref:`modem_info_readme`
  * :ref:`at_cmd_readme`
  * :ref:`at_cmd_parser_readme`
  * :ref:`at_notif_readme`
  * :ref:`sms_readme`

From nrfxlib
  * :ref:`nrfxlib:nrf_modem`

References
**********

`nRF9160 Hardware Verification Guidelines - UART interface`_
