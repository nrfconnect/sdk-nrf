.. _ug_zigbee_tools:

Zigbee tools
############

.. contents::
   :local:
   :depth: 2

The tools listed on this page can be helpful when developing your Zigbee application with the |NCS|.

.. _ug_zigbee_tools_sniffer:

nRF Sniffer for 802.15.4
************************

The nRF Sniffer for 802.15.4 is a tool for learning about and debugging applications that are using protocols based on IEEE 802.15.4, like Thread or Zigbee.
It provides a near real-time display of 802.15.4 packets that are sent back and forth between devices, even when the link is encrypted.

See `nRF Sniffer for 802.15.4`_ for documentation.

.. _ug_zigbee_tools_ncp_host:

ZBOSS NCP Host
**************

The NCP Host is a ZBOSS-based tool for running the host side of the :ref:`ug_zigbee_platform_design_ncp_details` design.
|zigbee_ncp_package|

The tool is available for download as a standalone :file:`zip` package using the following link:

* `ZBOSS NCP Host`_ (|zigbee_ncp_package_version|)

|zigbee_ncp_package_more_info|

.. _ug_zigbee_tools_logger_endpoint:

Zigbee endpoint logger
**********************

.. include:: ../../libraries/zigbee/zigbee_logger_eprxzcl.rst
    :start-after: zigbee_logger_endpoint_intro_start
    :end-before: zigbee_logger_endpoint_intro_end

See :ref:`lib_zigbee_logger_endpoint` for documentation.
