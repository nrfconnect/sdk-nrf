.. _gps_with_supl_support_sample:
.. _agps_sample:
.. _gnss_sample:

nRF9160: GNSS
#############

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to retrieve `GNSS`_ data.
It also shows how to improve fix speed and accuracy with the :ref:`lib_nrf_cloud_agps` library and how to use the :ref:`lib_nrf_cloud_pgps` library.
Assistance data is downloaded from nRF Cloud using `nRF Cloud's REST-based device API <nRF Cloud REST API_>`_.

The sample first initializes the GNSS interface.
Then it handles events from the interface, reads the associated data and outputs information to the console.
Because `NMEA`_ data needs to be read as soon as an NMEA event is received, a :ref:`Zephyr message queue <zephyr:message_queues_v2>` is used for buffering the NMEA strings.
The event handler function reads the received NMEA strings and puts those into the message queue.
The consumer loop reads from the queue and outputs the strings to the console.

When A-GPS and/or P-GPS support is enabled, a :ref:`Zephyr workqueue <zephyr:workqueues_v2>` is used for downloading the assistance data.
Downloading the data can take some time and the workqueue ensures that the main thread is not blocked during the operation.

See :ref:`nrfxlib:gnss_interface` for more information.

Overview
********

This sample operates in two different modes.

In the default mode, the sample displays information from both PVT (Position, Velocity, and Time) and NMEA strings.
The sample can also be configured to run in NMEA-only mode, where only the NMEA strings are displayed in the console.
The NMEA-only mode can be used to visualize the data from the GNSS using a third-party tool.

You can enable A-GPS and P-GPS support for both the default mode (PVT and NMEA) and the NMEA-only mode.
When assistance support is enabled, the sample receives an A-GPS data request notification from the GNSS module, and it starts downloading the assistance data requested by the GNSS module.
The sample then displays the information in the terminal about the download process.
Finally, after the download completes, the sample switches back to the previous display mode.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160_ns

.. include:: /includes/spm.txt

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. option:: CONFIG_GNSS_SAMPLE_NMEA_ONLY - To enable NMEA-only mode

   The NMEA-only mode can be used for example with 3rd party tools to visualize the GNSS output.

.. option:: CONFIG_GNSS_SAMPLE_ANTENNA_EXTERNAL - To use an external GNSS antenna

   This configuration option should be enabled if an external GNSS antenna is used, so that the Low Noise Amplifier (LNA) can be configured accordingly.

.. option:: CONFIG_GNSS_SAMPLE_ASSISTANCE_NRF_CLOUD - To use nRF Cloud A-GPS

   This configuration option enables A-GPS usage.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/gnss`

.. include:: /includes/build_and_run_nrf9160.txt

If the sample is to be used with the SUPL client library, the library must be downloaded and enabled in the sample configuration.
You can download it from the `Nordic Semiconductor website`_.
See :ref:`supl_client` for information on installing and enabling the SUPL client library.

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
   #. Observe that the sample displays the following information upon acquiring a fix:

          .. code-block:: console

                Tracking: 7 Using: 5 Unhealthy: 0
                ---------------------------------
                Latitude:   61.491275
                Longitude:  23.771611
                Altitude:   116.3 m
                Accuracy:   4.2 m
                Speed:      0.0 m/s
                Heading:    0.0 deg
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

   If A-GPS and/or P-GPS support is enabled:

   a. Observe that the following message is displayed in the terminal emulator immediately after the device boots:

           .. code-block:: console

              Assistance data needed, ephe 0xffffffff, alm 0xffffffff, flags 0x3b

   b. Observe the following actions in the terminal emulator:

      i. The sample downloads the requested assistance data if needed (with P-GPS, the data may already be available in the flash memory).
      #. The sample continues after the download has completed.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`secure_partition_manager`
* :ref:`lib_nrf_cloud_agps`
* :ref:`lib_nrf_cloud_pgps`
* :ref:`lib_nrf_cloud_rest`
* :ref:`supl_client`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr library:

* :ref:`net_socket_offloading`
