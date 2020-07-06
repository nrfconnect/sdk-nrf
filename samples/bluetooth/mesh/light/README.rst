.. _bluetooth_mesh_light:

Bluetooth: Mesh Light
#####################

The Bluetooth Mesh Light sample demonstrates how to set up a basic Mesh server model application and control LEDs with the Bluetooth Mesh, using the :ref:`bt_mesh_onoff_readme`.

.. note::
    This sample is self-contained and can be tested on its own.
    However, it is required when testing the the :ref:`bluetooth_mesh_light_switch` sample.

Overview
********

This sample is split into two source files:

* A :file:`main.c` file to handle initialization.
* One additional file for handling Mesh models, :file:`model_handler.c`.

After provisioning and configuring the Mesh models supported by the sample in the `nRF Mesh mobile app`_, you can control the LEDs on the development kit from the app.

Provisioning
============

The provisioning is handled by the :ref:`bt_mesh_dk_prov`.

Models
======

The following table shows the Mesh light composition data for this sample:

.. table::
   :align: center

   =================  =================  =================  =================
   Element 1          Element 2          Element 3          Element 4
   =================  =================  =================  =================
   Config Server      Gen. OnOff Server  Gen. OnOff Server  Gen. OnOff Server
   Health Server
   Gen. OnOff Server
   =================  =================  =================  =================

The models are used for the following purposes:

* :ref:`bt_mesh_onoff_srv_readme` instances in elements 1 to 4 each control LEDs 1 to 4, respectively.
* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.

The model handling is implemented in :file:`src/model_handler.c`, which uses the :ref:`dk_buttons_and_leds_readme` to control each LED on the board according to the matching received messages of Generic OnOff Server.

Requirements
************

The sample supports the following development kits:

.. include:: /includes/boardname_tables/sample_boardnames.txt
   :start-after: set1_start
   :end-before: set1_end

.. note::
   If you use nRF5340 PDK, add the following options to the configuration of the network sample:

   .. code-block:: none

      CONFIG_BT_CTLR_TX_BUFFER_SIZE=74
      CONFIG_BT_CTLR_DATA_LENGTH_MAX=74

   This is required because Bluetooth Mesh has different |BLE| Controller requirements than other Bluetooth samples.

The sample also requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

  * `nRF Mesh mobile app for Android`_
  * `nRF Mesh mobile app for iOS`_

User interface
**************

Buttons:
   Can be used to input the OOB authentication value during provisioning.
   All buttons have the same functionality during this procedure.

LEDs:
   Show the OOB authentication value during provisioning if the "Push button" OOB method is used.
   Show the OnOff state of the Generic OnOff server of the corresponding element.


Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/light`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_light_testing:

Testing
=======

After programming the sample to your board, you can test it by using a smartphone with Nordic Semiconductor's nRF Mesh app installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

Provisioning the device
-----------------------

The provisioning assigns an address range to the device, and adds it to the mesh network.
Complete the following steps in the nRF Mesh app:

1. Tap :guilabel:`Add node` to start scanning for unprovisioned mesh devices.
#. Select the :guilabel:`Mesh Light` device to connect to it.
#. Tap :guilabel:`Identify` and then :guilabel:`Provision` to provision the device.
#. When prompted, select the OOB method and follow the instructions in the app.

Once the provisioning is complete, the app returns to the Network screen.

Configuring models
------------------

Complete the following steps in the nRF Mesh app to configure models:

1. On the Network screen, tap the :guilabel:`Mesh Light` node.
   Basic information about the mesh node and its configuration is displayed.
#. In the Mesh node view, expand the first element.
   It contains the list of models in the first element of the node.
#. Tap :guilabel:`Generic OnOff Server` to see the model's configuration.
#. Bind the model to application keys to make it open for communication:

   1. Tap :guilabel:`BIND KEY` at the top of the screen.
   #. Select :guilabel:`Application Key 1` from the list.

   You are now able to control the first LED on the device by using the Generic On Off Controls in the model view.
#. Tap :guilabel:`ON` to light up the first LED on the development kit.

Repeat steps 3-5 for each of the elements on the node to enable controling each of the remaining three LEDs.

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
