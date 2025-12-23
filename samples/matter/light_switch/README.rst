.. |matter_name| replace:: Light switch
.. |matter_type| replace:: sample
.. |matter_dks_thread| replace:: ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf54l15dk/nrf54l15/cpuapp``, and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets
.. |matter_dks_wifi| replace:: ``nrf54lm20dk/nrf54lm20a/cpuapp`` board target with the ``nrf7002eb2`` shield attached
.. |matter_dks_internal| replace:: nRF54LM20 DK
.. |matter_dks_tfm| replace:: ``nrf54l15dk/nrf54l15/cpuapp`` board target
.. |sample path| replace:: :file:`samples/matter/light_switch`
.. |matter_qr_code_payload| replace:: MT:K.K9042C00KA0648G00
.. |matter_pairing_code| replace:: 34970112332
.. |matter_qr_code_image| image:: /images/matter_qr_code_light_switch.png

.. include:: /includes/matter/shortcuts.txt

.. _matter_light_switch_sample:
.. _chip_light_switch_sample:

Matter: Light switch
####################

.. contents::
   :local:
   :depth: 2

This light switch sample demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a switch device that binds with lighting devices and changes the state of their LEDs.
When configured together with the :ref:`Matter light bulb <matter_light_bulb_sample>` sample (or other lighting sample) and when using a Matter controller, the light switch can control one light bulb directly or a group of light bulbs remotely over a Matter network built on top of a low-power, 802.15.4 Thread, or on top of a Wi-Fi® network.

.. include:: /includes/matter/introduction/sleep_thread_wifi.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/matter/requirements/thread_wifi.txt

For this sample to work, you also need at least one :ref:`Matter light bulb <matter_light_bulb_sample>` sample programmed to another supported development kit.

Overview
********

The sample controls the state of the state-indication LED on connected light bulbs devices.
After configuring the light switch sample, the lighting devices get proper `Access Control List`_ from the Matter controller to start receiving commands sent from the light switch.
Then, the light switch device prepares a new binding table to be able to discover light bulb devices and perform :ref:`matter_light_switch_sample_binding`.

After the binding is complete, the application can control the state of the connected lighting devices in one of the following ways:

* With a single light bulb, it uses a Certificate-Authenticated Session Establishment session (CASE session) for direct communication with the single light bulb.
* With a group of light bulbs, it uses multicast messages sent through the IPv6 network using :ref:`matter_light_switch_sample_groupcast` with all light bulbs in the group.

Light switch features
=====================

The light switch sample implements the following features:

* :ref:`Access Control List <matter_light_switch_sample_acl>` - The light switch can control the state of the connected lighting devices.
* :ref:`Group communication <matter_light_switch_sample_groupcast>` - The light switch can control a group of lighting devices.
* :ref:`Binding <matter_light_switch_sample_binding>` - The light switch can bind with the lighting devices to be able to control them.
* :ref:`LIT ICD <matter_light_switch_sample_lit>` - The light switch can be used as an Intermittently Connected Device (ICD) with a :ref:`Long Idle Time (LIT)<ug_matter_device_low_power_icd_sit_lit>`.

Use the ``click to show`` toggle to expand the content.

.. _matter_light_switch_sample_acl:

.. include:: /includes/matter/overview/acl.txt

.. _matter_light_switch_sample_groupcast:

.. include:: /includes/matter/overview/groups.txt

.. _matter_light_switch_sample_binding:

.. include:: /includes/matter/overview/binding.txt

.. _matter_light_switch_sample_lit:

.. include:: /includes/matter/overview/icd_lit.txt

Configuration
*************

.. include:: /includes/matter/configuration/intro.txt

The |matter_type| supports the following build configurations:

.. include:: /includes/matter/configuration/basic_internal.txt

Advanced configuration options
==============================

.. include:: /includes/matter/configuration/advanced/intro.txt
.. include:: /includes/matter/configuration/advanced/tfm.txt
.. include:: /includes/matter/configuration/advanced/dfu.txt
.. include:: /includes/matter/configuration/advanced/factory_data.txt
.. include:: /includes/matter/configuration/advanced/custom_board.txt
.. include:: /includes/matter/configuration/advanced/internal_memory.txt

.. _matter_light_switch_snippets:

Snippets
========

.. |snippet| replace:: :makevar:`light_switch_SNIPPET`

.. include:: /includes/sample_snippets.txt

The following snippet is available:

* ``lit_icd`` - Enables experimental LIT ICD support.

  .. |snippet_zap_file| replace:: :file:`snippets/lit_icd/light_switch.zap`
  .. |snippet_dir| replace:: :file:`snippets/lit_icd`

.. include:: /includes/matter/configuration/snippets_note.txt

.. _matter_light_switch_sample_ui:

User interface
**************

.. include:: /includes/matter/interface/intro.txt

First LED:
   .. include:: /includes/matter/interface/state_led.txt

Second LED:
   The LED starts blinking evenly (500 ms on/500 ms off) when the Identify command of the Identify cluster is received on the endpoint ``1``.
   The command's argument can be used to specify the duration of the effect.

First Button:
   .. include:: /includes/matter/interface/main_button.txt

Second Button:
   Controls the light on the bound lighting device.
   Depending on how long you press the button:

   * If pressed for less than 0.5 seconds, it changes the light state to the opposite one on the bound lighting device (:ref:`light bulb <matter_light_bulb_sample>`).
   * If pressed for more than 0.5 seconds, it changes the brightness of the light on the bound lighting bulb device (:ref:`light bulb <matter_light_bulb_sample>`).
      The brightness is changing from 0% to 100% with 1% increments every 300 milliseconds as long as the |Second Button| is pressed.

Third Button:
   Functions as the User Active Mode Trigger (UAT) button.
   For more information about Intermittently Connected Devices (ICD) and User Active Mode Trigger, see the :ref:`ug_matter_device_low_power_icd` documentation section.

   .. note::
      To enable this functionality, :ref:`activate the lit_icd snippet <matter_light_switch_snippets>`.
      ICD and UAT functionality is currently supported only for Matter over Thread.

.. include:: /includes/matter/interface/segger_usb.txt
.. include:: /includes/matter/interface/nfc.txt

.. _matter_light_switch_sample_ui_matter_cli:

Matter CLI commands
===================

You can use a series of commands to control the light switch device.
These commands can be sent to one device (unicast) or a group of devices (groupcast).

Unicast commands
----------------

.. toggle::

   You can use the following commands for direct communication with the single lighting device:

   switch onoff on
      This command turns on the state-indication LED on the bound lighting device.
      For example:

      .. parsed-literal::
         :class: highlight

         uart:~$ matter switch onoff on

   switch onoff off
      This command turns off the state-indication LED on the bound lighting device.
      For example:

      .. parsed-literal::
         :class: highlight

         uart:~$ matter switch onoff off

   switch onoff toggle
      This command changes the state of the state-indication LED to the opposite state on the bound lighting device.
      For example:

      .. parsed-literal::
         :class: highlight

         uart:~$ matter switch onoff toggle

Groupcast commands
------------------

.. toggle::

   You can use the following commands a group of devices that are programmed with the Light Switch Example application by using the Matter CLI:

   switch groups onoff on
      This command turns on the state-indication LED on each bound lighting device connected to the same group.
      For example:

      .. parsed-literal::
         :class: highlight

         uart:~$ matter switch groups onoff on

   switch groups onoff off
      This command turns off the state-indication LED on each bound lighting device connected to the same group.
      For example:

      .. parsed-literal::
         :class: highlight

         uart:~$ matter switch groups onoff off

   switch groups onoff toggle
      This command changes the state of the state-indication LED to the opposite state on each bound lighting device connected to the same group.
      For example:

      .. parsed-literal::
         :class: highlight

         uart:~$ matter switch groups onoff toggle

Building and running
********************

.. include:: /includes/matter/building_and_running/intro.txt
.. include:: /includes/matter/building_and_running/select_configuration.txt
.. include:: /includes/matter/building_and_running/commissioning.txt

|matter_ble_advertising_auto|

.. include:: /includes/matter/building_and_running/onboarding.txt

Advanced building options
=========================

.. include:: /includes/matter/building_and_running/advanced/intro.txt
.. include:: /includes/matter/building_and_running/advanced/building_nrf54lm20dk_7002eb2.txt
.. include:: /includes/matter/building_and_running/advanced/wifi_flash.txt

Testing
*******

.. |endpoint_name| replace:: **OnOff cluster**
.. |endpoint_id| replace:: **1**

.. include:: /includes/matter/testing/intro.txt
.. include:: /includes/matter/testing/prerequisites.txt

* In this sample, the ACL cluster is inserted into the light bulb's endpoint ``0``, and the Binding cluster is inserted into the light switch's endpoint ``1``.

.. include:: /includes/matter/testing/prepare.txt

Testing steps
=============

.. matter_light_switch_sample_prepare_to_testing_start

1. If the devices were not erased during the programming, press and hold the |First Button| on each device until the factory reset takes place.
#. On each device, press the |First Button| to start the Bluetooth LE advertising.
#. Commission the devices to the Matter network.
   See `Commissioning the device`_ for more information.

   During the commissioning process, write down the values for the light switch node ID and the light bulb node ID (or IDs, if you are using more than one light bulb).
   These IDs are used in the next steps (*<light_switch_node_ID>* and *<light_bulb_node_ID>*, respectively).
#. Use the :doc:`CHIP Tool <matter:chip_tool_guide>` ("Writing ACL to the ``accesscontrol`` cluster" section) to add proper ACL for the light bulb devices, establish a group for groupcast and bind the light switch.
   Depending on the number of the light bulb devices you are using, use one of the following commands, with *<light_switch_node_ID>* and *<light_bulb_node_ID>* values from the previous step about commissioning.

   If you are using only one light bulb device, follow the instructions in the **unicast** tab to bind the light switch with the light bulb device.
   If you are using more than one light bulb device, follow the instructions in the **groupcast** tab to connect all devices to the multicast group

.. tabs::

   .. group-tab:: unicast

      The unicast binding is used to bind the light switch with only one light bulb device.
      Run the following commands to bind the light switch with the light bulb device:

      1. Write ACL to the ``accesscontrol`` cluster:

         .. parsed-literal::
            :class: highlight

            chip-tool accesscontrol write acl '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [112233], "targets": null}, {"fabricIndex": 1, "privilege": 3, "authMode": 2, "subjects": [<light_switch_node_ID>], "targets": [{"cluster": 6, "endpoint": 1, "deviceType": null}, {"cluster": 8, "endpoint": 1, "deviceType": null}]}]' <light_bulb_node_ID> 0

      2. Write a binding table to the light switch to inform the device about all endpoints by running this command (only for light switch):

         .. parsed-literal::
            :class: highlight

            chip-tool binding write binding '[{"fabricIndex": 1, "node": <light bulb node id>, "endpoint": 1, "cluster": 6}, {"fabricIndex": 1, "node": <light bulb node id>, "endpoint": 1, "cluster": 8}]' <light switch node id> 1

   .. group-tab:: groupcast

      The groupcast binding is used to bind the light switch with multiple light bulb devices.
      The following example shows how to estabilish a new group communication with the following parameters:

      * Group ID: 258
      * Group Name: Test_Group
      * Group Key Set ID: 349
      * Group Key Set Security Policy: 0
      * Epoch Key 0: a0a1a2a3a4a5a6a7a8a9aaabacad7531
      * Epoch Start Time 0: 1110000
      * Epoch Key 1: b0b1b2b3b4b5b6b7b8b9babbbcbd7531
      * Epoch Start Time 1: 1110001
      * Epoch Key 2: c0c1c2c3c4c5c6c7c8c9cacbcccd7531
      * Epoch Start Time 2: 1110002

      To learn more about groups in Matter, see the :ref:`ug_matter_group_communication` user guide.

      Run the following commands to bind the light switch with all light bulb devices:

      1. Set up a new group in the doc:`CHIP Tool <matter:chip_tool_guide>`:

         .. parsed-literal::
            :class: highlight

            chip-tool groupsettings add-keysets 349 0 2220000 hex:a0a1a2a3a4a5a6a7a8a9aaabacad7531
            chip-tool groupsettings bind-keyset 258 349


      #. Set up all light bulb devices:

         .. parsed-literal::
            :class: highlight

            chip-tool groupkeymanagement key-set-write {"groupKeySetID": 349, "groupKeySecurityPolicy": 0, "epochKey0": "a0a1a2a3a4a5a6a7a8a9aaabacad7531", "epochStartTime0": 1110000, "epochKey1": "b0b1b2b3b4b5b6b7b8b9babbbcbd7531", "epochStartTime1": 1110001, "epochKey2": "c0c1c2c3c4c5c6c7c8c9cacbcccd7531", "epochStartTime2": 1110002} <light bulb node id> 0
            chip-tool groupkeymanagement write group-key-map [{"groupId": 258, "groupKeySetID": 349, "fabricIndex": 1}] <light bulb node id> 0
            chip-tool groups add-group 258 Test_Group <light bulb node id> 1

            chip-tool accesscontrol write acl '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [112233], "targets": null}, {"fabricIndex": 1, "privilege": 3, "authMode": 3, "subjects": [258], "targets": [{"cluster": null, "endpoint": 1, "deviceType": null}]}]' <light bulb node id> 0

      #. Set up the light switch device:

         .. parsed-literal::
            :class: highlight

            chip-tool groupkeymanagement key-set-write {"groupKeySetID": 349, "groupKeySecurityPolicy": 0, "epochKey0": "a0a1a2a3a4a5a6a7a8a9aaabacad7531", "epochStartTime0": 1110000, "epochKey1": "b0b1b2b3b4b5b6b7b8b9babbbcbd7531", "epochStartTime1": 1110001, "epochKey2": "c0c1c2c3c4c5c6c7c8c9cacbcccd7531", "epochStartTime2": 1110002} <light_switch_node_ID> 0
            chip-tool groupkeymanagement write group-key-map [{"groupId": 258, "groupKeySetID": 349, "fabricIndex": 1}] <light_switch_node_ID> 0
            chip-tool accesscontrol write acl [{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [112233], "targets": null}, {"fabricIndex": 1, "privilege": 3, "authMode": 3, "subjects": [258], "targets": null}] <light_switch_node_ID> 0
            chip-tool binding write binding [{"fabricIndex": 1, "group": 258}] <light_switch_node_ID> 1

All devices are now bound and ready for testing communication.

.. matter_light_switch_sample_prepare_to_testing_end
.. matter_light_switch_sample_testing_start

Complete the following steps using the light switch device:

1. On the light switch device, use the :ref:`buttons <matter_light_switch_sample_ui>` to control the bound light bulbs:

   a. Press the |Second Button| to turn off the state-indication LED located on the bound light bulb device.
   #. Press the |Second Button| to turn the LED on again.
   #. Press the |Second Button| and hold it for more than 0.5 seconds to test the dimmer functionality.

      The state-indication LED on the bound light bulb device changes its brightness from 0% to 100% with 1% increments every 300 milliseconds as long as the |Second Button| is pressed.

#. Using the terminal emulator connected to the light switch, run the following :ref:`Matter CLI commands <matter_light_switch_sample_ui_matter_cli>`:

   a. Write the following command to turn on state-indication LED located on the bound light bulb device:

      * For a single bound light bulb:

        .. parsed-literal::
           :class: highlight

            matter switch onoff on

      * For a group of light bulbs:

        .. parsed-literal::
           :class: highlight

           matter switch groups onoff on

   #. Write the following command to turn off the state-indication LED located on the bound light bulb device:

      * For a single bound light bulb:

        .. parsed-literal::
           :class: highlight

           matter switch onoff off

      * For a group of light bulbs:

        .. parsed-literal::
           :class: highlight

           matter switch groups onoff off

.. matter_light_switch_sample_testing_end

Factory reset
=============

|matter_factory_reset|

Dependencies
************

.. include:: /includes/matter/dependencies.txt
