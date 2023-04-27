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

.. include:: /libraries/modem/nrf_modem_lib.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

User interface
**************

When the connection is established, set switch 2 to **N.C.** to send GNSS data to nRF Cloud, if available.

On the nRF9160 DK, the LEDs display the following information regarding the application state:

**LED 3** and **LED 4**:
    * **LED 3** blinking: The device is connecting to the LTE network.
    * **LED 3** lit: The device is connected to the LTE network.
    * **LED 4** blinking: The device is connecting to nRF Cloud.
    * **LED 3** and **LED 4** blinking: The MQTT connection has been established and the user association procedure with nRF Cloud has been initiated.
    * **LED 4** lit: The device is connected and ready for sensor data transfer.

    .. figure:: /images/nrf_cloud_led_states.svg
       :alt: Application state indicated by LEDs

All LEDs (1-4):
    * Blinking in groups of two (**LED 1** and **LED 3**, **LED 2** and **LED 4**): Modem fault.
    * Blinking in a cross pattern (**LED 1** and **LED 4**, **LED 2** and **LED 3**): Communication error with nRF Cloud.
    * Blinking in groups of two (**LED 1** and **LED 2**, **LED 3** and **LED 4**): Other error.


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

   .. note::
      To build the sample successfully, you must specify the board version along with the build target.
      The board version is printed on the label of your DK, just below the PCA number.
      For example, for board version 1.1.0, the sample must be built in the following way:

      .. parsed-literal::
         :class: highlight

         west build --board nrf9160dk_nrf52840@1.1.0

#. Verify that the programming was successful.
   Use a terminal emulator, like PuTTY, to connect to the second serial port and check the output.
   See :ref:`putty` for the required settings.

After programming the board controller, you must program the main controller with the LTE Sensor Gateway sample.
Program the main controller as follows:

1. Set the **SW10** switch, marked as *debug/prog*, in the **NRF91** position.
   On nRF9160 DK board version 0.9.0 and earlier versions, the switch was called **SW5**.
#. Build the LTE Sensor Gateway sample (this sample) for the nrf9160dk_nrf9160_ns build target and program the main controller with it.

   .. note::
      To build the sample successfully, you must specify the board version along with the build target.
      For example, for board version 1.1.0, the sample must be built in the following way:

      .. parsed-literal::
         :class: highlight

         west build --board nrf9160dk_nrf9160_ns@1.1.0

#. Verify that the programming was successful.
   To do so, use a terminal emulator, like PuTTY, to connect to the first serial port and check the output.
   See :ref:`putty` for the required settings.

Testing
=======

After programming the main controller with the sample, test it by performing the following steps:

1. Power on your Thingy:52 and observe that it starts blinking blue.
#. Open a web browser and navigate to https://nrfcloud.com/.
   Follow the instructions to set up your account and to add an LTE device.
   See :ref:`creating_cloud_account` for more information.
#. Power on or reset the DK.
#. Observe in the terminal window connected to the first serial port that the DK starts up.
   This is indicated by an output similar to the following line:

   .. code-block:: console

      ***** Booting Zephyr OS build v3.0.99-ncs1-9-g8ffc2ab25eaa *****

#. Observe that the message ``LTE Sensor Gateway sample started`` is shown in the terminal window, to ensure that the application has started.
#. The nRF9160 DK now connects to the network. This might take several minutes.
#. Observe that **LED 3** starts blinking as the connection to nRF Cloud is established.
#. The first time you start the sample the device will be paired to your account.
#. Observe that **LED 4** is turned on to indicate that the connection to nRF Cloud is established.
#. In the nRF Cloud portal, select :guilabel:`Device Management` in the left pane and select :guilabel:`Devices`.
#. Observe that the device is shown as connected in the Devices screen.
#. Set **Switch 2** in the position marked as **N.C.**.
   If a GNSS position fix is acquired, GNSS data is now added to the sensor data.
#. Make sure that the Thingy:52 has established a connection to the application.
   This is indicated by its led blinking green.
#. Select the device from your device list in the nRF Cloud portal.
#. Flip the Thingy:52, with the USB port pointing upward, to trigger the sending of the sensor data to nRF Cloud.
#. Observe that the sensor data is received from the DK to the nRF Cloud.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`lte_lc_readme`
* :ref:`uart_nrf_sw_lpuart`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr library:

* :ref:`zephyr:bluetooth_api`

It also uses the following sample:

* :ref:`bluetooth-hci-lpuart-sample`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
