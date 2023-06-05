.. _nrf_desktop_usb_state_pm:

USB state power manager module
##############################

.. contents::
   :local:
   :depth: 2

The |usb_state_pm| is minor, stateless module that imposes an application power level restriction related to the USB state.
The application power level is managed by the :ref:`power manager module <caf_power_manager>`.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_usb_state_pm_start
    :end-before: table_usb_state_pm_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module is enabled by selecting :ref:`CONFIG_DESKTOP_USB_PM_ENABLE <config_desktop_app_options>` option.
It depends on :ref:`CONFIG_DESKTOP_USB_ENABLE <config_desktop_app_options>` and :kconfig:option:`CONFIG_CAF_POWER_MANAGER` options.

The log level is inherited from the :ref:`nrf_desktop_usb_state`.

Implementation details
**********************

For the change of the restricted power level, the module reacts to :c:struct:`usb_state_event`.
Upon reception of the event and depending on the current USB state, the module requests different power restrictions:

* If the USB state is set to :c:enum:`USB_STATE_POWERED`, the module restricts the power down level to the :c:enum:`POWER_MANAGER_LEVEL_SUSPENDED`.
* If the USB state is set to :c:enum:`USB_STATE_ACTIVE`, the :c:enum:`POWER_MANAGER_LEVEL_ALIVE` is required.
* If the USB state is set to :c:enum:`USB_STATE_DISCONNECTED`, any power level is allowed.
* If the USB state is set to :c:enum:`USB_STATE_SUSPENDED`, the :c:enum:`POWER_MANAGER_LEVEL_SUSPENDED` is imposed.
  The module restricts the power down level to the :c:enum:`POWER_MANAGER_LEVEL_SUSPENDED` and generates :c:struct:`force_power_down_event`.

For more information about the USB states in nRF Desktop, see the :ref:`nrf_desktop_usb_state`.
