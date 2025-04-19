.. _bluetooth_central_hids:

Bluetooth: Central HIDS
#######################

.. contents::
   :local:
   :depth: 2

The Central HIDS sample demonstrates how to use the :ref:`hogp_readme` to interact with a HIDS server.
Basically, the sample simulates a computer that connects to a mouse or a keyboard.

.. note::
   |nrf_desktop_HID_ref|

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires a HIDS device to connect with (for example, another development kit running the :ref:`peripheral_hids_mouse` sample or :ref:`peripheral_hids_keyboard` sample, or a computer with a Bluetooth Low Energy dongle and the `Bluetooth Low Energy app`_).

Overview
********

The sample scans available devices, searching for a HIDS server.
If a HIDS server is found, the sample connects to it and discovers all characteristics.

If any input reports are detected, the sample subscribes to them to receive notifications.
If any boot reports are detected, the behavior depends on if they are boot mouse reports or boot keyboard reports:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      * If a boot mouse report is detected, the sample subscribes to it.
      * If a boot keyboard report is detected, the sample subscribes to its input report, and the sample functionality of changing the CAPSLOCK LED is enabled (**Button 1** and **Button 3**).

   .. group-tab:: nRF54 DKs

      * If a boot mouse report is detected, the sample subscribes to it.
      * If a boot keyboard report is detected, the sample subscribes to its input report, and the sample functionality of changing the CAPSLOCK LED is enabled (**Button 0** and **Button 2**).

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      Button 1:
         Toggle the CAPSLOCK LED on the connected keyboard using Write without response.
         This function is available only if the connected keyboard is set to work in Boot Protocol Mode.

         When pairing, press this button to confirm the passkey value that is printed on the COM listener to pair with the other device.

      Button 2:
         Switch between Boot Protocol Mode and Report Protocol Mode.
         This function is available only if the connected peer supports the Protocol Mode Characteristic.

         When pairing, press this button to reject the passkey value that is printed on the COM listener to prevent pairing with the other device.

      Button 3:
         Toggle the CAPSLOCK LED on the connected keyboard using Write with response.
         This function is available only if the connected HID has boot keyboard reports.
         It always writes CAPSLOCK information to the boot report, even if Report Protocol Mode is selected.

   .. group-tab:: nRF54 DKs

      Button 0:
         Toggle the CAPSLOCK LED on the connected keyboard using Write without response.
         This function is available only if the connected keyboard is set to work in Boot Protocol Mode.

         When pairing, press this button to confirm the passkey value that is printed on the COM listener to pair with the other device.

      Button 1:
         Switch between Boot Protocol Mode and Report Protocol Mode.
         This function is available only if the connected peer supports the Protocol Mode Characteristic.

         When pairing, press this button to reject the passkey value that is printed on the COM listener to prevent pairing with the other device.

      Button 2:
         Toggle the CAPSLOCK LED on the connected keyboard using Write with response.
         This function is available only if the connected HID has boot keyboard reports.
         It always writes CAPSLOCK information to the boot report, even if Report Protocol Mode is selected.

Building and Running
********************
.. |sample path| replace:: :file:`samples/bluetooth/central_hids`

.. include:: /includes/build_and_run_ns.txt

.. include:: /includes/nRF54H20_erase_UICR.txt

Testing
=======

After programming the sample to your development kit, you can test it either by connecting to another development kit that is running the :ref:`peripheral_hids_keyboard` sample, or by using the `Bluetooth Low Energy app`_ that emulates a HIDS server.


Testing with another development kit
------------------------------------

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. |connect_terminal_specific|
      #. Reset the kit.
      #. Program the other kit with the :ref:`peripheral_hids_keyboard` sample and reset it.
      #. When connected, press **Button 1** on both devices to confirm the passkey value used for bonding, or press **Button 2** to reject it.
      #. Wait until the HIDS keyboard is detected by the central.
         All detected descriptors are listed.
         In the terminal window, check for information similar to the following::

            HIDS is ready to work
            Subscribe to report id: 1
            Subscribe to boot keyboard report

      #. Press **Button 1** and **Button 2** one after another on the kit that runs the keyboard sample and observe the notification values in the terminal window.
         See :ref:`peripheral_hids_keyboard` for the expected values::

            Notification, id: 0, size: 8, data: 0x0 0x0 0xb 0x0 0x0 0x0 0x0 0x0
            Notification, id: 0, size: 8, data: 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0

      #. Press **Button 2** on the kit that runs the Central HIDS sample and observe that the protocol mode is updated into boot mode::

            Setting protocol mode: BOOT

      #. Press **Button 1** and **Button 2** one after another on the kit that runs the keyboard sample and observe the notification of the boot report values::

            Notification, keyboard boot, size: 8, data: 0x0 0x0 0xf 0x0 0x0 0x0 0x0 0x0
            Notification, keyboard boot, size: 8, data: 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0

      #. Press **Button 1** and **Button 3** one after another on the Central HIDS kit and observe that **LED 1** on the keyboard kit changes its state.
         The following information is also displayed in the terminal window.

         If **Button 1** was pressed::

            Caps lock send (val: 0x2)
            Caps lock sent

         If **Button 3** was pressed::

            Caps lock send using write with response (val: 0x2)
            Capslock write result: 0
            Received data (size: 1, data[0]: 0x2)

   .. group-tab:: nRF54 DKs

      1. |connect_terminal_specific|
      #. Reset the kit.
      #. Program the other kit with the :ref:`peripheral_hids_keyboard` sample and reset it.
      #. When connected, press **Button 0** on both devices to confirm the passkey value used for bonding, or press **Button 1** to reject it.
      #. Wait until the HIDS keyboard is detected by the central.
         All detected descriptors are listed.
         In the terminal window, check for information similar to the following::

            HIDS is ready to work
            Subscribe to report id: 1
            Subscribe to boot keyboard report

      #. Press **Button 0** and **Button 1** one after another on the kit that runs the keyboard sample and observe the notification values in the terminal window.
         See :ref:`peripheral_hids_keyboard` for the expected values::

            Notification, id: 0, size: 8, data: 0x0 0x0 0xb 0x0 0x0 0x0 0x0 0x0
            Notification, id: 0, size: 8, data: 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0

      #. Press **Button 1** on the kit that runs the Central HIDS sample and observe that the protocol mode is updated into boot mode::

            Setting protocol mode: BOOT

      #. Press **Button 0** and **Button 1** one after another on the kit that runs the keyboard sample and observe the notification of the boot report values::

            Notification, keyboard boot, size: 8, data: 0x0 0x0 0xf 0x0 0x0 0x0 0x0 0x0
            Notification, keyboard boot, size: 8, data: 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0

      #. Press **Button 0** and **Button 2** one after another on the Central HIDS kit and observe that **LED 0** on the keyboard kit changes its state.
         The following information is also displayed in the terminal window.

         If **Button 0** was pressed::

            Caps lock send (val: 0x2)
            Caps lock sent

         If **Button 2** was pressed::

            Caps lock send using write with response (val: 0x2)
            Capslock write result: 0
            Received data (size: 1, data[0]: 0x2)


Testing with Bluetooth Low Energy app
-------------------------------------

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. |connect_terminal_specific|
      #. Reset the kit.
      #. Start `nRF Connect for Desktop`_.
      #. Open the `Bluetooth Low Energy app`_ and select the connected dongle that is used for communication.
      #. Open the :guilabel:`SERVER SETUP` tab.
         Click the dongle configuration and select :guilabel:`Load setup`.
         Load the :file:`hids_keyboard.ncs` file that is located under :file:`samples/bluetooth/central_hids` in the |NCS| folder structure.
      #. Click :guilabel:`Apply to device`.
      #. Open the :guilabel:`CONNECTION MAP` tab.
         Click the dongle configuration and select :guilabel:`Advertising setup`.

         The current version of the app cannot store the advertising setup, so it must be configured manually.
         See the following image for the required target configuration:

         .. figure:: /images/bt_central_hids_nrfc_ad.png
            :alt: Advertising setup for HIDS keyboard simulator

            Advertising setup for HIDS keyboard simulator

         Complete the following steps to configure the advertising setup:

         a. Delete the default **Complete local name** from **Advertising data**.
         #. Add a **Custom AD type** with **AD type value** set to ``19`` and **Value** set to ``03c1``.
            This is the GAP Appearance advertising data.
         #. Add a **Custom AD type** with **AD type value** set to ``01`` and **Value** set to ``06``.
            This is the AD data with "General Discoverable" and "BR/EDR not supported" flags set.
         #. Add a **UUID 16 bit complete list** with two comma-separated values: ``1812`` and ``180F``.
            These are the values for HIDS and BAS.
         #. Add a **Complete local name** of your choice to the **Scan response data**.
         #. Click :guilabel:`Apply` and :guilabel:`Close`.

      #. In the **Adapter settings**, select :guilabel:`Start advertising`.
      #. Wait until the kit that runs the Central HIDS sample connects.
         All detected descriptors are listed.
         Check for information similar to the following::

            HIDS is ready to work
            Subscribe in report id: 1
            Subscribe in boot keyboard report

      #. Explore the first report inside Human Interface Device (the one with eight values).
         Change any of the values and note that the kit logs the change.
      #. Press **Button 2** on the kit and observe that the **Protocol Mode** value changes from ``01`` to ``00``.
      #. Press **Button 1** and **Button 3** one after another and observe that the **Boot Keyboard Output Report** value toggles between ``00`` and ``02``.

   .. group-tab:: nRF54 DKs

      1. |connect_terminal_specific|
      #. Reset the kit.
      #. Start `nRF Connect for Desktop`_.
      #. Open the `Bluetooth Low Energy app`_ and select the connected dongle that is used for communication.
      #. Open the :guilabel:`SERVER SETUP` tab.
         Click the dongle configuration and select :guilabel:`Load setup`.
         Load the :file:`hids_keyboard.ncs` file that is located under :file:`samples/bluetooth/central_hids` in the |NCS| folder structure.
      #. Click :guilabel:`Apply to device`.
      #. Open the :guilabel:`CONNECTION MAP` tab.
         Click the dongle configuration and select :guilabel:`Advertising setup`.

         The current version of the app cannot store the advertising setup, so it must be configured manually.
         See the following image for the required target configuration:

         .. figure:: /images/bt_central_hids_nrfc_ad.png
            :alt: Advertising setup for HIDS keyboard simulator

            Advertising setup for HIDS keyboard simulator

         Complete the following steps to configure the advertising setup:

         a. Delete the default **Complete local name** from **Advertising data**.
         #. Add a **Custom AD type** with **AD type value** set to ``19`` and **Value** set to ``03c1``.
            This is the GAP Appearance advertising data.
         #. Add a **Custom AD type** with **AD type value** set to ``01`` and **Value** set to ``06``.
            This is the AD data with "General Discoverable" and "BR/EDR not supported" flags set.
         #. Add a **UUID 16 bit complete list** with two comma-separated values: ``1812`` and ``180F``.
            These are the values for HIDS and BAS.
         #. Add a **Complete local name** of your choice to the **Scan response data**.
         #. Click :guilabel:`Apply` and :guilabel:`Close`.

      #. In the **Adapter settings**, select :guilabel:`Start advertising`.
      #. Wait until the kit that runs the Central HIDS sample connects.
         All detected descriptors are listed.
         Check for information similar to the following::

            HIDS is ready to work
            Subscribe in report id: 1
            Subscribe in boot keyboard report

      #. Explore the first report inside Human Interface Device (the one with eight values).
         Change any of the values and note that the kit logs the change.
      #. Press **Button 1** on the kit and observe that the **Protocol Mode** value changes from ``01`` to ``00``.
      #. Press **Button 0** and **Button 2** one after another and observe that the **Boot Keyboard Output Report** value toggles between ``00`` and ``02``.

Dependencies
*************

This sample uses the following |NCS| libraries:

* :ref:`hogp_readme`
* :ref:`gatt_dm_readme`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`nrf_bt_scan_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/sys/byteorder.h`
* :file:`include/zephyr/types.h`
* :file:`lib/libc/minimal/include/errno.h`
* :file:`include/sys/printk.h`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/uuid.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
