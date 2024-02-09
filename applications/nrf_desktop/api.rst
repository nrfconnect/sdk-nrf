.. _nrf_desktop_api:

nRF Desktop: API documentation
##############################

.. contents::
   :local:
   :depth: 2

Following are the API elements used by the application.

HID reports
***********

| Header file: :file:`applications/nrf_desktop/configuration/common/hid_report_desc.h`
| Source file: :file:`applications/nrf_desktop/configuration/common/hid_report_desc.c`

.. doxygengroup:: nrf_desktop_hid_reports
   :project: nrf
   :members:

LED states
**********

| Header file: :file:`applications/nrf_desktop/configuration/common/led_state.h`
| Source file: :file:`applications/nrf_desktop/src/modules/led_state.c`

.. doxygengroup:: nrf_desktop_led_state
   :project: nrf
   :members:

USB events
**********

| Header file: :file:`applications/nrf_desktop/src/events/usb_event.h`
| Source file: :file:`applications/nrf_desktop/src/modules/usb_state.c`

.. doxygengroup:: nrf_desktop_usb_event
   :project: nrf
   :members:
