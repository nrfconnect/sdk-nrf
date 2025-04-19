.. _central_and_peripheral_hrs:

Bluetooth: Central and Peripheral HRS
#####################################

.. contents::
   :local:
   :depth: 2

The Central and Peripheral HRS sample demonstrates how to use Bluetooth® with Central and Peripheral roles concurrently.
It also demonstrates how to use the :ref:`lib_hrs_client_readme` library.
It uses the HRS Client to retrieve heart rate measurement data from a remote device that provides a Heart Rate service.
It relays this data to another remote device that provides a Heart Rate Service client implementation.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

To test just the Bluetooth LE Central Role operation, you need one of the following setups:

  * A smartphone or a tablet running a compatible application.
  * Another development kit running the :zephyr:code-sample:`ble_peripheral_hr` sample.
    See the documentation for that sample for detailed instructions.

To test the Relay mode operation, you need one of the following setups:

  * A smartphone or a tablet running a compatible application.
  * Two additional development kits running :zephyr:code-sample:`ble_central_hr` and :zephyr:code-sample:`ble_peripheral_hr` samples.

You can also mix devices when testing this sample.
For a simple echo test, you only need one additional device.
Alternatively, you can use a smartphone providing the HRS functionality and a development kit running the :zephyr:code-sample:`ble_central_hr` sample.

For testing, you can also use the `Bluetooth Low Energy app`_.

Overview
********

The sample demonstrates the following Bluetooth LE roles:

  * Central role - Scans for a remote device providing Heart Rate Service.
  * Peripheral role - Advertises and exposes a Heart Rate Service.

When a device is connected as central, the sample starts the service discovery procedure to search for the Heart Rate Service.
If this succeeds, the sample reads the Body Sensor Location characteristic and subscribes to the Heart Rate Measurement characteristic to receive notifications.
When connected also as peripheral to the device acting as a Heart Rate Service client, the sample starts working as relay.
It collects data from a remote device with Heart Rate Service that is sending notifications and sends this data to another remote device providing a Heart Rate Service client.

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 2:
         Lit when the development kit is connected as central.

      LED 3:
         Lit when the development kit is connected as peripheral.

   .. group-tab:: nRF54 DKs

      LED 0:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 1:
         Lit when the development kit is connected as central.

      LED 2:
         Lit when the development kit is connected as peripheral.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/central_and_peripheral_hr`

.. include:: /includes/build_and_run_ns.txt

.. include:: /includes/nRF54H20_erase_UICR.txt

.. _central_and_peripheral_hrs_testing:

Testing
=======

After programming the sample to your development kit, test it either by connecting to other development kits that are running the :zephyr:code-sample:`ble_peripheral_hr` sample, or by using the `Bluetooth Low Energy app`_ from the `nRF Connect for Desktop`_, which emulates an HRS server.

Testing with other development kits
-----------------------------------

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. |connect_terminal_specific|
      #. Program the other development kit with the :zephyr:code-sample:`ble_peripheral_hr` sample and reset it.
      #. Wait until the HRS is detected by the central.
         Observe that **LED 2** is on.
      #. In the terminal window, check for information similar to the following::

            Heart Rate Sensor body location: Chest
            Heart Rate Measurement notification received:

                  Heart Rate Measurement Value Format: 8 - bit
                  Sensor Contact detected: 1
                  Sensor Contact supported: 1
                  Energy Expended present: 0
                  RR-Intervals present: 0

                  Heart Rate Measurement Value: 134 bpm

         Notifications will be displayed periodically with a frequency determined by the HR server.

      #. Program another development kit with the :zephyr:code-sample:`ble_central_hr` sample and reset it.
      #. Wait until central is connected to your development kit.
         Observe that **LED 3** is lit.
      #. In terminal windows connected to device with the :zephyr:code-sample:`ble_central_hr` sample, check for information similar to following::

            [NOTIFICATION] data 0x20006779 length 2

      The sample works now as relay for the Heart Rate Service.

   .. group-tab:: nRF54 DKs

      1. |connect_terminal_specific|
      #. Program the other development kit with the :zephyr:code-sample:`ble_peripheral_hr` sample and reset it.
      #. Wait until the HRS is detected by the central.
         Observe that **LED 1** is on.
      #. In the terminal window, check for information similar to the following::

            Heart Rate Sensor body location: Chest
            Heart Rate Measurement notification received:

                  Heart Rate Measurement Value Format: 8 - bit
                  Sensor Contact detected: 1
                  Sensor Contact supported: 1
                  Energy Expended present: 0
                  RR-Intervals present: 0

                  Heart Rate Measurement Value: 134 bpm

         Notifications will be displayed periodically with a frequency determined by the HR server.

      #. Program another development kit with the :zephyr:code-sample:`ble_central_hr` sample and reset it.
      #. Wait until central is connected to your development kit.
         Observe that **LED 2** is lit.
      #. In terminal windows connected to device with the :zephyr:code-sample:`ble_central_hr` sample, check for information similar to following::

            [NOTIFICATION] data 0x20006779 length 2

      The sample works now as relay for the Heart Rate Service.

Testing with Bluetooth Low Energy app
-------------------------------------

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. |connect_terminal_specific|
      #. Reset the development kit.
      #. Start `nRF Connect for Desktop`_.
      #. Open the `Bluetooth Low Energy app`_ and select the connected dongle that is used for communication.
      #. Open the **SERVER SETUP** tab.
      #. Click the dongle configuration and select :guilabel:`Load setup`.
      #. Load the :file:`hr_service.ncs` file that is located under :file:`samples/bluetooth/central_and_peripheral_hr` in the |NCS| folder structure.
      #. Click :guilabel:`Apply to device`.
      #. Open the **CONNECTION MAP** tab.
      #. Click the dongle configuration (gear icon) and select :guilabel:`Advertising setup`.

         The current version of nRF Connect can store the advertising setup.

      #. Click :guilabel:`Load setup`.
         Load the :file:`hrs_adv_setup.ncs` file that is located under :file:`samples/bluetooth/central_and_peripheral_hr` in the |NCS| folder structure.
      #. Click :guilabel:`Apply` and :guilabel:`Close`.
      #. Click the gear icon to open the Adapter settings and select :guilabel:`Start advertising`.
      #. Wait until the development kit running the Central and Peripheral HRS connects.
         Observe that **LED 2** is lit.
      #. To explore the Heart Rate Measurement characteristic, complete the following steps:

         a. Write value ``06 80`` and click the :guilabel:`Play` button to send a notification.
            In the terminal window, check for information similar to the following::

               Heart Rate Sensor body location: Chest
               Heart Rate Measurement notification received:

                     Heart Rate Measurement Value Format: 8 - bit
                     Sensor Contact detected: 1
                     Sensor Contact supported: 1
                     Energy Expended present: 0
                     RR-Intervals present: 0

                     Heart Rate Measurement Value: 128 bpm

            The `Bluetooth Low Energy app`_ also detects the Central and Peripheral HRS sample Heart Rate Service.

         #. Enable the notification for the Heart Rate Measurement characteristic.
         #. Write again value ``06 80`` and click the :guilabel:`Play` button to send a notification.

            The same value appears for the Heart Rate Measurement characteristic.

      The sample works now as relay for the Heart Rate Service.

   .. group-tab:: nRF54 DKs

      1. |connect_terminal_specific|
      #. Reset the development kit.
      #. Start `nRF Connect for Desktop`_.
      #. Open the `Bluetooth Low Energy app`_ and select the connected dongle that is used for communication.
      #. Open the **SERVER SETUP** tab.
      #. Click the dongle configuration and select :guilabel:`Load setup`.
      #. Load the :file:`hr_service.ncs` file that is located under :file:`samples/bluetooth/central_and_peripheral_hr` in the |NCS| folder structure.
      #. Click :guilabel:`Apply to device`.
      #. Open the **CONNECTION MAP** tab.
      #. Click the dongle configuration (gear icon) and select :guilabel:`Advertising setup`.

         The current version of nRF Connect can store the advertising setup.

      #. Click :guilabel:`Load setup`.
         Load the :file:`hrs_adv_setup.ncs` file that is located under :file:`samples/bluetooth/central_and_peripheral_hr` in the |NCS| folder structure.
      #. Click :guilabel:`Apply` and :guilabel:`Close`.
      #. Click the gear icon to open the Adapter settings and select :guilabel:`Start advertising`.
      #. Wait until the development kit running the Central and Peripheral HRS connects.
         Observe that **LED 1** is lit.
      #. To explore the Heart Rate Measurement characteristic, complete the following steps:

         a. Write value ``06 80`` and click the :guilabel:`Play` button to send a notification.
            In the terminal window, check for information similar to the following::

               Heart Rate Sensor body location: Chest
               Heart Rate Measurement notification received:

                     Heart Rate Measurement Value Format: 8 - bit
                     Sensor Contact detected: 1
                     Sensor Contact supported: 1
                     Energy Expended present: 0
                     RR-Intervals present: 0

                     Heart Rate Measurement Value: 128 bpm

            The `Bluetooth Low Energy app`_ also detects the Central and Peripheral HRS sample Heart Rate Service.

         #. Enable the notification for the Heart Rate Measurement characteristic.
         #. Write again value ``06 80`` and click the :guilabel:`Play` button to send a notification.

            The same value appears for the Heart Rate Measurement characteristic.

      The sample works now as relay for the Heart Rate Service.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_hrs_client_readme`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`gatt_dm_readme`
* :ref:`nrf_bt_scan_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr.h`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/services/hrs.h`
  * :file:`include/bluetooth/services/bas.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
