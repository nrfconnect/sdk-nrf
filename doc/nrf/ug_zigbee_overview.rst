.. _zigbee_ug_intro:

Zigbee overview
###############

|zigbee_description|
It also defines an application layer that provides interoperability among all vendors.

In combination with |NCS| and the integrated Zephyr RTOS, Zigbee allows for easy development of low-power connected solutions.

For more information about Zigbee, visit the `Zigbee Alliance`_ page and read `Zigbee Specification`_.

.. _zigbee_ug_supported_features:

Supported features
******************

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

See :ref:`nrfxlib:zboss` for more information.
