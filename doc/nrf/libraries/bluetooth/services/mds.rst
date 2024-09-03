.. _mds_readme:

Memfault Diagnostic Service (MDS)
#################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® Low Energy (LE) GATT Memfault Diagnostic Service is a custom service that forwards diagnostic data collected by firmware through a Bluetooth gateway.
The diagnostic data is collected by the `Memfault SDK`_ integrated with the |NCS|.

To get started with Memfault integration in |NCS|, see :ref:`ug_memfault`.

The MDS is used in the :ref:`peripheral_mds` sample.

Service UUID
************

The 128-bit service UUID is ``54220000-f6a5-4007-a371-722f4ebd8436``.

Characteristics
***************

The MDS characteristics are described in detail in the `Memfault Diagnostic GATT Service`_ documentation.
The service implementation available in the |NCS| follows these requirements.

.. note::
   Access restriction to diagnostic data is implemented differently from the one described in the `Memfault Diagnostic GATT Service`_ documentation.
   See :ref:`mds_readme_access_restriction` for more details.

Configuration
*************

Set the :kconfig:option:`CONFIG_BT_MDS` Kconfig option to enable the service.

The following configuration options are available for this service:

   * :kconfig:option:`CONFIG_BT_MDS_MAX_URI_LENGTH` sets the maximum length of the URI to which diagnostic data should be forwarded.
     The URI contains the device ID.
     See :ref:`mod_memfault` for more details.
   * :kconfig:option:`CONFIG_BT_MDS_PERM` provides permission to the service characteristic.
     In the default configuration, it is set to :kconfig:option:`CONFIG_BT_MDS_PERM_RW_ENCRYPT`.
     This is because the potentially sensitive Memfault project key and diagnostic data are sent through service's characteristics.
     There is not hard requirement for access restriction to diagnostic data.
   * :kconfig:option:`CONFIG_BT_MDS_PIPELINE_COUNT` sets the maximum number of service notifications that can be queued in the Bluetooth stack.
   * :kconfig:option:`CONFIG_BT_MDS_DATA_POLL_INTERVAL` sets the interval of checking if any diagnostic data is available for sending.

See the Kconfig help for details.

Implementation details
**********************

The implementation uses :c:macro:`BT_GATT_SERVICE_DEFINE` to statically define and register the Memfault Diagnostic GATT service.
The service automatically checks if there is data available to be sent with the interval defined by the :kconfig:option:`CONFIG_BT_MDS_DATA_POLL_INTERVAL` and sends it using the notification mechanism.
No application input is required to send diagnostic data.
However, if you pass :c:struct:`bt_mds_cb` to the :c:func:`bt_mds_cb_register` function, the application needs to confirm that the connected client can access the diagnostic data every time the client performs a ``read`` or ``write`` operation on the service characteristic.

Use the :c:func:`bt_mds_cb_register` function to register callbacks the service.

.. note::
   Call the :c:func:`bt_mds_cb_register` function before enabling Bluetooth stack to ensure a proper access restriction to MDS service data.

.. note::
   Only one connected client can operate on the MDS characteristic and receive a notification with diagnostic data.

.. _mds_readme_access_restriction:

Restricting access
******************

The Memfault service characteristics data might contain sensitive data.
It is recommended to use the Bluetooth privacy and encrypted link to access the diagnostic data.
Enable the :kconfig:option:`CONFIG_BT_SMP` Kconfig option to require encryption for access in the default configuration.
It is also highly recommended to implement the :c:member:`access_enable` callback.
See :ref:`peripheral_mds` for an implementation example.

Bluetooth privacy
-----------------

It is recommended to use the Bluetooth Privacy feature when you use this service.
To enable privacy, set the :kconfig:option:`CONFIG_BT_PRIVACY` option.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/mds.h`
| Source file: :file:`subsys/bluetooth/services/mds.c`

.. doxygengroup:: bt_mds
   :project: nrf
   :members:
