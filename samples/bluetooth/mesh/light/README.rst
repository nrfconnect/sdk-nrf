.. _bluetooth_mesh_light:

Bluetooth: Mesh light
#####################

.. contents::
   :local:
   :depth: 2

The Bluetooth® mesh light sample demonstrates how to set up a mesh server model application, and control LEDs with Bluetooth mesh using the :ref:`bt_mesh_onoff_readme`.

.. note::
   This sample is self-contained, and can be tested on its own.
   However, it is required when testing the :ref:`bluetooth_mesh_light_switch` sample.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuapp_ns, nrf52840dk_nrf52840, nrf52dk_nrf52832, nrf52833dk_nrf52833, nrf52833dk_nrf52820, thingy53_nrf5340_cpuapp_and_cpuapp_ns

The sample also requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

  * `nRF Mesh mobile app for Android`_
  * `nRF Mesh mobile app for iOS`_

.. note::
   |thingy53_sample_note|

Overview
********

The mesh light sample is a Generic OnOff Server with a provisionee role in a mesh network.
There can be one or more servers in the network, for example light bulbs.

The sample instantiates up to four instances of the Generic OnOff Server model for controlling LEDs.
The number of OnOff Server instances depends on available LEDs, as defined in board DTS file.

Provisioning is performed using the `nRF Mesh mobile app`_.
This mobile application is also used to configure key bindings, and publication and subscription settings of the Bluetooth mesh model instances in the sample.
After provisioning and configuring the mesh models supported by the sample in the `nRF Mesh mobile app`_, you can control the LEDs on the development kit from the app.

Provisioning
============

The provisioning is handled by the :ref:`bt_mesh_dk_prov`.
It supports four types of out-of-band (OOB) authentication methods, and uses the Hardware Information driver to generate a deterministic UUID to uniquely represent the device.

Models
======

The following table shows the mesh light composition data for this sample:

.. table::
   :align: center

   =================  =================  =================  =================
   Element 1          Element 2          Element 3          Element 4
   =================  =================  =================  =================
   Config Server      Gen. OnOff Server  Gen. OnOff Server  Gen. OnOff Server
   Health Server
   Gen. OnOff Server
   =================  =================  =================  =================

.. note::
   When used with :ref:`zephyr:thingy53_nrf5340`, Element 4 is not available.
   :ref:`zephyr:thingy53_nrf5340` supports only one RGB LED, and treats each RGB LED channel as a separate LED.

The models are used for the following purposes:

* :ref:`bt_mesh_onoff_srv_readme` instances in elements 1 to N, where N is number of on board LEDs, each control LEDs 1 to N, respectively.
* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.

The model handling is implemented in :file:`src/model_handler.c`, which uses the :ref:`dk_buttons_and_leds_readme` library to control each LED on the development kit according to the matching received messages of Generic OnOff Server.

User interface
**************

Buttons:
   Can be used to input the OOB authentication value during provisioning.
   All buttons have the same functionality during this procedure.

LEDs:
   Show the OOB authentication value during provisioning if the "Push button" OOB method is used.
   Show the OnOff state of the Generic OnOff Server of the corresponding element.

Configuration
*************

|config|

Source file setup
=================

This sample is split into the following source files:

* :file:`main.c` used to handle initialization.
* :file:`model_handler.c` used to handle mesh models.
* :file:`thingy53.c` used to handle preinitialization of the :ref:`zephyr:thingy53_nrf5340` board.
  Only compiled when the sample is built for :ref:`zephyr:thingy53_nrf5340` board.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/light`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_light_testing:

Testing
=======

After programming the sample to your development kit, you can test it by using a smartphone with `nRF Mesh mobile app`_ installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

Provisioning the device
-----------------------

.. |device name| replace:: :guilabel:`Mesh Light`

.. include:: /includes/mesh_device_provisioning.txt

Configuring models
------------------

See :ref:`ug_bt_mesh_model_config_app` for details on how to configure the mesh models with the nRF Mesh mobile app.

Configure the Generic OnOff Server model on each element on the :guilabel:`Mesh Light` node:

* Bind the model to :guilabel:`Application Key 1`.

  Once the model is bound to the application key, you can control the first LED on the device.
* In the model view, tap :guilabel:`ON` (one of the Generic On Off Controls) to light up the first LED on the development kit.

Make sure to complete the configuration on each of the elements on the node to enable controlling each of the remaining three LEDs.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_onoff_srv_readme`
* :ref:`bt_mesh_dk_prov`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/drivers/hwinfo.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``

* :ref:`zephyr:bluetooth_mesh`:

  * ``include/bluetooth/mesh.h``
