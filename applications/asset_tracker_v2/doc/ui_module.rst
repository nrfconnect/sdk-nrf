.. _asset_tracker_v2_ui_module:

User Interface module
#####################

.. contents::
   :local:
   :depth: 2

The User Interface module controls and monitors the UI elements on nRF91 Series development kits, Thingy:91, and Thingy:91 X.

Features
********

This section documents the various features implemented by the module.

Button presses
==============

When a button is pressed on a supported board, the UI module notifies the rest of the application with the :c:enum:`UI_EVT_BUTTON_DATA_READY` event that contains information about the button press.
The buttons used by the module and their functionality are listed in the following table:

.. _button_behavior:

+--------+-------------------------------------+------------------------------------+
| Button | Thingy:91 and Thingy:91 X           | nRF91 Series DK                    |
+========+=====================================+====================================+
| 1      | Send a message to the cloud service | Send message to the cloud service. |
+--------+-------------------------------------+------------------------------------+
| 2      |                                     | Send message to the cloud service. |
+--------+-------------------------------------+------------------------------------+

.. _led_indication:

LED indication
==============

The module supports multiple LED patterns to visualize the operating state of the application.
The following table describes the supported LED states:

+---------------------------+------------------------------------+----------------------------+
| State                     | Thingy:91 and Thingy:91 X RGB LEDs | nRF91 Series DK solid LEDs |
+===========================+====================================+============================+
| LTE connection search     | Yellow, blinking                   | LED1 blinking              |
+---------------------------+------------------------------------+----------------------------+
| Location search           | Purple, blinking                   | LED2 blinking              |
+---------------------------+------------------------------------+----------------------------+
| Cloud association         | White, double pulse blinking       | LED3 double pulse blinking |
+---------------------------+------------------------------------+----------------------------+
| Connecting to cloud       | Green, triple pulse blinking       | LED3 triple pulse blinking |
+---------------------------+------------------------------------+----------------------------+
| Publishing data           | Green, blinking                    | LED3 blinking              |
+---------------------------+------------------------------------+----------------------------+
| Active mode               | Light blue, blinking               | LED4 blinking              |
+---------------------------+------------------------------------+----------------------------+
| Passive mode              | Dark blue, blinking                | LED3 and LED4 blinking     |
+---------------------------+------------------------------------+----------------------------+
| Error                     | Red, static                        | All 4 LEDs blinking        |
+---------------------------+------------------------------------+----------------------------+
| FOTA update               | Orange, rapid blinking             | LED1 and LED2 blinking     |
+---------------------------+------------------------------------+----------------------------+
| Completion of FOTA update | Orange, static                     | LED1 and LED2 static       |
+---------------------------+------------------------------------+----------------------------+

.. note::
   The LED pattern that indicates the device mode is visible for a few seconds after an update has been sent to cloud.

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

* ``STATE_INIT`` - The initial state of the module.
* ``STATE_RUNNING`` - The module has performed all the required initializations and can initiate the display of LED patterns based on incoming events from other modules.

   * ``SUB_STATE_ACTIVE`` - The application is in the active mode. The module reverts to the active mode LED pattern after cloud publication.
   * ``SUB_STATE_PASSIVE`` - The application is in the passive mode. The module reverts to the passive mode LED pattern after cloud publication.

      * ``SUB_SUB_STATE_LOCATION_ACTIVE`` - The application is performing a location search. The module reverts to the location search LED pattern.
      * ``SUB_SUB_STATE_LOCATION_INACTIVE`` - A location search is not being performed.

* ``STATE_LTE_CONNECTING`` - The modem module is performing an LTE connection search. The UI module triggers the LTE connection search LED pattern.
* ``STATE_CLOUD_CONNECTING`` - The cloud module is connecting to cloud. The UI module triggers the cloud connection LED pattern.
* ``STATE_CLOUD_ASSOCIATING`` - The cloud module is performing user association. The UI module triggers the cloud association LED pattern.
* ``STATE_FOTA_UPDATING`` - The cloud module is performing a FOTA update. The UI module triggers the FOTA update LED pattern.
* ``STATE_SHUTDOWN`` - The module has been shut down after receiving a request from the utility module. It triggers the appropriate LED pattern that corresponds to the shutdown reason.

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
| Source files: :file:`asset_tracker_v2/src/events/ui_module_event.c`, :file:`asset_tracker_v2/src/modules/ui_module.c`

.. doxygengroup:: ui_module_event
   :project: nrf
   :members:
