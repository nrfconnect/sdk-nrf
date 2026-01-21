.. _nrf_desktop_usb_state_pm:

USB state power manager module
##############################

.. contents::
   :local:
   :depth: 2

The |usb_state_pm| is a minor, stateless module that imposes the following power state restrictions related to the USB state:

* Application power level restrictions.
  The application power level is managed by the :ref:`caf_power_manager`.
* Zephyr's :ref:`zephyr:pm-system` latency restrictions.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_usb_state_pm_start
    :end-before: table_usb_state_pm_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

To enable the module, use the :option:`CONFIG_DESKTOP_USB_PM_ENABLE` Kconfig option.
It depends on the options :option:`CONFIG_DESKTOP_USB_ENABLE` and :kconfig:option:`CONFIG_CAF_PM_EVENTS`.

The log level is inherited from the :ref:`nrf_desktop_usb_state`.

System power management integration
===================================

Zephyr's system power management (:kconfig:option:`CONFIG_PM`) does not automatically take into account (expect) wakeups related to user input and finalized HID report transfers over USB.
This results in entering low power states if no work is scheduled to be done in the nearest future.
If you use Zephyr's system power management, the module automatically requires zero latency in the power management while USB is active.
This is done to prevent entering power states that introduce wakeup latency and ensure high performance.
You can control this feature using the :option:`CONFIG_DESKTOP_USB_PM_REQ_NO_PM_LATENCY` Kconfig option.

Implementation details
**********************

The module reacts to the :c:struct:`usb_state_event`.
Upon reception of the event and depending on the current USB state, the module requests different power restrictions.
For more information about the USB states in nRF Desktop, see the :ref:`nrf_desktop_usb_state`.

Application power level
=======================

The application power level is imposed using the :c:struct:`power_manager_restrict_event`.

* If the USB state is set to :c:enum:`USB_STATE_POWERED`, the module restricts the power down level to the :c:enum:`POWER_MANAGER_LEVEL_SUSPENDED`.
* If the USB state is set to :c:enum:`USB_STATE_ACTIVE`, the :c:enum:`POWER_MANAGER_LEVEL_ALIVE` is required.
* If the USB state is set to :c:enum:`USB_STATE_SUSPENDED`, the :c:enum:`POWER_MANAGER_LEVEL_SUSPENDED` is imposed.
  The module restricts the power down level to the :c:enum:`POWER_MANAGER_LEVEL_SUSPENDED`.
  The module also submits a :c:struct:`force_power_down_event` to force a quick power down.
* If the USB state is set to :c:enum:`USB_STATE_DISCONNECTED`, any power level is allowed.
  While disconnecting the USB cable, the :c:enum:`USB_STATE_SUSPENDED` USB state might be reported before the :c:enum:`USB_STATE_DISCONNECTED` USB state.
  For the application to behave consistently regardless of whether the :c:enum:`USB_STATE_SUSPENDED` USB state was reported, the module also submits a :c:struct:`force_power_down_event` to force a quick power down.
  The module initially restricts the power down level to the :c:enum:`POWER_MANAGER_LEVEL_SUSPENDED`.
  Then, after the :option:`CONFIG_DESKTOP_USB_PM_RESTRICT_REMOVE_DELAY_MS` configurable delay, the module removes the power down level restriction.
  This allows you to take actions, such as restart Bluetooth LE advertising, after disconnecting the USB cable without going through reboot.

System power management latency
===============================

The latency requirements of the system power management are updated using the :c:func:`pm_policy_latency_request_add` and :c:func:`pm_policy_latency_request_remove` functions.
The zero latency requirement is added when USB state is set to :c:enum:`USB_STATE_ACTIVE`.
The requirement is removed if USB enters another state.
