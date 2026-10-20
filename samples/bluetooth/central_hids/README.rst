.. _bluetooth_central_hids:

.. ncs-sample::
   :title: Bluetooth: Central HIDS

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

   .. group-tab:: nRF53 DKs

      * If a boot mouse report is detected, the sample subscribes to it.
      * If a boot keyboard report is detected, the sample subscribes to its input report, and the sample functionality of changing the CAPSLOCK LED is enabled (**Button 1** and **Button 3**).

   .. group-tab:: nRF54 DKs

      * If a boot mouse report is detected, the sample subscribes to it.
      * If a boot keyboard report is detected, the sample subscribes to its input report, and the sample functionality of changing the CAPSLOCK LED is enabled (**Button 0** and **Button 2**).

Continuous report receiving
===========================

The continuous report receiving feature is enabled by default.
You can toggle it off using the :option:`CONFIG_SAMPLE_BT_CENTRAL_HIDS_CONTINUOUS_REPORT_RX` Kconfig option.

If the connected device sends GATT notifications with HID input reports fast enough, the sample automatically enters the continuous report receiving mode.

In that mode, the sample stops printing every notification payload on the UART.
It prints a short summary every :option:`CONFIG_SAMPLE_BT_CENTRAL_HIDS_CONTINUOUS_REPORT_RX_STATS_RATE` reports, allowing you to assess the quality of the communication.

* Enter - After each group of 10 reports, if their time span is shorter than one second, the continuous mode turns on.
* Exit - If no report is received for 500 milliseconds, the continuous mode turns off and per-notification logging resumes.

To test the continuous report receiving feature, run the :ref:`peripheral_hids_mouse` sample with the :option:`CONFIG_SAMPLE_BT_HIDS_CONTINUOUS_REPORT_SENDING` Kconfig option enabled.
After pairing, hold one of the buttons on the DK that is running the :ref:`peripheral_hids_mouse` for more than two seconds to enter the continuous report sending mode.
The central will then automatically enter the continuous report receiving mode.

User interface
**************

.. tabs::

   .. group-tab:: nRF53 DKs

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

      Button 4:
        Switch to the "Alternative button functions" mode.
        In this mode, the buttons have different functionalities than in the normal mode.
        The available functionalities depend on the sample configuration and are printed in the terminal window after button press.

        Currently, the following functionalities are available:

        * **Button 1**: Resume scanning.
        * **Button 2**: Request the next HID SCI mode.
          Only available if :kconfig:option:`CONFIG_BT_HOGP_SCI` is enabled.
        * **Button 4**: Exit the "Alternative button functions" mode.

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

      Button 3:
        Switch to the "Alternative button functions" mode.
        In this mode, the buttons have different functionalities than in the normal mode.
        The available functionalities depend on the sample configuration and are printed in the terminal window after button press.

        Currently, the following functionalities are available:

        * **Button 0**: Resume scanning.
        * **Button 1**: Request the next HID SCI mode.
          Only available if :kconfig:option:`CONFIG_BT_HOGP_SCI` is enabled.
        * **Button 3**: Exit the "Alternative button functions" mode.


Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (defined in :file:`samples/bluetooth/central_hids/Kconfig`):

.. options-from-kconfig::
   :show-type:

Building and Running
********************
.. |sample path| replace:: :file:`samples/bluetooth/central_hids`

.. include:: /includes/build_and_run_ns.txt

To build the sample with HID SCI support, use ``-DFILE_SUFFIX=hid_sci``

To build with HID SCI support and two peripherals, use ``-DFILE_SUFFIX=hid_sci`` together with ``-DEXTRA_CONF_FILE=hid-sci-two-periphs.conf``.

Testing
=======

After programming the sample to your development kit, you can test it either by connecting to another development kit that is running the :ref:`peripheral_hids_keyboard` sample, or by using the `Bluetooth Low Energy app`_ that emulates a HIDS server.


Testing with another development kit
------------------------------------

.. tabs::

   .. group-tab:: nRF53 DKs

      1. |connect_terminal_specific|
      #. Reset the kit.
      #. Program the other kit with the :ref:`peripheral_hids_keyboard` sample and reset it.
      #. If you see "NFC configuration done" on the peripheral kit, press **Button 4** on the peripheral kit to make it start advertising.
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
      #. If you see "NFC configuration done" on the peripheral kit, press **Button 3** on the peripheral kit in order for it to start advertising.
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

   .. group-tab:: nRF53 DKs

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
            Subscribe to report id: 1
            Subscribe to boot keyboard report

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

.. _bluetooth_central_hids_testing_hid_sci:

Testing with HID SCI support
----------------------------

To test the sample with HID SCI support, build both the central and the peripheral with ``-DFILE_SUFFIX=hid_sci``.
You can use either the :ref:`peripheral_hids_keyboard` or the :ref:`peripheral_hids_mouse` sample as the peripheral.
The following steps use the keyboard sample.
The mouse sample follows the same SCI mode change procedure after pairing.

.. tabs::

   .. group-tab:: nRF53 DKs

      1. |connect_terminal_specific|
      #. Reset the kit.
      #. Program the other kit with the :ref:`peripheral_hids_keyboard` sample, using ``-DFILE_SUFFIX=hid_sci``, and reset it.
      #. If you see "NFC configuration done" on the peripheral kit, press **Button 4** on the peripheral kit to make it start start advertising.
      #. When connected, press **Button 1** on both devices to confirm the passkey value used for bonding, or press **Button 2** to reject it.
      #. Wait until the HIDS keyboard is detected by the central.
         Check for information similar to the following::

            HIDS is ready to work
            Subscribe to report id: 1
            Subscribe to boot keyboard report
      #. Press **Button 4** on the central kit to enter the "Alternative button functions" mode.
      #. Press **Button 2** on the central kit and wait for the SCI mode to change to DEFAULT.
         On the central kit, check for information similar to the following::

            Requesting SCI mode DEFAULT on all ready devices
            Sending SCI DEFAULT mode request
            Sent DEFAULT SCI mode request to the device
            SCI mode changed notification received, new mode: DEFAULT

         On the peripheral kit, check for information similar to the following::

            Control point event received for connection 0: SCI_DEFAULT_REQ

            Connection 0: Connection rate changed:
            interval us: ...
            subrate factor: ...
            peripheral latency: ...
            continuation number: ...
            supervision timeout (10ms units): ...

         The values might differ depending on the used board and the HID SCI configuration.

      #. Cycle through the HID SCI modes by pressing **Button 2** on the central kit.
         Each button press requests the next mode in the sequence: FAST, LOW POWER, FULL RANGE, and DEFAULT again.
         You might need to wait a few seconds for the mode to change each time.
         On the central kit, you will see a message similar to the following::

            SCI mode changed notification received, new mode: <mode_name>

         On the peripheral kit, you will see a control point event (for example, ``SCI_FAST_REQ``) followed by a connection rate change log.

   .. group-tab:: nRF54 DKs

      1. |connect_terminal_specific|
      #. Reset the kit.
      #. Program the other kit with the :ref:`peripheral_hids_keyboard` sample, using ``-DFILE_SUFFIX=hid_sci``, and reset it.
      #. If you see "NFC configuration done" on the peripheral kit, press **Button 3** on the peripheral kit to make it start start advertising.
      #. When connected, press **Button 0** on both devices to confirm the passkey value used for bonding, or press **Button 1** to reject it.
      #. Wait until the HIDS keyboard is detected by the central.
         Check for information similar to the following::

            HIDS is ready to work
            Subscribe to report id: 1
            Subscribe to boot keyboard report
      #. Press **Button 3** on the central kit to enter the "Alternative button functions" mode.
      #. Press **Button 1** on the central kit and wait for the SCI mode to change to DEFAULT.
         On the central kit, check for information similar to the following::

            Requesting SCI mode DEFAULT on all ready devices
            Sending SCI DEFAULT mode request
            Sent DEFAULT SCI mode request to the device
            SCI mode changed notification received, new mode: DEFAULT

         On the peripheral kit, check for information similar to the following::

            Control point event received for connection 0: SCI_DEFAULT_REQ

            Connection 0: Connection rate changed:
            interval us: ...
            subrate factor: ...
            peripheral latency: ...
            continuation number: ...
            supervision timeout (10ms units): ...

         The values might differ depending on the used board and the HID SCI configuration.

      #. Cycle through the HID SCI modes by pressing **Button 1** on the central kit.
         Each button press requests the next mode in the sequence: FAST, LOW POWER, FULL RANGE, and DEFAULT again.
         You might need to wait a few seconds for the mode to change each time.
         On the central kit, you will see a message similar to the following::

            SCI mode changed notification received, new mode: <mode_name>

         On the peripheral kit, you will see a control point event (for example, ``SCI_FAST_REQ``) followed by a connection rate change log.

Testing with two HID SCI peripherals
------------------------------------

This test requires three development kits: one running the central sample and one for each of the :ref:`peripheral_hids_keyboard` and :ref:`peripheral_hids_mouse` samples.

Build the samples using the following configuration:

* :ref:`peripheral_hids_mouse` with ``-DFILE_SUFFIX=hid_sci``
* :ref:`peripheral_hids_keyboard` with ``-DFILE_SUFFIX=hid_sci``
* Central HIDS with ``-DFILE_SUFFIX=hid_sci`` and ``-DEXTRA_CONF_FILE=hid-sci-two-periphs.conf``

.. tabs::

   .. group-tab:: nRF53 DKs

      1. Disconnect all three development kits from the computer.
      #. Connect only the kit that will run the :ref:`peripheral_hids_mouse` sample.
      #. Program it using the :ref:`peripheral_hids_mouse` image then disconnect it from the computer and set it aside.
      #. Connect the kit that will run the :ref:`peripheral_hids_keyboard` sample and the kit that will run the central sample.
      #. Program the keyboard using the :ref:`peripheral_hids_keyboard` image and the central using the :ref:`bluetooth_central_hids` image.
      #. Connect a terminal to both the central and the keyboard kits with a terminal emulator (for example, the `Serial Terminal app`_).
      #. Reset the central and keyboard kits.
      #. If you see "NFC configuration done" on the keyboard kit, press **Button 4** to make it start advertising.
      #. Wait until the central connects to the keyboard.
      #. When prompted, press **Button 1** on both the central and the keyboard kit to confirm the passkey value used for bonding, or press **Button 2** to reject it.
      #. Wait until the keyboard is ready.
         In the central terminal, check for information similar to the following::

            HIDS is ready to work
            Subscribe to report id: 1
            Subscribe to boot keyboard report

      #. Connect a terminal to the preprogrammed mouse kit, then reset or power-cycle it.
         The mouse starts advertising automatically after reset.
      #. Press **Button 4** on the central kit to enter the "Alternative button functions" mode.
      #. Press **Button 1** on the central kit to resume scanning for the second peripheral.
      #. Wait until the central connects to the mouse.
      #. When prompted, press **Button 1** on both the central and the mouse kit to confirm the passkey value used for bonding, or press **Button 2** to reject it.
      #. Wait until the mouse is ready.
         In the central terminal, check for information similar to the following::

            HIDS is ready to work
            Subscribe to boot mouse report

      #. Press **Button 4** on the central kit to enter the "Alternative button functions" mode again (pairing with the mouse switched the central back to the normal button mode).
      #. Press **Button 2** on the central kit and wait until both peripherals report the DEFAULT SCI mode.
         The central prefixes messages with the peripheral index (for example, ``Peripheral 0:`` and ``Peripheral 1:``).
         Check for information similar to the following on the central kit::

            Requesting SCI mode DEFAULT on all ready devices
            Sending SCI DEFAULT mode request
            Peripheral 0: Sent DEFAULT SCI mode request to the device
            Sending SCI DEFAULT mode request
            Peripheral 1: Sent DEFAULT SCI mode request to the device
            Peripheral 0: SCI mode changed notification received, new mode: DEFAULT
            Peripheral 1: SCI mode changed notification received, new mode: DEFAULT

         On each peripheral kit, check for a control point event (for example, ``SCI_DEFAULT_REQ``) and a connection rate change log.
      #. Cycle through the remaining HID SCI modes by pressing **Button 2** on the central kit.
         Each button press requests the next mode (FAST, LOW POWER, FULL RANGE, and DEFAULT again) on all ready HID devices.
         You might need to wait a few seconds for the mode to change each time.
         On the central kit, you will see a message similar to the following for each connected peripheral::

            Peripheral 0: SCI mode changed notification received, new mode: <mode_name>
            Peripheral 1: SCI mode changed notification received, new mode: <mode_name>

   .. group-tab:: nRF54 DKs

      1. Disconnect all three development kits from the computer.
      #. Connect only the kit that will run the :ref:`peripheral_hids_mouse` sample.
      #. Program it using the :ref:`peripheral_hids_mouse` image then disconnect it from the computer and set it aside.
      #. Connect the kit that will run the :ref:`peripheral_hids_keyboard` sample and the kit that will run the central sample.
      #. Program the keyboard using the :ref:`peripheral_hids_keyboard` image and the central using the :ref:`bluetooth_central_hids` image.
      #. Connect a terminal to both the central and the keyboard kits with a terminal emulator (for example, the `Serial Terminal app`_).
      #. Reset the central and keyboard kits.
      #. If you see "NFC configuration done" on the keyboard kit, press **Button 3** to make it start advertising.
      #. Wait until the central connects to the keyboard.
      #. When prompted, press **Button 0** on both the central and the keyboard kit to confirm the passkey value used for bonding, or press **Button 1** to reject it.
      #. Wait until the keyboard is ready.
         In the central terminal, check for information similar to the following::

            HIDS is ready to work
            Subscribe to report id: 1
            Subscribe to boot keyboard report

      #. Connect a terminal to the preprogrammed mouse kit, then reset or power-cycle it.
         The mouse starts advertising automatically after reset.
      #. Press **Button 3** on the central kit to enter the "Alternative button functions" mode.
      #. Press **Button 0** on the central kit to resume scanning for the second peripheral.
      #. Wait until the central connects to the mouse.
      #. When prompted, press **Button 0** on both the central and the mouse kit to confirm the passkey value used for bonding, or press **Button 1** to reject it.
      #. Wait until the mouse is ready.
         In the central terminal, check for information similar to the following::

            HIDS is ready to work
            Subscribe to boot mouse report

      #. Press **Button 3** on the central kit to enter the "Alternative button functions" mode again (pairing with the mouse switched the central back to the normal button mode).
      #. Press **Button 1** on the central kit and wait until both peripherals report the DEFAULT SCI mode.
         The central prefixes messages with the peripheral index (for example, ``Peripheral 0:`` and ``Peripheral 1:``).
         Check for information similar to the following on the central kit::

            Requesting SCI mode DEFAULT on all ready devices
            Sending SCI DEFAULT mode request
            Peripheral 0: Sent DEFAULT SCI mode request to the device
            Sending SCI DEFAULT mode request
            Peripheral 1: Sent DEFAULT SCI mode request to the device
            Peripheral 0: SCI mode changed notification received, new mode: DEFAULT
            Peripheral 1: SCI mode changed notification received, new mode: DEFAULT

         On each peripheral kit, check for a control point event (for example, ``SCI_DEFAULT_REQ``) and a connection rate change log.
      #. Cycle through the remaining HID SCI modes by pressing **Button 1** on the central kit.
         Each button press requests the next mode (FAST, LOW POWER, FULL RANGE, and DEFAULT again) on all ready HID devices.
         You might need to wait a few seconds for the mode to change each time.
         On the central kit, you will see a message similar to the following for each connected peripheral::

            Peripheral 0: SCI mode changed notification received, new mode: <mode_name>
            Peripheral 1: SCI mode changed notification received, new mode: <mode_name>

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
