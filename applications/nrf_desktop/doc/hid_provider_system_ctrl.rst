.. _nrf_desktop_hid_provider_system_ctrl:

HID provider system control module
##################################

.. contents::
   :local:
   :depth: 2

The HID provider system control module is responsible for providing system control HID reports.
The module listens to the :c:struct:`button_event` event and communicates with the :ref:`nrf_desktop_hid_state`.
It provides new system control HID reports when requested by the :ref:`nrf_desktop_hid_state`.
It also notifies the :ref:`nrf_desktop_hid_state` when new data is available.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_hid_provider_system_ctrl_start
    :end-before: table_hid_provider_system_ctrl_end

.. |consumer_system| replace:: system

.. |config_consumer_system_crtl| replace:: :ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_SYSTEM_CTRL <config_desktop_app_options>`

.. |config_consumer_system_crtl_alt| replace:: :ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_SYSTEM_CTRL_ALT <config_desktop_app_options>`

.. |config_consumer_system_crtl_event_queue_size| replace:: :ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_SYSTEM_CTRL_EVENT_QUEUE_SIZE <config_desktop_app_options>`

.. |config_consumer_system_crtl_keypress_expiration| replace:: :ref:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_SYSTEM_CTRL_KEYPRESS_EXPIRATION <config_desktop_app_options>`

.. include:: /includes/hid_provider_consumer_system_control.txt
