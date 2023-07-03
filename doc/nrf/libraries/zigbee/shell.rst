.. _lib_zigbee_shell:

Zigbee shell
############

.. contents::
   :local:
   :depth: 2

The Zigbee shell library implements a set of Zigbee shell commands that allow you to control the device behavior and perform operations in the Zigbee network, such as steering the network, discovering devices, and reading the network properties.
Enabling a device to work with these commands simplifies testing and debugging of an existing network and reduces the amount of reprogramming operations.

.. _zigbee_shell_extending_samples:

Configuration
*************

The Zigbee shell commands are implemented using Zephyr's :ref:`zephyr:shell_api` interface.
By default, Zephyr shell uses the UART backend.
To change this and other Zephyr's shell settings (for example, the prompt or the maximum amount of accepted command arguments), read the documentation page in Zephyr.

|zigbee_shell_config|

To configure the Zigbee shell library, use the following options:

* :kconfig:option:`CONFIG_ZIGBEE_SHELL`
* :kconfig:option:`CONFIG_ZIGBEE_SHELL_ENDPOINT`
* :kconfig:option:`CONFIG_ZIGBEE_SHELL_DEBUG_CMD`
* :kconfig:option:`CONFIG_ZIGBEE_SHELL_LOG_LEVEL`
* :kconfig:option:`CONFIG_ZIGBEE_SHELL_CTX_MGR_ENTRIES_NBR`
* :kconfig:option:`CONFIG_ZIGBEE_SHELL_MONITOR_CACHE_SIZE`
* :kconfig:option:`CONFIG_ZIGBEE_SHELL_ZCL_CMD_TIMEOUT`

For detailed steps about configuring the library in a Zigbee sample or application, see :ref:`ug_zigbee_configuring_components_logger_ep`.

Supported backends
******************

Zigbee shell commands are available for the following backends when testing samples:

* UART
* Segger RTT

You can run the Zigbee shell commands after connecting and configuring the backend for testing.
For more information, see :ref:`testing`.

.. _zigbee_shell_reference:

Zigbee shell command list
*************************

This section lists commands that are supported by :ref:`Zigbee samples <zigbee_samples>`.

Description convention
======================

Every command prints ``Done`` when it is finished, or an error with the reason why it occurs.

The command argument description uses the following convention:

* Square brackets mean that an argument is optional:

  .. parsed-literal::
     :class: highlight

     command [*arg*]

* A single letter before an argument name defines the format of the argument:

  .. parsed-literal::
     :class: highlight

     command *d:arg1* *h:arg2*

  * *h* stands for hexadecimal strings.
  * *d* stands for decimal values.

* The ellipsis after an argument means that the preceding argument can be repeated several times:

  .. parsed-literal::
     :class: highlight

     command *arg* ...

----

.. _shell_help:

help
====

Display help for all available shell commands.

.. parsed-literal::
   :class: highlight

   [*group*] help

If the optional argument is not provided, displays help for all command groups.

If the optional argument is provided, displays help for subcommands of the specified command group.
For example, ``zdo help`` displays help for all ``zdo`` commands.

Example:

.. zigbee_help_output_start

.. code-block::

   help
   Please press the <Tab> button to see all available commands.
   You can also use the <Tab> button to prompt or auto-complete all commands or its subcommands.
   You can try to call commands with <-h> or <--help> parameter for more information.

   Shell supports following meta-keys:
   Ctrl + (a key from: abcdefklnpuw)
   Alt  + (a key from: bf)
   Please refer to shell documentation for more details.

   Available commands:
   bdb                :Base device behaviour manipulation.
   clear              :Clear screen.
   debug              :Return state of debug mode.
   device             :Device commands
   devmem             :Read/write physical memory"devmem address [width [value]]"
   flash              :Flash shell commands
   help               :Prints the help message.
   history            :Command history.
   kernel             :Kernel commands
   nbr                :Zigbee neighbor table.
   nrf_clock_control  :Clock control commmands
   nvram              :Zigbee NVRAM manipulation.
   resize             :Console gets terminal screen size or assumes default in
                       case the readout fails. It must be executed after each
                       terminal width change to ensure correct text display.
   sensor             :Sensor commands
   shell              :Useful, not Unix-like shell commands.
   version            :Print firmware version
   zcl                :ZCL subsystem commands.
   zdo                :ZDO manipulation.
   zscheduler         :Zigbee scheduler manipulation.

.. zigbee_help_output_end

----

.. _bdb_role:

bdb role
========

Set or get the Zigbee role of a device.

.. parsed-literal::
   :class: highlight

   bdb role [*role*]

.. note::
    |precondition|

If the optional argument is not provided, get the state of the device.
Returns the following values:

* ``zc`` if it is a coordinator.
* ``zr`` it it is a router.
* ``zed`` if it is an end device.

If the optional argument is provided, set the device role to *role*.
Can be either ``zc`` or ``zr``.

.. note::
    Zigbee End Device is not currently supported by the Shell sample.


----

.. _bdb_extpanid:

bdb extpanid
============

Set or get the Zigbee Extended Pan ID value.


.. parsed-literal::
   :class: highlight

   bdb extpanid [*h:id*]

.. note::
    |precondition|

If the optional argument is not provided, gets the extended PAN ID of the joined network.

If the optional argument is provided, gets the extended PAN ID to *id*.

----

.. _bdb_panid:

bdb panid
=========

Set or get the Zigbee PAN ID value.

.. parsed-literal::
   :class: highlight

   bdb panid [*h:id*]

.. note::
    |precondition|

If the optional argument is not provided, gets the PAN ID of the joined network.
If the optional argument is provided, sets the PAN ID to *id*.

----

.. _bdb_start:

bdb start
=========

Start the commissioning process.

.. code-block::

   > bdb start
   Started coordinator
   Done

----

.. _bdb_channel:

bdb channel
===========

Set or get the 802.15.4 channel.

.. parsed-literal::
   :class: highlight

   bdb channel *n*

.. note::
    |precondition2|

If the optional argument is not provided, get the current number and bitmask of the channel.

If the optional argument is provided:

* If *n* is in ``[11:26]`` range, set to that channel.
* Otherwise, treat *n* as bitmask (logical or of a single bit shifted by channel number).


Example:

.. code-block::

   > bdb channel 0x110000
   Setting channel bitmask to 110000
   Done

----

.. _bdb_ic_add:

bdb ic add
==========

Add information about the install code on the trust center.

.. parsed-literal::
   :class: highlight

   bdb ic add *h:install code* *h:eui64*

For *h:eui64*, use the address of the joining device.
For *h:install code*, use 128bit install code with correct CRC value.

.. note::
    |precondition3|

    |precondition5|

    |precondition6|

    |precondition7|

Example:

.. code-block::

   > bdb ic add 83FED3407A939723A5C639B26916D505C3B5 0B010E2F79E9DBFA
   Done

----

.. _bdb_ic_list:

bdb ic list
===========

Read and print install codes stored on the device.

.. parsed-literal::
   :class: highlight

   bdb ic list

.. note::
    |precondition4|

    |precondition6|

    |precondition7|

Example:

.. code-block::

   > bdb ic list
   [idx] EUI64:           IC:                                  options:
   [  0] 0b010e2f79e9dbfa 83fed3407a939723a5c639b26916d505c3b5 0x3
   Total entries for the install codes table: 1
   Done

----

.. _bdb_ic_policy:

bdb ic policy
=============

Set the trust center install code policy.

.. parsed-literal::
   :class: highlight

   bdb ic policy *enable|disable*

.. note::
    |precondition6|

    |precondition7|

Example:

.. code-block::

    > bdb ic policy enable
    Done


----

.. _bdb_ic_set:

bdb ic set
==========

Set install code on the device.

.. parsed-literal::
   :class: highlight

   bdb ic set *h:install code*

Must only be used on a joining device.

.. note::
    |precondition2|

    |precondition5|

    |precondition6|

Example:

.. code-block::

   > bdb ic set 83fed3407a939723a5c639b26916d505c3b5
   Done

----

.. _bdb_legacy:

bdb legacy
==========

Enable or disable the legacy device support.

.. parsed-literal::
   :class: highlight

   bdb legacy *enable|disable*

Allow or disallow legacy pre-r21 devices on the Zigbee network.

Example:

.. code-block::

   > bdb legacy enable
   Done

----

.. _bdb_nwkkey:

bdb nwkkey
==========

Set network key.

.. parsed-literal::
   :class: highlight

   bdb nwkkey *h:key*

Set a pre-defined network key *key* instead of a random one.

.. note::
    |precondition2|

Example:

.. code-block::

   > bdb nwkkey 00112233445566778899aabbccddeeff
   Done

----

.. _bdb_factory_reset:

bdb factory_reset
=================

Perform a factory reset using local action.
See Base Device Behavior specification chapter 9.5 for details.

.. code-block::

   > bdb factory_reset
   Done

----

.. _bdb_child_max:

bdb child_max
=============

Set the amount of child devices that is equal to *d:nbr*.

.. parsed-literal::
   :class: highlight

   > bdb child_max *d:nbr*

.. note::
    |precondition2|

Example:

.. code-block::

   > bdb child_max 16
   Setting max children to: 16
   Done

----

.. _zcl_cmd:

zcl cmd
=======

Send a generic ZCL command to the remote node.

.. parsed-literal::
   :class: highlight

   zcl cmd [-d] *h:dst_addr* *d:ep* *h:cluster* [-p *h:profile*] *h:cmd_ID* [-l *h:payload*]

.. note::
    By default, the profile is set to Home Automation Profile, and the payload is empty.

    The payload requires the **little-endian** byte order.

    To send a request using binding table entries, set ``dst_addr`` and ``ep`` to ``0``.
    To send as groupcast, set ``dst_addr`` to a group address and ``ep`` to ``0``.

Send a generic ZCL command with ID ``cmd_ID`` and payload ``payload`` to the cluster ``cluster``.
The cluster belongs to the profile ``profile``, which resides on the endpoint ``ep`` of the remote node ``dst_addr``.
Optional default response can be requested with ``-d``.

Examples:

.. code-block::

   zcl cmd 0x1234 10 0x0006 0x00

This command sends the Off command from the On/Off cluster (ZCL specification 3.8.2.3.1) to the device with the short address ``0x1234`` and endpoint ``10``.

.. code-block::

   zcl cmd 0x1234 10 0x0008 0x00 -l FF0A00

This command sends the Move to Level command from the Level Control cluster (ZCL specification 3.10.2.3.1) to the device with the short address ``0x1234`` and endpoint ``10``, asking it to move the CurrentLevel attribute to a new value ``255`` in 1 second.

.. code-block::

   zcl cmd -d 0x1234 10 0x0008 -p 0x0104 0x00 -l FF0A00

This command sends the same Move to Level command and requests additional Default response.
The same Home Automation Profile is used, but is set directly instead.

----

.. _zcl_read_attr:

zcl attr read
=============

Retrieve the attribute value of the remote node.

.. parsed-literal::
   :class: highlight

   zcl attr read *h:dst_addr* *d:ep* *h:cluster* [-c] *h:profile* *h:attr_id*

Read the value of the attribute ``attr_id`` in the cluster ``cluster``.
The cluster belongs to the profile ``profile``, which resides on the endpoint ``ep`` of the remote node ``dst_addr``.
If the attribute is on the client role side of the cluster, use the ``-c`` switch.

Example:

.. code-block::

   > zcl attr read 0x1234 10 0x0000 0x0104 0x00
   ID: 0 Type: 20 Value: 3
   Done

This command sends the Read Attributes command (ZCL specification 2.5.1) to the device with the short address ``0x1234`` and endpoint ``10``, asking it to reply with the ZCLVersion attribute value of the Basic cluster.

----

.. _zcl_attr_write:

zcl attr write
==============

Write the attribute value to the remote node.

.. parsed-literal::
   :class: highlight

   zcl attr write *h:dst_addr* *d:ep* *h:cluster* [-c] *h:profile* *h:attr_id* *h:attr_type* *h:attr_value*

Write the ``attr_value`` value of the attribute ``attr_id`` of the type ``attr_type`` in the cluster ``cluster``.
The cluster belongs to the profile ``profile``, which resides on the endpoint ``ep`` of the remote node ``dst_addr``.
If the attribute is on the client role side of the cluster, use the``-c`` switch.

.. note::
    The ``attr_value`` value must be in the hexadecimal format, unless it is a string (``attr_type == 42``), then it must be a string.

Example:

.. code-block::

   > zcl attr write 0x1234 10 0x0003 0x0104 0x00 0x21 0x0F
   Done

This command sends the Write Attributes command (ZCL specification 2.5.3) to the device with the short address ``0x1234`` and endpoint ``10``, asking it to set the IdentifyTime attribute value to ``15`` in the identify cluster.

----

.. _zcl_subscribe_on:

zcl subscribe on
================

Subscribe to the attribute changes on the remote node.

.. parsed-literal::
   :class: highlight

   zcl subscribe on *h:addr* *d:ep* *h:cluster* *h:profile* *h:attr_id* *d:attr_type* [*d:min interval (s)*] [*d:max interval (s)*]

Enable reporting on the node identified by the address ``addr``, with the endpoint ``ep``
that uses the profile ``profile`` of the attribute ``attr_id`` with the type
``attr_type`` in the cluster ``cluster``.

The reports must be generated in intervals not shorter than ``min interval``
(1 second by default) and not longer than ``max interval`` (60 seconds by default).

Example:

.. code-block::

   > zcl subscribe on 0x1234 10 0x0006 0x0104 0x00 16 5 20
   Done


This command sends the Configure Reporting command (ZCL specification 2.5.7) to the device with the short address ``0x1234`` and endpoint ``10``, asking it to configure reporting for the OnOff attribute of the On/Off cluster with minimum reporting interval of ``5`` seconds and maximum reporting interval of ``20`` seconds.

----

.. _zcl_subscribe_off:

zcl subscribe off
=================

Unsubscribe from the attribute reports.

.. parsed-literal::
   :class: highlight

   zcl subscribe off *h:addr* *d:ep* *h:cluster* *h:profile* *h:attr_id* *d:attr_type*

Disable reporting on the node identified by the address ``addr``, with the endpoint ``ep``
that uses the profile ``profile`` of the attribute ``attr_id`` with the type
``attr_type`` in the cluster ``cluster``.

Example:

.. code-block::

   > zcl subscribe off 0x1234 10 0x0006 0x0104 0x00 16
   Done

This command sends the Configure Reporting command (ZCL specification 2.5.7) to the device with the short address ``0x1234`` and endpoint ``10``, asking it to stop issuing reports for the OnOff attribute of the On/Off cluster, by setting maximum reporting interval to ``0xffff``.

----

.. _zcl_ping:

zcl ping
========

Ping other devices using ZCL.

.. parsed-literal::
   :class: highlight

   zcl ping [--no-echo] [--aps-ack] *h:dst_addr* *d:payload_size*

Example:

.. code-block::

   zcl ping 0b010eaafd745dfa 32

.. note::
    |precondition4|

Issue a ping-style shell command to another Shell node with the given 16-bit destination address (*dst_addr*) by using a payload equal to *payload_size* bytes.
The command is sent and received on endpoints with the same ID.

This shell command uses a custom ZCL frame, which is constructed as a ZCL frame of a custom ping ZCL cluster with the cluster ID ``0xBEEF``.
For details, see the implementation of :c:func:`ping_request_send` in :file:`subsys/zigbee/lib/zigbee_shell/src/zigbee_shell_cmd_ping.c`.

The command measures the time needed for a Zigbee frame to travel between two nodes in the network (there and back again).
The shell command sends a ping request ZCL command, which is followed by a ping reply ZCL command.
The IDs of the ping request change depending on optional arguments.
The ping reply ID stays the same (``0x01``).

The following optional argument are available:

* ``--aps-ack`` requests an APS acknowledgment
* ``--no-echo`` asks the destination node not to send the ping reply

Both arguments can be used at the same time.
See the following graphs for use cases.

Case 1: Ping with echo, but without the APS acknowledgment
    This is the default case, without optional arguments.

        .. msc::
            hscale = "1.3";
            App1 [label="Application 1"],Node1 [label="Node 1"],Node2 [label="Node 2"];
            App1 rbox Node2     [label="Command ID: 0x02 - Ping request without the APS acknowledgment"];
            App1>>Node1         [label="ping"];
            Node1>>Node2        [label="ping request"];
            Node1<<Node2        [label="MAC ACK"];
            App1 rbox Node2     [label="Command ID: 0x01 - Ping reply"];
            Node1<<Node2        [label="ping reply"];
            Node1>>Node2        [label="MAC ACK"];
            App1<<Node1         [label="Done"];
        ..

    In this default case, the ``zcl ping`` command measures the time between sending the ping request and receiving the ping reply.

Case 2: Ping with echo and with the APS acknowledgment
    This is a case with the ``--aps-ack`` optional argument.

        .. msc::
            hscale = "1.3";
            App1 [label="Application 1"],Node1 [label="Node 1"],Node2 [label="Node 2"];
            App1 rbox Node2     [label="Command ID: 0x00 - Ping request with the APS acknowledgment"];
            App1>>Node1         [label="ping"];
            Node1>>Node2        [label="ping request"];
            Node1<<Node2        [label="MAC ACK"];
            Node1<<Node2        [label="APS ACK"];
            Node1>>Node2        [label="MAC ACK"];
            App1 rbox Node2     [label="Command ID: 0x01 - Ping reply"];
            Node1<<Node2        [label="ping reply"];
            Node1>>Node2        [label="MAC ACK"];
            Node1>>Node2        [label="APS ACK"];
            Node1<<Node2        [label="MAC ACK"];
            App1<<Node1         [label="Done"];
        ..

     In this case, the ``zcl ping`` command measures the time between sending the ping request and receiving the ping reply.

Case 3: Ping without echo, but with the APS acknowledgment
    This is a case with both optional arguments provided, ``--aps-ack`` and ``--no-echo``.

        .. msc::
            hscale = "1.3";
            App1 [label="Application 1"],Node1 [label="Node 1"],Node2 [label="Node 2"];
            App1 rbox Node2     [label="Command ID: 0x03 - Ping request without echo"];
            App1>>Node1         [label="ping"];
            Node1>>Node2        [label="ping request"];
            Node1<<Node2        [label="MAC ACK"];
            Node1<<Node2        [label="APS ACK"];
            Node1>>Node2        [label="MAC ACK"];
            App1<<Node1         [label="Done"];
        ..

    In this case, the ``zcl ping`` command measures the time between sending the ping request and receiving the APS acknowledgment.

Case 4: Ping without echo and without the APS acknowledgment
    This is a case with the ``--no-echo`` optional argument.

        .. msc::
            hscale = "1.3";
            App1 [label="Application 1"],Node1 [label="Node 1"],Node2 [label="Node 2"];
            App1 rbox Node2     [label="Command ID: 0x03 - Ping request without echo"];
            App1>>Node1         [label="ping"];
            Node1>>Node2        [label="ping request"];
            App1<<Node1         [label="Done"];
            Node1<<Node2        [label="MAC ACK"];
        ..

    In this case, the ``zcl ping`` command does not measure time after sending the ping request.

----

.. _zcl_groups_add:

zcl groups add
==============

Add the endpoint on the remote node to the group.

.. parsed-literal::
   :class: highlight

   zcl groups add *h:dst_addr* *d:ep* [-p h:profile] *h:group_id*

Send Add Group command to the endpoint ``ep`` of the remote node ``dst_addr`` and add it to the group ``group_id``.
Providing profile ID ``profile`` is optional  (Home Automation Profile is used by default).
This command can be sent as groupcast.
To send the command as groupcast, set ``dst_addr`` to a group address and ``ep`` to ``0``.

Example 1:

.. code-block::

   > zcl groups add 0x1234 10 0x4321
   Response command received, group: 0x4321, status: 0
   Done

This command sends the Add Group command (ZCL specification 3.6.2.3.2) to the device with the short address ``0x1234`` and endpoint ``10``, asking it to join the group ``0x4321``.

Example 2:

.. code-block::

   > zcl groups add 0x1234 10 -p 0x0104 0x3210
   Response command received, group: 0x3210, status: 0
   Done

This command sends the Add Group command (ZCL specification 3.6.2.3.2) to the device with the short address ``0x1234`` and endpoint ``10`` with profile set to ``0x0104``, asking it to join the group ``0x3210``.

Example 3:

.. code-block::

   > zcl groups add 0x3210 0 0x1000
   Done

This command sends the Add Group command (ZCL specification 3.6.2.3.2) as groupcast to devices that belong to the group ``0x3210``, asking them to join the group ``0x1000``.

----

.. _zcl_groups_add_if_identify:

zcl groups add_if_identify
==========================

Add the endpoints on the remote nodes to the group if the endpoints are in the Identify mode.

.. parsed-literal::
   :class: highlight

   zcl groups add_if_identify *h:dst_addr* *d:ep* [-p h:profile] *h:group_id*

Send Add Group If Identifying command to the endpoint ``ep`` of the remote node ``dst_addr`` and add it to the group ``group_id`` if the endpoint on the remote node is in the Identify mode.
Providing profile ID ``profile`` is optional  (Home Automation Profile is used by default).
This command can be sent as groupcast.
To send the command as groupcast, set ``dst_addr`` to a group address and ``ep`` to ``0``.

Example 1:

.. code-block::

   > zcl groups add_if_identify 0x1234 10 0x4321
   Done

This command sends the Add Group If Identifying command (ZCL specification 3.6.2.3.7) to the device with the short address ``0x1234`` and endpoint ``10``, asking it to join the group ``0x4321`` if is in the Identify mode.

Example 2:

.. code-block::

   > zcl groups add_if_identify 0x1234 10 -p 0x0104 0x3210
   Done

This command sends the Add Group If Identifying command (ZCL specification 3.6.2.3.7) to the device with the short address ``0x1234`` and endpoint ``10`` with profile set to ``0x0104``, asking it to join the group ``0x3210`` if it is in the Identify mode.

Example 3:

.. code-block::

   > zcl groups add_if_identify 0x3210 0 0x1000
   Done

This command sends the Add Group If Identifying command (ZCL specification 3.6.2.3.7) as groupcast to devices that belong to the group ``0x3210``, asking them to join the group ``0x1000`` if they are in the Identify mode.

----

.. _zcl_groups_get_member:

zcl groups get_member
=====================

Get group membership from the endpoint on the remote node.

.. parsed-literal::
   :class: highlight

   zcl groups get_member *h:dst_addr* *d:ep* [-p h:profile] [h:group IDs ...]

Send Get Group Membership command to the endpoint ``ep`` of the remote node ``dst_addr`` to get which groups from the provided group list are including the endpoint on the remote node.
If no group ID is provided, the remote node is requested to response with information about all groups it is part of.
Providing profile ID ``profile`` is optional  (Home Automation Profile is used by default).
This command can be sent as groupcast.
To send the command as groupcast, set ``dst_addr`` to a group address and ``ep`` to ``0``.
Response from the remote node is parsed and printed only if the command was sent as unicast.

Example 1:

.. code-block::

   > zcl groups get_member 0x1234 10
   short_addr=0x1234 ep=10 capacity=13 group_cnt=3 group_list=0x4321,0x3210,0x1000,
   Done

This command sends the Get Group Membership command (ZCL specification 3.6.2.3.4) to the device with the short address ``0x1234`` and endpoint ``10``, asking it to respond with the list of groups that include the device.

Example 2:

.. code-block::

   > zcl groups get_member 0x1234 10 -p 0x0104 0x4321 0x3210
   short_addr=0x1234 ep=10 capacity=13 group_cnt=3 group_list=0x4321,0x3210,
   Done

This command sends the Get Group Membership command (ZCL specification 3.6.2.3.4) to the device with the short address ``0x1234`` and endpoint ``10`` with profile set to ``0x0104``, asking it to respond with the list of groups that include the device and that are present in the group list provided in the command.

----

.. _zcl_groups_remove:

zcl groups remove
=================

Remove the endpoint on the remote node from the group.

.. parsed-literal::
   :class: highlight

   zcl groups remove *h:dst_addr* *d:ep* [-p h:profile] *h:group_id*

Send Remove Group command to the endpoint ``ep`` of the remote node ``dst_addr`` and remove it from the group ``group_id``.
Providing profile ID ``profile`` is optional  (Home Automation Profile is used by default).
This command can be sent as groupcast.
To send the command as groupcast, set ``dst_addr`` to a group address and ``ep`` to ``0``.

Example 1:

.. code-block::

   > zcl groups remove 0x1234 10 0x4321
   Response command received, group: 0x4321, status: 0
   Done

This command sends the Remove Group command (ZCL specification 3.6.2.3.5) to the device with the short address ``0x1234`` and endpoint ``10``, asking it to remove itself from the group ``0x4321``.

Example 2:

.. code-block::

   > zcl groups remove 0x1234 10 -p 0x0104 0x3210
   Response command received, group: 0x3210, status: 0
   Done

This command sends the Remove Group command (ZCL specification 3.6.2.3.5) to the device with the short address ``0x1234`` and endpoint ``10`` with profile set to ``0x0104``, asking it to remove itself from the group ``0x3210``.

Example 3:

.. code-block::

   > zcl groups remove 0x3210 0 0x1000
   Done

This command sends the Remove Group command (ZCL specification 3.6.2.3.5) as groupcast to devices that belong to the group ``0x3210``, asking them to remove themselves from the group ``0x1000``.

----

.. _zcl_groups_remove_all:

zcl groups remove_all
=====================

Remove the endpoint on the remote node from all groups.

.. parsed-literal::
   :class: highlight

   zcl groups remove_all *h:dst_addr* *d:ep* [-p h:profile]


Send Remove All Groups command to the endpoint ``ep`` to the remote node ``dst_addr``.
Providing profile ID ``profile`` is optional  (Home Automation Profile is used by default).
This command can be sent as groupcast.
To send the command as groupcast, set ``dst_addr`` to a group address and ``ep`` to ``0``.

Example 1:

.. code-block::

   > zcl groups remove_all 0x1234 10
   Done

This command sends the Remove All Groups command (ZCL specification 3.6.2.3.6) to the device with the short address ``0x1234`` and endpoint ``10``, asking it to remove itself from all groups.

Example 2:

.. code-block::

   > zcl groups remove_all 0x1234 10 -p 0x0104
   Done

This command sends the Remove All Groups command (ZCL specification 3.6.2.3.6) to the device with the short address ``0x1234`` and endpoint ``10`` with profile set to ``0x0104``, asking it to remove itself from all groups.

Example 3:

.. code-block::

   > zcl groups remove_all 0x1000 0
   Done

This command sends the Remove All Groups command (ZCL specification 3.6.2.3.6) as groupcast to devices that belong to the group ``0x3210``, asking them to remove themselves from all groups.

----

.. _zdo_simple_desc_req:

zdo simple_desc_req
===================

Send Simple Descriptor Request.

.. parsed-literal::
   :class: highlight

   zdo simple_desc_req *h:dst_addr* *d:ep*

Send Simple Descriptor Request to the given 16-bit destination address of the node (*dst_addr*) and the endpoint *ep*.

Example:

.. code-block::

   > zdo simple_desc_req 0xefba 10
   src_addr=0xEFBA ep=0x260 profile_id=0x0102 app_dev_id=0x0 app_dev_ver=0x5
   in_clusters=0x0000,0x0003,0x0004,0x0005,0x0006,0x0008,0x0300 out_clusters=0x0300
   Done

----

.. _zdo_active_ep:

zdo active_ep
=============

Send Active Endpoint Request.

.. parsed-literal::
   :class: highlight

   zdo active_ep *h:dst_addr*

Send Active Endpoint Request to the 16-bit destination address of the node (*dst_addr*).

Example:

.. code-block::

   > zdo active_ep 0xb4fc
   > src_addr=B4FC ep=10,11,12
   Done

----

.. _zdo_match_desc:

zdo match_desc
==============

Send match descriptor request.

.. parsed-literal::
   :class: highlight

   zdo match_desc *h:dst_addr*
                  *h:req_addr* *h:prof_id*
                  *d:n_input_clusters* [*h:input cluster IDs* ...]
                  *d:n_output_clusters* [*h:output cluster IDs* ...]

Send Match Descriptor Request to the 16-bit destination address of the node (*dst_addr*) that is a query about the requested address/type node (*req_addr*) of the *prof_id* profile ID, which must have at least one of input clusters (*n_input_clusters*), whose IDs are listed in ``[...]``, or at least one of output clusters (*n_output_clusters*), whose IDs are listed in ``[...]``.
The IDs can be either decimal values or hexadecimal strings.

Example:

.. code-block::

   match_desc 0xfffd 0xfffd 0x0104 1 6 0

In this example, the command sends a Match Descriptor Request to all non-sleeping nodes regarding all non-sleeping nodes that have 1 input cluster ON/OFF (``ID 6``) and 0 output clusters.


----

.. _zdo_bind:

zdo bind on
===========

Create a binding between two endpoints on two nodes.

.. parsed-literal::
   :class: highlight

   zdo bind on *h:source_eui64* *d:source_ep* *h:dst_addr*
               *d:dst_ep* *h:source_cluster_id* *h:request_dst_addr*

.. note::
    To bind the device to a group address, set ``dst_addr`` to a group address and ``dst_ep`` to ``0``.

Create bound connection between a device identified by *source_eui64* and endpoint *source_ep*, and a device identified by destination address *dst_addr* and destination endpoint *dst_ep*.
The connection is created for ZCL commands and attributes assigned to the ZCL cluster *source_cluster_id* on the *request_dst_addr* node (usually short address corresponding to *source_eui64* argument).

Example:

.. code-block::

   zdo bind on 0B010E0405060708 1 0B010E4050607080 2 8

----

.. _zdo_unbind:

zdo bind off
============

Remove a binding between two endpoints on two nodes.

.. parsed-literal::
   :class: highlight

   zdo bind off *h:source_eui64* *d:source_ep* *h:dst_eui64*
                *d:destination_ep* *h:source_cluster_id* *h:request_dst_addr*

.. note::
    To unbind the device from a group address, set ``dst_addr`` to a group address and ``dst_ep`` to ``0``.

Remove bound connection between a device identified by *source_eui64* and endpoint *source_ep*, and a device identified by destination address *dst_eui64* and destination endpoint *dst_ep*.
The connection is removed for ZCL commands and attributes assigned to the ZCL cluster *source_cluster_id* on the *request_dst_addr* node (usually, the same address as for the *source_eui64* device).

----

.. _zdo_mgmt_bind:

zdo mgmt_bind
=============

Read the binding table from a node.

.. parsed-literal::
   :class: highlight

   zdo mgmt_bind *h:dst_addr* [*d:start_index*]

Send a request to the remote device identified by the 16-bit destination address (*dst_addr*) to read the binding table through ``zdo mgmt_bind_req`` (see spec. 2.4.3.3.4).
If the whole binding table does not fit into a single ``mgmt_bind_resp frame``, the request initiates a series of ``mgmt_bind_req`` requests to perform the full download of the binding table.
*start_index* is the index of the first entry in the binding table where the reading starts.
It is zero by default.

Example:

.. code-block::

   zdo mgmt_bind 0x1234

This command sends ``mgmt_bind_req`` to the device with short address ``0x1234``, asking it to return its binding table.

Sample output:

.. code-block::

   [idx] src_address      src_endp cluster_id dst_addr_mode dst_addr         dst_endp
   [  0] 0b010ef8872c633e       10     0x0402             3 0b010e21591eef3e       64
   [  1] 0b010ef8872c633e       10     0x0403             3 0b010e21591eef3e       64
   Total entries for the binding table: 2
   Done

----

.. _zdo_mgmt_lqi:

zdo mgmt_lqi
============

Send a ZDO Mgmt_Lqi_Req command to a remote device with the short address *short*.

.. parsed-literal::
   :class: highlight

   zdo mgmt_lqi *h:short* [*d:start_index*]

*start_index* is the index of the first entry in the binding table where the reading starts.
It is zero by default.

Example:

.. code-block::

   zdo mgmt_lqi 0x1234

This command sends ``mgmt_lqi_req`` to the device with short address ``0x1234``, asking it to return its neighbor table.

----

.. _zdo_nwk_addr:

zdo nwk_addr
============

Resolve the EUI64 address *eui64* to a short network address.

.. parsed-literal::
   :class: highlight

   zdo nwk_addr *h:eui64*

Example:

.. code-block::

   zdo nwk_addr 0B010E0405060708

----

.. _zdo_ieee_addr:

zdo ieee_addr
=============

Resolve the EUI64 address *short_addr* by sending the IEEE address request.

.. parsed-literal::
   :class: highlight

   zdo ieee_addr *h:short_addr*

----

.. _zdo_eui64:

zdo eui64
=========

Get the EUI64 address of the Zigbee device.

.. code-block::

   > zdo eui64
   0b010eaafd745dfa
   Done

----

.. _zdo_short:

zdo short
=========

Get the short 16-bit address of the Zigbee device.

.. code-block::

   > zdo short
   0000
   Done

----

.. _zdo_mgmt_leave:

zdo mgmt_leave
==============

Send a request to a remote device to leave the network through ``zdo mgmt_leave_req`` (see the specification section 2.4.3.3.5).

.. parsed-literal::
   :class: highlight

   zdo mgmt_leave *h:dst_addr* [*h:device_address*] [--children] [--rejoin]

Send ``mgmt_leave_req`` to a remote node specified by 16-bit destination address *dst_addr*.
If the EUI64 *device_address* is omitted or it has a value equal to ``0000000000000000``, the remote device at address *dst_addr* will remove itself from the network.
If *device_address* has other value, it must be a long address corresponding to *dst_addr* or a long address of child node of *dst_addr*.
The request is sent with `Remove Children` and `Rejoin` flags set to ``0`` by default.
Use options ``\--children`` or ``\--rejoin`` to change the respective flags to ``1``.
For more details, see the section 2.4.3.3.5 of the specification.

Examples:

.. code-block::

   zdo mgmt_leave 0x1234

This command sends ``mgmt_leave_req`` to the device with the short address ``0x1234``, asking it to remove itself from the network.

.. code-block::

   zdo mgmt_leave 0x1234 --rejoin

This command sends ``mgmt_leave_req`` to the device with the short address ``0x1234``, asking it to remove itself from the network and perform rejoin.

.. code-block::

   zdo mgmt_leave 0x1234 0b010ef8872c633e

This command sends ``mgmt_leave_req`` to the device with the short address ``0x1234``, asking it to remove device ``0b010ef8872c633e`` from the network.
If the target device with the short address ``0x1234`` also has a long address ``0b010ef8872c633e``, it will remove itself from the network.
If the target device with the short address ``0x1234`` has a child with long address ``0b010ef8872c633e``, it will remove the child from the network.

.. code-block::

   zdo mgmt_leave 0x1234 --children

This command sends ``mgmt_leave_req`` to the device with the short address ``0x1234``, asking it to remove itself and all its children from the network.

----

.. _version:

version
=======

Print the firmware version.

.. code-block::

   version

Example:

.. code-block::

   > version
   Shell: Sep  3 2020 13:34:28
   ZBOSS: 3.3.0.2
   Zephyr kernel version: 2.3.99
   Done

----

.. _debug:

debug
=====

Enable or disable the debug mode in the Shell.

.. parsed-literal::
   :class: highlight

   debug *on|off*

This command unblocks several additional commands in the Shell.

.. note::
    When used, the additional commands can render the device unstable.

----

.. _zscheduler_suspend:

zscheduler suspend
==================

Suspend Zigbee scheduler processing.

.. code-block::

   zscheduler suspend

.. note::
    |precondition4|

----

.. _zscheduler_resume:

zscheduler resume
=================

Resume Zigbee scheduler processing.

.. code-block::

   zscheduler resume

.. note::
    |precondition4|

----

.. _nbr_monitor_on:

nbr monitor on
==============

Start monitoring the list of active Zigbee neighbors.

.. code-block::

   nbr monitor on

----

.. _nbr_monitor_off:

nbr monitor off
===============

Stop monitoring the list of active Zigbee neighbors.

.. code-block::

   nbr monitor off

----

.. _nbr_monitor_trigger:

nbr monitor trigger
===================

Trigger logging the list of active Zigbee neighbors.

.. note::
    This function will trigger logging only if the Zigbee active neighbor monitor is started (see :ref:`nbr_monitor_on`).

.. code-block::

   nbr monitor trigger

----

.. _nvram_enable:

nvram enable
============

Enable Zigbee NVRAM.

.. note::
    |precondition2|

.. code-block::

   nvram enable

----

.. _nvram_disable:

nvram disable
=============

Disable Zigbee NVRAM.

.. note::
    |precondition2|

.. code-block::

   nvram disable

.. |precondition| replace:: Setting only before :ref:`bdb_start`.
   Reading only after :ref:`bdb_start`.

.. |precondition2| replace:: Setting only before :ref:`bdb_start`.

.. |precondition3| replace:: Adding install codes only after :ref:`bdb_start`.

.. |precondition4| replace:: Use only after :ref:`bdb_start`.

.. |precondition5| replace:: Provide the install code as an ASCII-encoded hexadecimal file that includes CRC16/X-25 in little-endian order.

.. |precondition6| replace:: For production devices, make sure that an install code is installed by the production configuration present in the flash.

.. |precondition7| replace:: Must only be used on a coordinator.
