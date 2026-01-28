.. _rreq_readme:

Ranging Requestor (RREQ)
########################

.. contents::
   :local:
   :depth: 2

Overview
********

This library implements the Ranging Requestor for Channel Sounding with the corresponding set of characteristics defined in the `Ranging Service Specification`_ and the `Ranging Profile Specification`_.

This library supports On Demand Ranging Data and Real-time Ranging Data.

Configuration
*************

To enable this library, use the :kconfig:option:`CONFIG_BT_RAS` Kconfig option.

Check and adjust the following Kconfig options:

* :kconfig:option:`CONFIG_BT_RAS_MAX_ANTENNA_PATHS` - Sets the maximum number of antenna paths supported by the device.

* :kconfig:option:`CONFIG_BT_RAS_MODE_3_SUPPORTED` - Sets support for storing Mode 3 Channel Sounding steps.

* :kconfig:option:`CONFIG_BT_RAS_RREQ` - Enables RREQ Kconfig options.

* :kconfig:option:`CONFIG_BT_RAS_RREQ_MAX_ACTIVE_CONN` - Sets the number of simultaneously supported RREQ instances.

* :kconfig:option:`CONFIG_BT_RAS_RREQ_LOG_LEVEL` - Sets the logging level of the RREQ library.

Usage
*****

You can set up the RREQ either as a Channel Sounding Initiator or Reflector.

| See the sample: :file:`samples/bluetooth/channel_sounding/ras_initiator`

API documentation
*****************

| Header file: :file:`include/bluetooth/services/ras.h`
| Source files: :file:`subsys/bluetooth/services/ras`

.. doxygengroup:: bt_ras
