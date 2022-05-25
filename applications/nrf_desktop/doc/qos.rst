.. _nrf_desktop_qos:

Quality of Service module
#########################

.. contents::
   :local:
   :depth: 2

The Quality of Service (QoS) module provides the QoS information through the Bluetooth® GATT service.
The module can be used only by nRF Desktop peripheral with the SoftDevice Link Layer (:kconfig:option:`CONFIG_BT_LL_SOFTDEVICE`).

The module is made available in case the peripheral is meant to be paired with a third-party dongle.
In such case, the vendor can use the Quality of Service data provided by the nRF Desktop peripheral to improve the link quality.

.. note::
   There is no need to enable the Quality of Service module if you want to pair the peripheral device with the nRF Desktop dongle.
   The dongle does not depend on the service provided by the Quality of Service module on peripheral devices.

For more information about the format of the Quality of Service data, see the :ref:`nrf_desktop_ble_qos` documentation.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_qos_start
    :end-before: table_qos_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module requires the basic Bluetooth® configuration, as described in :ref:`nrf_desktop_bluetooth_guide`.

The module is enabled with :ref:`CONFIG_DESKTOP_QOS_ENABLE <config_desktop_app_options>` option.
The module is available on the :ref:`peripheral devices <nrf_desktop_bluetooth_guide_peripheral>` only and requires the :ref:`nrf_desktop_ble_qos` to be enabled.

Implementation details
**********************

The module uses :c:macro:`BT_GATT_SERVICE_DEFINE` to define a custom GATT Service.
The UUID of the service is located in the module source file, :file:`qos.c`.
For more information about GATT, see Zephyr's :ref:`zephyr:bt_gatt` documentation.

The module uses the Quality of Service data read from the SoftDevice Link Layer.
This data is delivered in the form of a ``ble_qos_event`` event by the :ref:`nrf_desktop_ble_qos`.
This is the same data that is used by the nRF Desktop dongle to avoid congested RF channels.
