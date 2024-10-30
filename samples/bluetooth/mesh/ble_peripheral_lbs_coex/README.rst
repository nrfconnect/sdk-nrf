.. _bluetooth_ble_peripheral_lbs_coex:

Bluetooth Mesh: Coexistence with other LE services
##################################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to combine Bluetooth® Mesh and another Bluetooth Low Energy (LE) service in a single application.

Requirements
************

This sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires a smartphone with `nRF Connect for Mobile`_ and one of the following apps:

* `nRF Mesh mobile app for Android`_
* `nRF Mesh mobile app for iOS`_

Overview
********

The purpose of this sample is to showcase an application where another Bluetooth LE service can operate independently of Bluetooth Mesh.
It combines the features of the :ref:`peripheral_lbs` sample and the :ref:`bluetooth_mesh_light` sample into a single application.

The :ref:`lbs_readme` controls the state of a LED and monitors the state of a button on the device.
Bluetooth Mesh controls and monitors the state of a separate LED on the device.
The LBS and Bluetooth Mesh do not share any states on the device and operate independently of each other.

To be truly independent, the LBS must be able to advertise its presence independently of Bluetooth Mesh.
Because Bluetooth Mesh uses the advertiser as its main channel of communication, you must configure the application to allow sharing of this resource.
To achieve this, extended advertising is enabled with two simultaneous advertising sets.
One of these sets is used to handle all Bluetooth Mesh communication, while the other is used for advertising the LBS.

.. note::
   Extended advertising is a requirement for achieving multiple advertisers in this sample.

LED Button Service
==================

.. tabs::

   .. group-tab:: nRF21, nRF52 and nRF53 DKs

      When connected, the :ref:`lbs_readme` sends the state of **Button 1** on the development kit to the connected device, such as a phone or tablet.
      The mobile application on the device can display the received button state and control the state of **LED 2** on the development kit.

   .. group-tab:: nRF54 DKs

      When connected, the :ref:`lbs_readme` sends the state of **Button 0** on the development kit to the connected device, such as a phone or tablet.
      The mobile application on the device can display the received button state and control the state of **LED 1** on the development kit.

Mesh provisioning
=================

The provisioning is handled by the :ref:`bt_mesh_dk_prov`.
It supports four types of out-of-band (OOB) authentication methods, and uses the Hardware Information driver to generate a deterministic UUID to uniquely represent the device.

Mesh models
===========

The following table shows the mesh light composition data for this sample:

   +---------------------+
   |  Element 1          |
   +=====================+
   | Config Server       |
   +---------------------+
   | Health Server       |
   +---------------------+
   | Gen. OnOff Server   |
   +---------------------+

The models are used for the following purposes:

.. tabs::

   .. group-tab:: nRF21, nRF52 and nRF53 DKs

      * :ref:`bt_mesh_onoff_srv_readme` instance in element 1 controls **LED 1**.
      * Config Server allows configurator devices to configure the node remotely.
      * Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
        These callbacks trigger blinking of the LEDs.

   .. group-tab:: nRF54 DKs

      * :ref:`bt_mesh_onoff_srv_readme` instance in element 1 controls **LED 0**.
      * Config Server allows configurator devices to configure the node remotely.
      * Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
        These callbacks trigger blinking of the LEDs.

The model handling is implemented in :file:`src/model_handler.c`, which uses the :ref:`dk_buttons_and_leds_readme` library to control each LED on the development kit according to the matching received messages of Generic OnOff Server.

User interface
**************

.. tabs::

   .. group-tab:: nRF21, nRF52 and nRF53 DKs

      Buttons (Common):
         Can be used to input the OOB authentication value during provisioning.
         All buttons have the same functionality during this procedure.

      Button 1:
         Send a notification through the LED Button Service with the button pressed or released state.

      LEDs (Common):
         Show the OOB authentication value during provisioning if the Push button OOB method is used.

      LED 1:
         Controlled by the Generic OnOff Server.

      LED 2:
         Controlled remotely by the LED Button Service from the connected device.

   .. group-tab:: nRF54 DKs

      Buttons (Common):
         Can be used to input the OOB authentication value during provisioning.
         All buttons have the same functionality during this procedure.

      Button 0:
         Send a notification through the LED Button Service with the button pressed or released state.

      LEDs (Common):
         Show the OOB authentication value during provisioning if the Push button OOB method is used.

      LED 0:
         Controlled by the Generic OnOff Server.

      LED 1:
         Controlled remotely by the LED Button Service from the connected device.

Configuration
*************

|config|

Source file setup
=================

This sample is split into the following source files:

* :file:`main.c` used to handle initialization.
* :file:`model_handler.c` used to handle mesh models.
* :file:`lb_service_handler.c` used to handle the LBS interaction.

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/ble_peripheral_lbs_coex`

.. include:: /includes/build_and_run.txt

Testing Generic OnOff Server (Bluetooth Mesh)
=============================================

After programming the sample to your development kit, you can test it by using a smartphone with `nRF Mesh mobile app`_ installed.
Testing consists of provisioning the device and configuring it for communication with the Bluetooth Mesh models.

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Mesh Light`

.. include:: /includes/mesh_device_provisioning.txt

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Configure the Generic OnOff Server model on the root element of the **Mesh and Peripheral Coex** node:

.. tabs::

   .. group-tab:: nRF21, nRF52 and nRF53 DKs

      1. Bind the model to **Application Key 1**.
         Once the model is bound to the application key, you can control **LED 1** on the device.

      #. In the model view, tap :guilabel:`ON`/:guilabel:`OFF` (one of the Generic On Off Controls).
         This switches the **LED 1** on the development kit on and off respectively.

   .. group-tab:: nRF54 DKs

      1. Bind the model to **Application Key 1**.
         Once the model is bound to the application key, you can control **LED 0** on the device.

      #. In the model view, tap :guilabel:`ON`/:guilabel:`OFF` (one of the Generic On Off Controls).
         This switches the **LED 0** on the development kit on and off respectively.

Testing LED Button Service (peripheral)
=======================================

After programming the sample to your development kit, test it by performing the following steps:

.. tabs::

   .. group-tab:: nRF21, nRF52 and nRF53 DKs

      1. Start the `nRF Connect for Mobile`_ application on your smartphone or tablet.
      #. Power on the development kit.
      #. Connect to the device from the nRF Connect application.
         The device is advertising as "Mesh and Peripheral Coex".
         The services of the connected device are shown.
      #. In **Nordic LED Button Service**, enable notifications for the **Button** characteristic.
      #. Press **Button 1** on the device.
      #. Observe that notifications with the following values are displayed:

         * ``Button released`` when **Button 1** is released.
         * ``Button pressed`` when **Button 1** is pressed.

      #. Write the following values to the LED characteristic in the **Nordic LED Button Service**:

         * Value ``OFF`` to switch the **LED 2** on the development kit off.
         * Value ``ON`` to switch the **LED 2** on the development kit on.

   .. group-tab:: nRF54 DKs

      1. Start the `nRF Connect for Mobile`_ application on your smartphone or tablet.
      #. Power on the development kit.
      #. Connect to the device from the nRF Connect application.
         The device is advertising as "Mesh and Peripheral Coex".
         The services of the connected device are shown.
      #. In **Nordic LED Button Service**, enable notifications for the **Button** characteristic.
      #. Press **Button 0** on the device.
      #. Observe that notifications with the following values are displayed:

         * ``Button released`` when **Button 0** is released.
         * ``Button pressed`` when **Button 0** is pressed.

      #. Write the following values to the LED characteristic in the **Nordic LED Button Service**:

         * Value ``OFF`` to switch the **LED 1** on the development kit off.
         * Value ``ON`` to switch the **LED 1** on the development kit on.

.. note::
   Once connected, the device will not advertise with the LBS Service UUID until the device is rebooted.
   Reboot the device to restart the LBS Service UUID advertisements.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_onoff_srv_readme`
* :ref:`bt_mesh_dk_prov`
* :ref:`lbs_readme`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* :file:`include/drivers/hwinfo.h`
* :file:`include/zephyr/types.h`
* :file:`lib/libc/minimal/include/errno.h`
* :file:`include/sys/printk.h`
* :file:`include/sys/byteorder.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/uuid.h`
  * :file:`include/bluetooth/gatt.h`

* :ref:`zephyr:bluetooth_mesh`:

  * :file:`include/bluetooth/mesh.h`
