.. _peripheral_hids_mouse:

Bluetooth: Peripheral HIDS mouse
################################

.. contents::
   :local:
   :depth: 2

The Peripheral HIDS mouse sample demonstrates how to use the :ref:`hids_readme` to implement a mouse input device that you can connect to your computer.
This sample also shows how to perform directed advertising.

.. note::
   |nrf_desktop_HID_ref|

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample uses the buttons on a development kit to simulate the movement of a mouse.
The four buttons simulate movement to the left, up, right, and down, respectively.
Mouse clicks are not simulated.

This sample exposes the HID GATT Service.
It uses a report map for a generic mouse.

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      Button 1:
         Simulate moving the mouse pointer five pixels to the left.

         If Man-In-The-Middle (MITM) protection based on passkey display (``CONFIG_BT_HIDS_SECURITY_MITM_ENABLED``) is enabled, you can press this button while pairing to confirm the passkey value that is printed on the COM listener to pair with the other device.

      Button 2:
         Simulate moving the mouse pointer five pixels up.

         If Man-In-The-Middle (MITM) protection based on passkey display (``CONFIG_BT_HIDS_SECURITY_MITM_ENABLED``) is enabled, you can press this button while pairing to reject the passkey value that is printed on the COM listener to prevent pairing with the other device.

      Button 3:
         Simulate moving the mouse pointer five pixels to the right.

      Button 4:
         Simulate moving the mouse pointer five pixels down.

   .. group-tab:: nRF54 DKs

      Button 0:
         Simulate moving the mouse pointer five pixels to the left.

         If Man-In-The-Middle (MITM) protection based on passkey display (``CONFIG_BT_HIDS_SECURITY_MITM_ENABLED``) is enabled, you can press this button while pairing to confirm the passkey value that is printed on the COM listener to pair with the other device.

      Button 1:
         Simulate moving the mouse pointer five pixels up.

         If Man-In-The-Middle (MITM) protection based on passkey display (``CONFIG_BT_HIDS_SECURITY_MITM_ENABLED``) is enabled, you can press this button while pairing to reject the passkey value that is printed on the COM listener to prevent pairing with the other device.

      Button 2:
         Simulate moving the mouse pointer five pixels to the right.

      Button 3:
         Simulate moving the mouse pointer five pixels down.

Configuration
*************

|config|

Bluetooth LE security
=====================

By default, the sample enables Bluetooth LE security support through the ``CONFIG_BT_HIDS_SECURITY_ENABLED`` sample-specific Kconfig option.
This allows the sample to encrypt connections and create Bluetooth bonds.
The option also implies Bluetooth privacy (:kconfig:option:`CONFIG_BT_PRIVACY`).
You can disable the sample-specific Kconfig option to disable support for Bluetooth security.

Man-In-The-Middle (MITM) protection
-----------------------------------

By default, the sample also enables MITM protection based on passkey display (``CONFIG_BT_HIDS_SECURITY_MITM_ENABLED``).
This is done to prevent Man-In-The-Middle attacks through authentication.
Since the :c:func:`printk` function is used to display the passkey, the feature depends on :kconfig:option:`CONFIG_PRINTK`.

HID GATT characteristic attributes permissions
----------------------------------------------

Configurations of the sample require Bluetooth link encryption to access the HID Service (:kconfig:option:`CONFIG_BT_HIDS_DEFAULT_PERM_RW_ENCRYPT`).
The HID Service specification does not require encryption, but Bluetooth link encryption is required by the HID over GATT Profile Specification.
The Bluetooth LE security support (``CONFIG_BT_HIDS_SECURITY_ENABLED``) is required to encrypt a link.
Some of the HID hosts may not subscribe for HID input reports or disconnect the Bluetooth link if Bluetooth LE security is not supported.

If you disable Bluetooth LE security support (``CONFIG_BT_HIDS_SECURITY_ENABLED``), you need to allow access to HID Service without encryption (:kconfig:option:`CONFIG_BT_HIDS_DEFAULT_PERM_RW`) too.
This imposes a security risk of, among others, passive eavesdropping of the communication between HID device and HID host.
Because of that it is not recommended for production.

.. note::
   If MITM protection based on passkey display is enabled (``CONFIG_BT_HIDS_SECURITY_MITM_ENABLED``) and you want to pair the device with a computer running MacOS, set the :kconfig:option:`CONFIG_BT_HIDS_DEFAULT_PERM_RW_AUTHEN` Kconfig option to ``y``.

Bluetooth direct advertising
============================

By default, the sample enables the ``CONFIG_BT_DIRECTED_ADVERTISING`` sample-specific Kconfig option that enables using Bluetooth direct advertising.
The feature depends on Bluetooth LE security support (``CONFIG_BT_HIDS_SECURITY_ENABLED``).
This feature changes the way advertising works in comparison to the other Bluetooth Low Energy samples.
When the device wants to advertise, it starts with high duty cycle directed advertising provided that it has bonding information.
If the timeout occurs, the device starts directed advertising to the next bonded peer.
If all bonding information is used and there is still no connection, the regular advertising starts.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/peripheral_hids_mouse`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

.. _peripheral_hids_mouse_bt_rpc_build:

Bluetooth RPC build
===================

To build this sample with the :ref:`ble_rpc` library, add the following parameters:

* set the :makevar:`SNIPPET` option to ``nordic-bt-rpc``,
* set the :makevar:`FILE_SUFFIX` option to ``bt_rpc``.

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -S nordic-bt-rpc -- -DFILE_SUFFIX=bt_rpc


.. _peripheral_hids_mouse_ncd_build:

Bluetooth Low Energy app build
==============================

To build this sample in the configuration variant that is compatible with `Bluetooth Low Energy app`_, disable the following Bluetooth features:

* Privacy (:kconfig:option:`CONFIG_BT_PRIVACY`) - The `Bluetooth Low Energy app`_ does not fully support the Bluetooth Privacy feature by disallowing distribution of the Identity Resolving Key (IRK) during the pairing procedure.
* High-duty directed advertising (:kconfig:option:`CONFIG_BT_DIRECTED_ADVERTISING`) - High-duty directed advertising with 3.75 ms advertising interval and 1.28 s duration prevents the subsequent undirected advertising from being reported in the scanning list of `Bluetooth Low Energy app`_ .
  As a result, it is only possible to connect to the target DK during the very short interval of high-duty directed advertising.

To build the sample in the compatible configuration, use the following command:

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -- -DCONFIG_BT_DIRECTED_ADVERTISING=n -DCONFIG_BT_PRIVACY=n

.. note::
   If you want to combine this build configuration with the :ref:`peripheral_hids_mouse_bt_rpc_build`, use the following command:

   .. parsed-literal::
      :class: highlight

      west build -b *board_target* -S nordic-bt-rpc -- -DFILE_SUFFIX=bt_rpc -DCONFIG_BT_DIRECTED_ADVERTISING=n -DCONFIG_BT_PRIVACY=n -Dipc_radio_CONFIG_BT_PRIVACY=n

Memory footprint usage
======================

The sample supports a dedicated ``release`` configuration that disables debug features like logs or assertions.
You can use the configuration to verify the memory footprint of the sample as it is closer to the configuration used by a final product.

.. note::
   The ``release`` configuration does not emit logs (and :c:func:`printk` messages).
   Because of that, it cannot enable passkey display MITM protection (``CONFIG_BT_HIDS_SECURITY_MITM_ENABLED``).

Use the following command to build the sample in ``release`` configuration:

.. parsed-literal::
   :class: highlight

   west build -b *board_target* -- -DFILE_SUFFIX=release

To get the memory footprint, run the following commands from the sample directory (:file:`nrf/samples/bluetooth/peripheral_hids_mouse`):

.. parsed-literal::
   :class: highlight

   west build -d <build_dir>/peripheral_hids_mouse -t rom_report

.. parsed-literal::
   :class: highlight

   west build -d <build_dir>/peripheral_hids_mouse -t ram_report

For more information, see the Zephyr's :ref:`optimization_tools` documentation.

Testing
=======

After programming the sample to your development kit, you can test it either by connecting the development kit as a mouse device to a Microsoft Windows computer or by connecting to it with the `Bluetooth Low Energy app`_ of the `nRF Connect for Desktop`_.

Testing with a Microsoft Windows computer
-----------------------------------------

To test with a Microsoft Windows computer that has a Bluetooth radio, complete the following steps:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. Power on your development kit.
      #. On your Windows computer, search for Bluetooth devices and connect to the device named "NCS HIDS mouse".
      #. Push **Button 1** on the kit.
         Observe that the mouse pointer on the computer moves to the left.
      #. Push **Button 2**.
         Observe that the mouse pointer on the computer moves upward.
      #. Push **Button 3**.
         Observe that the mouse pointer on the computer moves to the right.
      #. Push **Button 4**.
         Observe that the mouse pointer on the computer moves downward.
      #. Disconnect the computer from the device by removing the device from the computer's devices list.

   .. group-tab:: nRF54 DKs

      1. Power on your development kit.
      #. On your Windows computer, search for Bluetooth devices and connect to the device named "NCS HIDS mouse".
      #. Push **Button 0** on the kit.
         Observe that the mouse pointer on the computer moves to the left.
      #. Push **Button 1**.
         Observe that the mouse pointer on the computer moves upward.
      #. Push **Button 2**.
         Observe that the mouse pointer on the computer moves to the right.
      #. Push **Button 3**.
         Observe that the mouse pointer on the computer moves downward.
      #. Disconnect the computer from the device by removing the device from the computer's devices list.

Testing with Bluetooth Low Energy app
-------------------------------------

To test with `Bluetooth Low Energy app`_, complete the following steps:

.. note::
   To execute the testing steps below, you must build the sample in the configuration that is compatible with `Bluetooth Low Energy app`_.
   See the :ref:`peripheral_hids_mouse_ncd_build` section to learn how to build a compatible sample configuration.

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. Power on your development kit.
      #. Start `nRF Connect for Desktop`_.
      #. Open the `Bluetooth Low Energy app`_.
      #. Connect to the device from the app. The device is advertising as "NCS HIDS mouse"
      #. Optionally, bond to the device.
         Click the :guilabel:`Settings` button for the device in the app, select **Pair**, check :guilabel:`Perform Bonding`, and click :guilabel:`Pair`.
         Optionally check :guilabel:`Enable MITM protection` to pair with MITM protection and use a button on the device to confirm or reject passkey value.
      #. Click :guilabel:`Match` in the app.
         Wait until the bond is established before you continue.
      #. Observe that the services of the connected device are shown.
      #. Click :guilabel:`Play` for all HID Report characteristics.
      #. Push **Button 1** on the kit.
         Observe that a notification is received on one of the HID Report characteristics, containing the value ``FB0F00``.

         Mouse motion reports contain data with an X-translation and a Y-translation.
         These are transmitted as 12-bit signed integers.
         The format used for mouse reports is the following byte array, where LSB/MSB is the least/most significant bit: ``[8 LSB (X), 4 LSB (Y) | 4 MSB(X), 8 MSB(Y)]``.

         Therefore, ``FB0F00`` denotes an X-translation of FFB = -5 (two's complement format), which means a movement of five pixels to the left, and a Y-translation of 000 = 0.
      #. Push **Button 2**.
         Observe that the value ``00B0FF`` is received on the same HID Report characteristic.
      #. Push **Button 3**.
         Observe that the value ``050000`` is received on the same HID Report characteristic.
      #. Push **Button 4**.
         Observe that the value ``005000`` is received on the same HID Report characteristic.
      #. Disconnect the device in the Bluetooth Low Energy app of nRF Connect for Desktop.
         Observe that no new notifications are received and the device is advertising.
      #. As bond information is preserved by the Bluetooth Low Energy app, you can immediately reconnect to the device by clicking the :guilabel:`Connect` button again.

   .. group-tab:: nRF54 DKs

      1. Power on your development kit.
      #. Start `nRF Connect for Desktop`_.
      #. Open the `Bluetooth Low Energy app`_.
      #. Connect to the device from the app. The device is advertising as "NCS HIDS mouse"
      #. Optionally, bond to the device.
         Click the :guilabel:`Settings` button for the device in the app, select **Pair**, check :guilabel:`Perform Bonding`, and click :guilabel:`Pair`.
         Optionally check :guilabel:`Enable MITM protection` to pair with MITM protection and use a button on the device to confirm or reject passkey value.
      #. Click :guilabel:`Match` in the app.
         Wait until the bond is established before you continue.
      #. Observe that the services of the connected device are shown.
      #. Click :guilabel:`Play` for all HID Report characteristics.
      #. Push **Button 0** on the kit.
         Observe that a notification is received on one of the HID Report characteristics, containing the value ``FB0F00``.

         Mouse motion reports contain data with an X-translation and a Y-translation.
         These are transmitted as 12-bit signed integers.
         The format used for mouse reports is the following byte array, where LSB/MSB is the least/most significant bit: ``[8 LSB (X), 4 LSB (Y) | 4 MSB(X), 8 MSB(Y)]``.

         Therefore, ``FB0F00`` denotes an X-translation of FFB = -5 (two's complement format), which means a movement of five pixels to the left, and a Y-translation of 000 = 0.
      #. Push **Button 1**.
         Observe that the value ``00B0FF`` is received on the same HID Report characteristic.
      #. Push **Button 2**.
         Observe that the value ``050000`` is received on the same HID Report characteristic.
      #. Push **Button 3**.
         Observe that the value ``005000`` is received on the same HID Report characteristic.
      #. Disconnect the device in the Bluetooth Low Energy app of nRF Connect for Desktop.
         Observe that no new notifications are received and the device is advertising.
      #. As bond information is preserved by the Bluetooth Low Energy app, you can immediately reconnect to the device by clicking the :guilabel:`Connect` button again.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`hids_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`lib/libc/minimal/include/assert.h`
* :file:`lib/libc/minimal/include/errno.h`
* :file:`include/sys/printk.h`
* :file:`include/sys/byteorder.h`
* :ref:`GPIO Interface <zephyr:api_peripherals>`
* :ref:`zephyr:settings_api`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/gatt.h`
  * :file:`samples/bluetooth/gatt/bas.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`

References
**********

* `HID Service Specification`_
* `HID Over GATT Profile Specification`_
* `HID usage tables`_
