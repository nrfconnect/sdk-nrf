.. _ug_matter_group_communication:

Matter Group Communication
##########################

.. contents::
   :local:
   :depth: 2

This section describes how Group Communication works in Matter.

Group Communication in the Matter standard refers to the ability of devices within a Matter network to communicate with multiple devices simultaneously using multicast messages.
This is particularly useful for scenarios where a command needs to be sent to multiple devices at once, such as turning off all lights in a house or setting a scene.

Group setup
***********

Devices are configured to be part of one or more groups.
Each group is identified by a unique group ID.
A device can be a member of multiple groups, and a group can include multiple devices.

Group Messaging
***************

When a command needs to be sent to all devices in a group, the command initiator (a controller, a smartphone app, or another device) sends a multicast message addressed to the group ID.
Only the devices that are members of that group will receive messages to or act on a message.
Using Group Communication reduces network traffic because a single multicast message can reach multiple devices.
This is more efficient than sending individual messages to each device.
It also enhances scalability as adding more devices to a group does not increase the number of messages required for group-wide commands.

.. note::

   Group Communication is by design not reliable, because it does not use any way of acknowledgment.
   Because of that, the device that sends the message cannot determine whether it reached all devices belonging to the group.
   Communication reliability depends on the underlying networking technology.
   For example, sleepy devices should not be targeted in the group messaging, as they will likely miss incoming messages, because their radio is inactive for most of the time.
   Multicast messages are not buffered by their parents, so they will not reach the destination.

Matter ensures that Group Communication is secure.
Messages are encrypted, and only authorized devices within the group can decrypt and act on them.
Security provisions are in place to manage who can add or remove devices from groups, preventing unauthorized control.
Common use cases include lighting control (for example, turning all lights in a room or building on or off), temperature adjustments for HVAC systems in multiple zones, and scene setting where multiple device types perform coordinated actions (such as lights dim, blinds close, temperature adjusts).

In the Matter specification, managing Group Communication effectively and securely involves specific clusters such as the Group Cluster and the Group Key Management Cluster.
These clusters play crucial roles in handling group configurations and ensuring secure communication within groups.

Group Cluster
=============

The Group Cluster is primarily responsible for managing group memberships within devices.
It allows devices to be added to or removed from groups and to query their group memberships.
This cluster is essential for organizing devices into logical sets that can receive common commands, which is fundamental for scenarios like controlling multiple lights with a single command.
Group Clusters should be supported only for devices that can receive groupcast communication.

The following are the key functions of a Group Cluster:

* Add Group/Remove Group - These commands allow devices to be dynamically added to or removed from specific groups.
* View Group Membership - Devices can report which groups they belong to, which is useful for controllers to make decisions about sending commands.
* Capacity Management - The cluster handles the limitations on how many groups a device can be a member of, ensuring that device capabilities are not exceeded.

Group Key Management Cluster
============================

The Group Key Management Cluster is crucial for the security aspect of Group Communication.
It manages the encryption keys that are used to secure multicast messages sent to groups.
This ensures that only the members of a group can read the messages intended for that group, maintaining confidentiality and integrity.
A cluster should be supported for both the device that is a transmitter and the receiver of Group Communication.

The following are the key functions of a Group Key Management Cluster:

* Key Set Creation and Distribution - This function involves generating and distributing the keys that will be used to encrypt and decrypt messages sent to the group.
  The distribution must be secure to prevent interception and unauthorized access.
* Key Storage and Handling - Secure storage and handling of keys within devices are managed to prevent unauthorized access and ensure that keys are available when needed for encryption and decryption processes.

Example use cases
*****************

The following use cases demonstrate how you can manage the Group Communication settings using the ``chip-tool``.

Settings for the ``chip-tool``
==============================

Check first your settings, keysets and groups.

* To see the Group Cluster and Group Key Management Cluster settings you can manage, use the following command:

  .. code-block:: none

     ./chip-tool groupsettings

     * show-groups
     * add-group
     * remove-group
     * show-keysets
     * bind-keyset
     * unbind-keyset
     * add-keysets
     * remove-keyset


* To see the predefined keysets, use the following command:

  .. code-block:: none

     ./chip-tool groupsettings show-keysets
     ...
       +-------------------------------------------------------------------------------------+
       | Available KeySets :                                                                 |
       +-------------------------------------------------------------------------------------+
       | KeySet Id   |   Key Policy                                                          |
       | 0x1a3           Trust First                                                         |
       | 0x1a2           Cache and Sync                                                      |
       | 0x1a1           Cache and Sync                                                      |
       +-------------------------------------------------------------------------------------+

     where:

        KeySetId 0x1a3 Epochs keys:
            epochKey0 = d0d1d2d3d4d5d6d7d8d9dadbdcdddedf
            epochStartTime0 = 2220000
            epochKey1 = d1d1d2d3d4d5d6d7d8d9dadbdcdddedf
            epochStartTime1 = 2220001
            epochKey2 = d2d1d2d3d4d5d6d7d8d9dadbdcdddedf
            epochStartTime2 = 2220002
        KeySetId 0x1a2 Epochs keys:
            epochKey0 = d0d1d2d3d4d5d6d7d8d9dadbdcdddedf
            epochStartTime0 = 2220000
            epochKey1 = e0e1e2e3e4e5e6e7e8e9eaebecedeeef
            epochStartTime1 = 2220001
            epochKey2 = f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff
            epochStartTime2 = 2220002
        KeySetId 0x1a1 Epochs keys:
            epochKey0 = a0a1a2a3a4a5a6a7a8a9aaabacadaeaf
            epochStartTime0 = 1110000
            epochKey1 = b0b1b2b3b4b5b6b7b8b9babbbcbdbebf
            epochStartTime1 = 1110001
            epochKey2 = c0c1c2c3c4c5c6c7c8c9cacbcccdcecf
            epochStartTime2 = 1110002

* To see the groups, use the following command:

  .. code-block:: none

     ./chip-tool groupsettings show-groups
     ...
       +-------------------------------------------------------------------------------------+
       | Available Groups :                                                                  |
       +-------------------------------------------------------------------------------------+
       | Group Id   |  KeySet Id     |   Group Name                                          |
       | 0x101           0x1a1            Group #1                                           |
       | 0x102           0x1a2            Group #2                                           |
       | 0x103           0x1a3            Group #3                                           |
       +-------------------------------------------------------------------------------------+

Group Communication triggered from the controller
=================================================

.. tabs::

   .. group-tab:: Nodes

      You need the `chip-tool`_ and the :ref:`matter_light_bulb_sample` sample to run through this use case.

   .. group-tab:: Precondition

      Commission the Light Bulb to the same fabric as the chip-tool.

   .. group-tab:: Steps

      Complete the following steps to create a custom group based on a custom keyset and start Group Communication:

      1. Generate a custom keyset::

         ./chip-tool groupsettings add-keysets 0xabcd 0 3330000 hex:0123456789abcdef0123456789abcdef
         ./chip-tool groupsettings bind-keyset 0xdcba 0xabcd

      #. Set the keyset on the Light Bulb based on generated controller's group settings::

         ./chip-tool groupkeymanagement key-set-write '{"groupKeySetID":"0xabcd","groupKeySecurityPolicy":0,"epochKey0":"0123456789abcdef0123456789abcdef","epochStartTime0":"3330000","epochKey1":"0123456789abcdef0123456789abcdee","epochStartTime1":"3330001","epochKey2":"0123456789abcdef0123456789abcded","epochStartTime2":"3330002"}' 1 0

      #. Set the keyset to the group map::

         ./chip-tool groupkeymanagement write group-key-map '[{"groupId":"0xdcba","groupKeySetID":"0xabcd","fabricIndex":"1"}]' 1 0

      #. Add the group on the application endpoint (endpoint 1 includes the on/off cluster)::

         ./chip-tool groups add-group 0xdcba Custom_Group 1 1

      #. Set the ACL to permit the Group Communication from a specific group ID::

         ./chip-tool accesscontrol write acl '[{"fabricIndex":"1","privilege":"5","authMode":"2","subjects":["112233"],"targets":null},{"fabricIndex":"1","privilege":"3","authMode":"3","subjects":["0xdcba"],"targets":[{"cluster":null,"endpoint":"1","deviceType":null}]}]' 1 0

      #. Send a command to toggle on/off on the application endpoint (endpoint 1 includes the on/off cluster)::

         ./chip-tool onoff toggle 0xffffffffffffdcba 1

      **LED2** (nRF53 and nRF52 Series devices) or **LED1** (nRF54 Series devices) on the Light Bulb should now change the state.

Group Communication triggered from the accessory (Light Switch)
===============================================================

The initial settings for this use case are the following:

.. code-block:: none

   Light Bulb nodeid = 2
   Light Switch nodeid = 1
   Test group keyset
        groupKeySetID=258
        epochKey0=a0a1a2a3a4a5a6a7a8a9aaabacad7531
        epochStartTime0=1110000
        epochKey1=b0b1b2b3b4b5b6b7b8b9babbbcbd7531
        epochStartTime1=1110001
        epochKey2=c0c1c2c3c4c5c6c7c8c9cacbcccd7531
        epochStartTime2=1110002
   Test group:
        groupId=30001
        name=Test_Group_30001

.. tabs::

   .. group-tab:: Nodes

      You need the following to run through this use case:

      * The `chip-tool`_
      * The :ref:`matter_light_bulb_sample` sample
      * The :ref:`matter_light_switch_sample` sample

   .. group-tab:: Precondition

      Commission the Light Bulb and Light Switch to the same fabric as the chip-tool.

   .. group-tab:: Steps

      Complete the following steps to start Group Communication:

      1. Set the group settings on the on/off client (Light Switch)::

         ./chip-tool groupkeymanagement key-set-write '{"groupKeySetID":"258","groupKeySecurityPolicy":0,"epochKey0":"a0a1a2a3a4a5a6a7a8a9aaabacad7531","epochStartTime0":"1110000","epochKey1":"b0b1b2b3b4b5b6b7b8b9babbbcbd7531","epochStartTime1":"1110001","epochKey2":"c0c1c2c3c4c5c6c7c8c9cacbcccd7531","epochStartTime2":"1110002"}' 1 0

      #. Set the keyset to the group map on the on/off client (Light Switch)::

         ./chip-tool groupkeymanagement write group-key-map '[{"groupId":"30001","groupKeySetID":"258","fabricIndex":"1"}]' 1 0

      #. Set the same group settings on the on/off server (Light Bulb)::

         ./chip-tool groupkeymanagement key-set-write '{"groupKeySetID":"258","groupKeySecurityPolicy":0,"epochKey0":"a0a1a2a3a4a5a6a7a8a9aaabacad7531","epochStartTime0":"1110000","epochKey1":"b0b1b2b3b4b5b6b7b8b9babbbcbd7531","epochStartTime1":"1110001","epochKey2":"c0c1c2c3c4c5c6c7c8c9cacbcccd7531","epochStartTime2":"1110002"}' 2 0

      #. Set the keyset to the group map on the on/off server (Light Bulb)::

         ./chip-tool groupkeymanagement write group-key-map '[{"groupId":"30001","groupKeySetID":"258","fabricIndex":"1"}]' 2 0

      #. Add the group to the Light Bulb application endpoint (endpoint 1 includes the on/off cluster)::

         ./chip-tool groups add-group 30001 Test_Group_30001 2 1

      #. Set the binding to the on/off client (Light Switch)::

         ./chip-tool binding write binding '[{"fabricIndex":"1","group":"30001"}]' 1 1

      #. Set ACL on the on/off server (Light Bulb)::

         ./chip-tool accesscontrol write acl '[{"fabricIndex":1,"privilege":5,"authMode":2,"subjects":[112233],"targets":null},{"fabricIndex":1,"privilege":3,"authMode":3,"subjects":[30001],"targets":[{"cluster":null,"endpoint":1,"deviceType":null}]}]' 2 0

      #. Click button **2** (nRF53 and nRF52 Series devices) or button **1** (nRF54 Series devices) on the Light Switch to send the toggle message.

      **LED2** (nRF53 and nRF52 Series devices) or **LED1** (nRF54 Series devices) on the Light Bulb should now change the state.
