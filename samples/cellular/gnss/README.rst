.. _gps_with_supl_support_sample:
.. _agps_sample:
.. _gnss_sample:

Cellular: GNSS
##############

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the :ref:`nrfxlib:gnss_interface` to control the `GNSS`_ module.
It also shows how to improve fix speed and accuracy with the :ref:`lib_nrf_cloud_agnss` library and how to use the :ref:`lib_nrf_cloud_pgps` library.
Assistance data is downloaded from nRF Cloud using `nRF Cloud's REST-based device API <nRF Cloud REST API_>`_.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample first initializes the GNSS module.
Then it handles events from the interface, reads the associated data and outputs information to the console.
Because `NMEA`_ data needs to be read as soon as an NMEA event is received, a :ref:`Zephyr message queue <zephyr:message_queues_v2>` is used for buffering the NMEA strings.
The event handler function reads the received NMEA strings and puts those into the message queue.
The consumer loop reads from the queue and outputs the strings to the console.

Operation modes
===============

The sample supports different operation modes:

* Continuous
* Periodic
* Time-to-first-fix (TTFF) test

By default, the sample runs in continuous tracking mode.
In continuous mode, GNSS tries to acquire a fix once a second.

In periodic mode, fixes are acquired periodically with the set interval.

In TTFF test mode, the sample acquires fixes periodically and calculates the TTFF for each fix.
You can use the TTFF test mode without assistance or with any supported assistance method.
You can also configure it to perform cold starts, where the stored data is deleted from GNSS before each start.
If you enable assistance with cold starts, new assistance data is also downloaded and injected to GNSS before each start.

Output modes
============

The sample supports two output modes:

* Position, Velocity, and Time (PVT) and NMEA
* NMEA-only

By default, the sample displays information from both PVT and NMEA strings.
You can also configure the sample to run in NMEA-only output mode, where only the NMEA strings are displayed in the console.
In the NMEA-only output mode, you can visualize the data from the GNSS using a third-party tool.

A-GNSS and P-GPS
================

When support for A-GNSS or P-GPS, or both, is enabled, a :ref:`Zephyr workqueue <zephyr:workqueues_v2>` is used for downloading the assistance data.
Downloading the data can take some time.
The workqueue ensures that the main thread is not blocked during the operation.

When assistance support is enabled, the sample receives an A-GNSS data request notification from the GNSS module, and it starts downloading the assistance data requested by the GNSS module.
The sample then displays the information in the terminal about the download process.
Finally, after the download completes, the sample switches back to the previous display mode.

.. note::
   To download assistance data, your device must have a valid JWT signing key installed and registered with `nRF Cloud`_.

   .. include:: /includes/nrf_cloud_rest_sample_requirements.txt
       :start-after: requirement_keysign_moreinfo_start

Minimal assistance
==================

GNSS satellite acquisition can also be assisted by providing factory almanac, GPS time, and coarse location to the GNSS module.
Using this information, GNSS can calculate which satellites it should search for and what are the expected Doppler frequencies.

The sample includes a factory almanac that is written to the file system when the sample starts.
The date for the factory almanac generation is in the :file:`factory_almanac.h` file.
The almanac gets inaccurate with time and should be updated occasionally.
GNSS can use an almanac until it is two years old, but generally it should be updated every few months.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following Kconfig options for the sample:

.. _CONFIG_GNSS_SAMPLE_NMEA_ONLY:

CONFIG_GNSS_SAMPLE_NMEA_ONLY - To enable NMEA-only output mode
   The NMEA-only output mode can be used for example with 3rd party tools to visualize the GNSS output.

.. _CONFIG_GNSS_SAMPLE_ASSISTANCE_NRF_CLOUD:

CONFIG_GNSS_SAMPLE_ASSISTANCE_NRF_CLOUD - To use nRF Cloud A-GNSS
   This configuration option enables A-GNSS usage.

.. _CONFIG_GNSS_SAMPLE_ASSISTANCE_MINIMAL:

CONFIG_GNSS_SAMPLE_ASSISTANCE_MINIMAL - To use minimal assistance
   This configuration option enables assistance with factory almanac, time and location.

.. _CONFIG_GNSS_SAMPLE_MODE_PERIODIC:

CONFIG_GNSS_SAMPLE_MODE_PERIODIC - To enable periodic fixes
   This configuration option enables periodic fixes instead of continuous tracking.

.. _CONFIG_GNSS_SAMPLE_PERIODIC_INTERVAL:

CONFIG_GNSS_SAMPLE_PERIODIC_INTERVAL - To set interval (in seconds) for periodic fixes
   This configuration option sets the desired fix interval.

.. _CONFIG_GNSS_SAMPLE_PERIODIC_TIMEOUT:

CONFIG_GNSS_SAMPLE_PERIODIC_TIMEOUT - To set desired timeout (in seconds) for periodic fixes
   This configuration option sets the desired timeout for periodic fixes.

.. _CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST:

CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST - To enable time-to-first-fix (TTFF) test mode
  This configuration enables the TTFF test mode instead of continuous tracking.
  When TTFF test mode is enabled, the :ref:`CONFIG_GNSS_SAMPLE_NMEA_ONLY <CONFIG_GNSS_SAMPLE_NMEA_ONLY>` option is automatically selected.

.. _CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST_INTERVAL:

CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST_INTERVAL - To set the time between fixes in TTFF test mode
   This configuration option sets the time between fixes in TTFF test mode.

.. _CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST_COLD_START:

CONFIG_GNSS_SAMPLE_MODE_TTFF_TEST_COLD_START - To perform cold starts in TTFF test mode
   This configuration option makes the sample perform GNSS cold starts instead of hot starts in TTFF test mode.
   When assistance is used, LTE may block the GNSS operation and increase the time needed to get a fix.

.. _CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND:

CONFIG_GNSS_SAMPLE_LTE_ON_DEMAND - To disable LTE after assistance download
   When using assistance, LTE may block the GNSS operation and increase the time needed to get a fix.
   This configuration option disables LTE after the assistance data has been downloaded, so that GNSS can run without interruptions.

Additional configuration
========================

Check and configure the following library option that is used by the sample:

* :kconfig:option:`CONFIG_MODEM_ANTENNA_GNSS_EXTERNAL` - Selects an external GNSS antenna.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/gnss`

.. include:: /includes/build_and_run_ns.txt

If the sample is to be used with the SUPL client library, the library must be downloaded and enabled in the sample configuration.
You can download it from the `Nordic Semiconductor website`_.
See :ref:`supl_client` for information on installing and enabling the SUPL client library.

Testing
=======

After programming the sample and all the prerequisites to the development kit, test it by performing the following steps:

1. Connect your nRF91 Series DK to the PC using a USB cable and power on or reset your nRF91 Series DK.
#. Open a terminal emulator.
#. Test the sample by performing the following steps:

   If the default output mode is enabled:

   a. Observe that the following information is displayed in the terminal emulator:

          .. code-block:: console

                Tracking:  0 Using:  0 Unhealthy: 0
                -----------------------------------
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

                Tracking:  7 Using:  5 Unhealthy: 0
                -----------------------------------
                Latitude:       61.491275
                Longitude:      23.771611
                Altitude:       116.3 m
                Accuracy:       4.2 m
                Speed:          0.0 m/s
                Speed accuracy: 0.8 m/s
                Heading:        0.0 deg
                Date:           2020-03-06
                Time (UTC):     05:48:24
                PDOP:           3.1
                HDOP:           2.1
                VDOP:           2.3
                TDOP:           1.8

                NMEA strings:

                $GPGGA,054824.58,6129.28608,N,02346.17887,E,1,07,2.05,116.27,M,0,,*22
                $GPGLL,6129.28608,N,02346.17887,E,054824.58,A,A*6B
                $GPGSA,A,3,10,12,17,24,28,,,,,,,,3.05,2.05,2.25,1*13
                $GPGSV,2,1,7,17,50,083,41,24,68,250,38,10,14,294,46,28,23,071,38,1*56
                $GPGSV,2,2,7,12,29,240,36,19,00,000,32,1,00,000,33,1*50
                $GPRMC,054824.58,A,6129.28608,N,02346.17887,E,0.08,0.00,030620,,,A,V*29
                ---------------------------------

   If NMEA-only output mode is enabled:

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

   If TTFF test mode is enabled:

   a. Observe that the following information is displayed in the terminal emulator:

          .. code-block:: console

                $GPGGA,000033.00,,,,,0,,99.99,,M,,M,,*66
                $GPGLL,,,,,000033.00,V,N*4A
                $GPGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99,1*2D
                $GPGSV,3,1,10,1,,,36,5,,,26,7,,,25,10,,,44,1*53
                $GPGSV,3,2,10,14,,,43,17,,,37,21,,,41,23,,,43,1*64
                $GPGSV,3,3,10,24,,,31,28,,,39,1*61
                $GPRMC,000033.00,V,,,,,,,060180,,,N,V*08
                $GPGGA,121300.68,6129.28608,N,02346.17887,E,1,05,2.41,123.44,M,,M,,*7A
                $GPGLL,6129.28608,N,02346.17887,E,121300.68,A,A*63
                $GPGSA,A,3,01,10,17,21,23,,,,,,,,6.32,2.41,5.84,1*12
                $GPGSV,4,1,14,1,17,047,37,7,-22,107,25,10,22,314,44,12,09,232,,1*41
                $GPGSV,4,2,14,13,29,173,,14,38,072,44,15,40,211,,17,46,106,37,1*65
                $GPGSV,4,3,14,19,35,139,,21,15,019,41,23,19,279,42,24,51,273,32,1*6F
                $GPGSV,4,4,14,28,,,39,30,00,110,,1*52
                $GPRMC,121300.68,A,6129.28608,N,02346.17887,E,0.10,0.00,200122,,,A,V*2C
                [00:00:34.790,649] <inf> gnss_sample: Time to fix: 34
                [00:00:34.796,447] <inf> gnss_sample: Sleeping for 120 seconds
                [00:02:34.699,493] <inf> gnss_sample: Starting GNSS
                $GPGGA,121500.82,,,,,0,,99.99,,M,,M,,*6B
                $GPGLL,,,,,121500.82,V,N*47
                $GPGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99,1*2D
                $GPGSV,1,1,0,,,,,,,,,,,,,,,,,1*54
                $GPRMC,121500.82,V,,,,,,,200122,,,N,V*09
                $GPGGA,121501.82,6129.28608,N,02346.17887,E,1,04,2.73,118.22,M,,M,,*78
                $GPGLL,6129.28608,N,02346.17887,E,121501.82,A,A*69
                $GPGSA,A,3,10,17,21,23,,,,,,,,,7.59,2.73,7.08,1*18
                $GPGSV,4,1,13,1,18,046,28,10,22,313,49,12,10,232,26,13,28,173,25,1*51
                $GPGSV,4,2,13,14,37,072,50,15,40,211,25,17,46,105,45,19,35,138,31,1*63
                $GPGSV,4,3,13,21,15,018,45,23,18,278,45,24,52,273,,28,,,44,1*57
                $GPGSV,4,4,13,30,00,110,,1*55
                $GPRMC,121501.82,A,6129.28608,N,02346.17887,E,0.16,0.00,200122,,,A,V*20
                [00:02:35.940,582] <inf> gnss_sample: Time to fix: 1
                [00:02:35.946,319] <inf> gnss_sample: Sleeping for 120 seconds

   #. Observe that the samples displays the time to fix for each fix.

   If A-GNSS and/or P-GPS support is enabled:

   a. Observe that the following message is displayed in the terminal emulator immediately after the device boots:

           .. code-block:: console

              [00:00:04.488,494] <inf> gnss_sample: Assistance data needed, ephe 0xffffffff, alm 0xffffffff, flags 0x3b

   #. Observe the following actions in the terminal emulator:

      i. The sample downloads the requested assistance data if needed (with P-GPS, the data may already be available in the flash memory).
      #. The sample continues after the download has completed.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud_agnss`
* :ref:`lib_nrf_cloud_pgps`
* :ref:`lib_nrf_cloud_rest`
* :ref:`supl_client`
* :ref:`lib_at_host`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr library:

* :ref:`net_socket_offloading`
* :ref:`settings_api`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
