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

* One of the following development boards:

  * |nRF9160DK|
  * |Thingy91|

* An nRF Cloud account


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
#. Make sure that GPS has started by confirming that the device requests GPS assistance data and receives a response.
   You can do this by checking the logs.
#. Observe that the GPS starts to track the position based on the satellite data received.
#. Observe that the current position of the device shows up on the GPS position map in nRF Cloud, after acquiring the GPS position fix.
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
