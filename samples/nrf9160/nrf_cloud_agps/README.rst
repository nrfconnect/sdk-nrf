.. _nrf_cloud_agps_sample:

nRF9160: nRF Cloud A-GPS
########################

The nRF Cloud A-GPS sample demonstrates how the `nRF Cloud`_ Assisted GPS (`A-GPS`_) feature can be used in an application.
You can use this sample as a starting point to implement A-GPS in your application.

Overview
********

The use of A-GPS speeds up the time to first GPS position fix (commonly known as TTFF or Time to first fix), as the required satellite information is downloaded from nRF Cloud instead of direct download of the data from the satellites.

The sample application uses the nRF Cloud A-GPS library to request and receive A-GPS data.
The Assistance GPS data includes the following information:

* `Ephemeris`_
* `Almanac`_
* UTC time
* integrity data
* approximate location
* GPS system clock
* ionospheric correction parameters for `Klobuchar Ionospheric Model`_

TTFF is expected to be less when compared to that in the scenario of GPS without assistance data, performing a cold or a warm start.
The TTFF is expected to be comparable to that of a GPS hot start and is usually in the range of 5 to 20 seconds in good satellite signaling conditions.
Good satellite signaling conditions in this context means that the device is positioned in a way to have a direct line of sight to a large portion of the sky.

Note that the radio component on the nRF9160-based device is shared between LTE and GPS and hence only one of these can be active at any moment of time.
LTE has priority over the radio resources, and GPS operates in the time interval between LTE required operations as per LTE specification.
Consequently, GPS is blocked from searching as long as the modem is in `RRC connected mode <RRC idle mode_>`_.

The modem is in RRC connected mode when sending and receiving data over the LTE network.
After data transmission is complete, the modem stays in RRC connected mode for a period of network-specific inactivity time, before entering RRC idle mode.
The inactivity time varies between networks.
It is typically in the range of 5 to 70 seconds for a specific network.
In RRC idle mode, the GPS is usually able to operate.

It is recommended to use LTE Power Saving Mode (PSM) and extended Discontinuous Reception (eDRX) mode to increase the allowed time of operation for GPS.
During the defined intervals of PSM and eDRX, LTE communication does not occur, and the GPS has full access to the radio resources.
In this sample, both PSM and eDRX are enabled by default.
You can enable or disable these features by setting the following configuration options to ``y`` or ``n``.

* ``CONFIG_LTE_POWER_SAVING_MODE``
* :option:`CONFIG_LTE_EDRX_REQ`

Requirements
************

The sample supports the following development kits:

.. include:: /includes/boardname_tables/sample_boardnames.txt
   :start-after: set6_start
   :end-before: set6_end

The sample also requires an nRF Cloud account.

.. include:: /includes/spm.txt

User interface
**************

A predefined message can be sent to nRF Cloud by pressing button 1.
The message can be changed by setting the :option:`CONFIG_CLOUD_MESSAGE` to a new message.

To ease outdoors and remote testing of A-GPS feature, two methods for resetting the board are provided.
Both are equivalent to pressing the reset button on the nRF9160 DK, or power-cycling the nRF9160 DK or Thingy:91.

Button 1:
   Press the button to send a predefined message to nRF Cloud.
   Press the button for a minimum of 3 seconds to reset the nRF9160-based device.

nRF Cloud:
   Use the terminal pane in the device view for a particular device in nRF Cloud and send the ``"{"reboot":true}"`` command to reset the nRF9160-based device.


Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/nrf_cloud_agps`

.. include:: /includes/build_and_run_nrf9160.txt

The configuration file for this sample is located in :file:`samples/nrf9160/nrf_cloud_agps`.
See :ref:`configure_application` for information on how to configure the parameters.

Testing
=======

After programming the sample to your nRF9160-based device, test it by performing the following steps:

#. Place the nRF9160-based device in a location with direct line of sight to a large portion of the sky.
   This is to achieve sufficient satellite coverage to get a position fix.
#. Optionally connect the nRF9160-based device to a PC with a terminal application to see log output.
   You can also use the Bluetooth LE service of Thingy:91 to see log output on a mobile device.
#. Log into your nRF Cloud account and select your device.
#. Power on the nRF9160-based device.
#. The device connects to the LTE network and nRF Cloud.

   .. code-block:: console

	  -------------------------
	  I: nRF Cloud and A-GPS sample has started
	  I: Connecting to LTE network. This may take minutes.
	  I: PSM mode requested
	  +CEREG: 2,"7725","0138E000",7,0,0,"11100000","11100000"
	  +CSCON: 1
	  +CEREG: 1,"7725","0138E000",7,,,"00000010","00000110"
	  I: Network registration status: Connected - home network

#. Make sure that GPS has started by confirming that the device requests GPS assistance data and receives a response.
   You can do this by checking the logs.

   .. code-block:: console

          D: GPS socket created, fd: 1232491587
	  I: CLOUD_EVT_CONNECTED
	  I: CLOUD_EVT_DATA_RECEIVED
	  I: CLOUD_EVT_PAIR_DONE
	  I: CLOUD_EVT_READY
	  I: CLOUD_EVT_DATA_RECEIVED
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 3
	  D: Sent A-GPS data to modem, type: 7
	  D: Sent A-GPS data to modem, type: 8
	  D: Sent A-GPS data to modem, type: 1
	  D: Sent A-GPS data to modem, type: 4
	  D: Sent A-GPS data to modem, type: 9
	  I: A-GPS data successfully processed
	  I: GPS will start in 3 seconds
	  I: CLOUD_EVT_DATA_RECEIVED
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  D: Sent A-GPS data to modem, type: 2
	  I: A-GPS data successfully processed
	  I: GPS will start in 3 seconds
	  D: GPS mode is enabled
	  D: GPS operational

#. Observe that the GPS starts to track the position based on the satellite data received.

   .. code-block:: console

	  I: Periodic GPS search started with interval 240 s, timeout 120 s
	  D: A-GPS data update needed
	  D: Waiting for time window to operate
	  +CSCON: 0
	  D: GPS has time window to operate
	  D: Tracking: 0 Using: 0 Unhealthy: 0
	  D: Seconds since last fix 23
	  I: Tracking: 29
	  D: Tracking SV 29: not used,     healthy
	  D: Tracking: 1 Using: 0 Unhealthy: 0
	  D: Seconds since last fix 24
	  I: Tracking: 5  29
	  D: Tracking SV 29: not used,     healthy
	  D: Tracking SV  5: not used,     healthy
	  D: Tracking: 2 Using: 0 Unhealthy: 0
	  D: Seconds since last fix 25
	  I: Tracking: 5  9  21  26  29
	  D: Tracking SV 29: not used,     healthy
	  D: Tracking SV  5: not used,     healthy
	  D: Tracking SV  9: not used,     healthy
	  D: Tracking SV 21: not used,     healthy
	  D: Tracking SV 26: not used,     healthy
	  D: Tracking: 5 Using: 0 Unhealthy: 0
	  D: Seconds since last fix 26
	  D: Tracking SV 29: not used,     healthy
	  D: Tracking SV  5: not used,     healthy
	  D: Tracking SV  9: not used,     healthy
	  D: Tracking SV 21: not used,     healthy
	  D: Tracking SV 26: not used,     healthy
	  D: Tracking: 5 Using: 0 Unhealthy: 0
	  D: Seconds since last fix 27
	  D: Tracking SV 29: not used,     healthy
	  D: Tracking SV  5: not used,     healthy
	  D: Tracking SV  9: not used,     healthy
	  D: Tracking SV 21: not used,     healthy
	  D: Tracking SV 26: not used,     healthy
	  D: Tracking: 5 Using: 0 Unhealthy: 0
	  D: Seconds since last fix 28
	  D: Tracking SV 29: not used,     healthy
	  D: Tracking SV  5: not used,     healthy
	  D: Tracking SV  9: not used,     healthy
          D: Tracking SV 21: not used,     healthy
	  D: Tracking SV 26: not used,     healthy
	  D: Tracking: 5 Using: 0 Unhealthy: 0
	  D: Seconds since last fix 29
	  D: Tracking SV 29:     used,     healthy
	  D: Tracking SV  5:     used,     healthy
	  D: Tracking SV  9:     used,     healthy
	  D: Tracking SV 21: not used,     healthy
	  D: Tracking SV 26:     used,     healthy
	  D: Tracking: 5 Using: 4 Unhealthy: 0
	  D: Seconds since last fix 30
	  D: Tracking SV 29:     used,     healthy
	  D: Tracking SV  5:     used,     healthy
	  D: Tracking SV  9:     used,     healthy
	  D: Tracking SV 21: not used,     healthy
	  D: Tracking SV 26:     used,     healthy
	  D: Tracking: 5 Using: 4 Unhealthy: 0
	  D: Seconds since last fix 31
	  D: Tracking SV 29:     used,     healthy
	  D: Tracking SV  5:     used,     healthy
	  D: Tracking SV  9:     used,     healthy
	  D: Tracking SV 21: not used,     healthy
	  D: Tracking SV 26:     used,     healthy
	  D: Tracking: 5 Using: 4 Unhealthy: 0
	  D: Seconds since last fix 32
	  D: Tracking SV 29:     used,     healthy
	  D: Tracking SV  5:     used,     healthy
	  D: Tracking SV  9:     used,     healthy
	  D: Tracking SV 21: not used,     healthy
	  D: Tracking SV 26:     used,     healthy
	  D: Tracking: 5 Using: 4 Unhealthy: 0
	  D: Seconds since last fix 33
	  D: PVT: Position fix
	  D: Stopping GPS

#. Observe that the current position of the device shows up on the GPS position map in nRF Cloud, after acquiring the GPS position fix.
   The position details are also displayed on the terminal.

   .. code-block:: console

          I: ---------       FIX       ---------
	  I: Time to fix: 20 seconds
	  I: Longitude:  5.399481
	  Latitude:   61.362466
	  Altitude:   10.276114
	  Speed:      0.118597
	  Heading:    0.000000
	  Date:       09-07-2020
	  Time (UTC): 08:52:29

	  I: -----------------------------------
	  D: Tracking SV 29:     used,     healthy
	  D: Tracking SV  5:     used,     healthy
	  D: Tracking SV  9:     used,     healthy
	  D: Tracking SV 21: not used,     healthy
	  D: Tracking SV 26:     used,     healthy
	  D: Tracking: 5 Using: 4 Unhealthy: 0
	  D: Seconds since last fix 0
	  D: NMEA: Position fix
	  I: GPS position sent to cloud


#. Optionally press Button 1 and observe that the predefined message is sent from the device to nRF Cloud.
   Note that this interrupts the GPS search for a period of time since LTE requires the use of the radio resources of the device.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_agps`


Known issues and limitations
****************************

.. agpslimitation_start

Approximate location assistance data is based on LTE cell location.
Since all cell locations are not always available, the location data is sometimes absent in the A-GPS response.

.. agpslimitation_end
