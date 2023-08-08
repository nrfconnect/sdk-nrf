.. _matter_light_switch_sample:
.. _chip_light_switch_sample:

Matter: Light switch
####################

.. contents::
   :local:
   :depth: 2

This light switch sample demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a switch device that binds with lighting devices and changes the state of their LEDs.
You can use this sample as a reference for creating your own application.

When configured together with the :ref:`Matter light bulb <matter_light_bulb_sample>` sample (or other lighting sample) and when using a Matter controller, the light switch can control one light bulb directly or a group of light bulbs remotely over a Matter network built on top of a low-power, 802.15.4 Thread, or on top of a Wi-Fi network.
Support for both Thread and Wi-Fi is mutually exclusive and depends on the hardware platform, so only one protocol can be supported for a specific light switch device.
Depending on the network you choose:

* In case of Thread, this device works as a Thread :ref:`Sleepy End Device <thread_ot_device_types>`.
* In case of Wi-Fi, this device works in the :ref:`Legacy Power Save mode <ug_nrf70_developing_powersave_dtim_unicast>`.
  This means that the device sleeps most of the time and wakes up on each Delivery Traffic Indication Message (DTIM) interval to poll for pending messages.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

For this sample to work, you also need at least one :ref:`Matter light bulb <matter_light_bulb_sample>` sample programmed to another supported development kit.

To commission the device and run all required commands, you need also a :ref:`Matter controller <ug_matter_configuring_controller>`.
By default, this sample is configured to use the CHIP Tool as Matter controller.
See the :doc:`matter:chip_tool_guide` page in the Matter documentation for the CHIP Tool's setup information.

If you decide to use :ref:`matter_light_switch_sample_ui_matter_cli`, you also need a USB cable for the serial connection.

.. note::
    |matter_gn_required_note|


IPv6 network support
====================

The development kits for this sample offer the following IPv6 network support for Matter:

* Matter over Thread is supported for ``nrf52840dk_nrf52840``, ``nrf5340dk_nrf5340_cpuapp``, and ``nrf21540dk_nrf52840``.
* Matter over Wi-Fi is supported for ``nrf5340dk_nrf5340_cpuapp`` with the ``nrf7002ek`` shield attached or for ``nrf7002dk_nrf5340_cpuapp``.

Overview
********

The sample controls the state of the **LED 2** on connected light bulbs devices.
After configuring the light switch sample, the lighting devices get proper `Access Control List`_ from the Matter controller to start receiving commands sent from the light switch.
Then, the light switch device prepares a new binding table to be able to discover light bulb devices and perform :ref:`matter_light_switch_sample_binding`.

After the binding is complete, the application can control the state of the connected lighting devices in one of the following ways:

* With a single light bulb, it uses a Certificate-Authenticated Session Establishment session (CASE session) for direct communication with the single light bulb.
* With a group of light bulbs, it uses multicast messages sent through the IPv6 network using :ref:`matter_light_switch_sample_groupcast` with all light bulbs in the group.

.. _matter_light_switch_sample_acl:

Access Control List
===================

The Access Control List (ACL) is a list related to the Access Control cluster.
The list contains rules for managing and enforcing access control for a node's endpoints and their associated cluster instances.
In this sample's case, this allows the lighting devices to receive messages from the light switch and run them.

You can read more about ACLs on the :doc:`matter:access-control-guide` in the Matter documentation.

.. _matter_light_switch_sample_groupcast:

Group communication
===================

Group communication (groupcast or multicast) refers to messages and commands sent to the address of a group that includes multiple devices with the same Groups cluster.
The cluster manages the content of a node-wide Group Table that is part of the underlying interaction layer.
This is done on per endpoint basis.
After creating the Group cluster with specific ``ID`` and ``Name``, a device gets its own IPv6 multicast address and is ready to receive groupcast commands.

In this sample, the light switch device is able to create a groupcast message and send it to the chosen IPv6 multicast address.
This allows the light switch more than one lighting devices at the same time.

.. note::
   Writing the groupcast table on the devices blocks sending unicast commands.
   If you want to go back to the original state, perform factory reset of the device.

.. _matter_light_switch_sample_binding:

Binding
=======

.. matter_light_switch_sample_binding_start

Binding refers to establishing a relationship between endpoints on the local and remote nodes.
With binding, local endpoints are pointed and bound to the corresponding remote endpoints.
Both must belong to the same cluster type.
Binding lets the local endpoint know which endpoints are going to be the target for the client-generated actions on one or more remote nodes.

.. matter_light_switch_sample_binding_end

In this sample, the light switch controls one or more lighting devices, but does not know the remote endpoints of the lights (on remote nodes).
Using binding, the light switch device updates its Binding cluster with all relevant information about the lighting devices, such as their IPv6 address, node ID, and the IDs of the remote endpoints that contains the On/Off cluster and the LevelControl cluster, respectively.



Configuration
*************

|config|

Matter light switch build types
===============================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_configuration_file_types_start
    :end-before: matter_door_lock_sample_configuration_file_types_end

FEM support
===========

.. include:: /includes/sample_fem_support.txt

Device Firmware Upgrade support
===============================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_build_with_dfu_start
    :end-before: matter_door_lock_sample_build_with_dfu_end

Factory data support
====================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_factory_data_start
    :end-before: matter_door_lock_sample_factory_data_end

.. _matter_light_switch_sample_ui:

User interface
**************

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_led1_start
    :end-before: matter_door_lock_sample_led1_end

LED 2:
   The LED starts blinking evenly (500 ms on/500 ms off) when the Identify command of the Identify cluster is received on the endpoint ``1``.
   The command's argument can be used to specify the duration of the effect.

All LEDs:
   Blink in unison when the factory reset procedure is initiated.

.. include:: ../lock/README.rst
   :start-after: matter_door_lock_sample_button1_start
   :end-before: matter_door_lock_sample_button1_end

Button 2:
   * On nRF52840 DK, nRF5340 DK and nRF21540 DK:

     * Controls the light on the bound lighting device.
       Depending on how long you press the button:

       * If pressed for less than 0.5 seconds, it changes the light state to the opposite one on the bound lighting device (:ref:`light bulb <matter_light_bulb_sample>`).
       * If pressed for more than 0.5 seconds, it changes the brightness of the light on the bound lighting bulb device (:ref:`light bulb <matter_light_bulb_sample>`).
         The brightness is changing from 0% to 100% with 1% increments every 300 milliseconds as long as **Button 2** is pressed.

   * On nRF7002 DK:

     * If the device is not commissioned to a Matter network, it starts the NFC tag emulation, enables Bluetooth LE advertising for the predefined period of time (15 minutes by default), and makes the device discoverable over Bluetooth LE.
       This button is used during the :ref:`commissioning procedure <matter_light_switch_sample_remote_control_commissioning>`.
     * If the device is commissioned to a Matter network, it controls the light on the bound lighting device.
       Depending on how long you press the button:

       * If pressed for less than 0.5 seconds, it changes the light state to the opposite one on the bound lighting device (:ref:`light bulb <matter_light_bulb_sample>`).
       * If pressed for more than 0.5 seconds, it changes the brightness of the light on the bound lighting bulb device (:ref:`light bulb <matter_light_bulb_sample>`).
         The brightness is changing from 0% to 100% with 1% increments every 300 milliseconds as long as **Button 2** is pressed.

Button 4:
   * On nRF52840 DK, nRF5340 DK and nRF21540 DK:
     Starts the NFC tag emulation, enables Bluetooth LE advertising for the predefined period of time (15 minutes by default), and makes the device discoverable over Bluetooth LE.
     This button is used during the :ref:`commissioning procedure <matter_light_switch_sample_remote_control_commissioning>`.

   * On nRF7002 DK: Not available.

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_jlink_start
    :end-before: matter_door_lock_sample_jlink_end

NFC port with antenna attached:
   Optionally used for obtaining the `Onboarding information`_ from the Matter accessory device to start the :ref:`commissioning procedure <matter_light_switch_sample_remote_control_commissioning>`.

.. _matter_light_switch_sample_ui_matter_cli:

Matter CLI commands
===================

If you build the application using the ``debug`` or ``no_dfu`` build type, you can use a series of commands to control the light switch device.
These commands can be sent to one device (unicast) or a group of devices (groupcast).

Unicast commands
----------------

You can use the following commands for direct communication with the single lighting device:

switch onoff on
   This command turns on **LED 2** on the bound lighting device.
   For example:

   .. parsed-literal::
      :class: highlight

      uart:~$ matter switch onoff on

switch onoff off
   This command turns off **LED 2** on the bound lighting device.
   For example:

   .. parsed-literal::
      :class: highlight

      uart:~$ matter switch onoff off

switch onoff toggle
   This command changes the **LED 2** state to the opposite one on the bound lighting device.
   For example:

   .. parsed-literal::
      :class: highlight

      uart:~$ matter switch onoff toggle

Groupcast commands
------------------

You can use the following commands a group of devices that are programmed with the Light Switch Example application by using the Matter CLI:

switch groups onoff on
   This command turns on **LED 2** on each bound lighting device connected to the same group.
   For example:

   .. parsed-literal::
      :class: highlight

      uart:~$ matter switch groups onoff on

switch groups onoff off
   This command turns off **LED 2** on each bound lighting device connected to the same group.
   For example:

   .. parsed-literal::
      :class: highlight

      uart:~$ matter switch groups onoff off

switch groups onoff toggle
   This command changes the **LED 2** state to the opposite one on each bound lighting device connected to the same group.
   For example:

   .. parsed-literal::
      :class: highlight

      uart:~$ matter switch groups onoff toggle

Building and running
********************

.. |sample path| replace:: :file:`samples/matter/light_switch`

.. include:: /includes/build_and_run.txt

See `Configuration`_ for information about building the sample with the DFU support.

Selecting a build type
======================

Before you start testing the application, you can select one of the `Matter light switch build types`_, depending on your building method.

Selecting a build type in |VSC|
-------------------------------

.. include:: /config_and_build/modifying.rst
   :start-after: build_types_selection_vsc_start
   :end-before: build_types_selection_vsc_end

Selecting a build type from command line
----------------------------------------

.. include:: /config_and_build/modifying.rst
   :start-after: build_types_selection_cmd_start
   :end-before: For example, you can replace the

For example, you can replace the *selected_build_type* variable to build the ``release`` firmware for ``nrf52840dk_nrf52840`` by running the following command in the project directory:

.. parsed-literal::
   :class: highlight

   west build -b nrf52840dk_nrf52840 -d build_nrf52840dk_nrf52840 -- -DCONF_FILE=prj_release.conf

The ``build_nrf52840dk_nrf52840`` parameter specifies the output directory for the build files.

.. note::
   If the selected board does not support the selected build type, the build is interrupted.
   For example, if the ``shell`` build type is not supported by the selected board, the following notification appears:

   .. code-block:: console

      File not found: ./ncs/nrf/samples/matter/light_switch/configuration/nrf52840dk_nrf52840/prj_shell.conf

Testing
*******

After building the sample and programming it to your development kit, complete the steps in the following sections.

.. _matter_light_switch_sample_prepare_for_testing:

Prepare for testing
===================

After building this and the :ref:`Matter Light Bulb <matter_light_bulb_sample>` samples, and programming them to the development kits, complete the following steps:

.. note::
   In both samples (light switch and light bulb), a Bluetooth LE discriminator is set with the same value by default (hexadecimal: ``0xF00``; decimal: ``3840``).
   This means that only one uncommissioned device can be powered up before commissioning.
   If both are powered up at the same time, the CHIP Tool can commission a random device and the node ID assignment is also random.
   When one device is commissioned, power up the next device and perform the commissioning.
   To avoid this unclear situation, you can set up your unique discriminator in :file:`src/chip_project_config.h` file by changing :kconfig:option:`CHIP_DEVICE_CONFIG_USE_TEST_SETUP_DISCRIMINATOR` value.
   Then build an example and commission with your unique discriminator.

.. matter_light_switch_sample_prepare_to_testing_start

1. |connect_kit|
#. |connect_terminal_ANSI|
#. If devices were not erased during the programming, press and hold **Button 1** on each device until the factory reset takes place.
#. On each device, press **Button 4** to start the Bluetooth LE advertising.
#. Commission devices to the Matter network.
   See `Commissioning the device`_ for more information.
   During the commissioning process, write down the values for the light switch node ID and the light bulb node ID (or IDs, if you are using more than one light bulb).
   These IDs are going to be used in the next steps (*<light_switch_node_ID>* and *<light_bulb_node_ID>*, respectively).
#. Use the :doc:`CHIP Tool <matter:chip_tool_guide>` ("Writing ACL to the ``accesscontrol`` cluster" section) to add proper ACL for the light bulb device.
   Depending on the number of the light bulb devices you are using, use one of the following commands, with *<light_switch_node_ID>* and *<light_bulb_node_ID>* values from the previous step about commissioning:

   * If you are using only one light bulb device, run the following command for the light bulb device:

     .. parsed-literal::
        :class: highlight

        chip-tool accesscontrol write acl '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [112233], "targets": null}, {"fabricIndex": 1, "privilege": 3, "authMode": 2, "subjects": [<light_switch_node_ID>], "targets": [{"cluster": 6, "endpoint": 1, "deviceType": null}, {"cluster": 8, "endpoint": 1, "deviceType": null}]}]' <light_bulb_node_ID> 0

   * If you are using more than one light bulb device, connect all devices to the multicast group by running the following command for each device, including the light switch:

     .. parsed-literal::
        :class: highlight

        chip-tool tests TestGroupDemoConfig --nodeId <node_ID>

     Use the *<node_ID>* values from the commissioning step.

#. Write a binding table to the light switch to inform the device about all endpoints by running this command (only for light switch):

   * For unicast binding to bind the light switch with only one light Bulb:

      .. parsed-literal::
         :class: highlight

         chip-tool binding write binding '[{"fabricIndex": 1, "node": <light bulb node id>, "endpoint": 1, "cluster": 6}, {"fabricIndex": 1, "node": <light bulb node id>, "endpoint": 1, "cluster": 8}]' <light switch node id> 1

   * For groupcast binding to bind the light switch with multiple light bulbs:

      .. parsed-literal::
         :class: highlight

         chip-tool binding write binding '[{"fabricIndex": 1, "group": 257}]' <light_switch_node_ID> 1

.. matter_light_switch_sample_prepare_to_testing_end

All devices are now bound and ready for testing communication.

.. note::

   In this sample, the ACL cluster is inserted into the light bulb's endpoint ``0``, and the Binding cluster is inserted into the light switch's endpoint ``1``.

Testing with bound light bulbs devices
======================================

.. matter_light_switch_sample_testing_start

After preparing devices for testing, you can test the communication either of a single light bulb or of a group of light bulbs with the light switch (but not both a single device and a group at the same time).

Complete the following steps:

1. On the light switch device, use :ref:`buttons <matter_light_switch_sample_ui>` to control the bound light bulbs:

   #. On the light switch device, press **Button 2** to turn off the **LED 2** located on the bound light bulb device.
   #. On the light switch device, press **Button 2** to turn on the light again.
      **LED 2** on the light bulb device turns back on.
   #. Press **Button 2** and hold it for more than 0.5 seconds to test the dimmer functionality.
      **LED 2** on the bound light bulb device changes its brightness from 0% to 100% with 1% increments every 300 milliseconds as long as **Button 2** is pressed.

#. Using the terminal emulator connected to the light switch, run the following :ref:`Matter CLI commands <matter_light_switch_sample_ui_matter_cli>`:

   a. Write the following command to turn on **LED 2** located on the bound light bulb devices:

      * For a single bound light bulb:

        .. parsed-literal::
           :class: highlight

           matter switch onoff on

      * For a group of light bulbs:

        .. parsed-literal::
           :class: highlight

           matter switch groups onoff on

   #. Write the following command to turn on **LED 2** located on the bound light bulb device:

      * For a single bound light bulb:

        .. parsed-literal::
           :class: highlight

           matter switch onoff off

      * For a group of light bulbs:

        .. parsed-literal::
           :class: highlight

           matter switch groups onoff off


.. matter_light_switch_sample_testing_end

.. _matter_light_switch_sample_remote_control_commissioning:

Commissioning the device
========================

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_commissioning_start
    :end-before: matter_door_lock_sample_commissioning_end

Before starting the commissioning procedure, the device must be made discoverable over Bluetooth LE.
By default, the device is not discoverable automatically upon startup.
Press the following button to enable the Bluetooth LE advertising:

* On nRF52840 DK, nRF5340 DK, and nRF21540 DK: Press **Button 4**.
* On nRF7002 DK: Press **Button 2**.

Onboarding information
----------------------

When you start the commissioning procedure, the controller must get the onboarding information from the Matter accessory device.
The onboarding information representation depends on your commissioner setup.

For this sample, you can use one of the following :ref:`onboarding information formats <ug_matter_network_topologies_commissioning_onboarding_formats>` to provide the commissioner with the data payload that includes the device discriminator and the setup PIN code:

  .. list-table:: Light switch sample onboarding information
     :header-rows: 1

     * - QR Code
       - QR Code Payload
       - Manual pairing code
     * - Scan the following QR code with the app for your ecosystem:

         .. figure:: ../../../doc/nrf/images/matter_qr_code_light_switch.png
            :width: 200px
            :alt: QR code for commissioning the light switch device

       - MT:4CT9142C00KA0648G00
       - 34970112332

.. include:: ../lock/README.rst
    :start-after: matter_door_lock_sample_onboarding_start
    :end-before: matter_door_lock_sample_onboarding_end

|matter_cd_info_note_for_samples|

Upgrading the device firmware
=============================

To upgrade the device firmware, complete the steps listed for the selected method in the :doc:`matter:nrfconnect_examples_software_update` tutorial in the Matter documentation.

Dependencies
************

This sample uses the Matter library that includes the |NCS| platform integration layer:

* `Matter`_

In addition, the sample uses the following |NCS| components:

* :ref:`dk_buttons_and_leds_readme`

The sample depends on the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`
