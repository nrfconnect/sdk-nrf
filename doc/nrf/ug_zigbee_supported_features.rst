.. _zigbee_ug_supported_features:

Supported Zigbee features
#########################

.. contents::
   :local:
   :depth: 2

The |NCS|'s Zigbee protocol uses the ZBOSS library, a third-party precompiled Zigbee stack.
It includes all mandatory features of the |zigbee_version| specification and provides an Application Programming Interface to access different services.
The stack comes with the following features:

* Complete implementation of the Zigbee core specification and Zigbee Pro feature set.
* Support for all device roles: Coordinator, Router, and End Device.
* Zigbee Cluster Library.
* Base Device Behavior.
* Devices, described in former Zigbee Home Automation and Light Link profiles.
* Zigbee Green Power Proxy Basic.
* Experimental support for ``ZB_ZCL_WWAH``.
* Experimental support for Green Power Combo.

See :ref:`nrfxlib:zboss` and pages in this user guide for more information.

The |NCS|'s Zigbee protocol also supports :ref:`Zigbee firmware over-the-air updates <lib_zigbee_fota>`.

For more information about Zigbee, visit the `Zigbee Alliance`_ page and read `Zigbee Specification`_.
