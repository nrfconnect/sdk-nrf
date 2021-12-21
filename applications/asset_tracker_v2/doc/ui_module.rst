.. _asset_tracker_v2_ui_module:

User Interface module
#####################

.. contents::
   :local:
   :depth: 2

The User Interface module controls and monitors the UI elements on the nRF9160 DK and Thingy:91.

Features
********

This section documents the various features implemented by the module.

Button presses
==============

When a button is pressed on a supported board, the UI module notifies the rest of the application with the :c:enum:`UI_EVT_BUTTON_DATA_READY` event that contains information about the button press.
The buttons used by the module and their functionality are listed in the following table:

.. _button_behavior:

+--------+-------------------------------------+------------------------------------------------------------------------------------------------------------------+
| Button | Thingy:91                           | nRF9160 DK                                                                                                       |
+========+=====================================+==================================================================================================================+
| 1      | Send a message to the cloud service | Send message to the cloud service.                                                                               |
+--------+-------------------------------------+------------------------------------------------------------------------------------------------------------------+
| 2      |                                     | Send message to the cloud service.                                                                               |
|        |                                     +------------------------------------------------------------------------------------------------------------------+
|        |                                     | Fake movement. For testing purposes, the nRF9160 DK does not have an external accelerometer to trigger movement. |
+--------+-------------------------------------+------------------------------------------------------------------------------------------------------------------+

.. _led_indication:

LED indication
==============

The module supports multiple LED patterns to visualize the operating state of the application.
The following table describes the supported LED states:

+---------------------------+-------------------------+-----------------------+
| State                     | Thingy:91 RGB LED       | nRF9160 DK solid LEDs |
+===========================+=========================+=======================+
| LTE connection search     | Yellow, blinking        | LED1 blinking         |
+---------------------------+-------------------------+-----------------------+
| GNSS fix search           | Purple, blinking        | LED2 blinking         |
+---------------------------+-------------------------+-----------------------+
| Publishing data           | Green, blinking         | LED3 blinking         |
+---------------------------+-------------------------+-----------------------+
| Active mode               | Light blue, blinking    | LED4 blinking         |
+---------------------------+-------------------------+-----------------------+
| Passive mode              | Dark blue, slow blinking| LED4 slow blinking    |
+---------------------------+-------------------------+-----------------------+
| Error                     | Red, static             | all 4 LEDs blinking   |
+---------------------------+-------------------------+-----------------------+
| Completion of FOTA update | White, rapid blinking   | all 4 LEDs on         |
+---------------------------+-------------------------+-----------------------+

Configuration options
*********************

.. _CONFIG_UI_THREAD_STACK_SIZE:

CONFIG_UI_THREAD_STACK_SIZE - UI module thread stack size
   This option configures the UI module's internal thread stack size.

Module states
*************

The UI module continuously updates its internal state based on the mode and operating state of the application.
This ensures that the correct LED pattern is displayed when the application changes its state of operation.
The UI module has an internal state machine with the following states:

* ``STATE_ACTIVE`` - The application is in the active mode, revert to the active mode LED pattern.
* ``STATE_PASSIVE`` - The application is in the passive mode, revert to the passive mode LED pattern.

   * ``SUB_STATE_GNSS_ACTIVE`` - The application is performing a GNSS search, revert to the GNSS search LED pattern.
   * ``SUB_STATE_GNSS_INACTIVE`` - A GNSS search is not being performed.

* ``STATE_SHUTDOWN`` - The module has been shut down after receiving a request from the utility module.


Module events
*************

The :file:`asset_tracker_v2/src/events/ui_module_event.h` header file contains a list of various events sent by the module.

Dependencies
************

This module uses the following |NCS| libraries and drivers:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`caf_leds`

API documentation
*****************

| Header file: :file:`asset_tracker_v2/src/events/ui_module_event.h`
| Source files: :file:`asset_tracker_v2/src/events/ui_module_event.c`
                :file:`asset_tracker_v2/src/modules/ui_module.c`

.. doxygengroup:: ui_module_event
   :project: nrf
   :members:
