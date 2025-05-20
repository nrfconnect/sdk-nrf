.. _slm_as_linux_modem:

nRF91 Series SiP as a modem for Linux device
############################################

.. contents::
   :local:
   :depth: 2

Overview
********

You can use the Serial LTE Modem (SLM) application to make an nRF91 Series SiP work as a standalone modem that can be used with a Linux device.
The Linux device can use a standard PPP daemon and ldattach utility to connect to the cellular network through the nRF91 Series SiP.

The setup differentiates from a typical dial-up modem connection as the GSM 0710 multiplexer protocol (CMUX) is used to multiplex multiple data streams over a single serial port.
This allows you to use the same serial port for both AT commands and PPP data.

Prerequisites
=============

The Linux device needs to have the following packages installed:

* ``pppd`` from the ppp package
* ``ldattach`` from the util-linux package

These should be available on all standard Linux distributions.

Configuration
=============

To build the SLM application, use the :file:`overlay-ppp-cmux-linux.conf` configuration overlay.

You can adjust the serial port baud rate using the devicetree overlay file.
By default, the baud rate is set to 115200.
If you change the baud rate, set the same rate in the :file:`scripts/slm_start_ppp.sh` and :file:`scripts/slm_stop_ppp.sh` scripts.

.. note::
   The standard ``ldattach`` utility sets MRU and MTU to 127 bytes.
   This is hard-coded and cannot be changed.
   If you change the SLM configuration, make sure that the :kconfig:option:`CONFIG_MODEM_CMUX_MTU` Kconfig option is set to 127 bytes.
   This is already configured in the :file:`overlay-ppp-cmux-linux.conf` configuration overlay.

Building and running
====================

To build and program the SLM application to the nRF91 Series device, use the :file:`overlay-ppp-cmux-linux.conf` overlay file.

Managing the connection
***********************

The start and stop scripts are provided in the :file:`scripts` directory of the SLM application.
The scripts assume that the nRF91 Series SiP is connected to the Linux device using the `/dev/ttyACM0` serial port.

If needed, adjust the serial port settings in the scripts as follows:

.. code-block:: none

   MODEM=/dev/ttyACM0
   BAUD=115200

To start the PPP connection, run the :file:`scripts/slm_start_ppp.sh` script.
To stop the PPP connection, run the :file:`scripts/slm_stop_ppp.sh` script.

The scripts need superuser privileges to run, so use `sudo`.
The PPP link is set as a default route if there is no existing default route.
The scripts do not manage the DNS settings from the Linux system.
Read the distribution manuals to learn how to configure the DNS settings.

The following example shows how to start the connection and verify its operation with various command-line utilities:

.. code-block:: shell

   $ sudo scripts/slm_start_ppp.sh
   Wait modem to boot
   Attach CMUX channel to modem...
   Connect and wait for PPP link...
   send (AT+CFUN=1^M)
   expect (OK)


   OK
   -- got it

   send ()
   expect (#XPPP: 1,0)




   #XPPP: 1,0
   -- got it

   $ ip addr show ppp0
   7: ppp0: <POINTOPOINT,MULTICAST,NOARP,UP,LOWER_UP> mtu 1464 qdisc fq_codel state UNKNOWN group default qlen 3
      link/ppp
      inet 10.139.130.66/32 scope global ppp0
         valid_lft forever preferred_lft forever
      inet6 2001:14bb:69b:50a3:ade3:2fce:6cc:ba3c/64 scope global temporary dynamic
         valid_lft 604720sec preferred_lft 85857sec
      inet6 2001:14bb:69b:50a3:40f9:1c4e:7231:638b/64 scope global dynamic mngtmpaddr
         valid_lft forever preferred_lft forever
      inet6 fe80::40f9:1c4e:7231:638b peer fe80::3c29:6401/128 scope link
         valid_lft forever preferred_lft forever

   $ ping -I ppp0 8.8.8.8 -c5
   PING 8.8.8.8 (8.8.8.8) from 10.139.130.66 ppp0: 56(84) bytes of data.
   64 bytes from 8.8.8.8: icmp_seq=1 ttl=60 time=320 ms
   64 bytes from 8.8.8.8: icmp_seq=2 ttl=60 time=97.6 ms
   64 bytes from 8.8.8.8: icmp_seq=3 ttl=60 time=140 ms
   64 bytes from 8.8.8.8: icmp_seq=4 ttl=60 time=132 ms
   64 bytes from 8.8.8.8: icmp_seq=5 ttl=60 time=145 ms

   --- 8.8.8.8 ping statistics ---
   5 packets transmitted, 5 received, 0% packet loss, time 4007ms
   rtt min/avg/max/mdev = 97.610/166.802/319.778/78.251 ms

   $ iperf3 -c ping.online.net%ppp0 -p 5202
   Connecting to host ping.online.net, port 5202
   [  5] local 10.139.130.66 port 54244 connected to 51.158.1.21 port 5202
   [ ID] Interval           Transfer     Bitrate         Retr  Cwnd
   [  5]   0.00-1.00   sec  0.00 Bytes  0.00 bits/sec    1   17.6 KBytes
   [  5]   1.00-2.00   sec  0.00 Bytes  0.00 bits/sec    0   25.8 KBytes
   [  5]   2.00-3.00   sec  0.00 Bytes  0.00 bits/sec    0   32.5 KBytes
   [  5]   3.00-4.00   sec   128 KBytes  1.05 Mbits/sec    0   35.2 KBytes
   [  5]   4.00-5.00   sec  0.00 Bytes  0.00 bits/sec    0   35.2 KBytes
   [  5]   5.00-6.00   sec  0.00 Bytes  0.00 bits/sec    0   35.2 KBytes
   [  5]   6.00-7.00   sec  0.00 Bytes  0.00 bits/sec    0   35.2 KBytes
   [  5]   7.00-8.00   sec  0.00 Bytes  0.00 bits/sec    0   35.2 KBytes
   [  5]   8.00-9.00   sec  0.00 Bytes  0.00 bits/sec    0   35.2 KBytes
   [  5]   9.00-10.00  sec  0.00 Bytes  0.00 bits/sec    0   35.2 KBytes
   - - - - - - - - - - - - - - - - - - - - - - - - -
   [ ID] Interval           Transfer     Bitrate         Retr
   [  5]   0.00-10.00  sec   128 KBytes   105 Kbits/sec    1             sender
   [  5]   0.00-11.58  sec  89.5 KBytes  63.3 Kbits/sec                  receiver

   $ sudo scripts/slm_stop_ppp.sh
   send (AT+CFUN=0^M)
   expect (#XPPP: 0,0)


   OK



   #XPPP: 0,0
   -- got it
