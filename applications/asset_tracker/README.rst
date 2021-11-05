.. _asset_tracker:

nRF9160: Asset Tracker
######################

.. contents::
   :local:
   :depth: 2

.. note::
   The Asset Tracker application is deprecated and succeeded by the :ref:`asset_tracker_v2` application.

The Asset Tracker demonstrates how to use the :ref:`lib_nrf_cloud` to connect an nRF9160-based kit to the `nRF Cloud`_ through LTE, transmit GPS and sensor data, and retrieve information about the device.

Overview
********

The application uses the LTE link control driver to establish a network connection.
It then collects various data locally, and transmits the data to Nordic Semiconductor's cloud solution, `nRF Cloud`_.
The data is visualized in nRF Cloud's web interface.

The collected data includes the GPS position, accelerometer readings (the device's physical orientation), and data from various environment sensors.

.. list-table::
   :header-rows: 1
   :align: center

   * - Sensor data
     - nRF Cloud sensor type
     - Data unit
   * - GPS coordinates
     - GPS
     - NMEA Gxxx string
   * - Accelerometer data
     - FLIP
     - String
   * - Temperature
     - TEMP
     - Celsius
   * - Humidity
     - HUMID
     - Percent
   * - Air pressure
     - AIR_PRESS
     - Pascal
   * - Light sensor
     - LIGHT
     - Lux

On the nRF9160 DK, the application uses simulated sensor data by default, but it can be configured with Kconfig options to use real sensors to collect data.
On the Thingy:91, onboard sensors are used by default.
GPS is enabled by default on both the kits.

In addition to the sensor data, the application retrieves information from the LTE modem, such as the signal strength, battery voltage, and current operator.
This information is available in nRF Cloud under the section **Cellular Link Monitor**.

The `LTE Link Monitor`_ application, implemented as part of `nRF Connect for Desktop`_  can be used to send AT commands to the device and receive the responses.
You can also send AT commands from the **Terminal** card on nRF Cloud when the device is connected.

By default, the Asset Tracker supports firmware updates through :ref:`lib_aws_fota`.

.. _power_opt:

Power optimization
==================

The Asset Tracker can run in three power modes that are configured in the Kconfig file of the application.
Currently the dynamic switching between eDRX and PSM during run-time is only supported on the nRF9160 DK.
However, both nRF9160 DK and Thingy:91 support build-time configuration for enabling eDRX, PSM or both.

.. note::
   Not all cellular network providers support these modes, and the granted parameters can vary between networks.
   Network operators can also limit the availability of power saving features for roaming users.
   SIM card subscription also affects the availability of the cellular IoT power saving features.


Demo mode
	This is the default setting.
	In this mode, the device maintains a continuous cellular link and can receive data at all times.
	To enable this mode, set the :kconfig:`CONFIG_POWER_OPTIMIZATION_ENABLE` to ``n``.

Request eDRX mode
	In this mode, the device requests the eDRX feature from the cellular network to save power.
	The device maintains a continuous cellular link.
	The device is reachable only at the configured eDRX intervals or when the device sends data.
	To enable this mode on an nRF9160 DK during run-time, set the :kconfig:`CONFIG_POWER_OPTIMIZATION_ENABLE` option to ``y`` and then set Switch 2 to the N.C. position.
	On Thingy:91 and nRF9160 DK, the :kconfig:`CONFIG_LTE_EDRX_REQ` option is used to enable eDRX during build-time.

Request Power Saving Mode (PSM)
	In this mode, the device requests the PSM feature from the cellular network to save power.
	The device maintains a continuous cellular link.
	The device is reachable only at the configured PSM intervals or when the device sends data.
	To enable this mode on an nRF9160 DK during run-time, set the :kconfig:`CONFIG_POWER_OPTIMIZATION_ENABLE` option to ``y`` and then set Switch 2 to the GND position.
	On Thingy:91 and nRF9160 DK, the :kconfig:`CONFIG_GPS_CONTROL_PSM_ENABLE_ON_START` option is used to enable PSM during build-time.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy91_nrf9160_ns, nrf9160dk_nrf9160_ns

.. include:: /includes/spm.txt

.. _asset_tracker_user_interface:

User interface
**************

The buttons and switches have the following functions when the connection is established:

Button 1 (SW3 on Thingy:91):
    * Send a BUTTON event to nRF Cloud.
    * Enable or disable GPS operation (long press the button for a minimum of 10 seconds).

Switch 1 (only on nRF9160 DK):
    * Toggle to simulate orientation change (flipping) of the kit.

Switch 2 (only on nRF9160 DK):
    * Set power optimization mode, see :ref:`power_opt`.

On the nRF9160 DK, the application state is indicated by the LEDs.

LED 3 and LED 4:
    * LED 3 blinking: The device is connecting to the LTE network.
    * LED 3 ON: The device is connected to the LTE network.
    * LED 4 blinking: The device is connecting to nRF Cloud.
    * LED 3 and LED 4 blinking: The MQTT connection has been established and the user association procedure with nRF Cloud has been initiated.
    * LED 4 ON: The device is connected and ready for sensor data transfer.

    .. figure:: /images/nrf_cloud_led_states.svg
       :alt: Application state indicated by LEDs

    Application state indicated by LEDs

All LEDs (1-4):
    * Blinking in groups of two (LED 1 and 3, LED 2 and 4): Recoverable error in the Modem library.
    * Blinking in cross pattern (LED 1 and 4, LED 2 and 3): Communication error with nRF Cloud.

On the Thingy:91, the application state is indicated by a single RGB LED as follows:

.. _thingy91_operating_states:

.. list-table::
   :header-rows: 1
   :align: center

   * - LED color
     - State
   * - White
     - Connecting to network
   * - Cyan
     - Connecting to nRF Cloud
   * - Yellow
     - Waiting for user association
   * - Blue
     - Connected, sending environment data
   * - Purple
     - Searching for GPS
   * - Green
     - GPS has fix, sending GPS and environment data
   * - Red
     - Error

If multicell location services are used and GPS is not enabled, the LED colors change depending on the number of neighboring cell towers reported by the modem as follows:

   * Blue = one cell tower
   * Purple = two cell towers
   * Green = three or more cell towers


.. _lwm2m_carrier_support:

Using the LwM2M carrier library
*******************************
This application supports the |NCS| :ref:`liblwm2m_carrier_readme` library.

To enable the LwM2M carrier library, add the following parameter to your build command:

``-DOVERLAY_CONFIG=lwm2m_carrier_overlay.conf``

In |SES|, select :guilabel:`Tools` > :guilabel:`Options` > :guilabel:`nRF Connect` to add the above CMake parameter.
See :ref:`cmake_options` for more information.

Alternatively, you can manually set the configuration options to match the contents of the overlay config file.

Using nRF Cloud A-GPS or P-GPS
******************************
By default, this application enables :ref:`lib_nrf_cloud_agps` (Assisted GPS) support.
Each time the GPS unit attempts to get a location fix, it may require additional information from  `nRF Cloud`_ to speed up the time to get that fix.

Alternatively, :ref:`lib_nrf_cloud_pgps` (Predicted GPS) downloads and stores assistance predictions in Flash for one or two weeks, and does not require the cloud to help with each fix.

In order to use P-GPS instead of A-GPS, you can add the following parameter to your build command:
``-DOVERLAY_CONFIG=overlay-pgps.conf``

In order to use A-GPS and P-GPS at the same time, use the following instead:
``-DOVERLAY_CONFIG=overlay-agps-pgps.conf``

Using nRF Cloud Cellular Positioning
************************************
If the accuracy of GPS is not required, battery life is very important, and reporting an approximate location is desired, cellular positioning is an option.

With cellular positioning:

   * The modem reports the current cell tower information to the application.
   * The application reports this to nRF Cloud.
   * nRF Cloud looks this up in a database.
   * If enabled, nRF Cloud reports back the location to the device.

There are two alternatives of cellular positioning:

   * Single cell is least costly but less accurate.
   * Multicell is more accurate.

Single cell is enabled with the :kconfig:`CONFIG_NRF_CLOUD_CELL_POS` option and multicell is enabled with the :kconfig:`CONFIG_CELL_POS_MULTICELL` option.


Using nRF Cloud FOTA
********************

You can add nRF Cloud FOTA update functionality to the application through the :ref:`lib_nrf_cloud` library.
The FOTA functionality is automatically enabled when you include the nRF Cloud library.
For more information, see :ref:`lib_nrf_cloud_fota`.


Building and running
********************

.. |sample path| replace:: :file:`applications/asset_tracker`

.. include:: /includes/build_and_run_nrf9160.txt

The Kconfig file of the application contains options to configure the application.
For example, configure the :kconfig:`CONFIG_POWER_OPTIMIZATION_ENABLE` option to enable power optimization or the :kconfig:`CONFIG_TEMP_USE_EXTERNAL` option to use an external temperature sensor instead of simulated temperature data.
In |SES|, select :guilabel:`Project` > :guilabel:`Configure nRF Connect SDK project` to browse and configure these options.
Alternatively, use the command line tool ``menuconfig`` or configure the options directly in :file:`prj.conf`.

.. external_antenna_note_start

.. note::
   For nRF9160 DK v0.15.0 and later, set the :kconfig:`CONFIG_NRF9160_GPS_ANTENNA_EXTERNAL` option to ``y`` when building the application to achieve the best external antenna performance.

.. external_antenna_note_end

This application supports the |NCS| :ref:`ug_bootloader`, but it is disabled by default.
To enable the immutable bootloader, set the :kconfig:`CONFIG_SECURE_BOOT` option to ``y``.


Testing
=======

After programming the application and all prerequisites to your kit, test the Asset Tracker application by performing the following steps:

1. Connect the kit to the computer using a USB cable.
   The kit is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. Connect to the kit with a terminal emulator, for example, LTE Link Monitor.
#. Reset the kit.
#. Observe in the terminal window that the kit starts up in the Secure Partition Manager and that the application starts.
   This is indicated by output similar to the following lines::

      SPM: prepare to jump to Non-Secure image
      ***** Booting Zephyr OS v1.13.99 *****
      Application started

#. Observe in the terminal window that the connection to nRF Cloud is established. This may take several minutes.
#. Open a web browser and navigate to https://nrfcloud.com/.
   Follow the instructions to set up your account and add an LTE device.
#. The first time you start the application, add the device to your account:

   a. Observe that the LED(s) indicate that the device is waiting for user association.
   #. Follow the instructions on `nRF Cloud`_ to add your device.
   #. If association is successful, the device reconnects to nRF Cloud.
      If the LED(s) indicate an error, check the details of the error in the terminal window.
      The device must be power-cycled to restart the association procedure.
#. Observe that the LED(s) indicate that the connection is established.
#. Observe that the device count on your nRF Cloud dashboard is incremented by one.
#. Select the device from your device list on nRF Cloud, and observe that sensor data and modem information is received from the kit.
#. Press Button 1 (SW3 on Thingy:91) to send BUTTON data to nRF Cloud.
#. Press Button 1 (SW3 on Thingy:91) for a minimum of 10 seconds to enable GPS tracking.
   The kit must be outdoors in clear space for a few minutes to get the first position fix.
#. Optionally send AT commands from the terminal, and observe that the response is received.


Dependencies
************

This application uses the following |NCS| libraries and drivers:


* :ref:`lib_nrf_cloud`
* :ref:`modem_info_readme`
* :ref:`at_cmd_parser_readme`
* ``drivers/nrf9160_gps``
* ``drivers/sensor/sensor_sim``
* :ref:`dk_buttons_and_leds_readme`
* :ref:`lte_lc_readme`
* |NCS| modules abstracted by the LwM2M carrier OS abstraction layer (:file:`lwm2m_os.h`)

.. include:: /libraries/bin/lwm2m_carrier/app_integration.rst
  :start-after: lwm2m_osal_mod_list_start
  :end-before: lwm2m_osal_mod_list_end

It uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`
