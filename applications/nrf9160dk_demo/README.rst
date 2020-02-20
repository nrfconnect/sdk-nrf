.. _nrf9160dk_demo:

nRF9160: nRF9160DK Demo
#######################

The nRF9160DK Demo demonstrates how to use the :ref:`lib_nrf_cloud` to connect an nRF9160-based board to the `nRF Cloud`_ via LTE, transmit GPS and sensor data, and retrieve information about the device.


Overview
********

The application uses the LTE link control driver to establish a network connection.
It then transmits a welcome message to Nordic Semiconductor's cloud solution, `nRF Cloud`_, which displays the message in its web interface.

The application will send messages to the cloud when the user presses or releases Button 1 or Button 2, or slides Switch 1 or Switch 2 left or right.

The user may enter a JSON-formatted message in the cloud solution's Terminal.  One use of this is to turn LED1 and LED2 on or off.

By default, the nRF9160DK Demo supports firmware updates through :ref:`lib_aws_fota`.


Requirements
************

* The following development board:

    * |nRF9160DK|

* .. include:: /includes/spm.txt

.. _nrf9160dk_demo_user_interface:

User interface
**************

The buttons and switches have the following functions when the connection is established:

Button 1:
    * Send a BUTTON event to the nRF Cloud with data set to 11 on press and 10 on release, with form ``{"appId":"BUTTON", "messageType":"DATA", "data":"11"}``.

Button 2:
    * Send a BUTTON event with data set to 21 on press and 20 on release.

Switch 1:
    * Send a BUTTON event with data set to 31 on left and 30 on right.

Switch 2:
    * Send a BUTTON event with data set to 41 on left and 40 on right. When moved left, the help text is repeated.

The user can control the state of two LEDs remotely.

LED 1 and LED 2:
    * Send a JSON message in the form ``{"appId":"LED", "messageType":"CFG_SET", "data":{"state":[1,0]}}`` which turns LED 1 on and LED 2 off.

The application state is indicated by the LEDs.

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

Building and running
********************

.. |sample path| replace:: :file:`applications/nrf9160dk_demo`

.. include:: /includes/build_and_run_nrf9160.txt

The Kconfig file of the application contains options to configure the application.
For example, configure ``CONFIG_POWER_OPTIMIZATION_ENABLE`` to enable power optimization or ``CONFIG_TEMP_USE_EXTERNAL`` to use an external temperature sensor instead of simulated temperature data.
In |SES|, select **Project** > **Configure nRF Connect SDK project** to browse and configure these options.
Alternatively, use the command line tool ``menuconfig`` or configure the options directly in ``prj.conf``.

This application supports the |NCS| :ref:`ug_bootloader`, but it is disabled by default.
To enable the immutable bootloader, set ``CONFIG_SECURE_BOOT=y``.

Testing
=======

After programming the application and all prerequisites to your board, test the nRF9160DK Demo application by performing the following steps:

1. Connect the board to the computer using a USB cable.
   The board is assigned a COM port (Windows) or ttyACM device (Linux), which is visible in the Device Manager.
#. Connect to the board with a terminal emulator, for example, LTE Link Monitor.
#. Reset the board.
#. Observe in the terminal window that the board starts up in the Secure Partition Manager and that the application starts.
   This is indicated by output similar to the following lines::

      SPM: prepare to jump to Non-Secure image
      *** Booting Zephyr OS build v2.1.99  ***
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
#. Press Button 1 to send BUTTON data to the nRF Cloud.
#. Optionally send commands from the terminal, and observe that the response is received.


Dependencies
************

This application uses the following |NCS| libraries and drivers:

    * :ref:`lib_nrf_cloud`
    * ``lib/bsd_lib``
    * :ref:`dk_buttons_and_leds_readme`
    * ``drivers/lte_link_control``

In addition, it uses the Secure Partition Manager sample:

* :ref:`secure_partition_manager`
