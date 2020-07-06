.. _bluetooth_mesh_light_switch:

Bluetooth: Mesh Light Switch
############################

The Bluetooth Mesh Light Switch sample demonstrates how to set up a basic Mesh client model application and control LEDs with the Bluetooth Mesh, using the :ref:`bt_mesh_onoff_readme`.

Overview
********

This sample is split into two source files:

* A :file:`main.c` file to handle initialization.
* One additional file for handling Mesh models, :file:`model_handler.c`.

Provisioning
============

Provisioning is handled by the :ref:`bt_mesh_dk_prov`.

Models
======

The following table shows the Mesh light composition data for this sample:

.. table::
   :align: center

   =================  =================  =================  =================
   Element 1          Element 2          Element 3          Element 4
   =================  =================  =================  =================
   Config Server      Gen. OnOff Client  Gen. OnOff Client  Gen. OnOff Client
   Health Server
   Gen. OnOff Client
   =================  =================  =================  =================

The models are used for the following purposes:

* :ref:`bt_mesh_onoff_cli_readme` instances in elements 1 to 4 are controlled by the buttons on the development kit.
* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.

The model handling is implemented in :file:`src/model_handler.c`, which uses the :ref:`dk_buttons_and_leds_readme` to detect button presses on the board.

If the model is configured to publish to a unicast address, the model handler calls :cpp:func:`bt_mesh_onoff_cli_set` to turn the LEDs of a Mesh Light device on or off.
The response from the target device updates the corresponding LED on the Mesh Light Switch device.
If the model is configured to publish to a group address, it calls :cpp:func:`bt_mesh_onoff_cli_set_unack` instead, to avoid getting responses from multiple devices at once.



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

The sample requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

  * `nRF Mesh mobile app for Android`_
  * `nRF Mesh mobile app for iOS`_

An additional requirement is the :ref:`bluetooth_mesh_light` sample application programmed on a separate device and configured according to the Mesh Light sample's :ref:`bluetooth_mesh_light_testing` guide.

User interface
**************

Buttons:
   Buttons are used to control the respective Generic OnOff Clients.
   When pressed, the button toggles the LED state on a :ref:`Mesh Light <bluetooth_mesh_light>` device.

LEDs:
   Show the last known OnOff state of the targeted :ref:`bluetooth_mesh_light` board.


Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/light_switch`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_light_switch_testing:

Testing
=======

.. important::
   The Light Switch sample cannot demonstrate any functionality on its own, and needs a device with the :ref:`bluetooth_mesh_light` sample running in the same mesh network.
   Before testing Mesh Light Switch, go through the Mesh Light's :ref:`bluetooth_mesh_light_testing` guide with a different board.

After programming the sample to your board, you can test it by using a smartphone with Nordic Semiconductor's nRF Mesh app installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

Provisioning the device
-----------------------

The provisioning assigns an address range to the device, and adds it to the mesh network.
Complete the following steps in the nRF Mesh app:

1. Tap :guilabel:`Add node` to start scanning for unprovisioned mesh devices.
#. Select the :guilabel:`Mesh Light Switch` device to connect to it.
#. Tap :guilabel:`Identify` and then :guilabel:`Provision` to provision the device.
#. When prompted, select the OOB method and follow the instructions in the app.

Once the provisioning and initial configuration is complete, the app will go back to the Network screen.

Configuring models
------------------

Complete the following steps in the nRF Mesh app to configure models:

1. On the Network screen, tap the :guilabel:`Mesh Light Switch` node.
   Basic information about the mesh node and its configuration is displayed.
#. In the Mesh node view, expand the first element.
   It contains the list of models in the first element of the node.
#. Tap :guilabel:`Generic OnOff Client` to see the model's configuration.
#. Bind the model to application keys to make it open for communication:

   1. Tap :guilabel:`BIND KEY` at the top of the screen.
   #. Select :guilabel:`Application Key 1` from the list.

#. Configure the Client model publish parameters, which define how the model will send its messages:

   1. Tap :guilabel:`SET PUBLICATION`.
   #. Set the Publish Address to the first unicast address of the Mesh Light node.
   #. Set the Retransmit Count to zero (:guilabel:`Disabled`) to prevent the model from sending each button press multiple times.
   #. Leave the rest of the publish parameters at their default values.
   #. Tap :guilabel:`APPLY` to confirm the configuration.

You are now be able to control the first LED on the Mesh Light device by pressing Button 1 on the Mesh Light Switch development kit.

Repeat steps 3-5 for each of the elements on the node to control each of the remaining three LEDs on the Mesh Light device.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_onoff_cli_readme`
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
