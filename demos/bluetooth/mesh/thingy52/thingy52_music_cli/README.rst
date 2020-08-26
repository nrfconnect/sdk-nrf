.. _bluetooth_mesh_thingy52_music_cli:

Bluetooth: Mesh Thingy:52 Music Client
#######################################

The Thingy:52 music client demonstrates how to send different audio frequency ranges to different groups using Bluetooth Mesh.
In this way one or more :ref:`bluetooth_mesh_thingy52_srv` nodes can act as a simple real time analyzer (RTA) for music, by displaying different frequency ranges with different colors.

Overview
********

By pushing the button on the Thingy:52, the on-board microphone starts sapling incoming sound. Pushing the button again deactivates the sampling.

Three frequency ranges are used as default:

.. table::
   :align: center

   =================  ================== ==================
   Range 0            Range 1            Range 2
   =================  ================== ==================
   0 Hz - 2500 Hz     2531 Hz - 3594 Hz  3625 Hz - 7969 Hz
   =================  ================== ==================

Provisioning
============

Provisioning is handled by the :ref:`bt_mesh_dk_prov`.

Models
======

The following table shows the Thingy:52 client composition data for this demo:

.. table::
   :align: center

   =================  ================= ================= =================
   Element 1          Element 2         Element 3         Element4
   =================  ================= ================= =================
   Config Server      Thingy:52 client  Thingy:52 client  Thingy:52 client
   Health Server
   =================  ================= ================= =================

The models are used for the following purposes:

* :ref:`bt_mesh_thingy52_mod_readme` instances to control RGB messages. One client per frequency range.
* Config Server allows configurator devices to configure the node remotely.
* Health Server provides ``attention`` callbacks that are used during provisioning to call your attention to the device.
  These callbacks trigger blinking of the LEDs.


Audio processing
=================

The Thingy:52 microphone outputs PDM data that must be processed. This is done by ARM DSP. By doing Fast Fourier Transform (FFT) and computing magnitue for each array element,
raw PDM data is converted to a discrete spectrum array. Each element of the spectrum array represents one frequency, and the value of the element is magnitue.
As default, a PDM buffer size of 512 is used. This generates a discrete spectrum array of 256 (half of the PDM buffer size).
To find what frequency each element of the spectrum array represents, use this formula:

.. math::

   f_{n} = \frac{\text{sampling frequency}}{\text{PDM buffer size}} * n

The sampling frequency is 16000 Hz.

Requirements
************

This demo project is made for Thingy:52 and will only run on a Thingy:52 device.

The demo requires a smartphone with Nordic Semiconductor's nRF Mesh mobile app installed in one of the following versions:

  * `nRF Mesh mobile app for Android`_
  * `nRF Mesh mobile app for iOS`_

An additional requirement is one or more :ref:`bluetooth_mesh_thingy52_srv` demo(s) programmed on a separate Thingy:52
device(s) and configured according to the :ref:`bluetooth_mesh_thingy52_music_cli_testing` guide.

User interface
**************

When Button 0 (the only button) on the Thingy:52 is pressed, the on-board microphone starts sapling incoming sound. The main LED lights up when the microphone is sampling.
If the total volume of a frequency range is above a threshold, a message is sent with the corresponding client.
The message contains an RGB value that is mapped according to the total frequency range volume. When Button 0 is pressed again the sampling stops.

Building and running
********************

.. |sample path| replace:: :file:`demos/bluetooth/mesh/thingy52/thingy52_music_cli`

.. include:: /includes/build_and_run.txt

.. _bluetooth_mesh_thingy52_music_cli_testing:

Testing
=======

.. important::
   The Mesh music client for Thingy:52 cannot demonstrate any functionality on its own, and needs at least one device with the :ref:`bluetooth_mesh_thingy52_srv` demo running in the same mesh network.
   If less than three server devices are used, not all default frequency ranges can be displayed.
After programming the demo to your Thingy:52, you can test it by using a smartphone with Nordic Semiconductor’s nRF Mesh app installed.
Testing consists of provisioning the device and configuring it for communication with the mesh models.

Provisioning the device
-----------------------

The provisioning assigns an address range to the device, and adds it to the mesh network.
Complete the following steps in the nRF Mesh app:
1. Tap :guilabel:`Add node` to start scanning for unprovisioned mesh devices.
#. Select the :guilabel:`Thingy:52 Music Client` device to connect to it.
#. Tap :guilabel:`Identify` and then :guilabel:`Provision` to provision the device.
#. When prompted, select an OOB method and follow the instructions in the app.

Once the provisioning is complete, the app returns to the Network screen.

.. _bluetooth_mesh_thingy52_music_cli_config_models:

Configuring models
------------------

Complete the following steps in the nRF Mesh app to configure models:

1. On the Network screen, tap the :guilabel:`Thingy:52 Music Client` node.
   Basic information about the mesh node and its configuration is displayed.
#. For the three last elements in the Elements list do the following:
    * Tap :guilabel:`Vendor Model` to see the model's configuration.
    * Bind the model to application keys to make it open for communication:

        a. Tap :guilabel:`BIND KEY` at the top of the screen.
        #. Select :guilabel:`Application Key 1` from the list.

    * Set the publishing parameters:

        a. Tap :guilabel:`SET PUBLICATION`.
        #. Tap :guilabel:`Publish Address`.
        #. Select :guilabel:`Groups` from the drop-down menu.
        #. Select an existing group or create a new one.

        .. note::
            A group represents one frequency range. Servers displaying this range must subscribe to the same group.

    #. Tap :guilabel:`OK`.
    #. Set the Retransmit Count to zero (:guilabel:`Disabled`) to reduce the amount of messages sent.
    #. Tap the confirmation button at the bottom right corner of the app to save the parameters.

The vendor client model is now configured and should be able to send data to servers.


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`bt_mesh_dk_prov`
* :ref:`dk_buttons_and_leds_readme`

It also uses the following custom libraries:

* :ref:`bt_mesh_thingy52_mod_readme`

In addition, it uses the following Zephyr libraries:

* ``include/drivers/hwinfo.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``

* :ref:`zephyr:bluetooth_mesh`:

  * ``include/bluetooth/mesh.h``
