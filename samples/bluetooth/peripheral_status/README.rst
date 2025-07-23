.. _peripheral_status:

Bluetooth: Peripheral Status
############################

.. contents::
   :local:
   :depth: 2


The peripheral status sample demonstrates how to use the :ref:`nsms_readme` library.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires a smartphone or tablet running a compatible application.
The `Testing`_ instructions refer to `nRF Connect for Mobile`_, but you can also use other similar applications (for example, `nRF Toolbox`_).

.. note::
   |thingy53_sample_note|

Overview
********

The sample implements two NSMS instances that result in two NSMS characteristics that describe the button status.
Each characteristic has a name set in Characteristic User Description (CUD) that corresponds to the button whose status is shown.

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      Status related to **Button 2** has configurable security settings.
      When :ref:`CONFIG_BT_STATUS_SECURITY_ENABLED <CONFIG_BT_STATUS_SECURITY_ENABLED>` is configured, the status related to **Button 2** has security enabled, thus it can be accessed only when bonded.

   .. group-tab:: nRF54 DKs

      Status related to **Button 1** has configurable security settings.
      When :ref:`CONFIG_BT_STATUS_SECURITY_ENABLED <CONFIG_BT_STATUS_SECURITY_ENABLED>` is configured, the status related to **Button 1** has security enabled, thus it can be accessed only when bonded.

User interface
**************

The user interface of the sample depends on the hardware platform you are using.

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Blinks with a period of two seconds with the duty cycle set to 50% when the main loop is running and the device is advertising.

      LED 2:
         Lit when the development kit is connected.

      Button 1:
         Set **Button 1** NSMS Status Characteristic to: "Pressed" or "Released".
         Notify the client if notification is enabled.

      Button 2 (the one hidden under the cover):
         Set **Button 2** NSMS Status Characteristic to: "Pressed" or "Released".
         Notify the client if notification is enabled.
         By default, this service requires the connection to be secured to read or notify.

   .. group-tab:: nRF54 DKs

      LED 0:
         Blinks with a period of two seconds with the duty cycle set to 50% when the main loop is running and the device is advertising.

      LED 1:
         Lit when the development kit is connected.

      Button 0:
         Set **Button 0** NSMS Status Characteristic to: "Pressed" or "Released".
         Notify the client if notification is enabled.

      Button 1 (the one hidden under the cover):
         Set **Button 1** NSMS Status Characteristic to: "Pressed" or "Released".
         Notify the client if notification is enabled.
         By default, this service requires the connection to be secured to read or notify.

   .. group-tab:: Thingy:53

      RGB LED:
         The RGB LED channels are used independently to display the following information:

         * Red - If the main loop is running (that is, the device is advertising).
           Blinks with a period of two seconds with the duty cycle set to 50% when the main loop is running and the device is advertising.
         * Green - If the device is connected.

         For example, if Thingy:53 is connected over Bluetooth, the LED color toggles between green and yellow.
         The green LED channel is kept on, and the red LED channel is blinking.

      Button 1:
         Set **Button 1** NSMS Status Characteristic to: "Pressed" or "Released".
         Notify the client if notification is enabled.

      Button 2 (the one hidden under the cover):
         Set **Button 2** NSMS Status Characteristic to: "Pressed" or "Released".
         Notify the client if notification is enabled.
         By default, this service requires the connection to be secured to read or notify.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. _CONFIG_BT_STATUS_SECURITY_ENABLED:

CONFIG_BT_STATUS_SECURITY_ENABLED - Enable Bluetooth® LE security
  This configuration enables Bluetooth® LE security implementation for Nordic Status Message instances.
  When you enable this option, one of the status messages will be displayed only when the connection is secure.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/peripheral_status`

.. include:: /includes/build_and_run_ns.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

Testing
=======

After programming the sample to your dongle or development kit, test it by performing the following steps:

1. Start the `nRF Connect for Mobile`_ application on your smartphone or tablet.
#. Power on the development kit, or insert your dongle into the USB port.
#. Connect to the device from the application.
   The device is advertising as ``Nordic_Status``.
   The services of the connected device are shown.
#. Find :guilabel:`Nordic Status Message Service` by its UUID listed in :ref:`nsms_readme`.
#. Read its :guilabel:`Characteristic User Description` to check which button it relates to.
#. Read :guilabel:`Nordic Status Message Service` message characteristic to check the initial status: if no button was pressed, it should be ``Unknown``.
#. Enable notification for the characteristic found.
#. Press the related button and observe the message change between ``Pressed`` and ``Released``.

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      .. note::
         Performing the same for **Button 2** characteristic requires a secured connection.

   .. group-tab:: nRF54 DKs

      .. note::
         Performing the same for **Button 1** characteristic requires a secured connection.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nsms_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/zephyr/types.h`
* :file:`lib/libc/minimal/include/errno.h`
* :file:`include/sys/printk.h`
* :file:`include/sys/byteorder.h`
* :ref:`GPIO Interface <zephyr:api_peripherals>`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/gatt.h`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
