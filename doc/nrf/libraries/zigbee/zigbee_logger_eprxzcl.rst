.. _lib_zigbee_logger_endpoint:

Zigbee endpoint logger
######################

.. contents::
   :local:
   :depth: 2

.. zigbee_logger_endpoint_intro_start

The Zigbee endpoint logger library provides the :c:func:`zigbee_logger_eprxzcl_ep_handler` function.
You can use this endpoint handler function for parsing incoming ZCL frames and logging their fields and command payload.

The library can work as a partial replacement of `nRF Sniffer for 802.15.4`_ for debugging and testing purposes.
Unlike the sniffer, it provides logging information only for incoming ZCL frames on a specific device.

.. zigbee_logger_endpoint_intro_end

.. _lib_zigbee_logger_endpoint_parsing:

Parsing incoming frames
***********************

When you enable the library and register the handler at an endpoint, ZCL frames directed to this endpoint are intercepted and the registered handler is called.
At that point, the handler counts and parses each frame: ZCL header is read and the payload of the command is obtained.

By default, the handler returns ``ZB_TRUE`` when the frame has been processed or ``ZB_FALSE`` if the stack must `process the frame`_ (as described in the ZBOSS API documentation).
This endpoint logger, however, only reads the incoming frame, but does not process the command, and so it always returns ``ZB_FALSE``.

After parsing and gathering the data, the "Received ZCL command" packet entry is added to the log.
This entry has the following fields:

* Source address (``src_addr``)
* Source endpoint (``src_ep``)
* Destination endpoint (``dst_ep``)
* Cluster ID (``cluster_id``)
* Profile ID (``profile_id``)
* Command direction (``cmd_dir``), with one of the following values:

  * ``0`` if from client to server
  * ``1`` if from server to client

* Common command (``common_cmd``), with one of the following values:

  * ``0`` if the command is local or specific to a cluster
  * ``1`` if the command is global for all clusters

* Command ID (``cmd_id``)
* Transaction sequence number (``cmd_seq``)
* Disable default response (``disable_def_resp``), with one of the following values:

  * ``0`` if the default response command is to be returned
  * ``1`` if the default response is to be disabled and the command is not to be returned

* Manufacturer specific data (``manuf_code``; ``void`` if no data is present)
* Payload of the command (``payload``)

The parentheses after the entry name and at the end of the entry indicate the packet counter.

For example, enabling the Zigbee endpoint logger library with the :ref:`zigbee_light_bulb_sample` sample allows it to log On/Off commands received from the :ref:`zigbee_light_switch_sample`:

  .. code-block:: shell

      I: Received ZCL command (0): src_addr=0x03ea(short) src_ep=1 dst_ep=10 cluster_id=0x0006 profile_id=0x0104 cmd_dir=0 common_cmd=0 cmd_id=0x00 cmd_seq=14 disable_def_resp=1 manuf_code=void payload=[] (0)
      I: Received ZCL command (1): src_addr=0x03ea(short) src_ep=1 dst_ep=10 cluster_id=0x0006 profile_id=0x0104 cmd_dir=0 common_cmd=0 cmd_id=0x01 cmd_seq=15 disable_def_resp=1 manuf_code=void payload=[] (1)
      I: Received ZCL command (2): src_addr=0x03ea(short) src_ep=1 dst_ep=10 cluster_id=0x0006 profile_id=0x0104 cmd_dir=0 common_cmd=0 cmd_id=0x00 cmd_seq=16 disable_def_resp=1 manuf_code=void payload=[] (2)
      I: Received ZCL command (3): src_addr=0x03ea(short) src_ep=1 dst_ep=10 cluster_id=0x0006 profile_id=0x0104 cmd_dir=0 common_cmd=0 cmd_id=0x01 cmd_seq=17 disable_def_resp=1 manuf_code=void payload=[] (3)

.. _lib_zigbee_logger_endpoint_options:

Configuration
*************

To enable the Zigbee endpoint logger library, set the :kconfig:option:`CONFIG_ZIGBEE_LOGGER_EP` Kconfig option.

To configure the logging level of the library, use the :kconfig:option:`CONFIG_ZIGBEE_LOGGER_EP_LOG_LEVEL` Kconfig option.

For detailed steps about configuring the library in a Zigbee sample or application, see :ref:`ug_zigbee_configuring_components_logger_ep`.

API documentation
*****************

| Header file: :file:`include/zigbee/zigbee_logger_eprxzcl.h`
| Source file: :file:`subsys/zigbee/lib/zigbee_logger_ep/zigbee_logger_eprxzcl.c`

.. doxygengroup:: zigbee_logger_ep
   :project: nrf
   :members:
