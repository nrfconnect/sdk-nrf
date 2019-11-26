.. _asset_tracker:

nRF9160: Asset Tracker
######################

The Asset Tracker demonstrates how to use the :ref:`lib_nrf_cloud` to connect an nRF9160-based board to the `nRF Cloud`_ via LTE, transmit GPS and sensor data, and retrieve information about the device.


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
   * - GPS coordinates
     - GPS
   * - Accelerometer data
     - FLIP
   * - Temperature
     - TEMP
   * - Humidity
     - HUMID
   * - Air pressure
     - AIR_PRESS
   * - Light sensor
     - LIGHT

On the nRF9160 DK, the application uses simulated sensor data by default, but it can be configured with Kconfig options to use real sensors to collect data.
On the Thingy:91, onboard sensors are used by default.
GPS is enabled by default on both the boards.

In addition to the sensor data, the application retrieves information from the LTE modem, such as the signal strength, battery voltage, and current operator.
This information is available in nRF Cloud under the section **Cellular Link Monitor**.

The `LTE Link Monitor`_ application, implemented as part of `nRF Connect for Desktop`_  can be used to send AT commands to the device and receive the responses.

By default, the asset tracker supports firmware updates through :ref:`lib_aws_fota`.


Requirements
************

* One of the following development boards:

    * |nRF9160DK|
    * |Thingy91|

* .. include:: /includes/spm.txt

.. _asset_tracker_user_interface:

User interface
**************

The buttons and switches have the following functions when the connection is established:

Button 1 (SW3 on Thingy:91):
    * Send a BUTTON event to the nRF Cloud.
    * Enable or disable GPS operation (long press the button for a minimum of 10 seconds).

Switch 1 (only on nRF9160 DK):
    * Toggle to simulate orientation change (flipping) of the board.

Switch 2 (only on nRF9160 DK):
    * Set power optimization mode, see :ref:`power_opt`.

On the nRF9160 DK, the application state is indicated by the LEDs.

LED 3 and LED 4:
    * LED 3 blinking: Connecting - The device is resolving DNS and connecting to the nRF Cloud.
    * LED 3 and LED 4 blinking: Pairing started - The MQTT connection has been established and the pairing procedure towards the nRF Cloud has been initiated.
    * LED 3 ON and LED 4 blinking: Pattern entry - The user has started entering the pairing pattern.
    * LED 4 blinking: Pattern sent - Pattern has been entered and sent to the nRF Cloud for verification.
    * LED 4 ON: Connected - The device is ready for sensor data transfer.

    .. figure:: /images/nrf_cloud_led_states.svg
       :alt: Application state indicated by LEDs

    Application state indicated by LEDs

All LEDs (1-4):
    * Blinking in groups of two (LED 1 and 3, LED 2 and 4): Recoverable error in the BSD library.
    * Blinking in cross pattern (LED 1 and 4, LED 2 and 3): Communication error with the nRF Cloud.

On the Thingy:91, the application state is indicated by a single RGB LED as follows:

.. list-table::
   :header-rows: 1
   :align: center

   * - LED color
     - State
   * - White
     - Connecting to network
   * - Cyan
     - Connecting to the nRF Cloud
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

.. _power_opt:

Power optimization
******************

The Asset Tracker can run in three power modes that are configured in the Kconfig file of the application.
These settings are currently only supported on the nRF9160 DK.

.. note::
   Not all cellular network providers support these modes, and the granted parameters can vary between networks.

Demo mode
	This is the default setting.
	In this mode, the device maintains a continuous cellular link.
	To enable this mode, set ``CONFIG_POWER_OPTIMIZATION_ENABLE=n``.

Request eDRX mode
	In this mode, the device requests the eDRX feature from the cellular network to save power.
	To enable this mode, set ``CONFIG_POWER_OPTIMIZATION_ENABLE=y`` and then
	set Switch 2 to the N.C. position.

Request Power Saving Mode (PSM)
	In this mode, the device requests the PSM feature from the cellular network to save power.
	To enable this mode, set ``CONFIG_POWER_OPTIMIZATION_ENABLE=y`` and then
	set Switch 2 to the GND position.


Building and running
********************

.. |sample path| replace:: :file:`applications/asset_tracker`

.. include:: /includes/build_and_run_nrf9160.txt

The Kconfig file of the application contains options to configure the application.
For example, configure ``CONFIG_POWER_OPTIMIZATION_ENABLE`` to enable power optimization or ``CONFIG_TEMP_USE_EXTERNAL`` to use an external temperature sensor instead of simulated temperature data.
In |SES|, select **Project** > **Configure nRF Connect SDK project** to browse and configure these options.
Alternatively, use the command line tool ``menuconfig`` or configure the options directly in ``prj.conf``.

Testing
=======

After programming the application and all prerequisites to your board, test the Asset Tracker application by performing the following steps:

1. Connect the board to the computer using a USB cable.
   The board is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. Connect to the board with a terminal emulator, for example, LTE Link Monitor.
#. Reset the board.
#. Observe in the terminal window that the board starts up in the Secure Partition Manager and that the application starts.
   This is indicated by output similar to the following lines::

      SPM: prepare to jump to Non-Secure image
      ***** Booting Zephyr OS v1.13.99 *****
      Application started

#. Observe in the terminal window that the connection to the nRF Cloud is established. This may take several minutes.
#. Open a web browser and navigate to https://nrfcloud.com/.
   Follow the instructions to set up your account and add an LTE device.
#. The first time you start the application, pair the device to your account:

   a. Observe that the LED(s) indicate that the device is waiting for user association.
   #. Follow the instructions on `nRF Cloud`_ to pair your device.
   #. If the pairing is successful, the board and your nRF Cloud account are paired, and the device reboots.
      If the LED(s) indicate an error, check the details of the error in the terminal window.
      The device must be power-cycled to restart the pairing procedure.
   #. After reboot, the board connects to the nRF Cloud.
#. Observe that the LED(s) indicate that the connection is established.
#. Observe that the device count on your nRF Cloud dashboard is incremented by one.
#. Select the device from your device list on nRF Cloud, and observe that sensor data and modem information is received from the board.
#. Press Button 1 (SW3 on Thingy:91) to send BUTTON data to the nRF Cloud.
#. Press Button 1 (SW3 on Thingy:91) for a minimum of 10 seconds to enable GPS tracking.
   The board must be outdoors in clear space for a few minutes to get the first position fix.
#. Optionally send AT commands from the terminal, and observe that the response is received.


Dependencies
************

This application uses the following |NCS| libraries and drivers:

    * :ref:`lib_nrf_cloud`
    * :ref:`modem_info_readme`
    * :ref:`at_cmd_parser_readme`
    * ``drivers/nrf9160_gps``
    * ``lib/bsd_lib``
    * ``drivers/sensor/sensor_sim``
    * :ref:`dk_buttons_and_leds_readme`
    * ``drivers/lte_link_control``

In addition, it uses the Secure Partition Manager sample:

* :ref:`secure_partition_manager`
