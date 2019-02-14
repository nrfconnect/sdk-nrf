.. _asset_tracker:

nRF9160: Asset Tracker
######################

The Asset Tracker demonstrates how to use the :ref:`lib_nrf_cloud` to connect an nRF9160 DK to the `nRF Cloud`_ via LTE and transmit GPS and sensor data.


Overview
********

The application sends GPS position, temperature, and accelerometer data that is collected by an nRF9160 DK to Nordic Semiconductor's cloud solution, `nRF Cloud`_, where the data is visualized.
The accelerometer data is used to infer the device's physical orientation, which is sent as the nRF Cloud sensor data type "FLIP".
Temperature data is sent as the nRF Cloud sensor type "TEMP".

By default, the application uses simulated sensor and GPS data, but it can be configured with Kconfig options to use real sensors to collect data.
The LTE link control driver is used to establish the LTE link automatically.
When the device is connected to the nRF Cloud, you can use the LTE Link Monitor to send AT commands and receive the response.


Requirements
************

* The following development board:

    * nRF9160 DK board (PCA10090)

* :ref:`secure_boot` must be programmed on the board.

  The applications is configured to compile and run as a non-secure application on nRF91's Cortex-M33.
  Therefore, it requires the :ref:`secure_boot` bootloader that prepares the required peripherals to be available for the application.

.. _asset_tracker_user_interface:

User interface
**************

The two buttons and two switches are used to enter a pairing pattern to associate a specific board with an nRF Cloud user account.
In addition, the two switches have the following function when the connection is established:

Switch 1:
    * Toggle to simulate flipping of the boards orientation.

Switch 2:
    * Set to **N.C.** to send simulated GPS data to the nRF Cloud once every 2 seconds.

The application state is indicated by the LEDs.

LED 3 and LED 4:
    * LED 3 blinking: Connecting - The device is resolving DNS and connecting to the nRF Cloud.
    * LED 3 and LED 4 blinking: Pairing started - The MQTT connection has been established and the pairing procedure towards nRF Cloud has been initiated.
    * LED 3 ON and LED 4 blinking: Pattern entry - The user has started entering the pairing pattern.
    * LED 4 blinking: Pattern sent - Pattern has been entered and sent to nRF Cloud for verification.
    * LED 4 ON: Connected - The device is ready for sensor data transfer.

    .. figure:: ../../doc/nrf/images/nrf_cloud_led_states.svg
       :alt: Application state indicated by LEDs

    Application state indicated by LEDs

All LEDs (1-4):
    * Blinking simultaneously: Irrecoverable error in the BSD library.
    * Blinking in groups of two (LED 1 and 3, LED 2 and 4): Recoverable error in the BSD library.
    * Blinking in cross pattern (LED 1 and 4, LED 2 and 3): Communication error with the nRF Cloud.

Building and running
********************

This application can be found under :file:`applications/asset_tracker` in the |NCS| folder structure.

The application is built as a non-secure firmware image for the nrf9160_pca10090ns board.
It can be programmed independently from the secure boot firmware.

See :ref:`gs_programming` for information about how to build and program the application.
The Kconfig file of the application contains options to configure the application.
For example, configure ``CONFIG_POWER_OPTIMIZATION_ENABLE`` to enable power optimization or ``CONFIG_TEMP_USE_EXTERNAL`` to use an external temperature sensor instead of simulated temperature data.
In |SES|, select **Project** > **Configure nRF Connect SDK project** to browse and configure these options.
Alternatively, use the command line tool ``menuconfig`` or configure the options directly in ``prj.conf``.

.. _power_opt:

Power optimization
******************

The Asset Tracker can run in three power modes that are configured in the Kconfig file of the application.

Demo mode
	This is the default setting.
	In this mode, the device sends GPS data every 2 seconds.
	To enable this mode, set ``CONFIG_POWER_OPTIMIZATION_ENABLE=n``.

Request eDRX mode
	In this mode, the device sends GPS data every 2 minutes.
	To enable this mode, set ``CONFIG_POWER_OPTIMIZATION_ENABLE=y`` and then
	set Switch 2 to ON.

Request Power Saving Mode (PSM)
	To enable PSM, set ``CONFIG_POWER_OPTIMIZATION_ENABLE=y`` and then
	set Switch 2 to OFF.

Testing
=======

After programming the :ref:`secure_boot` sample to your board, test the Asset Tracker application by performing the following steps:

1. Connect the board to the computer using a USB cable.
   The board is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. Connect to the board with a terminal emulator, for example, LTE Link Monitor.
#. Open a web browser and navigate to https://nrfcloud.com/.
   Follow the instructions to set up your account and add an LTE device.
   A pattern of switch and button actions is displayed.
#. Make sure that the :ref:`secure_boot` sample is programmed to the board.
#. Compile and program the Asset Tracker application to the board.
#. Observe in the terminal window that the board starts up in secure boot and that the application starts.
   This is indicated by output similar to the following lines::

      Secure Boot: prepare to jump to Non-Secure image
      ***** Booting Zephyr OS v1.13.99 *****
      Application started

#. Observe that LED 3 starts blinking as the LTE link is established. This may take several minutes.
#. Observe in the terminal window that connection to nRF Cloud is established.
#. The first time you start the application, pair the device to your account:

   a. Observe that both LED 3 and 4 start blinking, indicating that the pairing procedure has been initiated.
   #. Follow the instructions on `nRF Cloud`_ and enter the displayed pattern.
      In the terminal window, you can see the pattern that you have entered.
   #. If the pattern is entered correctly, the board and your nRF Cloud account are paired and the device reboots.
      If the LEDs start blinking in pairs, check in the terminal window which error occurred.
      The device must be power-cycled to restart the pairing procedure.
   #. After reboot, the board connects to the nRF Cloud, and the pattern disappears from the web page.
#. Observe that LED 4 is turned on to indicate that the connection is established.
#. Observe that the device count on your nRF Cloud dashboard is incremented by one.
#. Select the device from your device list on nRF Cloud, and observe that sensor data is received from the board.
#. Toggle switch 1 to simulate flipping the board orientation.
#. Set switch 2 in the position marked **N.C.** and observe that simulated GPS data is sent to the nRF Cloud.
#. Optionally send AT commands from the terminal, and observe that the reponse is received.


Dependencies
************

This application uses the following |NCS| libraries and drivers:

    * :ref:`lib_nrf_cloud`
    * ``drivers/gps_sim``
    * ``lib/bsd_lib``
    * ``drivers/sensor/sensor_sim``
    * :ref:`dk_buttons_and_leds_readme`
    * ``drivers/lte_link_control``

In addition, it uses the secure boot sample:

* :ref:`secure_boot`
