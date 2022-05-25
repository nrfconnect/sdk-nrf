.. _lte_sensor_gateway:

nRF9160: LTE Sensor Gateway
###########################

.. contents::
   :local:
   :depth: 2

The LTE Sensor Gateway sample demonstrates how to transmit sensor data from an nRF9160 development kit to the `nRF Cloud`_.

The sensor data is collected using Bluetooth® Low Energy.
Therefore, this sample acts as a gateway between the Bluetooth LE and the LTE connections to nRF Cloud.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

The sample also requires a `Nordic Thingy:52`_.

.. include:: /includes/tfm.txt

Overview
*********

The sample connects using Bluetooth LE to a Thingy:52 running the factory preloaded application.
When the connection is established, it starts collecting data from two sensors:

* The flip state of the Thingy:52
* The GNSS position data of the nRF9160 DK

The sample aggregates the data from both sensors in memory.
You can then trigger an alarm that sends the aggregated data over LTE to `nRF Cloud`_ by flipping the Thingy:52, which causes a change in the flip state to ``UPSIDE_DOWN``.

This sample is also supported on the Thingy:91.
However, it must be programmed using a debugger and a 10-pin SWD cable.
Serial communication and firmware updates over serial using MCUboot are not supported in this configuration.

Configuration
*************

|config|

Additional configuration
========================

Check and configure the following library option that is used by the sample:

* :kconfig:option:`CONFIG_MODEM_ANTENNA_GNSS_EXTERNAL` - Selects an external GNSS antenna.

User interface
**************

Two buttons and two switches are used to enter a pairing pattern to associate a specific development kit with an nRF Cloud user account.

When the connection is established, set switch 2 to **N.C.** to send simulated GNSS data to nRF Cloud once every 2 seconds.


Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/lte_ble_gateway`

.. include:: /includes/build_and_run_ns.txt


Programming the sample
======================

When you connect the nRF9160 development kit to your computer, three virtual serial ports of the USB CDC class should become available:

* The first port is connected to the *main controller* on the nRF9160 development kit, the nRF9160.
* The second port is connected to the *board controller* on the nRF9160 development kit, the nRF52840.

You must program the *board controller* with the :ref:`bluetooth-hci-lpuart-sample` sample first, before programming the main controller with the LTE Sensor Gateway sample application.
Program the board controller as follows:

1. Set the **SW10** switch, marked as *debug/prog*, in the **NRF52** position.
   On nRF9160 DK board version 0.9.0 and earlier versions, the switch was called **SW5**.
#. Build the :ref:`bluetooth-hci-lpuart-sample` sample for the nrf9160dk_nrf52840 build target and program the board controller with it.
#. Verify that the programming was successful.
   Use a terminal emulator, like PuTTY, to connect to the second serial port and check the output.
   See :ref:`putty` for the required settings.

After programming the board controller, you must program the main controller with the LTE Sensor Gateway sample, which also includes the :ref:`secure_partition_manager` sample.
Program the main controller as follows:

1. Set the **SW10** switch, marked as *debug/prog*, in the **NRF91** position.
   On nRF9160 DK board version 0.9.0 and earlier versions, the switch was called **SW5**.
#. Build the LTE Sensor Gateway sample (this sample) for the nrf9160dk_nrf9160_ns build target and program the main controller with it.
#. Verify that the program was successful.
   To do so, use a terminal emulator, like PuTTY, to connect to the first serial port and check the output.
   See :ref:`putty` for the required settings.

Testing
=======

After programming the main controller with the sample, test it by performing the following steps:

1. Power on your Thingy:52 and observe that it starts blinking blue.
#. Open a web browser and navigate to https://nrfcloud.com/.
   Follow the instructions to set up your account and to add an LTE device.
   A pattern of switch and button actions is displayed on the webpage.
#. Power on or reset the kit.
#. Observe in the terminal window connected to the first serial port that the kit starts up in Trusted Firmware-M.
   This is indicated by an output similar to the following lines:

   .. code-block:: console

      SPM: prepare to jump to Non-Secure image
      ***** Booting Zephyr OS v1.13.99 *****

#. Observe that the message ``LTE Sensor Gateway sample started`` is shown in the terminal window, to ensure that the application started.
#. The nRF9160 DK now connects to the network. This might take several minutes.
#. Observe that LED 3 starts blinking as the connection to nRF Cloud is established.
#. The first time you start the sample, pair the device to your account:

   a. Observe that both LED 3 and 4 start blinking, indicating that the pairing procedure has been initiated.
   #. Follow the instructions on `nRF Cloud`_ and enter the displayed pattern.
      In the terminal window, you can see the pattern that you have entered.
   #. If the pattern is entered correctly, the kit and your nRF Cloud account are paired and the device reboots.
      If the LEDs start blinking in pairs, check in the terminal window which error occurred.
      The device must be power-cycled to restart the pairing procedure.
   #. After reboot, the kit connects to nRF Cloud, and the pattern disappears from the web page.
#. Observe that LED 4 is turned on to indicate that the connection is established.
#. Observe that the device count on your nRF Cloud dashboard is incremented by one.
#. Set switch 2 in the position marked as **N.C.** and observe that GNSS data is sent to nRF Cloud.
#. Make sure that the Thingy:52 has established a connection to the application.
   This is indicated by its led blinking green.
#. Flip the Thingy:52, with the USB port pointing upward, to trigger the sending of the sensor data to nRF Cloud.
#. Select the device from your device list on nRF Cloud, and observe that the sensor data is received from the kit.
#. Observe that the data is updated in nRF Cloud.


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
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
