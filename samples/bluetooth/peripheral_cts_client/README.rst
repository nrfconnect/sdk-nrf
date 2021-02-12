.. _peripheral_cts_client:

Bluetooth: Peripheral CTS client
################################

.. contents::
   :local:
   :depth: 2

The Peripheral CTS client sample demonstrates how to use the :ref:`cts_client_readme`.

Overview
********

The CTS client sample implements a Current Time Service client. It uses the Current Time Service to read the current time. The time received is printed on the UART.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52840dk_nrf52840, nrf52833dk_nrf52833, nrf52dk_nrf52832

The sample also requires a device running a CTS Server to connect with (for example, a Bluetooth Low Energy dongle and nRF Connect for Desktop)

User interface
**************

LED 1:
   * Blinks with a period of 2 seconds, duty cycle 50%, when the main loop is running.

LED 2:
   * On when connected.

Button 1:
   * Read the current time.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/peripheral_cts_client`

.. include:: /includes/build_and_run.txt

.. _peripheral_cts_client_testing:

Testing
=======

After programming the sample to your development kit, you can test it with `nRF Connect for Desktop`_ by performing the following steps.

1. |connect_terminal_specific|
#. Reset the kit.
#. Start `nRF Connect for Desktop`_ and select the connected dongle that is used for communication.
#. Go to the :guilabel:`Server setup` tab.
   Click the dongle configuration and select :guilabel:`Load setup`.
   Load the ``cts_central.ncs`` file that is located under :file:`samples/bluetooth/peripheral_cts_client` in the |NCS| folder structure.
#. Click :guilabel:`Apply to device`.
#. Go to the :guilabel:`Connection Map` tab.
   Click the dongle configuration and select :guilabel:`Security parameters`.
   Check :guilabel:`Perform Bonding`, and click :guilabel:`Apply`.
#. Set the value of :guilabel:`Current Time Service` -> :guilabel:`Current Time` to ``C2 07 0B 0F 0C 22 38 06 80 02`` and click Write.
#. Connect to the device from nRF Connect. The device is advertising as "Nordic_CTS".
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

#. Change the value of :guilabel:`Current Time Service` -> :guilabel:`Current Time` to ``C2 07 0B 0F 0D 25 2A 06 FE 08``. It generates a notification. Verify that the current time printed on the UART matches the time that was input::

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

#. Disconnect the device in nRF Connect.
#. As bond information is preserved by nRF Connect, you can immediately reconnect to the device by clicking the Connect button again.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`cts_client_readme`
* :ref:`gatt_dm_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``lib/libc/minimal/include/errno.h``
* ``include/sys/printk.h``
* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/conn.h``
  * ``include/bluetooth/uuid.h``
  * ``include/bluetooth/gatt.h``
