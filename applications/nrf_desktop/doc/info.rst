.. _nrf_desktop_info:

Info module
###########

.. contents::
   :local:
   :depth: 2

The Info module provides the following device information through the configuration channel:

* Highest ID of configuration channel listener
* Board name
* Hardware ID (HW ID)

The data provided by Info module is required by :ref:`nrf_desktop_config_channel_script` to identify, discover, and configure the device.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_info_start
    :end-before: table_info_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module selects the :ref:`CONFIG_DESKTOP_HWID <config_desktop_app_options>` option to make sure that nRF Desktop Hardware ID utility is enabled.
The utility uses Zephyr's :ref:`zephyr:hwinfo_api` to obtain the hardware ID and selects the :kconfig:option:`CONFIG_HWINFO` Kconfig option to automatically enable the required driver.

The module is enabled with the same Kconfig option as the :ref:`nrf_desktop_config_channel`: :ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE <config_desktop_app_options>`.

Implementation details
**********************

Providing the highest ID of the configuration channel listener is based on the number of elements in the ``config_channel_modules`` section.
:c:macro:`GEN_CONFIG_EVENT_HANDLERS` adds an element to the section for every registered configuration channel listener.

The board name provided by the module through the :ref:`nrf_desktop_config_channel` is a part of the Zephyr board name (:kconfig:option:`CONFIG_BOARD`), ending with the predefined character (:c:macro:`BOARD_NAME_SEPARATOR`).
