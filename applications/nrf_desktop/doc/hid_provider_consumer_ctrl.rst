.. _nrf_desktop_hid_provider_consumer_ctrl:

HID provider consumer control module
####################################

.. contents::
   :local:
   :depth: 2

The HID provider consumer control module is a HID report provider integrated with the :ref:`nrf_desktop_hid_state`.
The module is responsible for providing the HID consumer control input report.

The module listens to the user input (:c:struct:`button_event`) and communicates with the :ref:`nrf_desktop_hid_state`.
It provides HID consumer control reports when requested by the :ref:`nrf_desktop_hid_state`.
It also notifies the :ref:`nrf_desktop_hid_state` when new data is available.

For details related to the HID report providers integration in the HID state module, see the :ref:`nrf_desktop_hid_state_providing_hid_input_reports` documentation section.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_hid_provider_consumer_ctrl_start
    :end-before: table_hid_provider_consumer_ctrl_end

.. |consumer_system| replace:: consumer

.. |config_consumer_system_crtl| replace:: :option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_CONSUMER_CTRL`

.. |config_consumer_system_crtl_support| replace:: :option:`CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT`

.. |config_consumer_system_crtl_alt| replace:: :option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_CONSUMER_CTRL_ALT`

.. |config_consumer_system_crtl_event_queue_size| replace:: :option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_CONSUMER_CTRL_EVENT_QUEUE_SIZE`

.. |config_consumer_system_crtl_keypress_expiration| replace:: :option:`CONFIG_DESKTOP_HID_REPORT_PROVIDER_CONSUMER_CTRL_KEYPRESS_EXPIRATION`

.. include:: /includes/hid_provider_consumer_system_control.txt
