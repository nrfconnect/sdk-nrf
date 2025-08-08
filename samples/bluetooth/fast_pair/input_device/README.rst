.. _fast_pair_input_device:

Bluetooth Fast Pair: Input device
#################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates :ref:`how to use Google Fast Pair with the nRF Connect SDK <ug_bt_fast_pair>`.

Google Fast Pair Service (GFPS) is a standard for pairing Bluetooth® and Bluetooth LE devices with as little user interaction required as possible.
Google also provides additional features built upon the Fast Pair standard.
For detailed information about supported functionalities, see the official `Fast Pair`_ documentation.
The software maturity level for the input device use case is outlined in the :ref:`Google Fast Pair use case support <software_maturity_fast_pair_use_case>` table.

.. note::
   Support for Fast Pair input device use case is also integrated into :ref:`nrf_desktop`.
   The nRF Desktop is a complete reference application design of :term:`Human Interface Device (HID)`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

.. note::
   This sample does not build or run for the ``nrf54h20dk/nrf54h20/cpuapp`` board target due to the IronSide SE migration.
   See the ``NCSDK-34821`` in the :ref:`known_issues` page for more information.
   The codebase and documentation will be updated in the future releases to address this issue.

Overview
********

The sample works as a Fast Pair Provider (one of the `Fast Pair roles`_) and a simple HID multimedia controller.
Two buttons are used to control audio volume of the connected Bluetooth Central.

The device can be used to bond with the following devices:

* Fast Pair Seeker - For example, an Android device.
  The bonding follows the official `Fast Pair Procedure`_ with Bluetooth man-in-the-middle (MITM) protection.
  The device is linked with the user's Google account.
* Bluetooth Central that is not a Fast Pair Seeker - Normal Bluetooth LE bonding is used in this scenario and there is no Bluetooth MITM protection.

.. note::
   The normal Bluetooth LE bonding can be used only if the Fast Pair discoverable advertising mode is selected.
   In other Fast Pair advertising modes, the device rejects the normal Bluetooth LE bonding.

The sample supports only one simultaneous Bluetooth connection, but it can be bonded with multiple Bluetooth Centrals.

The sample supports both the discoverable and not discoverable Fast Pair advertising.
The device keeps the Bluetooth advertising active until a connection is established.
While maintaining the connection, the Bluetooth advertising is disabled.
The advertising is restarted after disconnection.

See `Fast Pair Advertising`_ for detailed information about discoverable and not discoverable advertising.

Fast Pair device registration
=============================

Before you can use your device as a Fast Pair Provider, the device model must be registered with Google.
This is required to obtain Model ID and Anti-Spoofing Private Key.
You can register your own device or use the debug Model ID and Anti-Spoofing Public/Private Key pair obtained by Nordic for development purposes.

.. note::
   To support the input device use case, you must select :guilabel:`Input Device` option in the **Device Type** list when registering your device model.

By default, if Model ID and Anti-Spoofing Private Key are not specified, the following debug Fast Pair provider is used:

* NCS input device - The input device Fast Pair provider:

   * Device Name: NCS input device
   * Model ID: ``0x2A410B``
   * Anti-Spoofing Private Key (Base64, uncompressed): ``Unoh+nycK/ZJ7k3dHsdcNpiP1SfOy0P/Lx5XixyYois=``
   * Device Type: Input Device
   * Notification Type: Fast Pair
   * Data-Only connection: true
   * No Personalized Name: false

See :ref:`ug_bt_fast_pair_provisioning` in the Fast Pair user guide for details.

.. include:: /applications/nrf_desktop/description.rst
   :start-after: nrf_desktop_fastpair_important_start
   :end-before: nrf_desktop_fastpair_important_end

.. tip::
   The sample provides TX power in the Bluetooth advertising data.
   There is no need to provide the TX power value during device model registration.
   The device is using only Bluetooth LE, so you must select :guilabel:`Skip connecting audio profiles (e.g. A2DP or HFP)` option when registering the device.

Seeker device
=============

A Fast Pair Seeker device is required to test the Fast Pair procedure.
This is one of the two `Fast Pair roles`_.

For example, you can use an Android device as the Seeker device.
To test with a debug mode Model ID, you must configure the Android device to include debug results while displaying the nearby Fast Pair Providers.
For details, see `Verifying Fast Pair`_ in the GFPS documentation.

Not discoverable advertising requirements
-----------------------------------------

Testing not discoverable advertising requires using at least two Android devices registered to the same Google account.
The first one needs to be bonded with Fast Pair Provider before the not discoverable advertising can be detected by the second one.

User interface
**************

The sample supports a simple user interface.
You can control the sample using predefined buttons, while LEDs are used to display information.

Buttons
=======

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      Button 1:
         Toggles between three Fast Pair advertising modes:

         * Fast Pair discoverable advertising.
         * Fast Pair not discoverable advertising (with the show UI indication).
         * Fast Pair not discoverable advertising (with the hide UI indication).

         The advertising pairing mode (:c:member:`bt_le_adv_prov_adv_state.pairing_mode`) is enabled only if the Fast Pair discoverable advertising mode is selected.

         .. note::
            The Bluetooth advertising is active only until the Fast Pair Provider connects to a Bluetooth Central.
            After the connection, you can still switch the advertising modes, but the switch will come into effect only after disconnection.
            The discoverable advertising is automatically switched to the not discoverable advertising with the show UI indication mode in the following cases:

            * After 10 minutes of active advertising.
            * After a Bluetooth Central successfully pairs.

            After the device reaches the maximum number of paired devices (:kconfig:option:`CONFIG_BT_MAX_PAIRED`), the device stops looking for new peers.
            Therefore, the device no longer advertises in the pairing mode (:c:member:`bt_le_adv_prov_adv_state.pairing_mode`), and the Fast Pair advertising mode is automatically set to the Fast Pair not discoverable advertising with the hide UI indication on every advertising start.

      Button 2:
         Increases audio volume of the connected Bluetooth Central.

      Button 3:
         Removes the Bluetooth bonds.
         This operation does not clear Fast Pair storage data.
         The stored Account Keys are not removed.

      Button 4:
         Decreases audio volume of the connected Bluetooth Central.

   .. group-tab:: nRF54 DKs

      Button 0:
         Toggles between three Fast Pair advertising modes:

         * Fast Pair discoverable advertising.
         * Fast Pair not discoverable advertising (with the show UI indication).
         * Fast Pair not discoverable advertising (with the hide UI indication).

         The advertising pairing mode (:c:member:`bt_le_adv_prov_adv_state.pairing_mode`) is enabled only if the Fast Pair discoverable advertising mode is selected.

         .. note::
            The Bluetooth advertising is active only until the Fast Pair Provider connects to a Bluetooth Central.
            After the connection, you can still switch the advertising modes, but the switch will come into effect only after disconnection.
            The discoverable advertising is automatically switched to the not discoverable advertising with the show UI indication mode in the following cases:

            * After 10 minutes of active advertising.
            * After a Bluetooth Central successfully pairs.

            After the device reaches the maximum number of paired devices (:kconfig:option:`CONFIG_BT_MAX_PAIRED`), the device stops looking for new peers.
            Therefore, the device no longer advertises in the pairing mode (:c:member:`bt_le_adv_prov_adv_state.pairing_mode`), and the Fast Pair advertising mode is automatically set to the Fast Pair not discoverable advertising with the hide UI indication on every advertising start.

      Button 1:
         Increases audio volume of the connected Bluetooth Central.

      Button 2:
         Removes the Bluetooth bonds.
         This operation does not clear Fast Pair storage data.
         The stored Account Keys are not removed.

      Button 3:
         Decreases audio volume of the connected Bluetooth Central.

LEDs
====

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Keeps blinking with constant interval to indicate that firmware is running.

      LED 2:
         Depending on the Bluetooth Central connection status:

         * On if the Central is connected over Bluetooth.
         * Off if there is no Central connected.

      LED 3:
         Depending on the Fast Pair advertising mode setting:

         * On if the device is Fast Pair discoverable.
         * Blinks with 0.5 secs interval if the selected mode is the Fast Pair not discoverable advertising with the show UI indication.
         * Blinks with 1.5 secs interval if the selected mode is the Fast Pair not discoverable advertising with the hide UI indication.

   .. group-tab:: nRF54 DKs

      LED 0:
         Keeps blinking with constant interval to indicate that firmware is running.

      LED 1:
         Depending on the Bluetooth Central connection status:

         * On if the Central is connected over Bluetooth.
         * Off if there is no Central connected.

      LED 2:
         Depending on the Fast Pair advertising mode setting:

         * On if the device is Fast Pair discoverable.
         * Blinks with 0.5 secs interval if the selected mode is the Fast Pair not discoverable advertising with the show UI indication.
         * Blinks with 1.5 secs interval if the selected mode is the Fast Pair not discoverable advertising with the hide UI indication.


Configuration
*************

|config|

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/fast_pair/input_device`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

When building the sample, you can provide the Fast Pair Model ID (``SB_CONFIG_BT_FAST_PAIR_MODEL_ID``) and the Fast Pair Anti-Spoofing Key (``SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY``) as sysbuild Kconfig options.
If the data is not provided, the sample uses the default provisioning data obtained for the *NCS input device* (the input device debug Fast Pair provider).
See :ref:`ug_bt_fast_pair_provisioning` for detailed guide.

.. note::
   The sample cannot be used without the Fast Pair provisioning data.
   Programming device with the sample firmware without providing the proper Fast Pair provisioning data would result in assertion failure during boot.

Testing
=======

|test_sample|

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. |connect_terminal_specific|
         The sample provides Fast Pair debug logs to inform about state of the Fast Pair procedure.
      #. Reset the kit.
      #. Observe that **LED 1** is blinking (firmware is running) and **LED 3** is lit (device is Fast Pair discoverable).
         This means that the device is now working as Fast Pair Provider and is advertising.
      #. On the Android device, go to :guilabel:`Settings` > :guilabel:`Google` > :guilabel:`Devices & sharing` (or :guilabel:`Device connections`, depending on your Android device configuration) > :guilabel:`Devices`.
      #. Move the Android device close to the Fast Pair Provider that is advertising.
      #. Wait for Android device's notification about the detected Fast Pair Provider.
         If you use the Model ID certified by Google, the notification is similar to the following:

         .. figure:: /images/bt_fast_pair_discoverable_notification.png
            :scale: 33 %
            :alt: Fast Pair discoverable advertising Android notification

         The device model name and displayed logo depend on the data provided during the device model registration.

         If you use the debug Model ID (for example, the default is *NCS input device*), the notification is similar to the following:

         .. figure:: /images/bt_fast_pair_discoverable_notification_debug.png
            :scale: 50 %
            :alt: Fast Pair discoverable advertising Android notification for debug Model ID.

         The device model name is covered by asterisks and the default Fast Pair logo is displayed instead of the one specified during the device model registration.
      #. Tap the :guilabel:`Connect` button to initiate the connection and trigger the Fast Pair procedure.
         After the procedure has completed, the pop-up is updated to inform about successfully completed Fast Pair procedure.
         **LED 2** is lit to indicate that the device is connected with the Bluetooth Central.

         .. note::
            Some Android devices might disconnect right after the Fast Pair procedure has completed.
            Go to :guilabel:`Settings` > :guilabel:`Bluetooth` and tap on the bonded Fast Pair Provider to reconnect.

         You can now use the connected Fast Pair Provider to control audio volume of the Bluetooth Central.
      #. Press **Button 2** to increase the audio volume.
      #. Press **Button 4** to decrease the audio volume.

   .. group-tab:: nRF54 DKs

      1. |connect_terminal_specific|
         The sample provides Fast Pair debug logs to inform about state of the Fast Pair procedure.
      #. Reset the kit.
      #. Observe that **LED 0** is blinking (firmware is running) and **LED 2** is lit (device is Fast Pair discoverable).
         This means that the device is now working as Fast Pair Provider and is advertising.
      #. On the Android device, go to :guilabel:`Settings` > :guilabel:`Google` > :guilabel:`Devices & sharing` (or :guilabel:`Device connections`, depending on your Android device configuration) > :guilabel:`Devices`.
      #. Move the Android device close to the Fast Pair Provider that is advertising.
      #. Wait for Android device's notification about the detected Fast Pair Provider.
         If you use the Model ID certified by Google, the notification is similar to the following:

         .. figure:: /images/bt_fast_pair_discoverable_notification.png
            :scale: 33 %
            :alt: Fast Pair discoverable advertising Android notification

         The device model name and displayed logo depend on the data provided during the device model registration.

         If you use the debug Model ID (for example, the default is *NCS input device*), the notification is similar to the following:

         .. figure:: /images/bt_fast_pair_discoverable_notification_debug.png
            :scale: 50 %
            :alt: Fast Pair discoverable advertising Android notification for debug Model ID.

         The device model name is covered by asterisks and the default Fast Pair logo is displayed instead of the one specified during the device model registration.
      #. Tap the :guilabel:`Connect` button to initiate the connection and trigger the Fast Pair procedure.
         After the procedure has completed, the pop-up is updated to inform about successfully completed Fast Pair procedure.
         **LED 1** is lit to indicate that the device is connected with the Bluetooth Central.

         .. note::
            Some Android devices might disconnect right after the Fast Pair procedure has completed.
            Go to :guilabel:`Settings` > :guilabel:`Bluetooth` and tap on the bonded Fast Pair Provider to reconnect.

         You can now use the connected Fast Pair Provider to control audio volume of the Bluetooth Central.
      #. Press **Button 1** to increase the audio volume.
      #. Press **Button 3** to decrease the audio volume.

Not discoverable advertising
----------------------------

Testing not discoverable advertising requires using a second Android device that is registered to the same Google account as the first Android device.

Test not discoverable advertising by completing `Testing`_ and the following additional steps:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      #. Disconnect the Android device that was used during the default `Testing`_:

         a. Go to :guilabel:`Settings` > :guilabel:`Bluetooth`.
         #. Tap on the connected device name to disconnect it.

            .. note::
               Do not remove Bluetooth bond information related to the Fast Pair Provider.

            After disconnection, the provider automatically switches from the discoverable advertising to the not discoverable advertising with the show UI indication mode.
            **LED 3** is blinking rapidly.

      #. Make sure that the Fast Pair Provider is added to :guilabel:`Saved devices` on the Android device that was used for `Testing`_:

         a. Go to :guilabel:`Settings` > :guilabel:`Google` > :guilabel:`Devices & sharing` (or :guilabel:`Device connections`) > :guilabel:`Devices` > :guilabel:`Saved devices`.
         #. Verify that the paired device is appearing on the list.

      #. If you want to test the Fast Pair not discoverable advertising with the hide UI indication mode, press **Button 1**.
         **LED 3** starts blinking slowly.

      #. Wait until the Fast Pair Provider is added to :guilabel:`Saved devices` on the second Android device:

         a. Go to :guilabel:`Settings` > :guilabel:`Google` > :guilabel:`Devices & sharing` (or :guilabel:`Device connections`) > :guilabel:`Devices` > :guilabel:`Saved devices`.
            The paired device appears on the list.
         #. If the device does not appear on the list, wait until the data is synced between phones.

      #. Move the second Android device close to the Fast Pair Provider.
         If you use the Model ID certified by Google and the device is in the show UI indication advertising mode, a notification similar to the following one appears:

         .. figure:: /images/bt_fast_pair_not_discoverable_notification.png
            :scale: 50 %
            :alt: Fast Pair not discoverable advertising Android notification

         If you use the debug Model ID (for example, the default is *NCS input device*) and the device is in the show UI indication advertising mode, a notification similar to the following one appears:

         .. figure:: /images/bt_fast_pair_not_discoverable_notification_debug.png
            :scale: 50 %
            :alt: Fast Pair not discoverable advertising Android notification for debug Model ID

         The *Nordic* name is replaced by your own Google account name as this is a default name created by the Fast Pair Seeker during the initial pairing.

         If the device is in the hide UI indication advertising mode, no notification appears.
         This is because the device advertises, but does not want to be paired with.
         You can verify that the device is advertising using the `nRF Connect for Mobile`_ application.

      #. In the show UI indication mode, when the notification appears, tap on it to trigger the Fast Pair procedure.
      #. Wait for the notification about successful Fast Pair procedure.
         **LED 2** is lit to inform that the device is connected with the Bluetooth Central.

         .. note::
            Some Android devices might disconnect right after Fast Pair procedure is finished.
            Go to :guilabel:`Settings` > :guilabel:`Bluetooth` and tap on the bonded Fast Pair Provider to reconnect.

         You can now use the connected Fast Pair Provider to control the audio volume of the Bluetooth Central.
      #. Press **Button 2** to increase the audio volume.
      #. Press **Button 4** to decrease the audio volume.

   .. group-tab:: nRF54 DKs

      #. Disconnect the Android device that was used during the default `Testing`_:

         a. Go to :guilabel:`Settings` > :guilabel:`Bluetooth`.
         #. Tap on the connected device name to disconnect it.

            .. note::
               Do not remove Bluetooth bond information related to the Fast Pair Provider.

            After disconnection, the provider automatically switches from the discoverable advertising to the not discoverable advertising with the show UI indication mode.
            **LED 2** is blinking rapidly.

      #. Make sure that the Fast Pair Provider is added to :guilabel:`Saved devices` on the Android device that was used for `Testing`_:

         a. Go to :guilabel:`Settings` > :guilabel:`Google` > :guilabel:`Devices & sharing` (or :guilabel:`Device connections`) > :guilabel:`Devices` > :guilabel:`Saved devices`.
         #. Verify that the paired device is appearing on the list.

      #. If you want to test the Fast Pair not discoverable advertising with the hide UI indication mode, press **Button 0**.
         **LED 2** starts blinking slowly.

      #. Wait until the Fast Pair Provider is added to :guilabel:`Saved devices` on the second Android device:

         a. Go to :guilabel:`Settings` > :guilabel:`Google` > :guilabel:`Devices & sharing` (or :guilabel:`Device connections`) > :guilabel:`Devices` > :guilabel:`Saved devices`.
            The paired device appears on the list.
         #. If the device does not appear on the list, wait until the data is synced between phones.

      #. Move the second Android device close to the Fast Pair Provider.
         If you use the Model ID certified by Google and the device is in the show UI indication advertising mode, a notification similar to the following one appears:

         .. figure:: /images/bt_fast_pair_not_discoverable_notification.png
            :scale: 50 %
            :alt: Fast Pair not discoverable advertising Android notification

         If you use the debug Model ID (for example, the default is *NCS input device*) and the device is in the show UI indication advertising mode, a notification similar to the following one appears:

         .. figure:: /images/bt_fast_pair_not_discoverable_notification_debug.png
            :scale: 50 %
            :alt: Fast Pair not discoverable advertising Android notification for debug Model ID

         The *Nordic* name is replaced by your own Google account name as this is a default name created by the Fast Pair Seeker during the initial pairing.

         If the device is in the hide UI indication advertising mode, no notification appears.
         This is because the device advertises, but does not want to be paired with.
         You can verify that the device is advertising using the `nRF Connect for Mobile`_ application.

      #. In the show UI indication mode, when the notification appears, tap on it to trigger the Fast Pair procedure.
      #. Wait for the notification about successful Fast Pair procedure.
         **LED 1** is lit to inform that the device is connected with the Bluetooth Central.

         .. note::
            Some Android devices might disconnect right after Fast Pair procedure is finished.
            Go to :guilabel:`Settings` > :guilabel:`Bluetooth` and tap on the bonded Fast Pair Provider to reconnect.

         You can now use the connected Fast Pair Provider to control the audio volume of the Bluetooth Central.
      #. Press **Button 1** to increase the audio volume.
      #. Press **Button 3** to decrease the audio volume.

Personalized Name extension
----------------------------

Testing Personalized Name extension is described in `Fast Pair Certification Guidelines for Personalized Name`_.

.. note::
   For Android devices running Android 15 or lower, to mitigate Android Personalized Name write issues when you change the Personalized Name, perform the following:

   1. Write the new Personalized Name.
   #. Disconnect the phone from the Fast Pair Provider.
   #. Put the Fast Pair Provider in not discoverable advertising mode.
   #. The phone reconnects automatically or you need to reconnect it manually.
   #. The phone sends new Personalized Name to the Fast Pair Provider.

Battery Notification extension
------------------------------

Complete the following steps to test `Fast Pair Battery Notification extension`_:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      #. Pair the Fast Pair Provider with at least one Fast Pair Seeker.
      #. Disconnect the Fast Pair Seeker from the Fast Pair Provider.
      #. After disconnection, the provider automatically switches from the discoverable advertising to the not discoverable advertising with the show UI indication mode and starts advertising sample battery data.
         **LED 3** is blinking rapidly.
      #. Verify that the Provider is advertising sample battery data using the `nRF Connect for Mobile`_ application:

         a. Open the `nRF Connect for Mobile`_ application.
         #. Open the :guilabel:`SCANNER` tab.
         #. Identify your test device that acts as the Fast Pair Provider in the scanning list.
         #. Tap on it to expand advertising details.
         #. Verify that the sample battery level and charging status are displayed in the **Fast Pair** section.

   .. group-tab:: nRF54 DKs

      #. Pair the Fast Pair Provider with at least one Fast Pair Seeker.
      #. Disconnect the Fast Pair Seeker from the Fast Pair Provider.
      #. After disconnection, the provider automatically switches from the discoverable advertising to the not discoverable advertising with the show UI indication mode and starts advertising sample battery data.
         **LED 2** is blinking rapidly.
      #. Verify that the Provider is advertising sample battery data using the `nRF Connect for Mobile`_ application:

         a. Open the `nRF Connect for Mobile`_ application.
         #. Open the :guilabel:`SCANNER` tab.
         #. Identify your test device that acts as the Fast Pair Provider in the scanning list.
         #. Tap on it to expand advertising details.
         #. Verify that the sample battery level and charging status are displayed in the **Fast Pair** section.

.. note::
   Currently, Android phones have trouble with the Battery Notification extension and sometimes do not display battery information as a user indication.

Dependencies
************

The sample uses subsystems and firmware components available in the |NCS|.
For details, see the sections below.

Fast Pair GATT Service
======================

This sample uses the :ref:`bt_fast_pair_readme` and its dependencies and is configured to meet the requirements of the Fast Pair standard.
See :ref:`ug_bt_fast_pair` for details about integrating Fast Pair in the |NCS|.

By default, this sample sets the ``SB_CONFIG_BT_FAST_PAIR_MODEL_ID`` and ``SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY`` Kconfig options to use the Nordic device model that is intended for demonstration purposes.
With these options set, the build system calls the :ref:`bt_fast_pair_provision_script` that automatically generates a hexadecimal file containing Fast Pair Model ID and the Anti-Spoofing Private Key.
For more details about enabling Fast Pair for your application, see the :ref:`ug_bt_fast_pair_prerequisite_ops_kconfig` section in the Fast Pair integration guide.

Bluetooth LE advertising data providers
=======================================

The :ref:`bt_le_adv_prov_readme` are used to generate Bluetooth advertising and scan response data.
The sample uses the following providers to generate the advertising packet payload:

* Advertising flags provider (:kconfig:option:`CONFIG_BT_ADV_PROV_FLAGS`)
* TX power provider (:kconfig:option:`CONFIG_BT_ADV_PROV_TX_POWER`)
* Google Fast Pair provider (:kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR`)
* Sample-specific provider that appends UUID16 values of GATT Human Interface Device Service (HIDS) and GATT Battery Service (BAS)

The sample uses the following providers to generate the scan response data:

* Bluetooth device name provider (:kconfig:option:`CONFIG_BT_ADV_PROV_DEVICE_NAME`)
* Generic Access Profile (GAP) appearance provider (:kconfig:option:`CONFIG_BT_ADV_PROV_GAP_APPEARANCE`)

Other
=====

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
