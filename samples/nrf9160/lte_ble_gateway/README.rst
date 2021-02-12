.. _lte_sensor_gateway:

nRF9160: LTE Sensor Gateway
###########################

.. contents::
   :local:
   :depth: 2

The LTE Sensor Gateway sample demonstrates how to transmit sensor data from an nRF9160 development kit to the `nRF Connect for Cloud`_.

The sensor data is collected via Bluetooth LE, unlike the :ref:`asset_tracker` sample.
Therefore, this sample acts as a gateway between the Bluetooth LE and the LTE connections to nRF Connect for Cloud.

Overview
*********

The sample connects via Bluetooth LE to a Thingy:52 running the factory pre-loaded application.
When the connection is established, it starts collecting data from two sensors:

* The flip state of the Thingy:52
* The simulated GPS position data

The sample aggregates the data from both sensors in memory.
You can then trigger an alarm that sends the aggregated data over LTE to `nRF Connect for Cloud`_ by flipping the Thingy:52, which causes a change in the flip state to ``UPSIDE_DOWN``.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns

The sample also requires a `Nordic Thingy:52`_.

.. include:: /includes/spm.txt

User interface
**************

Two buttons and two switches are used to enter a pairing pattern to associate a specific development kit with an nRF Connect for Cloud user account.

When the connection is established, set switch 2 to **N.C.** to send simulated GPS data to nRF Connect for Cloud once every 2 seconds.

See the :ref:`asset_tracker_user_interface` in the :ref:`asset_tracker` documentation for detailed information about the different LED states used by the sample.


Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/lte_ble_gateway`

.. include:: /includes/build_and_run_nrf9160.txt


Programming the sample
======================

When you connect the nRF9160 development kit to your computer, three virtual serial ports of the USB CDC class should become available:

* The first port is connected to the *main controller* on the development kit, the nRF9160.
* The second port is connected to the *board controller* on the development kit, the nRF52840.

You must program the *board controller* with the :ref:`bluetooth-hci-lpuart-sample` sample first, before programming the main controller with the LTE Sensor Gateway sample application.
You can program the board controller as follows:

1. Set the **SW5** switch, marked as *debug/prog*, in the **NRF52** position.
#. Build the :ref:`bluetooth-hci-lpuart-sample` sample for the nrf9160dk_nrf52840 build target and program the board controller with it.
#. Verify that the programming was successful.
   To do so, use a terminal emulator, like PuTTY, to connect to the second serial port and check the output.
   See :ref:`putty` for the required settings.

After programming the board controller, you must program the main controller with the LTE Sensor Gateway sample, which also includes the :ref:`secure_partition_manager` sample.
You can program the main controller as follows:

1. Set the **SW5** switch, marked as *debug/prog*, in the **NRF91** position.
#. Build the LTE Sensor Gateway sample (this sample) for the nrf9160dk_nrf9160ns build target and program the main controller with it.
#. Verify that the program was successful.
   To do so, use a terminal emulator, like PuTTY, to connect to the first serial port and check the output.
   See :ref:`putty` for the required settings.

Testing
=======

After programming the main controller with the sample, you can test it as follows:

1. Power on your Thingy:52 and observe that it starts blinking blue.
#. Open a web browser and navigate to https://nrfcloud.com/.
   Follow the instructions to set up your account and to add an LTE device.
   A pattern of switch and button actions is displayed on the webpage.
#. Power on or reset the kit.
#. Observe in the terminal window connected to the first serial port that the kit starts up in the Secure Partition Manager.
   This is indicated by an output similar to the following lines:

   .. code-block:: console

      SPM: prepare to jump to Non-Secure image
      ***** Booting Zephyr OS v1.13.99 *****

#. Observe that the message ``Application started`` is shown in the terminal window after the LTE link is established, to ensure that the application started.
   This might take several minutes.
#. Observe that LED 3 starts blinking as the connection to nRF Connect for Cloud is established.
#. The first time you start the sample, pair the device to your account:

   a. Observe that both LED 3 and 4 start blinking, indicating that the pairing procedure has been initiated.
   #. Follow the instructions on `nRF Connect for Cloud`_ and enter the displayed pattern.
      In the terminal window, you can see the pattern that you have entered.
   #. If the pattern is entered correctly, the kit and your nRF Connect for Cloud account are paired and the device reboots.
      If the LEDs start blinking in pairs, check in the terminal window which error occurred.
      The device must be power-cycled to restart the pairing procedure.
   #. After reboot, the kit connects to nRF Connect for Cloud, and the pattern disappears from the web page.
#. Observe that LED 4 is turned on to indicate that the connection is established.
#. Observe that the device count on your nRF Connect for Cloud dashboard is incremented by one.
#. Set switch 2 in the position marked as **N.C.** and observe that simulated GPS data is sent to nRF Connect for Cloud.
#. Make sure that the Thingy:52 has established a connection to the application.
   This is indicated by its led blinking green.
#. Flip the Thingy:52, with the USB port pointing upward, to trigger the sending of the sensor data to nRF Connect for Cloud.
#. Select the device from your device list on nRF Connect for Cloud, and observe that the sensor data is received from the kit.
#. Observe that the data is updated in nRF Connect for Cloud.


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* ``drivers/gps_sim``
* ``drivers/sensor/sensor_sim``
* :ref:`dk_buttons_and_leds_readme`
* :ref:`lte_lc_readme`
* :ref:`uart_nrf_sw_lpuart`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr library:

* :ref:`zephyr:bluetooth_api`

It also uses the following samples:

* :ref:`secure_partition_manager`
* :ref:`bluetooth-hci-lpuart-sample`
