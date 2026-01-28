.. _rrsp_readme:

Ranging Responder (RRSP)
########################

.. contents::
   :local:
   :depth: 2

Overview
********

This library implements the Ranging Responder for Channel Sounding with the corresponding set of characteristics defined in the `Ranging Service Specification`_ and the `Ranging Profile Specification`_.

This library supports On Demand Ranging Data and Real-time Ranging Data.

Configuration
*************

To enable this library, use the :kconfig:option:`CONFIG_BT_RAS` Kconfig option.

Check and adjust the following Kconfig options:

* :kconfig:option:`CONFIG_BT_RAS_MAX_ANTENNA_PATHS` - Sets the maximum number of antenna paths supported by the device.
  This sets the antenna paths for each step that can be stored inside the Ranging Service.
  This value must match the supported Channel Sounding capabilities of the device.
  This affects the per-instance memory usage of the Ranging Service.

* :kconfig:option:`CONFIG_BT_RAS_MODE_3_SUPPORTED` - Sets support for storing Mode 3 Channel Sounding steps.
  This will allocate memory for the Ranging Service to store Mode 3 Channel Sounding steps.
  This value must match the supported Channel Sounding capabilities of the device.
  This affects the per-instance memory usage of the Ranging Service.

* :kconfig:option:`CONFIG_BT_RAS_RRSP` - Enables RRSP Kconfig options.

* :kconfig:option:`CONFIG_BT_RAS_RRSP_AUTO_ALLOC_INSTANCE` - Sets new connections to be allocated a RRSP instance automatically.

* :kconfig:option:`CONFIG_BT_RAS_RRSP_MAX_ACTIVE_CONN` - Sets the number of simultaneously supported RRSP instances.

* :kconfig:option:`CONFIG_BT_RAS_RRSP_RD_BUFFERS_PER_CONN` - Set the number of ranging data buffers per connection.

* :kconfig:option:`CONFIG_BT_RAS_RRSP_LOG_LEVEL` - Sets the logging level of the RRSP library.

Usage
*****

You can set up the RRSP either as a Channel Sounding Initiator or Reflector.

| See the sample: :file:`samples/bluetooth/channel_sounding/ras_reflector`

API documentation
*****************

| Header file: :file:`include/bluetooth/services/ras.h`
| Source files: :file:`subsys/bluetooth/services/ras`

.. doxygengroup:: bt_ras
