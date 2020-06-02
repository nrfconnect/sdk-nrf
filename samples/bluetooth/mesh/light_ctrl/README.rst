.. _bluetooth_mesh_light_lc:

Bluetooth: Mesh Light Control
#############################

The Bluetooth Mesh Light Control sample demonstrates how to set up a light control Mesh server model application and control a dimmable LED with the Bluetooth Mesh, using the :ref:`bt_mesh_onoff_readme`.

Overview
********

This sample is split into three source files:

* A :file:`main.c` file to handle initialization.
* A file for handling Mesh models, :file:`model_handler.c`.
* A file for handling PWM driven control of the dimmable LED, :file:`lc_pwm_led.c`.

After provisioning and configuring the Mesh models supported by the sample in the `nRF Mesh mobile app`_, you can control the dimmable LED on the development kit from the app.

Provisioning
============

The provisioning is handled by the :ref:`bt_mesh_dk_prov`.

Models
======

The following table shows the Mesh light controller composition data for this sample:

.. table::
   :align: center

   +-------------------------------+----------------------------+
   | Element 1                     | Element 2                  |
   +===============================+============================+
   | Config Server                 | Gen. OnOff Server          |
   +-------------------------------+----------------------------+
   | Health Server                 | Light Control Server       |
   +-------------------------------+----------------------------+
   | Gen. Level Server             | Light Control Setup Server |
   +-------------------------------+----------------------------+
   | Gen. OnOff Server             |                            |
   +-------------------------------+----------------------------+
   | Gen. DTT Server               |                            |
   +-------------------------------+----------------------------+
   | Gen. Power OnOff Server       |                            |
   +-------------------------------+----------------------------+
   | Gen. Power OnOff Setup Server |                            |
   +-------------------------------+----------------------------+
   | Light Lightness Server        |                            |
   +-------------------------------+----------------------------+
   | Light Lightness Setup Server  |                            |
   +-------------------------------+----------------------------+

The models are used for the following purposes:

- The first element consists of a Config Server and a Health Server.
  The Config Server allows configurator devices to configure the node remotely.
  The Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.
- The eight other models contained within the first element are a result of a single implementation of the Light Lightness Server.
  These models are bound internally to the light source hardware you want to control (in this case the first LED of the DK).
  External control of the light source can be achieved directly by using either the Generic Level Server or the Generic OnOff Server in the first element.
- The three models contained within the second element are a result of a single implementation of the Light Control Server.
  Together they establish a controller that, if enabled, can take control over a Light Lightness Server instance, where the controller decides parameters such as fade time, lighting levels for different states, and inactivity timing.
  In this sample the Light Control Server is bound internally to the Light Lightness Server instance in the second element, and enabled by default on boot up.
  External control of the light source associated with the Light Lightness Server can be achieved indirectly by the Light Control Server, by using the Generic OnOff Server in the second element.

For more details, see :ref:`bt_mesh_lightness_srv_readme` and :ref:`bt_mesh_light_ctrl_srv_readme`.
:ref:`bt_mesh_light_ctrl_srv_readme`.

The model handling is implemented in :file:`src/model_handler.c`, which uses the :ref:`dk_buttons_and_leds_readme` and the
:ref:`zephyr:pwm_api` API to control the LEDs on the board.

Requirements
************

* One of the following development kits:

  * |nRF52DK|
  * |nRF52840DK|

* Smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

  * `nRF Mesh mobile app for Android`_
  * `nRF Mesh mobile app for iOS`_

User interface
**************

Buttons:
   Can be used to input the out-of-band (OOB) authentication value during provisioning.
   All buttons have the same functionality during this procedure.

LEDs:
   Show the OOB authentication value during provisioning if the "Push button" OOB method is used.
   First LED outputs the current light level of the Light Lightness Server in the second element.


Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/mesh/light_lc`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_light_ctrl_testing:

Testing
=======

After programming the sample to your board, you can test it by using a smartphone with Nordic Semiconductor's nRF Mesh app installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

Provisioning the device
-----------------------

The provisioning assigns an address range to the device, and adds it to the mesh network.
Complete the following steps in the nRF Mesh app:

1. Tap :guilabel:`Add node` to start scanning for unprovisioned mesh devices.
#. Select the :guilabel:`Mesh Light LC` device to connect to it.
#. Tap :guilabel:`Identify` and then :guilabel:`Provision` to provision the device.
#. When prompted, select the OOB method and follow the instructions in the app.

Once the provisioning is complete, the app returns to the Network screen.

Configuring models
------------------

Complete the following steps in the nRF Mesh app to configure models:

1. On the Network screen, tap the :guilabel:`Mesh Light LC` node.
   Basic information about the mesh node and its configuration is displayed.
#. In the Mesh node view, expand the second element.
   It contains the list of models in the second element of the node.
#. Tap :guilabel:`Generic OnOff Server` to see the model's configuration.
#. Bind the model to application keys to make it open for communication:

   1. Tap :guilabel:`BIND KEY` at the top of the screen.
   #. Select :guilabel:`Application Key 1` from the list.

   You are now able to control the first LED on the device by using the Generic On Off Controls in the model view.
#. Tap :guilabel:`ON` to light up the first LED on the development kit.

You should now see the following actions:

1. The LED fades from 0% to 100% over one second :guilabel:`Standby -> On`.
#. The LED stays at 100% for three seconds :guilabel:`On`.
#. The LED fades from 100% to 50% over one second :guilabel:`On -> Prolong`.
#. The LED stays at 50% for three seconds :guilabel:`Prolong`.
#. The LED fades from 50% to 0% over one second :guilabel:`Prolong -> Standby`.

.. figure:: /images/bt_mesh_light_ctrl_levels.svg
   :alt: Light level transitions over time

   Light level transitions over time

.. note::
    The configuration of light levels, fade time and timeouts can be changed by altering the configuration parameters
    in :file:`prj.conf`, and rebuilding the sample.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_lightness_srv_readme`
* :ref:`bt_mesh_light_ctrl_srv_readme`
* :ref:`bt_mesh_dk_prov`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following Zephyr libraries:

* ``include/drivers/hwinfo.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:pwm_api`:

  * ``drivers/pwm.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``

* :ref:`zephyr:bluetooth_mesh`:

  * ``include/bluetooth/mesh.h``
