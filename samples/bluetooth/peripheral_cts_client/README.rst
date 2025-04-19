.. _peripheral_cts_client:

Bluetooth: Peripheral CTS client
################################

.. contents::
   :local:
   :depth: 2

The Peripheral CTS client sample demonstrates how to use the :ref:`cts_client_readme`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires a device running a CTS Server to connect with (for example, a Bluetooth® Low Energy dongle and the `Bluetooth Low Energy app`_).

Overview
********

The CTS client sample implements a Current Time Service client.
It uses the Current Time Service to read the current time.
The time received is printed on the UART.

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 2:
         Lit when connected.

      Button 1:
         Read the current time.

   .. group-tab:: nRF54 DKs

      LED 0:
         Blinks, toggling on/off every second, when the main loop is running and the device is advertising.

      LED 1:
         Lit when connected.

      Button 0:
         Read the current time.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_cts_client`

.. include:: /includes/build_and_run_ns.txt

.. _peripheral_cts_client_testing:

Testing
=======

After programming the sample to your development kit, you can test it with the `Bluetooth Low Energy app`_ by performing the following steps.

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. |connect_terminal_specific|
      #. Reset the kit.
      #. Start `nRF Connect for Desktop`_
      #. Open the `Bluetooth Low Energy app`_ and select the connected dongle that is used for communication.
      #. Open the :guilabel:`SERVER SETUP` tab.
         Click the dongle configuration and select **Load setup**.
         Load the :file:`cts_central.ncs` file that is located under :file:`samples/bluetooth/peripheral_cts_client` in the |NCS| folder structure.
      #. Click :guilabel:`Apply to device`.
      #. Open the :guilabel:`CONNECTION MAP` tab.
         Click the dongle configuration and select **Security parameters**.
         Check :guilabel:`Perform Bonding` and :guilabel:`Enable LE Secure Connection pairing`, and click :guilabel:`Apply`.
      #. Set the value of **Current Time Service** > **Current Time** to ``C2 07 0B 0F 0C 22 38 06 80 02`` and click :guilabel:`Write`.
      #. Connect to the device from the app. The device is advertising as "Nordic_CTS".
      #. Wait until the bond is established. Verify that the UART data is received as follows::

            Connected xx:xx:xx:xx:xx:xx (random)
            The discovery procedure succeeded
            Security changed: xx:xx:xx:xx:xx:xx (random) level 2
            Pairing completed: xx:xx:xx:xx:xx:xx (random), bonded: 1

      #. Press **Button 1** on the kit.
         Verify that the current time printed on the UART matches the time that was input in the Current Time characteristic (UUID 0x2A2B)::

            Current Time:

            Date:
               Day of week   Saturday
               Day of month  15
               Month of year November
               Year          1986

            Time:
               Hours     12
               Minutes   34
               Seconds   56
               Fractions 128/256 of a second

            Adjust Reason:
               Daylight savings 0
               Time zone        0
               External update  1
               Manual update    0

      #. Change the value of :guilabel:`Current Time Service` > :guilabel:`Current Time` to ``C2 07 0B 0F 0D 25 2A 06 FE 08``. It generates a notification. Verify that the current time printed on the UART matches the time that was input::

            Current Time:

            Date:
               Day of week   Saturday
               Day of month  15
               Month of year November
               Year          1986

            Time:
               Hours     13
               Minutes   37
               Seconds   42
               Fractions 254/256 of a second

            Adjust Reason:
               Daylight savings 1
               Time zone        0
               External update  0
               Manual update    0

      #. Disconnect the device in the app.
      #. As bond information is preserved by the app, you can immediately reconnect to the device by clicking the :guilabel:`Connect` button again.

   .. group-tab:: nRF54 DKs

      1. |connect_terminal_specific|
      #. Reset the kit.
      #. Start `nRF Connect for Desktop`_
      #. Open the `Bluetooth Low Energy app`_ and select the connected dongle that is used for communication.
      #. Open the :guilabel:`SERVER SETUP` tab.
         Click the dongle configuration and select **Load setup**.
         Load the :file:`cts_central.ncs` file that is located under :file:`samples/bluetooth/peripheral_cts_client` in the |NCS| folder structure.
      #. Click :guilabel:`Apply to device`.
      #. Open the :guilabel:`CONNECTION MAP` tab.
         Click the dongle configuration and select **Security parameters**.
         Check :guilabel:`Perform Bonding` and :guilabel:`Enable LE Secure Connection pairing`, and click :guilabel:`Apply`.
      #. Set the value of **Current Time Service** > **Current Time** to ``C2 07 0B 0F 0C 22 38 06 80 02`` and click :guilabel:`Write`.
      #. Connect to the device from the app. The device is advertising as "Nordic_CTS".
      #. Wait until the bond is established. Verify that the UART data is received as follows::

            Connected xx:xx:xx:xx:xx:xx (random)
            The discovery procedure succeeded
            Security changed: xx:xx:xx:xx:xx:xx (random) level 2
            Pairing completed: xx:xx:xx:xx:xx:xx (random), bonded: 1

      #. Press **Button 0** on the kit.
         Verify that the current time printed on the UART matches the time that was input in the Current Time characteristic (UUID 0x2A2B)::

            Current Time:

            Date:
               Day of week   Saturday
               Day of month  15
               Month of year November
               Year          1986

            Time:
               Hours     12
               Minutes   34
               Seconds   56
               Fractions 128/256 of a second

            Adjust Reason:
               Daylight savings 0
               Time zone        0
               External update  1
               Manual update    0

      #. Change the value of :guilabel:`Current Time Service` > :guilabel:`Current Time` to ``C2 07 0B 0F 0D 25 2A 06 FE 08``. It generates a notification. Verify that the current time printed on the UART matches the time that was input::

            Current Time:

            Date:
               Day of week   Saturday
               Day of month  15
               Month of year November
               Year          1986

            Time:
               Hours     13
               Minutes   37
               Seconds   42
               Fractions 254/256 of a second

            Adjust Reason:
               Daylight savings 1
               Time zone        0
               External update  0
               Manual update    0

      #. Disconnect the device in the app.
      #. As bond information is preserved by the app, you can immediately reconnect to the device by clicking the :guilabel:`Connect` button again.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`cts_client_readme`
* :ref:`gatt_dm_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`lib/libc/minimal/include/errno.h`
* :file:`include/sys/printk.h`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/gatt.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
