.. _nrf_desktop_ble_adv_ctrl:

Bluetooth LE advertising control module
#######################################

.. contents::
   :local:
   :depth: 2

The Bluetooth LE advertising control module is responsible for controlling :ref:`caf_ble_adv`.
The module is used to suspend and resume the |ble_adv|.
For now, the module can only react to the USB state changes.
It suspends the |ble_adv| when the active USB device is connected (USB state is set to :c:enum:`USB_STATE_ACTIVE`).
It resumes the |ble_adv| when the USB is disconnected (USB state is set to :c:enum:`USB_STATE_DISCONNECTED`) and the |ble_adv| was earlier suspended.
This improves the USB High-Speed performance.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_ble_adv_ctrl_start
    :end-before: table_ble_adv_ctrl_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

This module is disabled by default.
To enable it, set the :option:`CONFIG_DESKTOP_BLE_ADV_CTRL_ENABLE` Kconfig option to ``y``.
To enable the module to suspend and resume the :ref:`caf_ble_adv` when USB state changes, set the :option:`CONFIG_DESKTOP_BLE_ADV_CTRL_SUSPEND_ON_USB` Kconfig option to ``y``.
It is recommended to enable this option if the device supports the USB High-Speed.

Implementation details
**********************

This module subscribes to :c:struct:`usb_state_event` events sent by the |usb_state|.
It reacts to these events by submitting :c:struct:`module_suspend_req_event` and :c:struct:`module_resume_req_event` events to the |ble_adv| when it is required.
