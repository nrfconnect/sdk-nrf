.. _lib_zigbee_zcl_scenes:

Zigbee ZCL scene helper
#######################

.. contents::
   :local:
   :depth: 2

The Zigbee ZCL scene helper library provides a set of functions that implement the callbacks required by the ZCL scene cluster in the application.
You can use this library to implement these mandatory callbacks and save the configured scenes in the non-volatile memory.
For this purpose, it uses Zephyr's :ref:`settings_api` subsystem.

This library is capable of recalling attribute values for the following ZCL clusters:

* On/off
* Level control
* Window covering

If you are implementing clusters that are not included in this list, you must implement their logic manually instead of using this library.

.. _lib_zigbee_zcl_scenes_options:

Configuration
*************

To enable the Zigbee ZCL scene helper library, set the :kconfig:option:`CONFIG_ZIGBEE_SCENES` Kconfig option.

Because the library uses Zephyr's :ref:`settings_api` subsystem, the application must call the following functions for the library to work correctly:

* :c:func:`zcl_scenes_init()`
* :c:func:`zcl_scenes_cb()`
* :c:func:`settings_subsys_init()`
* :c:func:`settings_load()`

You must implement these functions in the following manner:

* :c:func:`settings_subsys_init()` must be called before :c:func:`zcl_scenes_init()`
* :c:func:`settings_load()` must be called after :c:func:`zcl_scenes_init()`
* :c:func:`zcl_scenes_cb()` must be called in the ZCL device callback:

  .. code-block:: C

      static void zcl_device_cb(zb_bufid_t bufid)
      {
         zb_zcl_device_callback_param_t  *device_cb_param =
            ZB_BUF_GET_PARAM(bufid, zb_zcl_device_callback_param_t);
         switch (device_cb_param->device_cb_id) {
         ...
         default:
            if (zcl_scenes_cb(bufid) == ZB_FALSE) {
                  device_cb_param->status = RET_ERROR;
            }
            break;
         }
      }

Setting the :kconfig:option:`CONFIG_ZIGBEE_SCENES` option allows you to configure the following library-specific Kconfig options:

* :kconfig:option:`CONFIG_ZIGBEE_SCENES_ENDPOINT` - This option sets the endpoint number on which the device implements the ZCL scene cluster.
* :kconfig:option:`CONFIG_ZIGBEE_SCENE_TABLE_SIZE` - This option sets the value for the number of scenes that can be configured.

To configure the logging level of the library, use the :kconfig:option:`CONFIG_ZIGBEE_SCENES_LOG_LEVEL` Kconfig option.

API documentation
*****************

| Header file: :file:`include/zigbee/zigbee_zcl_scenes.h`
| Source file: :file:`subsys/zigbee/lib/zigbee_scenes/zigbee_zcl_scenes.c`

.. doxygengroup:: zigbee_scenes
   :project: nrf
   :members:
