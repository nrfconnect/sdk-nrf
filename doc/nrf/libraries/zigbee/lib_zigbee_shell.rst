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

* :option:`CONFIG_ZIGBEE_SHELL`
* :option:`CONFIG_ZIGBEE_SHELL_ENDPOINT`
* :option:`CONFIG_ZIGBEE_SHELL_DEBUG_CMD`
* :option:`CONFIG_ZIGBEE_SHELL_LOG_LEVEL`

For detailed steps about configuring the library in a Zigbee sample or application, see :ref:`ug_zigbee_configuring_components_logger_ep`.

Supported backends
******************

Zigbee shell commands are available for the following backends when testing samples:

* UART
* Segger RTT

You can run the Zigbee shell commands after connecting and configuring the backend for testing.
For more information, see :ref:`gs_testing`.

.. _zigbee_cli_reference:

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
    Zigbee End Device is not currently supported by the CLI sample.


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

.. _bdb_ic:

bdb ic
======

Set install code on the device, add information about the install code on the trust center, set the trust center install code policy.

.. parsed-literal::
   :class: highlight

   bdb ic add *h:install code* *h:eui64*
   bdb ic set *h:install code*
   bdb ic policy *enable|disable*

.. note::
    |precondition3|

* ``bdb ic set`` must only be used on a joining device.

* ``bdb ic add`` must only be used on a coordinator.
  For *h:eui64*, use the address of the joining device.

* ``bdb ic policy`` must only be used on a coordinator.

Provide the install code as an ASCII-encoded :file:`HEX` file that includes CRC16/X-25 in little-endian order.

For production devices, an install code must be installed by the production
configuration present in flash.


Example:

.. code-block::

   > bdb ic add 83FED3407A939723A5C639B26916D505C3B5 0B010E2F79E9DBFA
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

Perform a factory reset via local action.
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

Issue a ping-style shell command to another CLI node with the given 16-bit destination address (*dst_addr*) by using a payload equal to *payload_size* bytes.
The command is sent and received on endpoints with the same ID.

This shell command uses a custom ZCL frame, which is constructed as a ZCL frame of a custom ping ZCL cluster with the cluster ID ``0xBEEF``.
For details, see the implementation of :c:func:`ping_request_send` in :file:`subsys/zigbee/cli/zigbee_cli_cmd_ping.c`.

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
   CLI: Sep  3 2020 13:34:28
   ZBOSS: 3.3.0.2
   Zephyr kernel version: 2.3.99
   Done

----

.. _debug:

debug
=====

Enable or disable the debug mode in the CLI.

.. parsed-literal::
   :class: highlight

   debug *on|off*

This command unblocks several additional commands in the CLI.

.. note::
    When used, the additional commands can render the device unstable.

----

.. _zscheduler_suspend:

zscheduler suspend
==================

Suspend Zigbee scheduler processing.

.. code-block::

   zscheduler suspend

----

.. _zscheduler_resume:

zscheduler resume
=================

Resume Zigbee scheduler processing.

.. code-block::

   zscheduler resume

.. |precondition| replace:: Setting only before :ref:`bdb_start`.
   Reading only after :ref:`bdb_start`.

.. |precondition2| replace:: Setting only before :ref:`bdb_start`.

.. |precondition3| replace:: Setting and defining policy only before :ref:`bdb_start`.
   Adding only after :ref:`bdb_start`.

.. |precondition4| replace:: Use only after :ref:`bdb_start`.
