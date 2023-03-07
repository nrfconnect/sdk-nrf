.. _zigbee_ug_supported_features:

Supported Zigbee features
#########################

.. contents::
   :local:
   :depth: 2

The |NCS|'s Zigbee protocol uses the ZBOSS library, a third-party precompiled Zigbee stack.
It includes all mandatory features of the |zigbee_version| specification and provides an Application Programming Interface (API) to access different services.
The stack comes with the following features:

* Complete implementation of the Zigbee core specification and Zigbee Pro feature set
* Support for all device roles: Coordinator, Router, and End Device
* Zigbee Cluster Library, including :ref:`lib_zigbee_fota`
* Base Device Behavior
* Devices definitions for devices that were implemented for Zigbee Home Automation and Light Link profiles
* Zigbee Green Power Proxy Basic
* :ref:`Experimental <software_maturity>` support for ``ZB_ZCL_WWAH``

Experimental support also means that the feature is either not certified or no sample is provided for the given feature (or both).

See the :ref:`nrfxlib:zboss` page in nrfxlib and the `external ZBOSS development guide and API documentation`_ for more information about the ZBOSS library.

For more information about Zigbee, download the `Zigbee Specification <CSA Specifications Download Request_>`_ from Connectivity Standards Alliance.
