.. _gps_with_supl_support_sample:

nRF9160: GPS sockets and SUPL client library
############################################

.. contents::
   :local:
   :depth: 2

The GPS sample demonstrates how to retrieve `GPS`_ data.
If Secure User-Plane Location (SUPL) support is enabled, it also shows how to improve the GPS fix accuracy and fix speed with `A-GPS`_ data from a SUPL server.
See :ref:`supl_client` for information on enabling SUPL support for the sample.

The sample first creates a `GNSS`_ socket.
Then it reads the GPS data from the socket and parses the received frames to interpret the frame contents.
See :ref:`nrfxlib:gnss_extension` for more information.

Overview
********

This sample operates in two different modes.

In the default mode, the sample displays information from both PVT (Position, Velocity, and Time) and `NMEA`_ strings.
The sample can also be configured to run in NMEA-only mode, where only the NMEA strings are displayed in the console.
The NMEA-only mode can be used to visualize the data from the GPS using a third-party tool.

SUPL support can be enabled for both the default mode (PVT and NMEA) and the NMEA-only mode.
When the SUPL support is enabled, the sample receives an A-GPS data request notification from the GPS module, and it starts downloading the A-GPS data requested by the GPS module.
The sample then displays the information in the terminal about the download process.
Finally, after the download completes, the sample switches back to the previous display mode.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns

The sample can be optionally used with the SUPL Client library (for details on download, see :ref:`supl_client`).

.. include:: /includes/spm.txt

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

After programming the sample and all the prerequisites to the development kit, you can test the sample by performing the following steps:

1. Connect your nRF9160 DK to the PC using a USB cable and power on or reset your nRF9160 DK.
2. Open a terminal emulator.
#. Test the sample by performing the following steps:

   If the default mode is enabled:

   a. Observe that the following information is displayed in the terminal emulator:

          .. code-block:: console

                Tracking: 0 Using: 0 Unhealthy: 0
                ---------------------------------
                Seconds since last fix: 1
                Searching [-]

                NMEA strings:

                $GPGGA,000000.00,,,,,0,,99.99,,M,0,,*37
                $GPGLL,,,,,000000.00,V,A*45
                $GPGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99,1*2D
                $GPGSV,1,1,0,,,,,,,,,,,,,,,,,1*54
                $GPRMC,000000.00,V,,,,,,,060180,,,N,V*08
                ---------------------------------

   #. Observe that the numbers associated with the displayed parameters **Tracking** and **Using** change.
   #. Observe that the sample displays the following information upon acquiring the GPS lock:

          .. code-block:: console

                Tracking: 7 Using: 5 Unhealthy: 0
                ---------------------------------
                Longitude:  23.771611
                Latitude:   61.491275
                Altitude:   116.274658
                Speed:      0.039595
                Heading:    0.000000
                Date:       2020-03-06
                Time (UTC): 05:48:24

                NMEA strings:

                $GPGGA,054824.58,6128.77008,N,02351.48387,E,1,07,2.05,116.27,M,0,,*22
                $GPGLL,6129.28608,N,02346.17887,E,054824.58,A,A*6B
                $GPGSA,A,3,10,12,17,24,28,,,,,,,,3.05,2.05,2.25,1*13
                $GPGSV,2,1,7,17,50,083,41,24,68,250,38,10,14,294,46,28,23,071,38,1*56
                $GPGSV,2,2,7,12,29,240,36,19,00,000,32,1,00,000,33,1*50
                $GPRMC,054824.58,A,6129.28608,N,02346.17887,E,0.08,0.00,030620,,,A,V*29
                ---------------------------------

   If NMEA-only mode is enabled:

   a. Observe that the following information is displayed in the terminal emulator:

          .. code-block:: console

                $GPGGA,000000.00,,,,,0,,99.99,,M,0,,*37
                $GPGLL,,,,,000000.00,V,A*45
                $GPGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99,1*2D
                $GPGSV,1,1,0,,,,,,,,,,,,,,,,,1*54
                $GPRMC,000000.00,V,,,,,,,060180,,,N,V*08
                $GPGGA,000001.00,,,,,0,02,99.99,,M,0,,*34
                $GPGLL,,,,,000001.00,V,A*44
                $GPGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99,1*2D
                $GPGSV,1,1,2,17,,,24,1,,,28,1*6D
                $GPRMC,000001.00,V,,,,,,,060180,,,N,V*09
                $GPGGA,000002.00,,,,,0,02,99.99,,M,0,,*37
                $GPGLL,,,,,000002.00,V,A*47
                $GPGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99,1*2D
                $GPGSV,1,1,2,17,,,24,1,,,28,1*6D
                $GPRMC,000002.00,V,,,,,,,060180,,,N,V*0A

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

This sample uses the following |NCS| libraries:

* :ref:`secure_partition_manager`
* :ref:`at_cmd_readme`
* :ref:`at_notif_readme`
* :ref:`supl_client`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr library:

* :ref:`net_socket_offloading`
