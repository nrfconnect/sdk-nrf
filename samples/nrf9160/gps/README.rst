.. _gps_with_supl_support_sample:

nRF9160: GPS sockets and SUPL client library
############################################

The GPS sample demonstrates how to retrieve `GPS`_ data.
If Secure User-Plane Location (SUPL) support is enabled, it also shows how to improve the GPS fix accuracy and fix speed with `A-GPS`_ data from a SUPL server.
See :ref:`supl_client` for information on enabling SUPL support for the sample.

The sample first creates a `GNSS`_ socket.
See :ref:`nrfxlib:bsdlib` and :ref:`nrfxlib:bsdlib_extensions` for more information about GNSS sockets.
Then it reads the GPS data from the socket and parses the received frames to interpret the frame contents.

Overview
********

This sample operates in three different modes.
Each of these modes depends on the state of the GPS module.

Mode 1
======

This is the startup mode in which the sample waits for the GPS module to acquire a fix on the current position.
Information about the progress is displayed on a console.

Mode 2
======

The sample switches to the second mode when the GPS module has acquired at least one fix.
In this mode, the sample also displays information about the last good fix received from the GPS module.
The displayed information includes both PVT (Position, Velocity, and Time) and `NMEA`_ strings.

Mode 3
======

The sample operates in this mode only if SUPL client support is enabled.
When the sample receives an A-GPS notification from the GPS module, the sample switches from the current mode to the third mode and starts downloading the A-GPS data requested by the GPS module.
The sample then displays the information on the screen about the download process.
Finally, the sample switches back to the previous mode when the download is completed.

Requirements
************

* The following development board:

  * |nRF9160DK|

* Optional: SUPL Client library (for details on download, see :ref:`supl_client`)

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/gps`

.. include:: /includes/build_and_run_nrf9160.txt

Using the sample with the SUPL client library
=============================================

If the sample is to be used with the SUPL client library, the library must be downloaded and enabled in the sample configuration.
You can download it from the `Nordic Semiconductor website`_.
See :ref:`supl_client` for information on installing and enabling the SUPL client library.

The SUPL client library is not required, and the sample will work without `A-GPS`_ support if the library is not available.

Testing
=======

After programming the sample and all the prerequisites to the board, you can test the sample by performing the following steps:

1. Connect your nRF9160 DK to the PC using a USB cable and power on or reset
   your nRF9160 DK.
2. Open a terminal emulator.
#. Test the sample by performing the following steps:

   If SUPL client library support is not enabled:

   a. Observe that the following information is displayed in the terminal emulator:

          .. code-block:: console

                Tracking: 0 Using: 0 Unhealthy: 0
                Seconds since last fix 0

                Scanning [/]

   #. Observe that the numbers associated with the displayed parameters **Tracking** and **Using** change.
   #. Observe that the sample displays the following information upon acquiring the first GPS lock:

          .. code-block:: console

		        Tracking: 7 Using: 7 Unhealthy: 0
		        Seconds since last fix 0
		        ---------------------------------
		        Longitude:  10.437814
		        Latitude:   63.421546
		        Altitude:   163.747833
		        Speed:      0.023171
		        Heading:    0.000000
		        Date:       30-01-2020
		        Time (UTC): 09:42:38

		        NMEA strings:
		        $GPGGA,094238.25,6325.29275,N,01026.26884,E,1,08,1.49,163.75,M,0,,*2E
		        $GPGLL,6325.29275,N,01026.26884,E,094238.25,A,A*66
		        $GPGSA,A,3,05,07,08,09,16,21,27,30,,,,,2.60,1.49,2.12,1*17
		        $GPGSV,2,1,8,7,60,093,44,9,14,106,39,21,17,339,41,16,07,016,22,1*5B
		        $GPGSV,2,2,8,27,15,041,20,8,07,071,24,30,62,176,29,5,57,239,34,1*51
		        $GPRMC,094238.25,A,6325.29275,N,01026.26884,E,0.05,0.00,300120,,,A,V*2E


   If SUPL client library support is enabled:

   a. Observe that the following message is displayed in the terminal emulator immediately after the device boots:

           .. code-block:: console

              New AGPS data requested, contacting SUPL server, flags 59.

   b. Observe the following actions in the terminal emulator:

      i. The sample switches to LTE and connects to a SUPL server.
      #. The A-GPS data gets downloaded.
      #. The sample continues after the SUPL session is complete.

Dependencies
************

This sample uses the following libraries:

From |NCS|
  * :ref:`secure_partition_manager`
  * :ref:`at_cmd_readme`
  * :ref:`at_notif_readme`
  * :ref:`supl_client`

From nrfxlib
  * :ref:`nrfxlib:bsdlib`

From Zephyr
  * :ref:`net_socket_offloading`
