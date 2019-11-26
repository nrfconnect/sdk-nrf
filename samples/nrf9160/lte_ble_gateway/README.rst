.. _lte_sensor_gateway:

nRF9160: LTE Sensor Gateway
###########################

The LTE Sensor Gateway sample demonstrates how to transmit sensor data from an nRF9160 DK to the `nRF Cloud`_.
Unlike the :ref:`asset_tracker` sample, the sensor data is collected via Bluetooth LE.
Therefore, this sample acts as a gateway between Bluetooth LE and the LTE connection to the nRF Cloud.

Overview
*********

The sample connects via Bluetooth LE to a Thingy:52 that is running the factory pre-loaded application.
It then starts collecting data from two sensors:

 * The flip state of the Thingy:52
 * Simulated GPS position data

The data from both sensors is aggregated in memory.
Flipping the Thingy:52, which causes a change in the flip state to "UPSIDE_DOWN", triggers an alarm that sends the aggregated data over LTE to the `nRF Cloud`_.

Requirements
************

* A `Nordic Thingy:52`_
* The following development board:

  * |nRF9160DK|

* :ref:`zephyr:bluetooth-hci-uart-sample` must be programmed to the nRF52 board controller on the board.
* .. include:: /includes/spm.txt

User interface
**************
The two buttons and two switches are used to enter a pairing pattern to associate a specific board with an nRF Cloud user account.

When the connection is established, set switch 2 to **N.C.** to send simulated GPS data to the nRF Cloud once every 2 seconds.

See :ref:`asset_tracker_user_interface` in the :ref:`asset_tracker` documentation for detailed information about the different LED states used by the sample.


Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/lte_ble_gateway`

.. include:: /includes/build_and_run_nrf9160.txt


Programming the sample
======================

When you connect the nRF9160 DK board to your computer, three virtual serial ports (USB CDC class) should appear.
The first port is connected to the main controller (nRF9160) on the board, while the second port is connected to the board controller (nRF52840).

Before you program the sample application onto the main controller, you must program the :ref:`zephyr:bluetooth-hci-uart-sample` sample onto the board controller:

1. Put the **SW5** switch (marked debug/prog) in the **NRF52** position to program the board controller.
#. Build the :ref:`zephyr:bluetooth-hci-uart-sample` sample for the nrf52840_pca10090 board and program it.
#. Verify that the sample was programmed successfully by connecting to the second serial port with a terminal emulator (for example, PuTTY) and checking the output.
   See :ref:`putty` for the required settings.

After programming the board controller, you must program the LTE Sensor Gateway sample (which includes the :ref:`secure_partition_manager` sample) to the main controller:

1. Put the **SW5** switch (marked debug/prog) in the **NRF91** position to program the main controller.
#. Build the LTE Sensor Gateway sample (this sample) for the nrf9160_pca10090ns board and program it.
#. Verify that the sample was programmed successfully by connecting to the first serial port with a terminal emulator (for example, PuTTY) and checking the output.
   See :ref:`putty` for the required settings.

Testing
=======

After programming the sample and all prerequisites to the board, test it by performing the following steps:

1. Power on your Thingy:52 and observe that it starts blinking blue.
#. Open a web browser and navigate to https://nrfcloud.com/.
   Follow the instructions to set up your account and add an LTE device.
   A pattern of switch and button actions is displayed.
#. Power on or reset the board.
#. Observe in the terminal window connected to the first serial port that the board starts up in the Secure Partition Manager and that the application starts.
   This is indicated by output similar to the following lines::

      SPM: prepare to jump to Non-Secure image
      ***** Booting Zephyr OS v1.13.99 *****

#. Observe that "Application started" is printed to the terminal window after the LTE link is established.
   This might take several minutes.
#. Observe that LED 3 starts blinking as the connection to nRF Cloud is established.
#. The first time you start the sample, pair the device to your account:

   a. Observe that both LED 3 and 4 start blinking, indicating that the pairing procedure has been initiated.
   #. Follow the instructions on `nRF Cloud`_ and enter the displayed pattern.
      In the terminal window, you can see the pattern that you have entered.
   #. If the pattern is entered correctly, the board and your nRF Cloud account are paired and the device reboots.
      If the LEDs start blinking in pairs, check in the terminal window which error occurred.
      The device must be power-cycled to restart the pairing procedure.
   #. After reboot, the board connects to the nRF Cloud, and the pattern disappears from the web page.
#. Observe that LED 4 is turned on to indicate that the connection is established.
#. Observe that the device count on your nRF Cloud dashboard is incremented by one.
#. Set switch 2 in the position marked **N.C.** and observe that simulated GPS data is sent to the nRF Cloud.
#. Make sure that the Thingy:52 has established a connection to the application.
   This is indicated by it blinking green.
#. Flip the Thingy:52 (so that the USB port points upward) to trigger sending the sensor data to the nRF Cloud.
#. Select the device from your device list on nRF Cloud, and observe that sensor data is received from the board.
#. Observe that the data is updated in nRF Cloud.


Dependencies
************

This sample uses the following libraries:

From |NCS|
  * :ref:`lib_nrf_cloud`
  * ``drivers/gps_sim``
  * ``lib/bsd_lib``
  * ``drivers/sensor/sensor_sim``
  * :ref:`dk_buttons_and_leds_readme`
  * ``drivers/lte_link_control``

From Zephyr
  * :ref:`zephyr:bluetooth_api`

In addition, it uses the following samples:

From |NCS|
  * :ref:`secure_partition_manager`

From Zephyr
  * :ref:`zephyr:bluetooth-hci-uart-sample`
